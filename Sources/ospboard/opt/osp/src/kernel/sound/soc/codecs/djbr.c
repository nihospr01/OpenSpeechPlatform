/*
* Analog Devices DJBR Audio Codec driver
*
* Copyright 2016 Analog Devices Inc.
* Author: Lars-Peter Clausen <lars@metafoo.de>
*
* Licensed under the GPL-2
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gcd.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pm.h>
#include <linux/slab.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/tlv.h>
#include <sound/soc.h>

#include "djbr.h"
#include "adau-utils.h"

struct djbr {
  struct clk *clk;
  struct regmap *regmap;
  void (*switch_mode)(struct device *dev);
  bool use_pll;
  bool enabled;

  struct snd_pcm_hw_constraint_list rate_constraints;
  unsigned int slot_width;

  struct clk *mclk;
  struct gpio_desc *pd_gpio;
  struct device *dev;
};

#define DJBR_REG_CLK_CTRL		0x00
#define DJBR_REG_PLL(x)		(0x01 + (x))
#define DJBR_REG_DAC_SOURCE		0x11
#define DJBR_REG_SOUT_SOURCE_0_1	0x13
#define DJBR_REG_SOUT_SOURCE_2_3	0x14
#define DJBR_REG_SOUT_SOURCE_4_5	0x15
#define DJBR_REG_SOUT_SOURCE_6_7	0x16
#define DJBR_REG_ADC_SDATA_CH	0x17
#define DJBR_REG_ASRCO_SOURCE_0_1	0x18
#define DJBR_REG_ASRCO_SOURCE_2_3	0x19
#define DJBR_REG_ASRC_MODE		0x1a
#define DJBR_REG_ADC_CTRL0		0x1b
#define DJBR_REG_ADC_CTRL1		0x1c
#define DJBR_REG_ADC_CTRL2		0x1d
#define DJBR_REG_ADC_CTRL3		0x1e
#define DJBR_REG_ADC_VOL(x)		(0x1f + (x))
#define DJBR_REG_PGA_CTRL(x)	(0x23 + (x))
#define DJBR_REG_PGA_BOOST		0x28
#define DJBR_REG_MICBIAS		0x2d
#define DJBR_REG_DAC_CTRL		0x2e
#define DJBR_REG_DAC_VOL(x)		(0x2f + (x))
#define DJBR_REG_OP_STAGE_MUTE	0x31
#define DJBR_REG_SAI0		0x32
#define DJBR_REG_SAI1		0x33
#define DJBR_REG_SOUT_CTRL		0x34
#define DJBR_REG_MODE_MP(x)		(0x38 + (x))
#define DJBR_REG_OP_STAGE_CTRL	0x43
#define DJBR_REG_DECIM_PWR		0x44
#define DJBR_REG_INTERP_PWR		0x45
#define DJBR_REG_BIAS_CTRL0		0x46
#define DJBR_REG_BIAS_CTRL1		0x47

#define DJBR_CLK_CTRL_PLL_EN	BIT(7)
#define DJBR_CLK_CTRL_XTAL_DIS	BIT(4)
#define DJBR_CLK_CTRL_CLKSRC	BIT(3)
#define DJBR_CLK_CTRL_CC_MDIV	BIT(1)
#define DJBR_CLK_CTRL_MCLK_EN	BIT(0)

#define DJBR_SAI0_DELAY1		(0x0 << 6)
#define DJBR_SAI0_DELAY0		(0x1 << 6)
#define DJBR_SAI0_DELAY_MASK	(0x3 << 6)
#define DJBR_SAI0_SAI_I2S		(0x0 << 4)
#define DJBR_SAI0_SAI_TDM2		(0x1 << 4)
#define DJBR_SAI0_SAI_TDM4		(0x2 << 4)
#define DJBR_SAI0_SAI_TDM8		(0x3 << 4)
#define DJBR_SAI0_SAI_MASK		(0x3 << 4)
#define DJBR_SAI0_FS_48		0x0
#define DJBR_SAI0_FS_8		0x1
#define DJBR_SAI0_FS_12		0x2
#define DJBR_SAI0_FS_16		0x3
#define DJBR_SAI0_FS_24		0x4
#define DJBR_SAI0_FS_32		0x5
#define DJBR_SAI0_FS_96		0x6
#define DJBR_SAI0_FS_192		0x7
#define DJBR_SAI0_FS_MASK		0xf

#define DJBR_SAI1_TDM_TS		BIT(7)
#define DJBR_SAI1_BCLK_TDMC		BIT(6)
#define DJBR_SAI1_LR_MODE		BIT(5)
#define DJBR_SAI1_LR_POL		BIT(4)
#define DJBR_SAI1_BCLKRATE		BIT(2)
#define DJBR_SAI1_BCLKEDGE		BIT(1)
#define DJBR_SAI1_MS		BIT(0)

static const unsigned int djbr_rates[] = {
  [DJBR_SAI0_FS_8] = 8000,
  [DJBR_SAI0_FS_12] = 12000,
  [DJBR_SAI0_FS_16] = 16000,
  [DJBR_SAI0_FS_24] = 24000,
  [DJBR_SAI0_FS_32] = 32000,
  [DJBR_SAI0_FS_48] = 48000,
  [DJBR_SAI0_FS_96] = 96000,
  [DJBR_SAI0_FS_192] = 192000,
};

/* 8k, 12k, 24k, 48k */
#define DJBR_RATE_MASK_TDM8 0x17
/* + 16k, 96k */
#define DJBR_RATE_MASK_TDM4 (DJBR_RATE_MASK_TDM8 | 0x48)
/* + 32k, 192k */
#define DJBR_RATE_MASK_TDM2 (DJBR_RATE_MASK_TDM4 | 0xa0)

static const DECLARE_TLV_DB_MINMAX(djbr_digital_tlv, -9563, 0);
static const DECLARE_TLV_DB_SCALE(djbr_pga_tlv, -1200, 75, 0);
static const DECLARE_TLV_DB_SCALE(djbr_pga_boost_tlv, 0, 1000, 0);

static const char * const djbr_bias_text[] = {
  "Normal operation", "Extreme power saving", "Enhanced performance",
  "Power saving",
};

static const unsigned int djbr_bias_adc_values[] = {
  0, 2, 3,
};

static const char * const djbr_bias_adc_text[] = {
  "Normal operation", "Enhanced performance", "Power saving",
};

static const char * const djbr_bias_dac_text[] = {
  "Normal operation", "Power saving", "Superior performance",
  "Enhanced performance",
};

static SOC_ENUM_SINGLE_DECL(djbr_bias_hp_enum,
    DJBR_REG_BIAS_CTRL0, 6, djbr_bias_text);
static SOC_ENUM_SINGLE_DECL(djbr_bias_afe0_1_enum,
    DJBR_REG_BIAS_CTRL0, 4, djbr_bias_text);
static SOC_VALUE_ENUM_SINGLE_DECL(djbr_bias_adc2_3_enum,
    DJBR_REG_BIAS_CTRL0, 2, 0x3, djbr_bias_adc_text,
    djbr_bias_adc_values);
static SOC_VALUE_ENUM_SINGLE_DECL(djbr_bias_adc0_1_enum,
    DJBR_REG_BIAS_CTRL0, 0, 0x3, djbr_bias_adc_text,
    djbr_bias_adc_values);
static SOC_ENUM_SINGLE_DECL(djbr_bias_afe2_3_enum,
    DJBR_REG_BIAS_CTRL1, 4, djbr_bias_text);
static SOC_ENUM_SINGLE_DECL(djbr_bias_mic_enum,
    DJBR_REG_BIAS_CTRL1, 2, djbr_bias_text);
static SOC_ENUM_SINGLE_DECL(djbr_bias_dac_enum,
    DJBR_REG_BIAS_CTRL1, 0, djbr_bias_dac_text);

static const char * const djbr_hpf_text[] = {
  "Off",
  "1 Hz",
  "4 Hz",
  "8 Hz",
};

static SOC_ENUM_SINGLE_DECL(djbr_hpf0_1_enum,
    DJBR_REG_ADC_CTRL2, 5, djbr_hpf_text);
static SOC_ENUM_SINGLE_DECL(djbr_hpf2_3_enum,
    DJBR_REG_ADC_CTRL3, 5, djbr_hpf_text);

static const struct snd_kcontrol_new djbr_controls[] = {
  SOC_SINGLE_TLV("DJBR ADC 0 Capture Volume", DJBR_REG_ADC_VOL(0),
      0, 0xff, 1, djbr_digital_tlv),
  SOC_SINGLE_TLV("DJBR ADC 1 Capture Volume", DJBR_REG_ADC_VOL(1),
      0, 0xff, 1, djbr_digital_tlv),
  SOC_SINGLE_TLV("DJBR ADC 2 Capture Volume", DJBR_REG_ADC_VOL(2),
      0, 0xff, 1, djbr_digital_tlv),
  SOC_SINGLE_TLV("DJBR ADC 3 Capture Volume", DJBR_REG_ADC_VOL(3),
      0, 0xff, 1, djbr_digital_tlv),
  SOC_SINGLE("DJBR ADC 0 Capture Switch", DJBR_REG_ADC_CTRL0, 3, 1, 1),
  SOC_SINGLE("DJBR ADC 1 Capture Switch", DJBR_REG_ADC_CTRL0, 4, 1, 1),
  SOC_SINGLE("DJBR ADC 2 Capture Switch", DJBR_REG_ADC_CTRL1, 3, 1, 1),
  SOC_SINGLE("DJBR ADC 3 Capture Switch", DJBR_REG_ADC_CTRL1, 4, 1, 1),

  SOC_ENUM("DJBR ADC 0+1 High-Pass-Filter", djbr_hpf0_1_enum),
  SOC_ENUM("DJBR ADC 2+3 High-Pass-Filter", djbr_hpf2_3_enum),

  SOC_SINGLE_TLV("DJBR PGA 0 Capture Volume", DJBR_REG_PGA_CTRL(0),
      0, 0x3f, 0, djbr_pga_tlv),
  SOC_SINGLE_TLV("DJBR PGA 1 Capture Volume", DJBR_REG_PGA_CTRL(1),
      0, 0x3f, 0, djbr_pga_tlv),
  SOC_SINGLE_TLV("DJBR PGA 2 Capture Volume", DJBR_REG_PGA_CTRL(2),
      0, 0x3f, 0, djbr_pga_tlv),
  SOC_SINGLE_TLV("DJBR PGA 3 Capture Volume", DJBR_REG_PGA_CTRL(3),
      0, 0x3f, 0, djbr_pga_tlv),
  SOC_SINGLE_TLV("DJBR PGA 0 Boost Capture Volume", DJBR_REG_PGA_BOOST,
      0, 1, 0, djbr_pga_boost_tlv),
  SOC_SINGLE_TLV("DJBR PGA 1 Boost Capture Volume", DJBR_REG_PGA_BOOST,
      1, 1, 0, djbr_pga_boost_tlv),
  SOC_SINGLE_TLV("DJBR PGA 2 Boost Capture Volume", DJBR_REG_PGA_BOOST,
      2, 1, 0, djbr_pga_boost_tlv),
  SOC_SINGLE_TLV("DJBR PGA 3 Boost Capture Volume", DJBR_REG_PGA_BOOST,
      3, 1, 0, djbr_pga_boost_tlv),
  SOC_SINGLE("DJBR PGA 0 Capture Switch", DJBR_REG_PGA_CTRL(0), 6, 1, 1),
  SOC_SINGLE("DJBR PGA 1 Capture Switch", DJBR_REG_PGA_CTRL(1), 6, 1, 1),
  SOC_SINGLE("DJBR PGA 2 Capture Switch", DJBR_REG_PGA_CTRL(2), 6, 1, 1),
  SOC_SINGLE("DJBR PGA 3 Capture Switch", DJBR_REG_PGA_CTRL(3), 6, 1, 1),

  SOC_SINGLE_TLV("DJBR DAC 0 Playback Volume", DJBR_REG_DAC_VOL(0),
      0, 0xff, 1, djbr_digital_tlv),
  SOC_SINGLE_TLV("DJBR DAC 1 Playback Volume", DJBR_REG_DAC_VOL(1),
      0, 0xff, 1, djbr_digital_tlv),
  SOC_SINGLE("DJBR DAC 0 Playback Switch", DJBR_REG_DAC_CTRL, 3, 1, 1),
  SOC_SINGLE("DJBR DAC 1 Playback Switch", DJBR_REG_DAC_CTRL, 4, 1, 1),

  SOC_ENUM("DJBR Headphone Bias", djbr_bias_hp_enum),
  SOC_ENUM("DJBR Microphone Bias", djbr_bias_hp_enum),
  SOC_ENUM("DJBR AFE 0+1 Bias", djbr_bias_afe0_1_enum),
  SOC_ENUM("DJBR AFE 2+3 Bias", djbr_bias_afe2_3_enum),
  SOC_ENUM("DJBR ADC 0+1 Bias", djbr_bias_adc0_1_enum),
  SOC_ENUM("DJBR ADC 2+3 Bias", djbr_bias_adc2_3_enum),
  SOC_ENUM("DJBR DAC 0+1 Bias", djbr_bias_dac_enum),
};

static const char * const djbr_decimator_mux_text[] = {
  "ADC",
  "DMIC",
};

static SOC_ENUM_SINGLE_DECL(djbr_decimator0_1_mux_enum,
    DJBR_REG_ADC_CTRL2, 2, djbr_decimator_mux_text);

static const struct snd_kcontrol_new djbr_decimator0_1_mux_control =
SOC_DAPM_ENUM("DJBR Decimator 0+1 Capture Mux",
    djbr_decimator0_1_mux_enum);

static SOC_ENUM_SINGLE_DECL(djbr_decimator2_3_mux_enum,
    DJBR_REG_ADC_CTRL3, 2, djbr_decimator_mux_text);

static const struct snd_kcontrol_new djbr_decimator2_3_mux_control =
SOC_DAPM_ENUM("DJBR Decimator 2+3 Capture Mux",
    djbr_decimator2_3_mux_enum);

static const unsigned int djbr_asrco_mux_values[] = {
  4, 5, 6, 7,
};

static const char * const djbr_asrco_mux_text[] = {
  "Decimator0",
  "Decimator1",
  "Decimator2",
  "Decimator3",
};

static SOC_VALUE_ENUM_SINGLE_DECL(djbr_asrco0_mux_enum,
    DJBR_REG_ASRCO_SOURCE_0_1, 0, 0xf, djbr_asrco_mux_text,
    djbr_asrco_mux_values);
static SOC_VALUE_ENUM_SINGLE_DECL(djbr_asrco1_mux_enum,
    DJBR_REG_ASRCO_SOURCE_0_1, 4, 0xf, djbr_asrco_mux_text,
    djbr_asrco_mux_values);
static SOC_VALUE_ENUM_SINGLE_DECL(djbr_asrco2_mux_enum,
    DJBR_REG_ASRCO_SOURCE_2_3, 0, 0xf, djbr_asrco_mux_text,
    djbr_asrco_mux_values);
static SOC_VALUE_ENUM_SINGLE_DECL(djbr_asrco3_mux_enum,
    DJBR_REG_ASRCO_SOURCE_2_3, 4, 0xf, djbr_asrco_mux_text,
    djbr_asrco_mux_values);

static const struct snd_kcontrol_new djbr_asrco0_mux_control =
SOC_DAPM_ENUM("DJBR Output ASRC0 Capture Mux", djbr_asrco0_mux_enum);
static const struct snd_kcontrol_new djbr_asrco1_mux_control =
SOC_DAPM_ENUM("DJBR Output ASRC1 Capture Mux", djbr_asrco1_mux_enum);
static const struct snd_kcontrol_new djbr_asrco2_mux_control =
SOC_DAPM_ENUM("DJBR Output ASRC2 Capture Mux", djbr_asrco2_mux_enum);
static const struct snd_kcontrol_new djbr_asrco3_mux_control =
SOC_DAPM_ENUM("DJBR Output ASRC3 Capture Mux", djbr_asrco3_mux_enum);

static const unsigned int djbr_sout_mux_values[] = {
  4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

static const char * const djbr_sout_mux_text[] = {
  "Output ASRC0",
  "Output ASRC1",
  "Output ASRC2",
  "Output ASRC3",
  "Serial Input 0",
  "Serial Input 1",
  "Serial Input 2",
  "Serial Input 3",
  "Serial Input 4",
  "Serial Input 5",
  "Serial Input 6",
  "Serial Input 7",
};

static SOC_VALUE_ENUM_SINGLE_DECL(djbr_sout0_mux_enum,
    DJBR_REG_SOUT_SOURCE_0_1, 0, 0xf, djbr_sout_mux_text,
    djbr_sout_mux_values);
static SOC_VALUE_ENUM_SINGLE_DECL(djbr_sout1_mux_enum,
    DJBR_REG_SOUT_SOURCE_0_1, 4, 0xf, djbr_sout_mux_text,
    djbr_sout_mux_values);
static SOC_VALUE_ENUM_SINGLE_DECL(djbr_sout2_mux_enum,
    DJBR_REG_SOUT_SOURCE_2_3, 0, 0xf, djbr_sout_mux_text,
    djbr_sout_mux_values);
static SOC_VALUE_ENUM_SINGLE_DECL(djbr_sout3_mux_enum,
    DJBR_REG_SOUT_SOURCE_2_3, 4, 0xf, djbr_sout_mux_text,
    djbr_sout_mux_values);
static SOC_VALUE_ENUM_SINGLE_DECL(djbr_sout4_mux_enum,
    DJBR_REG_SOUT_SOURCE_4_5, 0, 0xf, djbr_sout_mux_text,
    djbr_sout_mux_values);
static SOC_VALUE_ENUM_SINGLE_DECL(djbr_sout5_mux_enum,
    DJBR_REG_SOUT_SOURCE_4_5, 4, 0xf, djbr_sout_mux_text,
    djbr_sout_mux_values);
static SOC_VALUE_ENUM_SINGLE_DECL(djbr_sout6_mux_enum,
    DJBR_REG_SOUT_SOURCE_6_7, 0, 0xf, djbr_sout_mux_text,
    djbr_sout_mux_values);
static SOC_VALUE_ENUM_SINGLE_DECL(djbr_sout7_mux_enum,
    DJBR_REG_SOUT_SOURCE_6_7, 4, 0xf, djbr_sout_mux_text,
    djbr_sout_mux_values);

static const struct snd_kcontrol_new djbr_sout0_mux_control =
SOC_DAPM_ENUM("DJBR Serial Output 0 Capture Mux", djbr_sout0_mux_enum);
static const struct snd_kcontrol_new djbr_sout1_mux_control =
SOC_DAPM_ENUM("DJBR Serial Output 1 Capture Mux", djbr_sout1_mux_enum);
static const struct snd_kcontrol_new djbr_sout2_mux_control =
SOC_DAPM_ENUM("DJBR Serial Output 2 Capture Mux", djbr_sout2_mux_enum);
static const struct snd_kcontrol_new djbr_sout3_mux_control =
SOC_DAPM_ENUM("DJBR Serial Output 3 Capture Mux", djbr_sout3_mux_enum);
static const struct snd_kcontrol_new djbr_sout4_mux_control =
SOC_DAPM_ENUM("DJBR Serial Output 4 Capture Mux", djbr_sout4_mux_enum);
static const struct snd_kcontrol_new djbr_sout5_mux_control =
SOC_DAPM_ENUM("DJBR Serial Output 5 Capture Mux", djbr_sout5_mux_enum);
static const struct snd_kcontrol_new djbr_sout6_mux_control =
SOC_DAPM_ENUM("DJBR Serial Output 6 Capture Mux", djbr_sout6_mux_enum);
static const struct snd_kcontrol_new djbr_sout7_mux_control =
SOC_DAPM_ENUM("DJBR Serial Output 7 Capture Mux", djbr_sout7_mux_enum);

static const char * const djbr_asrci_mux_text[] = {
  "Serial Input 0+1",
  "Serial Input 2+3",
  "Serial Input 4+5",
  "Serial Input 6+7",
};

static SOC_ENUM_SINGLE_DECL(djbr_asrci_mux_enum,
    DJBR_REG_ASRC_MODE, 2, djbr_asrci_mux_text);

static const struct snd_kcontrol_new djbr_asrci_mux_control =
SOC_DAPM_ENUM("DJBR Input ASRC Playback Mux", djbr_asrci_mux_enum);

static const unsigned int djbr_dac_mux_values[] = {
  12, 13
};

static const char * const djbr_dac_mux_text[] = {
  "Input ASRC0",
  "Input ASRC1",
};

static SOC_VALUE_ENUM_SINGLE_DECL(djbr_dac0_mux_enum,
    DJBR_REG_DAC_SOURCE, 0, 0xf, djbr_dac_mux_text,
    djbr_dac_mux_values);
static SOC_VALUE_ENUM_SINGLE_DECL(djbr_dac1_mux_enum,
    DJBR_REG_DAC_SOURCE, 4, 0xf, djbr_dac_mux_text,
    djbr_dac_mux_values);

static const struct snd_kcontrol_new djbr_dac0_mux_control =
SOC_DAPM_ENUM("DJBR DAC 0 Playback Mux", djbr_dac0_mux_enum);
static const struct snd_kcontrol_new djbr_dac1_mux_control =
SOC_DAPM_ENUM("DJBR DAC 1 Playback Mux", djbr_dac1_mux_enum);

//End ALSA controls, begin DAPM widgets

static const struct snd_soc_dapm_widget djbr_dapm_widgets[] = {
  SND_SOC_DAPM_INPUT("AIN0"),
  SND_SOC_DAPM_INPUT("AIN1"),
  SND_SOC_DAPM_INPUT("AIN2"),
  SND_SOC_DAPM_INPUT("AIN3"),
  SND_SOC_DAPM_INPUT("DMIC0_1"),
  SND_SOC_DAPM_INPUT("DMIC2_3"),

  SND_SOC_DAPM_SUPPLY("MICBIAS0", DJBR_REG_MICBIAS, 4, 0, NULL, 0),
  SND_SOC_DAPM_SUPPLY("MICBIAS1", DJBR_REG_MICBIAS, 5, 0, NULL, 0),

  SND_SOC_DAPM_PGA("PGA0", DJBR_REG_PGA_CTRL(0), 7, 0, NULL, 0),
  SND_SOC_DAPM_PGA("PGA1", DJBR_REG_PGA_CTRL(1), 7, 0, NULL, 0),
  SND_SOC_DAPM_PGA("PGA2", DJBR_REG_PGA_CTRL(2), 7, 0, NULL, 0),
  SND_SOC_DAPM_PGA("PGA3", DJBR_REG_PGA_CTRL(3), 7, 0, NULL, 0),
  SND_SOC_DAPM_ADC("ADC0", NULL, DJBR_REG_ADC_CTRL2, 0, 0),
  SND_SOC_DAPM_ADC("ADC1", NULL, DJBR_REG_ADC_CTRL2, 1, 0),
  SND_SOC_DAPM_ADC("ADC2", NULL, DJBR_REG_ADC_CTRL3, 0, 0),
  SND_SOC_DAPM_ADC("ADC3", NULL, DJBR_REG_ADC_CTRL3, 1, 0),

  SND_SOC_DAPM_SUPPLY("ADC0 Filter", DJBR_REG_DECIM_PWR, 0, 0,	NULL, 0),
  SND_SOC_DAPM_SUPPLY("ADC1 Filter", DJBR_REG_DECIM_PWR, 1, 0,	NULL, 0),
  SND_SOC_DAPM_SUPPLY("ADC2 Filter", DJBR_REG_DECIM_PWR, 2, 0,	NULL, 0),
  SND_SOC_DAPM_SUPPLY("ADC3 Filter", DJBR_REG_DECIM_PWR, 3, 0,	NULL, 0),
  SND_SOC_DAPM_SUPPLY("Output ASRC0 Decimator", DJBR_REG_DECIM_PWR, 4, 0, NULL, 0),
  SND_SOC_DAPM_SUPPLY("Output ASRC1 Decimator", DJBR_REG_DECIM_PWR, 5, 0, NULL, 0),
  SND_SOC_DAPM_SUPPLY("Output ASRC2 Decimator", DJBR_REG_DECIM_PWR, 6, 0, NULL, 0),
  SND_SOC_DAPM_SUPPLY("Output ASRC3 Decimator", DJBR_REG_DECIM_PWR, 7, 0, NULL, 0),

  SND_SOC_DAPM_MUX("Decimator0 Mux", SND_SOC_NOPM, 0, 0,
      &djbr_decimator0_1_mux_control),
  SND_SOC_DAPM_MUX("Decimator1 Mux", SND_SOC_NOPM, 0, 0,
      &djbr_decimator0_1_mux_control),
  SND_SOC_DAPM_MUX("Decimator2 Mux", SND_SOC_NOPM, 0, 0,
      &djbr_decimator2_3_mux_control),
  SND_SOC_DAPM_MUX("Decimator3 Mux", SND_SOC_NOPM, 0, 0,
      &djbr_decimator2_3_mux_control),

  SND_SOC_DAPM_MUX("DJBR Output ASRC0 Mux", SND_SOC_NOPM, 0, 0,
      &djbr_asrco0_mux_control),
  SND_SOC_DAPM_MUX("DJBR Output ASRC1 Mux", SND_SOC_NOPM, 0, 0,
      &djbr_asrco1_mux_control),
  SND_SOC_DAPM_MUX("DJBR Output ASRC2 Mux", SND_SOC_NOPM, 0, 0,
      &djbr_asrco2_mux_control),
  SND_SOC_DAPM_MUX("DJBR Output ASRC3 Mux", SND_SOC_NOPM, 0, 0,
      &djbr_asrco3_mux_control),
  SND_SOC_DAPM_MUX("DJBR Serial Output 0 Capture Mux", SND_SOC_NOPM, 0, 0,
      &djbr_sout0_mux_control),
  SND_SOC_DAPM_MUX("DJBR Serial Output 1 Capture Mux", SND_SOC_NOPM, 0, 0,
      &djbr_sout1_mux_control),
  SND_SOC_DAPM_MUX("DJBR Serial Output 2 Capture Mux", SND_SOC_NOPM, 0, 0,
      &djbr_sout2_mux_control),
  SND_SOC_DAPM_MUX("DJBR Serial Output 3 Capture Mux", SND_SOC_NOPM, 0, 0,
      &djbr_sout3_mux_control),
  SND_SOC_DAPM_MUX("DJBR Serial Output 4 Capture Mux", SND_SOC_NOPM, 0, 0,
      &djbr_sout4_mux_control),
  SND_SOC_DAPM_MUX("DJBR Serial Output 5 Capture Mux", SND_SOC_NOPM, 0, 0,
      &djbr_sout5_mux_control),
  SND_SOC_DAPM_MUX("DJBR Serial Output 6 Capture Mux", SND_SOC_NOPM, 0, 0,
      &djbr_sout6_mux_control),
  SND_SOC_DAPM_MUX("DJBR Serial Output 7 Capture Mux", SND_SOC_NOPM, 0, 0,
      &djbr_sout7_mux_control),

  SND_SOC_DAPM_AIF_IN("Serial Input 0", NULL, 0, SND_SOC_NOPM, 0, 0),
  SND_SOC_DAPM_AIF_IN("Serial Input 1", NULL, 1, SND_SOC_NOPM, 0, 0),
  SND_SOC_DAPM_AIF_IN("Serial Input 2", NULL, 2, SND_SOC_NOPM, 0, 0),
  SND_SOC_DAPM_AIF_IN("Serial Input 3", NULL, 3, SND_SOC_NOPM, 0, 0),
  SND_SOC_DAPM_AIF_IN("Serial Input 4", NULL, 4, SND_SOC_NOPM, 0, 0),
  SND_SOC_DAPM_AIF_IN("Serial Input 5", NULL, 5, SND_SOC_NOPM, 0, 0),
  SND_SOC_DAPM_AIF_IN("Serial Input 6", NULL, 6, SND_SOC_NOPM, 0, 0),
  SND_SOC_DAPM_AIF_IN("Serial Input 7", NULL, 7, SND_SOC_NOPM, 0, 0),

  SND_SOC_DAPM_AIF_OUT("Serial Output 0", NULL, 0, SND_SOC_NOPM, 0, 0),
  SND_SOC_DAPM_AIF_OUT("Serial Output 1", NULL, 1, SND_SOC_NOPM, 0, 0),
  SND_SOC_DAPM_AIF_OUT("Serial Output 2", NULL, 2, SND_SOC_NOPM, 0, 0),
  SND_SOC_DAPM_AIF_OUT("Serial Output 3", NULL, 3, SND_SOC_NOPM, 0, 0),
  SND_SOC_DAPM_AIF_OUT("Serial Output 4", NULL, 4, SND_SOC_NOPM, 0, 0),
  SND_SOC_DAPM_AIF_OUT("Serial Output 5", NULL, 5, SND_SOC_NOPM, 0, 0),
  SND_SOC_DAPM_AIF_OUT("Serial Output 6", NULL, 6, SND_SOC_NOPM, 0, 0),
  SND_SOC_DAPM_AIF_OUT("Serial Output 7", NULL, 7, SND_SOC_NOPM, 0, 0),

  SND_SOC_DAPM_SUPPLY("Output ASRC Supply", DJBR_REG_ASRC_MODE, 1, 0,
      NULL, 0),
  SND_SOC_DAPM_SUPPLY("Input ASRC Supply", DJBR_REG_ASRC_MODE, 0, 0,
      NULL, 0),

  SND_SOC_DAPM_SUPPLY("DAC1 Modulator", DJBR_REG_INTERP_PWR,
      3, 0, NULL, 0),
  SND_SOC_DAPM_SUPPLY("DAC0 Modulator", DJBR_REG_INTERP_PWR,
      2, 0, NULL, 0),
  SND_SOC_DAPM_SUPPLY("Input ASRC1 Interpolator", DJBR_REG_INTERP_PWR,
      1, 0, NULL, 0),
  SND_SOC_DAPM_SUPPLY("Input ASRC0 Interpolator", DJBR_REG_INTERP_PWR,
      0, 0, NULL, 0),

  SND_SOC_DAPM_MUX("Input ASRC0 Mux", SND_SOC_NOPM, 0, 0,
      &djbr_asrci_mux_control),
  SND_SOC_DAPM_MUX("Input ASRC1 Mux", SND_SOC_NOPM, 0, 0,
      &djbr_asrci_mux_control),

  SND_SOC_DAPM_MUX("DJBR DAC 0 Mux", SND_SOC_NOPM, 0, 0,
      &djbr_dac0_mux_control),
  SND_SOC_DAPM_MUX("DJBR DAC 1 Mux", SND_SOC_NOPM, 0, 0,
      &djbr_dac1_mux_control),

  SND_SOC_DAPM_DAC("DAC0", NULL, DJBR_REG_DAC_CTRL, 0, 0),
  SND_SOC_DAPM_DAC("DAC1", NULL, DJBR_REG_DAC_CTRL, 1, 0),

  SND_SOC_DAPM_OUT_DRV("OP_STAGE_LP", DJBR_REG_OP_STAGE_CTRL, 0, 1,
      NULL, 0),
  SND_SOC_DAPM_OUT_DRV("OP_STAGE_LN", DJBR_REG_OP_STAGE_CTRL, 1, 1,
      NULL, 0),
  SND_SOC_DAPM_OUT_DRV("OP_STAGE_RP", DJBR_REG_OP_STAGE_CTRL, 2, 1,
      NULL, 0),
  SND_SOC_DAPM_OUT_DRV("OP_STAGE_RN", DJBR_REG_OP_STAGE_CTRL, 3, 1,
      NULL, 0),

  SND_SOC_DAPM_OUTPUT("HPOUTL"),
  SND_SOC_DAPM_OUTPUT("HPOUTR"),
};

#define DJBR_SOUT_ROUTES(x) \
{ "DJBR Serial Output " #x " Capture Mux", "Output ASRC0", "DJBR Output ASRC0 Mux" }, \
{ "DJBR Serial Output " #x " Capture Mux", "Output ASRC1", "DJBR Output ASRC1 Mux" }, \
{ "DJBR Serial Output " #x " Capture Mux", "Output ASRC2", "DJBR Output ASRC2 Mux" }, \
{ "DJBR Serial Output " #x " Capture Mux", "Output ASRC3", "DJBR Output ASRC3 Mux" }, \
{ "DJBR Serial Output " #x " Capture Mux", "Serial Input 0", "Serial Input 0" }, \
{ "DJBR Serial Output " #x " Capture Mux", "Serial Input 1", "Serial Input 1" }, \
{ "DJBR Serial Output " #x " Capture Mux", "Serial Input 2", "Serial Input 2" }, \
{ "DJBR Serial Output " #x " Capture Mux", "Serial Input 3", "Serial Input 3" }, \
{ "DJBR Serial Output " #x " Capture Mux", "Serial Input 4", "Serial Input 4" }, \
{ "DJBR Serial Output " #x " Capture Mux", "Serial Input 5", "Serial Input 5" }, \
{ "DJBR Serial Output " #x " Capture Mux", "Serial Input 6", "Serial Input 6" }, \
{ "DJBR Serial Output " #x " Capture Mux", "Serial Input 7", "Serial Input 7" }, \
{ "Serial Output " #x, NULL, "DJBR Serial Output " #x " Capture Mux" }, \
{ "Capture", NULL, "Serial Output " #x }

#define DJBR_ASRCO_ROUTES(x) \
{ "DJBR Output ASRC" #x " Mux", "Decimator0", "Decimator0 Mux" }, \
{ "DJBR Output ASRC" #x " Mux", "Decimator1", "Decimator1 Mux" }, \
{ "DJBR Output ASRC" #x " Mux", "Decimator2", "Decimator2 Mux" }, \
{ "DJBR Output ASRC" #x " Mux", "Decimator3", "Decimator3 Mux" }

static const struct snd_soc_dapm_route djbr_dapm_routes[] = {
  { "PGA0", NULL, "AIN0" },
  { "PGA1", NULL, "AIN1" },
  { "PGA2", NULL, "AIN2" },
  { "PGA3", NULL, "AIN3" },

  { "ADC0", NULL, "PGA0" },
  { "ADC1", NULL, "PGA1" },
  { "ADC2", NULL, "PGA2" },
  { "ADC3", NULL, "PGA3" },

  { "Decimator0 Mux", "ADC", "ADC0" },
  { "Decimator1 Mux", "ADC", "ADC1" },
  { "Decimator2 Mux", "ADC", "ADC2" },
  { "Decimator3 Mux", "ADC", "ADC3" },

  { "Decimator0 Mux", "DMIC", "DMIC0_1" },
  { "Decimator1 Mux", "DMIC", "DMIC0_1" },
  { "Decimator2 Mux", "DMIC", "DMIC2_3" },
  { "Decimator3 Mux", "DMIC", "DMIC2_3" },

  { "Decimator0 Mux", NULL, "ADC0 Filter" },
  { "Decimator1 Mux", NULL, "ADC1 Filter" },
  { "Decimator2 Mux", NULL, "ADC2 Filter" },
  { "Decimator3 Mux", NULL, "ADC3 Filter" },

  { "DJBR Output ASRC0 Mux", NULL, "Output ASRC Supply" },
  { "DJBR Output ASRC1 Mux", NULL, "Output ASRC Supply" },
  { "DJBR Output ASRC2 Mux", NULL, "Output ASRC Supply" },
  { "DJBR Output ASRC3 Mux", NULL, "Output ASRC Supply" },
  { "DJBR Output ASRC0 Mux", NULL, "Output ASRC0 Decimator" },
  { "DJBR Output ASRC1 Mux", NULL, "Output ASRC1 Decimator" },
  { "DJBR Output ASRC2 Mux", NULL, "Output ASRC2 Decimator" },
  { "DJBR Output ASRC3 Mux", NULL, "Output ASRC3 Decimator" },

  DJBR_ASRCO_ROUTES(0),
  DJBR_ASRCO_ROUTES(1),
  DJBR_ASRCO_ROUTES(2),
  DJBR_ASRCO_ROUTES(3),

  DJBR_SOUT_ROUTES(0),
  DJBR_SOUT_ROUTES(1),
  DJBR_SOUT_ROUTES(2),
  DJBR_SOUT_ROUTES(3),
  DJBR_SOUT_ROUTES(4),
  DJBR_SOUT_ROUTES(5),
  DJBR_SOUT_ROUTES(6),
  DJBR_SOUT_ROUTES(7),

  { "Serial Input 0", NULL, "Playback" },
  { "Serial Input 1", NULL, "Playback" },
  { "Serial Input 2", NULL, "Playback" },
  { "Serial Input 3", NULL, "Playback" },
  { "Serial Input 4", NULL, "Playback" },
  { "Serial Input 5", NULL, "Playback" },
  { "Serial Input 6", NULL, "Playback" },
  { "Serial Input 7", NULL, "Playback" },

  { "Input ASRC0 Mux", "Serial Input 0+1", "Serial Input 0" },
  { "Input ASRC1 Mux", "Serial Input 0+1", "Serial Input 1" },
  { "Input ASRC0 Mux", "Serial Input 2+3", "Serial Input 2" },
  { "Input ASRC1 Mux", "Serial Input 2+3", "Serial Input 3" },
  { "Input ASRC0 Mux", "Serial Input 4+5", "Serial Input 4" },
  { "Input ASRC1 Mux", "Serial Input 4+5", "Serial Input 5" },
  { "Input ASRC0 Mux", "Serial Input 6+7", "Serial Input 6" },
  { "Input ASRC1 Mux", "Serial Input 6+7", "Serial Input 7" },
  { "Input ASRC0 Mux", NULL, "Input ASRC Supply" },
  { "Input ASRC1 Mux", NULL, "Input ASRC Supply" },
  { "Input ASRC0 Mux", NULL, "Input ASRC0 Interpolator" },
  { "Input ASRC1 Mux", NULL, "Input ASRC1 Interpolator" },

  { "DJBR DAC 0 Mux", "Input ASRC0", "Input ASRC0 Mux" },
  { "DJBR DAC 0 Mux", "Input ASRC1", "Input ASRC1 Mux" },
  { "DJBR DAC 1 Mux", "Input ASRC0", "Input ASRC0 Mux" },
  { "DJBR DAC 1 Mux", "Input ASRC1", "Input ASRC1 Mux" },

  { "DAC0", NULL, "DJBR DAC 0 Mux" },
  { "DAC1", NULL, "DJBR DAC 1 Mux" },
  { "DAC0", NULL, "DAC0 Modulator" },
  { "DAC1", NULL, "DAC1 Modulator" },

  { "OP_STAGE_LP", NULL, "DAC0" },
  { "OP_STAGE_LN", NULL, "DAC0" },
  { "OP_STAGE_RP", NULL, "DAC1" },
  { "OP_STAGE_RN", NULL, "DAC1" },

  { "HPOUTL", NULL, "OP_STAGE_LP" },
  { "HPOUTL", NULL, "OP_STAGE_LN" },
  { "HPOUTR", NULL, "OP_STAGE_RP" },
  { "HPOUTR", NULL, "OP_STAGE_RN" },
};

static int djbr_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
  struct djbr *djbr = snd_soc_dai_get_drvdata(dai);
  unsigned int sai0 = 0, sai1 = 0;
  bool invert_lrclk = false;

  switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
    case SND_SOC_DAIFMT_CBM_CFM:
      sai1 |= DJBR_SAI1_MS;
      break;
    case SND_SOC_DAIFMT_CBS_CFS:
      break;
    default:
      return -EINVAL;
  }

  switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
    case SND_SOC_DAIFMT_NB_NF:
      invert_lrclk = false;
      break;
    case SND_SOC_DAIFMT_NB_IF:
      invert_lrclk = true;
      break;
    case SND_SOC_DAIFMT_IB_NF:
      invert_lrclk = false;
      sai1 |= DJBR_SAI1_BCLKEDGE;
      break;
    case SND_SOC_DAIFMT_IB_IF:
      invert_lrclk = true;
      sai1 |= DJBR_SAI1_BCLKEDGE;
      break;
  }

  switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
    case SND_SOC_DAIFMT_I2S:
      sai0 |= DJBR_SAI0_DELAY1;
      break;
    case SND_SOC_DAIFMT_LEFT_J:
      sai0 |= DJBR_SAI0_DELAY0;
      invert_lrclk = !invert_lrclk;
      break;
    case SND_SOC_DAIFMT_DSP_A:
      sai0 |= DJBR_SAI0_DELAY1;
      sai1 |= DJBR_SAI1_LR_MODE;
      break;
    case SND_SOC_DAIFMT_DSP_B:
      sai0 |= DJBR_SAI0_DELAY0;
      sai1 |= DJBR_SAI1_LR_MODE;
      break;
  }

  if (invert_lrclk)
    sai1 |= DJBR_SAI1_LR_POL;

  regmap_update_bits(djbr->regmap, DJBR_REG_SAI0,
      DJBR_SAI0_DELAY_MASK, sai0);
  regmap_update_bits(djbr->regmap, DJBR_REG_SAI1,
      DJBR_SAI1_MS | DJBR_SAI1_BCLKEDGE |
      DJBR_SAI1_LR_MODE | DJBR_SAI1_LR_POL, sai1);

  return 0;
}

static int djbr_hw_params(struct snd_pcm_substream *substream,
    struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
  struct djbr *djbr = snd_soc_dai_get_drvdata(dai);
  unsigned int rate = params_rate(params);
  unsigned int slot_width;
  unsigned int sai0, sai1;
  unsigned int i;

  for (i = 0; i < ARRAY_SIZE(djbr_rates); i++) {
    if (rate == djbr_rates[i])
      break;
  }

  if (i == ARRAY_SIZE(djbr_rates))
    return -EINVAL;

  sai0 = i;

  slot_width = djbr->slot_width;
  if (slot_width == 0)
    slot_width = params_width(params);

  switch (slot_width) {
    case 16:
      sai1 = DJBR_SAI1_BCLKRATE;
      break;
    case 32:
      sai1 = 0;
      break;
    default:
      return -EINVAL;
  }

  regmap_update_bits(djbr->regmap, DJBR_REG_SAI0,
      DJBR_SAI0_FS_MASK, sai0);
  regmap_update_bits(djbr->regmap, DJBR_REG_SAI1,
      DJBR_SAI1_BCLKRATE, sai1);

  return 0;
}

static int djbr_set_tdm_slot(struct snd_soc_dai *dai, unsigned int tx_mask,
    unsigned int rx_mask, int slots, int width)
{
  struct djbr *djbr = snd_soc_dai_get_drvdata(dai);
  unsigned int sai0, sai1;

  /* I2S mode */
  if (slots == 0) {
    /* The other settings dont matter in I2S mode */
    regmap_update_bits(djbr->regmap, DJBR_REG_SAI0,
        DJBR_SAI0_SAI_MASK, DJBR_SAI0_SAI_I2S);
    djbr->rate_constraints.mask = DJBR_RATE_MASK_TDM2;
    djbr->slot_width = 0;
    return 0;
  }

  /* We have 8 channels anything outside that is not supported */
  if ((tx_mask & ~0xff) != 0 || (rx_mask & ~0xff) != 0)
    return -EINVAL;

  switch (width) {
    case 16:
      sai1 = DJBR_SAI1_BCLK_TDMC;
      break;
    case 32:
      sai1 = 0;
      break;
    default:
      return -EINVAL;
  }

  switch (slots) {
    case 2:
      sai0 = DJBR_SAI0_SAI_TDM2;
      djbr->rate_constraints.mask = DJBR_RATE_MASK_TDM2;
      break;
    case 4:
      sai0 = DJBR_SAI0_SAI_TDM4;
      djbr->rate_constraints.mask = DJBR_RATE_MASK_TDM4;
      break;
    case 8:
      sai0 = DJBR_SAI0_SAI_TDM8;
      djbr->rate_constraints.mask = DJBR_RATE_MASK_TDM8;
      break;
    default:
      return -EINVAL;
  }

  djbr->slot_width = width;

  regmap_update_bits(djbr->regmap, DJBR_REG_SAI0,
      DJBR_SAI0_SAI_MASK, sai0);
  regmap_update_bits(djbr->regmap, DJBR_REG_SAI1,
      DJBR_SAI1_BCLK_TDMC, sai1);

  /* Mask is inverted in hardware */
  regmap_write(djbr->regmap, DJBR_REG_SOUT_CTRL, ~tx_mask);

  return 0;
}

static int djbr_set_tristate(struct snd_soc_dai *dai, int tristate)
{
  struct djbr *djbr = snd_soc_dai_get_drvdata(dai);
  unsigned int sai1;

  if (tristate)
    sai1 = DJBR_SAI1_TDM_TS;
  else
    sai1 = 0;

  return regmap_update_bits(djbr->regmap, DJBR_REG_SAI1,
      DJBR_SAI1_TDM_TS, sai1);
}

static int djbr_startup(struct snd_pcm_substream *substream,
    struct snd_soc_dai *dai)
{
  struct djbr *djbr = snd_soc_dai_get_drvdata(dai);

  snd_pcm_hw_constraint_list(substream->runtime, 0,
      SNDRV_PCM_HW_PARAM_RATE, &djbr->rate_constraints);

  return 0;
}

static void djbr_enable_pll(struct djbr *djbr)
{
  unsigned int val, timeout = 0;
  int ret;

  regmap_update_bits(djbr->regmap,
      DJBR_REG_CLK_CTRL, DJBR_CLK_CTRL_PLL_EN,
      DJBR_CLK_CTRL_PLL_EN);
  do {
    /* Takes about 1ms to lock */
    usleep_range(1000, 2000);
    ret = regmap_read(djbr->regmap, DJBR_REG_PLL(5), &val);
    if (ret)
      break;
    timeout++;
  } while (!(val & 1) && timeout < 3);

  if (ret < 0 || !(val & 1)) {
    dev_err(djbr->dev, "Failed to lock PLL\n");
  }
}

static void djbr_set_power(struct djbr *djbr, bool enable)
{
  if (djbr->enabled == enable)
    return;

  if (enable) {
    unsigned int clk_ctrl = DJBR_CLK_CTRL_MCLK_EN;

    clk_prepare_enable(djbr->mclk);
    if (djbr->pd_gpio)
      gpiod_set_value(djbr->pd_gpio, 0);

    if (djbr->switch_mode)
      djbr->switch_mode(djbr->dev);

    regcache_cache_only(djbr->regmap, false);

    /*
    * Clocks needs to be enabled before any other register can be
    * accessed.
     */
    if (djbr->use_pll) {
      djbr_enable_pll(djbr);
      clk_ctrl |= DJBR_CLK_CTRL_CLKSRC;
    }

    regmap_update_bits(djbr->regmap, DJBR_REG_CLK_CTRL,
        DJBR_CLK_CTRL_MCLK_EN | DJBR_CLK_CTRL_CLKSRC,
        clk_ctrl);
    regcache_sync(djbr->regmap);
  } else {
    if (djbr->pd_gpio) {
      /*
      * This will turn everything off and reset the register
      * map. No need to do any register writes to manually
      * turn things off.
       */
      gpiod_set_value(djbr->pd_gpio, 1);
      regcache_mark_dirty(djbr->regmap);
    } else {
      regmap_update_bits(djbr->regmap,
          DJBR_REG_CLK_CTRL,
          DJBR_CLK_CTRL_MCLK_EN |
          DJBR_CLK_CTRL_PLL_EN, 0);
    }
    clk_disable_unprepare(djbr->mclk);
    regcache_cache_only(djbr->regmap, true);
  }

  djbr->enabled = enable;
}

static int djbr_set_bias_level(struct snd_soc_codec *codec,
    enum snd_soc_bias_level level)
{
  struct djbr *djbr = snd_soc_codec_get_drvdata(codec);

  switch (level) {
    case SND_SOC_BIAS_ON:
      break;
    case SND_SOC_BIAS_PREPARE:
      break;
    case SND_SOC_BIAS_STANDBY:
      djbr_set_power(djbr, true);
      break;
    case SND_SOC_BIAS_OFF:
      djbr_set_power(djbr, false);
      break;
  }

  return 0;
}

static struct snd_soc_codec_driver djbr_codec_driver = {
  //.probe =	djbr_probe,
  //.resume =	adau1373_resume,
  //.set_pll = adau1373_set_pll,
  .set_bias_level = djbr_set_bias_level,
  .idle_bias_off = true,

  .component_driver = {
    .controls		= djbr_controls,
    .num_controls		= ARRAY_SIZE(djbr_controls),
    .dapm_widgets		= djbr_dapm_widgets,
    .num_dapm_widgets	= ARRAY_SIZE(djbr_dapm_widgets),
    .dapm_routes		= djbr_dapm_routes,
    .num_dapm_routes	= ARRAY_SIZE(djbr_dapm_routes),
  },
};

static const struct snd_soc_dai_ops djbr_dai_ops = {
  .set_fmt = djbr_set_dai_fmt,
  .set_tdm_slot = djbr_set_tdm_slot,
  .set_tristate = djbr_set_tristate,
  .hw_params = djbr_hw_params,
  .startup = djbr_startup,
};

#define DJBR_FORMATS (SNDRV_PCM_FMTBIT_S16_LE |	SNDRV_PCM_FMTBIT_S32_LE)

static struct snd_soc_dai_driver djbr_dai_driver = {
  .name = "djbr",
  .playback = {
    .stream_name = "Playback",
    .channels_min = 2,
    .channels_max = 8,
    .rates = SNDRV_PCM_RATE_KNOT,
    .formats = DJBR_FORMATS,
    .sig_bits = 24,
  },
  .capture = {
    .stream_name = "Capture",
    .channels_min = 2,
    .channels_max = 8,
    .rates = SNDRV_PCM_RATE_KNOT,
    .formats = DJBR_FORMATS,
    .sig_bits = 24,
  },
  .ops = &djbr_dai_ops,
  .symmetric_rates = 1,
};

static int djbr_setup_pll(struct djbr *djbr, unsigned int rate)
{
  uint8_t regs[5];
  unsigned int i;
  int ret;

  ret = adau_calc_pll_cfg(rate, 49152000, regs);
  if (ret < 0)
    return ret;

  for (i = 0; i < ARRAY_SIZE(regs); i++)
    regmap_write(djbr->regmap, DJBR_REG_PLL(i), regs[i]);

  return 0;
}

int djbr_probe(struct device *dev, struct regmap *regmap,
    void (*switch_mode)(struct device *dev))
{
  struct djbr *djbr;
  unsigned int clk_ctrl;
  unsigned long rate;
  int ret;

  if (IS_ERR(regmap)) {
    return PTR_ERR(regmap);
  }

  djbr = devm_kzalloc(dev, sizeof(*djbr), GFP_KERNEL);
  if (!djbr) {
    return -ENOMEM;
  }

  djbr->clk = devm_clk_get(dev, "mclk");
  if (IS_ERR(djbr->clk)) {
    ret =  PTR_ERR(djbr->clk);
    return ret;
  }

  djbr->pd_gpio = devm_gpiod_get_optional(dev, "powerdown",
      GPIOD_OUT_HIGH);
  if (IS_ERR(djbr->pd_gpio)) {
    ret = PTR_ERR(djbr->pd_gpio);
    return ret;
  }

  djbr->regmap = regmap;
  djbr->switch_mode = switch_mode;
  djbr->dev = dev;
  djbr->rate_constraints.list = djbr_rates;
  djbr->rate_constraints.count = ARRAY_SIZE(djbr_rates);
  djbr->rate_constraints.mask = DJBR_RATE_MASK_TDM2;

  dev_set_drvdata(dev, djbr);

  /*
  * The datasheet says that the internal MCLK always needs to run at
  * 12.288MHz. Automatically choose a valid configuration from the
  * external clock.
   */
  rate = clk_get_rate(djbr->clk);
  switch (rate) {
    case 12288000:
      clk_ctrl = DJBR_CLK_CTRL_CC_MDIV;
      break;
    case 24576000:
      clk_ctrl = 0;
      break;
    default:
      clk_ctrl = 0;
      ret = djbr_setup_pll(djbr, rate);
      if (ret < 0) {
        return ret;
      }
      djbr->use_pll = true;
      break;
  }

  /*
  * Most of the registers are inaccessible unless the internal clock is
  * enabled.
   */
  regcache_cache_only(regmap, true);

  regmap_update_bits(regmap, DJBR_REG_CLK_CTRL,
      DJBR_CLK_CTRL_CC_MDIV, clk_ctrl);

  /*
  * No pinctrl support yet, put the multi-purpose pins in the most
  * sensible mode for general purpose CODEC operation.
   */
  regmap_write(regmap, DJBR_REG_MODE_MP(1), 0x00); /* SDATA OUT */
  regmap_write(regmap, DJBR_REG_MODE_MP(6), 0x12); /* CLOCKOUT */

  regmap_write(regmap, DJBR_REG_OP_STAGE_MUTE, 0x0);

  regmap_write(regmap, 0x7, 0x01); /* CLOCK OUT */

  ret = snd_soc_register_codec(dev, &djbr_codec_driver,
      &djbr_dai_driver, 1);

  dev_info(dev, "device loaded\n");
  return ret;
}
EXPORT_SYMBOL(djbr_probe);

int djbr_remove(struct device *dev)
{
  snd_soc_unregister_codec(dev);
  return 0;
}
EXPORT_SYMBOL(djbr_remove);

static const struct reg_default djbr_reg_defaults[] = {
  { DJBR_REG_CLK_CTRL,		0x00 },
  { DJBR_REG_PLL(0),			0x00 },
  { DJBR_REG_PLL(1),			0x00 },
  { DJBR_REG_PLL(2),			0x00 },
  { DJBR_REG_PLL(3),			0x00 },
  { DJBR_REG_PLL(4),			0x00 },
  { DJBR_REG_PLL(5),			0x00 },
  { DJBR_REG_DAC_SOURCE,		0x10 },
  { DJBR_REG_SOUT_SOURCE_0_1,		0x54 },
  { DJBR_REG_SOUT_SOURCE_2_3,		0x76 },
  { DJBR_REG_SOUT_SOURCE_4_5,		0x54 },
  { DJBR_REG_SOUT_SOURCE_6_7,		0x76 },
  { DJBR_REG_ADC_SDATA_CH,		0x04 },
  { DJBR_REG_ASRCO_SOURCE_0_1,	0x10 },
  { DJBR_REG_ASRCO_SOURCE_2_3,	0x32 },
  { DJBR_REG_ASRC_MODE,		0x00 },
  { DJBR_REG_ADC_CTRL0,		0x19 },
  { DJBR_REG_ADC_CTRL1,		0x19 },
  { DJBR_REG_ADC_CTRL2,		0x00 },
  { DJBR_REG_ADC_CTRL3,		0x00 },
  { DJBR_REG_ADC_VOL(0),		0x00 },
  { DJBR_REG_ADC_VOL(1),		0x00 },
  { DJBR_REG_ADC_VOL(2),		0x00 },
  { DJBR_REG_ADC_VOL(3),		0x00 },
  { DJBR_REG_PGA_CTRL(0),		0x40 },
  { DJBR_REG_PGA_CTRL(1),		0x40 },
  { DJBR_REG_PGA_CTRL(2),		0x40 },
  { DJBR_REG_PGA_CTRL(3),		0x40 },
  { DJBR_REG_PGA_BOOST,		0x00 },
  { DJBR_REG_MICBIAS,			0x00 },
  { DJBR_REG_DAC_CTRL,		0x18 },
  { DJBR_REG_DAC_VOL(0),		0x00 },
  { DJBR_REG_DAC_VOL(1),		0x00 },
  { DJBR_REG_OP_STAGE_MUTE,		0x0f },
  { DJBR_REG_SAI0,			0x00 },
  { DJBR_REG_SAI1,			0x00 },
  { DJBR_REG_SOUT_CTRL,		0x00 },
  { DJBR_REG_MODE_MP(0),		0x00 },
  { DJBR_REG_MODE_MP(1),		0x10 },
  { DJBR_REG_MODE_MP(4),		0x00 },
  { DJBR_REG_MODE_MP(5),		0x00 },
  { DJBR_REG_MODE_MP(6),		0x11 },
  { DJBR_REG_OP_STAGE_CTRL,		0x0f },
  { DJBR_REG_DECIM_PWR,		0x00 },
  { DJBR_REG_INTERP_PWR,		0x00 },
  { DJBR_REG_BIAS_CTRL0,		0x00 },
  { DJBR_REG_BIAS_CTRL1,		0x00 },
};

static bool djbr_volatile_register(struct device *dev, unsigned int reg)
{
  if (reg == DJBR_REG_PLL(5))
    return true;

  return false;
}

const struct regmap_config djbr_regmap_config = {
  .val_bits = 8,
  .reg_bits = 16,
  .max_register = 0x4d,

  .reg_defaults = djbr_reg_defaults,
  .num_reg_defaults = ARRAY_SIZE(djbr_reg_defaults),
  .volatile_reg = djbr_volatile_register,
  .cache_type = REGCACHE_RBTREE,
};
EXPORT_SYMBOL_GPL(djbr_regmap_config);

MODULE_DESCRIPTION("ASoC DJBR CODEC driver");
MODULE_AUTHOR("Louis and Sean from THE_LAB, ggLars-Peter Clausen <lars@metafoo.de>");
MODULE_LICENSE("GPL v2");
