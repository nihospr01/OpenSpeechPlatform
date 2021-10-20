/*
 * Digital JellyBeans Unified driver: Driver core
 *
 * Copyright 2018 UCSD / THE_Lab
 *  Author: Louis Pisha <lpisha@eng.ucsd.edu>
 *
 * Based on ADAU1372 Driver
 * Copyright 2016 Analog Devices Inc.
 *  Author: Lars-Peter Clausen <lars@metafoo.de>
 *
 * Licensed under the GPL3
 *
 * Summary: There are four "things" in this driver: DJBUL and DJBUR, which
 * are the hardware devices for the L and R ear codecs, and DJBUM and DJBUS,
 * which are the ALSA codecs running mics (QUAT) and speakers (PRI) respectively.
 * To ALSA, DJBUM and DJBUS are codecs with their own register maps. Writes to
 * and reads from these register maps are intercepted and made to (usually) both
 * DJBUL and DJBUR.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gcd.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pm.h>
#include <linux/slab.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/tlv.h>
#include <sound/soc.h>

#include "djbu.h"
#include "adau-utils.h"


/*******************************************************************************
 Master driver data
*******************************************************************************/

struct djbu_data djbu;

/*******************************************************************************
 Device (DJBUL/DJBUR) register addresses
*******************************************************************************/

#define DJBU_LR_REG_CLK_CTRL         0x00
#define DJBU_LR_REG_PLL(x)          (0x01 + (x))
#define DJBU_LR_REG_DAC_SOURCE       0x11
#define DJBU_LR_REG_SOUT_SOURCE_0_1  0x13
#define DJBU_LR_REG_SOUT_SOURCE_2_3  0x14
#define DJBU_LR_REG_SOUT_SOURCE_4_5  0x15
#define DJBU_LR_REG_SOUT_SOURCE_6_7  0x16
#define DJBU_LR_REG_ADC_SDATA_CH     0x17
#define DJBU_LR_REG_ASRCO_SOURCE_0_1 0x18
#define DJBU_LR_REG_ASRCO_SOURCE_2_3 0x19
#define DJBU_LR_REG_ASRC_MODE        0x1a
#define DJBU_LR_REG_ADC_CTRL0        0x1b
#define DJBU_LR_REG_ADC_CTRL1        0x1c
#define DJBU_LR_REG_ADC_CTRL2        0x1d
#define DJBU_LR_REG_ADC_CTRL3        0x1e
#define DJBU_LR_REG_ADC_VOL(x)      (0x1f + (x))
#define DJBU_LR_REG_PGA_CTRL(x)     (0x23 + (x))
#define DJBU_LR_REG_PGA_BOOST        0x28
#define DJBU_LR_REG_POP_SUPPRESS     0x29
#define DJBU_LR_REG_MICBIAS          0x2d
#define DJBU_LR_REG_DAC_CTRL         0x2e
#define DJBU_LR_REG_DAC_VOL(x)      (0x2f + (x))
#define DJBU_LR_REG_OP_STAGE_MUTE    0x31
#define DJBU_LR_REG_SAI0             0x32
#define DJBU_LR_REG_SAI1             0x33
#define DJBU_LR_REG_SOUT_CTRL        0x34
#define DJBU_LR_REG_MODE_MP(x)      (0x38 + (x))
#define DJBU_LR_REG_OP_STAGE_CTRL    0x43
#define DJBU_LR_REG_DECIM_PWR        0x44
#define DJBU_LR_REG_INTERP_PWR       0x45
#define DJBU_LR_REG_BIAS_CTRL0       0x46
#define DJBU_LR_REG_BIAS_CTRL1       0x47
#define DJBU_LR_REG_PAD_CONTROL4     0x4C
#define DJBU_LR_REG_PAD_CONTROL5     0x4D

/*******************************************************************************
 Device (DJBUL/DJBUR) register bit values
*******************************************************************************/

#define DJBU_LR_CLK_CTRL_PLL_EN     BIT(7)
#define DJBU_LR_CLK_CTRL_XTAL_DIS   BIT(4)
#define DJBU_LR_CLK_CTRL_CLKSRC     BIT(3)
#define DJBU_LR_CLK_CTRL_CC_MDIV    BIT(1)
#define DJBU_LR_CLK_CTRL_MCLK_EN    BIT(0)

#define DJBU_LR_CLK_CTRL_STATE (DJBU_LR_CLK_CTRL_XTAL_DIS | DJBU_LR_CLK_CTRL_CC_MDIV)

#define DJBU_LR_SAI0_DELAY1     (0x0 << 6)
#define DJBU_LR_SAI0_DELAY0     (0x1 << 6)
#define DJBU_LR_SAI0_DELAY_MASK (0x3 << 6)
#define DJBU_LR_SAI0_SAI_I2S    (0x0 << 4)
#define DJBU_LR_SAI0_SAI_TDM2   (0x1 << 4)
#define DJBU_LR_SAI0_SAI_TDM4   (0x2 << 4)
#define DJBU_LR_SAI0_SAI_TDM8   (0x3 << 4)
#define DJBU_LR_SAI0_SAI_MASK   (0x3 << 4)
#define DJBU_LR_SAI0_FS_48       0x0
#define DJBU_LR_SAI0_FS_8        0x1
#define DJBU_LR_SAI0_FS_12       0x2
#define DJBU_LR_SAI0_FS_16       0x3
#define DJBU_LR_SAI0_FS_24       0x4
#define DJBU_LR_SAI0_FS_32       0x5
#define DJBU_LR_SAI0_FS_96       0x6
#define DJBU_LR_SAI0_FS_192      0x7
#define DJBU_LR_SAI0_FS_MASK     0xf

#define DJBU_LR_SAI1_TDM_TS      BIT(7)
#define DJBU_LR_SAI1_BCLK_TDMC   BIT(6)
#define DJBU_LR_SAI1_LR_MODE     BIT(5)
#define DJBU_LR_SAI1_LR_POL      BIT(4)
#define DJBU_LR_SAI1_SAI_MSB     BIT(3)
#define DJBU_LR_SAI1_BCLKRATE    BIT(2)
#define DJBU_LR_SAI1_BCLKEDGE    BIT(1)
#define DJBU_LR_SAI1_MS          BIT(0)

/*******************************************************************************
 Device (DJBUL/DJBUR) register map description
*******************************************************************************/

static const struct reg_default djbu_lr_reg_defaults[] = {
  { DJBU_LR_REG_CLK_CTRL,        0x00 },
  { DJBU_LR_REG_PLL(0),          0x00 },
  { DJBU_LR_REG_PLL(1),          0x00 },
  { DJBU_LR_REG_PLL(2),          0x00 },
  { DJBU_LR_REG_PLL(3),          0x00 },
  { DJBU_LR_REG_PLL(4),          0x00 },
  { DJBU_LR_REG_PLL(5),          0x00 },
  { DJBU_LR_REG_DAC_SOURCE,      0x10 },
  { DJBU_LR_REG_SOUT_SOURCE_0_1, 0x54 },
  { DJBU_LR_REG_SOUT_SOURCE_2_3, 0x76 },
  { DJBU_LR_REG_SOUT_SOURCE_4_5, 0x54 },
  { DJBU_LR_REG_SOUT_SOURCE_6_7, 0x76 },
  { DJBU_LR_REG_ADC_SDATA_CH,    0x04 },
  { DJBU_LR_REG_ASRCO_SOURCE_0_1,0x10 },
  { DJBU_LR_REG_ASRCO_SOURCE_2_3,0x32 },
  { DJBU_LR_REG_ASRC_MODE,       0x00 },
  { DJBU_LR_REG_ADC_CTRL0,       0x19 },
  { DJBU_LR_REG_ADC_CTRL1,       0x19 },
  { DJBU_LR_REG_ADC_CTRL2,       0x00 },
  { DJBU_LR_REG_ADC_CTRL3,       0x00 },
  { DJBU_LR_REG_ADC_VOL(0),      0x00 },
  { DJBU_LR_REG_ADC_VOL(1),      0x00 },
  { DJBU_LR_REG_ADC_VOL(2),      0x00 },
  { DJBU_LR_REG_ADC_VOL(3),      0x00 },
  { DJBU_LR_REG_PGA_CTRL(0),     0x40 },
  { DJBU_LR_REG_PGA_CTRL(1),     0x40 },
  { DJBU_LR_REG_PGA_CTRL(2),     0x40 },
  { DJBU_LR_REG_PGA_CTRL(3),     0x40 },
  { DJBU_LR_REG_PGA_BOOST,       0x00 },
  { DJBU_LR_REG_MICBIAS,         0x00 },
  { DJBU_LR_REG_DAC_CTRL,        0x18 },
  { DJBU_LR_REG_DAC_VOL(0),      0x00 },
  { DJBU_LR_REG_DAC_VOL(1),      0x00 },
  { DJBU_LR_REG_OP_STAGE_MUTE,   0x0f },
  { DJBU_LR_REG_SAI0,            0x00 },
  { DJBU_LR_REG_SAI1,            0x00 },
  { DJBU_LR_REG_SOUT_CTRL,       0x00 },
  { DJBU_LR_REG_MODE_MP(0),      0x00 },
  { DJBU_LR_REG_MODE_MP(1),      0x10 },
  { DJBU_LR_REG_MODE_MP(4),      0x00 },
  { DJBU_LR_REG_MODE_MP(5),      0x00 },
  { DJBU_LR_REG_MODE_MP(6),      0x11 },
  { DJBU_LR_REG_OP_STAGE_CTRL,   0x0f },
  { DJBU_LR_REG_DECIM_PWR,       0x00 },
  { DJBU_LR_REG_INTERP_PWR,      0x00 },
  { DJBU_LR_REG_BIAS_CTRL0,      0x00 },
  { DJBU_LR_REG_BIAS_CTRL1,      0x00 },
  { DJBU_LR_REG_PAD_CONTROL4,    0x00 },
  { DJBU_LR_REG_PAD_CONTROL5,    0x00 },
};

static bool djbu_lr_volatile_register(struct device *dev, unsigned int reg)
{
  if (reg == DJBU_LR_REG_PLL(5))
    return true;

  return false;
}

const struct regmap_config djbu_lr_regmap_config = {
  .val_bits = 8,
  .reg_bits = 16,
  .max_register = 0x4d,

  .reg_defaults = djbu_lr_reg_defaults,
  .num_reg_defaults = ARRAY_SIZE(djbu_lr_reg_defaults),
  .volatile_reg = djbu_lr_volatile_register,
  .cache_type = REGCACHE_RBTREE,
};
EXPORT_SYMBOL_GPL(djbu_lr_regmap_config);

/*******************************************************************************
 Sample rates
*******************************************************************************/

//For DJBUL/DJBUR
static const unsigned int djbu_lr_rates[] = {
  [DJBU_LR_SAI0_FS_8] = 8000,
  [DJBU_LR_SAI0_FS_12] = 12000,
  [DJBU_LR_SAI0_FS_16] = 16000,
  [DJBU_LR_SAI0_FS_24] = 24000,
  [DJBU_LR_SAI0_FS_32] = 32000,
  [DJBU_LR_SAI0_FS_48] = 48000,
  [DJBU_LR_SAI0_FS_96] = 96000,
  [DJBU_LR_SAI0_FS_192] = 192000,
};

#define DJBU_LR_RATE_MASK_TDM8 0x17                          // 8k, 12k, 24k, 48k
#define DJBU_LR_RATE_MASK_TDM4 (DJBU_RATE_MASK_TDM8 | 0x48)  // + 16k, 96k
#define DJBU_LR_RATE_MASK_TDM2 (DJBU_RATE_MASK_TDM4 | 0xa0)  // + 32k, 192k

//For DJBUM/DJBUS
static const unsigned int djbu_ms_rates[] = {
  [DJBU_LR_SAI0_FS_48] = 48000,
  [DJBU_LR_SAI0_FS_96] = 96000,
};

#define DJBU_MS_RATE_MASK 0x41 //48k and 96k

//For DAI driver
#define DJBU_MS_RATES SNDRV_PCM_RATE_KNOT //means "other non-continuous rates", could change to SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000 though the actual chip supports un-#define-d rates like 12000
#define DJBU_MS_FORMATS SNDRV_PCM_FMTBIT_S32_LE //Could also support SNDRV_PCM_FMTBIT_S16_LE but would require some changes in set_tdm_slot

/*******************************************************************************
 Codec (DJBUM/DJBUS) virtual registers
*******************************************************************************/

//Registers are +1 for right ear
#define DJBUM_REG_BIAS_MIC     0x00
#define DJBUM_REG_BIAS_AFE01   0x02
#define DJBUM_REG_BIAS_AFE23   0x04
#define DJBUM_REG_BIAS_ADC01   0x06
#define DJBUM_REG_BIAS_ADC23   0x08
#define DJBUM_REG_HPF_ADC01    0x0A
#define DJBUM_REG_HPF_ADC23    0x0C
#define DJBUM_REG_PGA_VOL(x)  (0x0E + ((x)<<1))
#define DJBUM_REG_PGA_MUTE     0x16
#define DJBUM_REG_PGA_BOOST    0x18
#define DJBUM_REG_ADC_VOL(x)  (0x1A + ((x)<<1))
#define DJBUM_REG_ADC_MUTE     0x22
#define DJBUM_REG_ASRCO01_SRC  0x24
#define DJBUM_REG_ASRCO23_SRC  0x26
//Registers without a +1 for right ear
#define DJBUM_REG_CAP_PWR      0x28

//Registers are +1 for right ear
#define DJBUS_REG_BIAS_DAC     0x00
#define DJBUS_REG_BIAS_HP      0x02
#define DJBUS_REG_DAC_VOL(x)  (0x04 + ((x)<<1))
#define DJBUS_REG_DAC_MUTE     0x08
#define DJBUS_REG_DAC_SRC      0x0A
//Registers without a +1 for right ear
#define DJBUS_REG_DACINVERT    0x0C
#define DJBUS_REG_PLY_PWR      0x0E


/*******************************************************************************
 Codec (DJBUM/DJBUS) ALSA controls
*******************************************************************************/

static const char * const djbu_bias_text[] = {
  "Normal operation", "Extreme power saving", "Enhanced performance",
  "Power saving",
};

static const char * const djbu_bias_adc_text[] = {
  "Normal operation", "Enhanced performance", "Power saving",
};
static const unsigned int djbu_bias_adc_values[] = {
  0, 2, 3,
};

static const char * const djbu_bias_dac_text[] = {
  "Normal operation", "Power saving", "Superior performance",
  "Enhanced performance",
};

static SOC_ENUM_SINGLE_DECL(djbum_l_bias_mic_enum,   DJBUM_REG_BIAS_MIC,     0, djbu_bias_text);
static SOC_ENUM_SINGLE_DECL(djbum_r_bias_mic_enum,   DJBUM_REG_BIAS_MIC+1,   0, djbu_bias_text);
static SOC_ENUM_SINGLE_DECL(djbum_l_bias_afe01_enum, DJBUM_REG_BIAS_AFE01,   0, djbu_bias_text);
static SOC_ENUM_SINGLE_DECL(djbum_r_bias_afe01_enum, DJBUM_REG_BIAS_AFE01+1, 0, djbu_bias_text);
static SOC_ENUM_SINGLE_DECL(djbum_l_bias_afe23_enum, DJBUM_REG_BIAS_AFE23,   0, djbu_bias_text);
static SOC_ENUM_SINGLE_DECL(djbum_r_bias_afe23_enum, DJBUM_REG_BIAS_AFE23+1, 0, djbu_bias_text);
static SOC_VALUE_ENUM_SINGLE_DECL(djbum_l_bias_adc01_enum, DJBUM_REG_BIAS_ADC01,   0, 0x3,
    djbu_bias_adc_text, djbu_bias_adc_values);
static SOC_VALUE_ENUM_SINGLE_DECL(djbum_r_bias_adc01_enum, DJBUM_REG_BIAS_ADC01+1, 0, 0x3,
    djbu_bias_adc_text, djbu_bias_adc_values);
static SOC_VALUE_ENUM_SINGLE_DECL(djbum_l_bias_adc23_enum, DJBUM_REG_BIAS_ADC23,   0, 0x3,
    djbu_bias_adc_text, djbu_bias_adc_values);
static SOC_VALUE_ENUM_SINGLE_DECL(djbum_r_bias_adc23_enum, DJBUM_REG_BIAS_ADC23+1, 0, 0x3,
    djbu_bias_adc_text, djbu_bias_adc_values);

static SOC_ENUM_SINGLE_DECL(djbus_l_bias_dac_enum, DJBUS_REG_BIAS_DAC,   0, djbu_bias_dac_text);
static SOC_ENUM_SINGLE_DECL(djbus_r_bias_dac_enum, DJBUS_REG_BIAS_DAC+1, 0, djbu_bias_dac_text);
static SOC_ENUM_SINGLE_DECL(djbus_l_bias_hp_enum,  DJBUS_REG_BIAS_HP,    0, djbu_bias_text);
static SOC_ENUM_SINGLE_DECL(djbus_r_bias_hp_enum,  DJBUS_REG_BIAS_HP+1,  0, djbu_bias_text);

static const char * const djbu_hpf_text[] = {
  "Off", "1 Hz", "4 Hz", "8 Hz",
};

static SOC_ENUM_SINGLE_DECL(djbum_l_hpf_adc01_enum, DJBUM_REG_HPF_ADC01,   0, djbu_hpf_text);
static SOC_ENUM_SINGLE_DECL(djbum_r_hpf_adc01_enum, DJBUM_REG_HPF_ADC01+1, 0, djbu_hpf_text);
static SOC_ENUM_SINGLE_DECL(djbum_l_hpf_adc23_enum, DJBUM_REG_HPF_ADC23,   0, djbu_hpf_text);
static SOC_ENUM_SINGLE_DECL(djbum_r_hpf_adc23_enum, DJBUM_REG_HPF_ADC23+1, 0, djbu_hpf_text);

static const DECLARE_TLV_DB_MINMAX(djbu_dig_tlv, -9563,    0);
static const DECLARE_TLV_DB_SCALE( djbu_pga_tlv, -1200,   75, 0);
static const DECLARE_TLV_DB_SCALE( djbu_pgab_tlv,    0, 1000, 0);

static const char * const djbu_dacsource_text[] = {
  "Serial Input (ASRC) Channel 0 (L)", "Serial Input (ASRC) Channel 1 (R)",
};
static const unsigned int djbu_dacsource_values[] = {
  0xC, 0xD,
};

static SOC_VALUE_ENUM_SINGLE_DECL(djbus_l_dac0source_enum, DJBUS_REG_DAC_SRC,   0, 0xF,
    djbu_dacsource_text, djbu_dacsource_values);
static SOC_VALUE_ENUM_SINGLE_DECL(djbus_r_dac0source_enum, DJBUS_REG_DAC_SRC+1, 0, 0xF,
    djbu_dacsource_text, djbu_dacsource_values);
static SOC_VALUE_ENUM_SINGLE_DECL(djbus_l_dac1source_enum, DJBUS_REG_DAC_SRC,   4, 0xF,
    djbu_dacsource_text, djbu_dacsource_values);
static SOC_VALUE_ENUM_SINGLE_DECL(djbus_r_dac1source_enum, DJBUS_REG_DAC_SRC+1, 4, 0xF,
    djbu_dacsource_text, djbu_dacsource_values);

static const char * const djbu_asrcosource_text[] = {
  "ADC0", "ADC1", "ADC2", "ADC3",
};
static const unsigned int djbu_asrcosource_values[] = {
  0x4, 0x5, 0x6, 0x7
};

static SOC_VALUE_ENUM_SINGLE_DECL(djbum_l_asrco0source_enum, DJBUM_REG_ASRCO01_SRC,   0, 0xF,
    djbu_asrcosource_text, djbu_asrcosource_values);
static SOC_VALUE_ENUM_SINGLE_DECL(djbum_r_asrco0source_enum, DJBUM_REG_ASRCO01_SRC+1, 0, 0xF,
    djbu_asrcosource_text, djbu_asrcosource_values);
static SOC_VALUE_ENUM_SINGLE_DECL(djbum_l_asrco1source_enum, DJBUM_REG_ASRCO01_SRC,   4, 0xF,
    djbu_asrcosource_text, djbu_asrcosource_values);
static SOC_VALUE_ENUM_SINGLE_DECL(djbum_r_asrco1source_enum, DJBUM_REG_ASRCO01_SRC+1, 4, 0xF,
    djbu_asrcosource_text, djbu_asrcosource_values);
static SOC_VALUE_ENUM_SINGLE_DECL(djbum_l_asrco2source_enum, DJBUM_REG_ASRCO23_SRC,   0, 0xF,
    djbu_asrcosource_text, djbu_asrcosource_values);
static SOC_VALUE_ENUM_SINGLE_DECL(djbum_r_asrco2source_enum, DJBUM_REG_ASRCO23_SRC+1, 0, 0xF,
    djbu_asrcosource_text, djbu_asrcosource_values);
static SOC_VALUE_ENUM_SINGLE_DECL(djbum_l_asrco3source_enum, DJBUM_REG_ASRCO23_SRC,   4, 0xF,
    djbu_asrcosource_text, djbu_asrcosource_values);
static SOC_VALUE_ENUM_SINGLE_DECL(djbum_r_asrco3source_enum, DJBUM_REG_ASRCO23_SRC+1, 4, 0xF,
    djbu_asrcosource_text, djbu_asrcosource_values);

static const char * const djbu_dacinvert_text[] = {
  "Normal", "Inverted"
};

static SOC_ENUM_SINGLE_DECL(djbus_dacinvert_enum, DJBUS_REG_DACINVERT, 0, djbu_dacinvert_text);


//DJBUM controls
static const struct snd_kcontrol_new djbum_controls[] = {
  //Biases
  SOC_ENUM("DJBUM L Microphone Bias", djbum_l_bias_mic_enum),
  SOC_ENUM("DJBUM R Microphone Bias", djbum_r_bias_mic_enum),
  SOC_ENUM("DJBUM L AFE 0+1 Bias", djbum_l_bias_afe01_enum),
  SOC_ENUM("DJBUM R AFE 0+1 Bias", djbum_r_bias_afe01_enum),
  SOC_ENUM("DJBUM L AFE 2+3 Bias", djbum_l_bias_afe23_enum),
  SOC_ENUM("DJBUM R AFE 2+3 Bias", djbum_r_bias_afe23_enum),
  SOC_ENUM("DJBUM L ADC 0+1 Bias", djbum_l_bias_adc01_enum),
  SOC_ENUM("DJBUM R ADC 0+1 Bias", djbum_r_bias_adc01_enum),
  SOC_ENUM("DJBUM L ADC 2+3 Bias", djbum_l_bias_adc23_enum),
  SOC_ENUM("DJBUM R ADC 2+3 Bias", djbum_r_bias_adc23_enum),
  //HPFs
  SOC_ENUM("DJBUM L ADC 0+1 High Pass Filter", djbum_l_hpf_adc01_enum),
  SOC_ENUM("DJBUM R ADC 0+1 High Pass Filter", djbum_r_hpf_adc01_enum),
  SOC_ENUM("DJBUM L ADC 2+3 High Pass Filter", djbum_l_hpf_adc23_enum),
  SOC_ENUM("DJBUM R ADC 2+3 High Pass Filter", djbum_r_hpf_adc23_enum),
  //PGAs
  SOC_SINGLE_TLV("DJBUM L PGA 0 Capture Volume", DJBUM_REG_PGA_VOL(0),   0, 0x3f, 0, djbu_pga_tlv),
  SOC_SINGLE_TLV("DJBUM R PGA 0 Capture Volume", DJBUM_REG_PGA_VOL(0)+1, 0, 0x3f, 0, djbu_pga_tlv),
  SOC_SINGLE_TLV("DJBUM L PGA 1 Capture Volume", DJBUM_REG_PGA_VOL(1),   0, 0x3f, 0, djbu_pga_tlv),
  SOC_SINGLE_TLV("DJBUM R PGA 1 Capture Volume", DJBUM_REG_PGA_VOL(1)+1, 0, 0x3f, 0, djbu_pga_tlv),
  SOC_SINGLE_TLV("DJBUM L PGA 2 Capture Volume", DJBUM_REG_PGA_VOL(2),   0, 0x3f, 0, djbu_pga_tlv),
  SOC_SINGLE_TLV("DJBUM R PGA 2 Capture Volume", DJBUM_REG_PGA_VOL(2)+1, 0, 0x3f, 0, djbu_pga_tlv),
  SOC_SINGLE_TLV("DJBUM L PGA 3 Capture Volume", DJBUM_REG_PGA_VOL(3),   0, 0x3f, 0, djbu_pga_tlv),
  SOC_SINGLE_TLV("DJBUM R PGA 3 Capture Volume", DJBUM_REG_PGA_VOL(3)+1, 0, 0x3f, 0, djbu_pga_tlv),
  SOC_SINGLE("DJBUM L PGA 0 Capture Switch", DJBUM_REG_PGA_MUTE,   0, 1, 1),
  SOC_SINGLE("DJBUM R PGA 0 Capture Switch", DJBUM_REG_PGA_MUTE+1, 0, 1, 1),
  SOC_SINGLE("DJBUM L PGA 1 Capture Switch", DJBUM_REG_PGA_MUTE,   1, 1, 1),
  SOC_SINGLE("DJBUM R PGA 1 Capture Switch", DJBUM_REG_PGA_MUTE+1, 1, 1, 1),
  SOC_SINGLE("DJBUM L PGA 2 Capture Switch", DJBUM_REG_PGA_MUTE,   2, 1, 1),
  SOC_SINGLE("DJBUM R PGA 2 Capture Switch", DJBUM_REG_PGA_MUTE+1, 2, 1, 1),
  SOC_SINGLE("DJBUM L PGA 3 Capture Switch", DJBUM_REG_PGA_MUTE,   3, 1, 1),
  SOC_SINGLE("DJBUM R PGA 3 Capture Switch", DJBUM_REG_PGA_MUTE+1, 3, 1, 1),
  SOC_SINGLE_TLV("DJBUM L PGA 0 Boost Capture Volume", DJBUM_REG_PGA_BOOST,   0, 1,0, djbu_pgab_tlv),
  SOC_SINGLE_TLV("DJBUM R PGA 0 Boost Capture Volume", DJBUM_REG_PGA_BOOST+1, 0, 1,0, djbu_pgab_tlv),
  SOC_SINGLE_TLV("DJBUM L PGA 1 Boost Capture Volume", DJBUM_REG_PGA_BOOST,   1, 1,0, djbu_pgab_tlv),
  SOC_SINGLE_TLV("DJBUM R PGA 1 Boost Capture Volume", DJBUM_REG_PGA_BOOST+1, 1, 1,0, djbu_pgab_tlv),
  SOC_SINGLE_TLV("DJBUM L PGA 2 Boost Capture Volume", DJBUM_REG_PGA_BOOST,   2, 1,0, djbu_pgab_tlv),
  SOC_SINGLE_TLV("DJBUM R PGA 2 Boost Capture Volume", DJBUM_REG_PGA_BOOST+1, 2, 1,0, djbu_pgab_tlv),
  SOC_SINGLE_TLV("DJBUM L PGA 3 Boost Capture Volume", DJBUM_REG_PGA_BOOST,   3, 1,0, djbu_pgab_tlv),
  SOC_SINGLE_TLV("DJBUM R PGA 3 Boost Capture Volume", DJBUM_REG_PGA_BOOST+1, 3, 1,0, djbu_pgab_tlv),
  //ADCs
  SOC_SINGLE_TLV("DJBUM L ADC 0 Capture Volume", DJBUM_REG_ADC_VOL(0),   0, 0xff, 1, djbu_dig_tlv),
  SOC_SINGLE_TLV("DJBUM R ADC 0 Capture Volume", DJBUM_REG_ADC_VOL(0)+1, 0, 0xff, 1, djbu_dig_tlv),
  SOC_SINGLE_TLV("DJBUM L ADC 1 Capture Volume", DJBUM_REG_ADC_VOL(1),   0, 0xff, 1, djbu_dig_tlv),
  SOC_SINGLE_TLV("DJBUM R ADC 1 Capture Volume", DJBUM_REG_ADC_VOL(1)+1, 0, 0xff, 1, djbu_dig_tlv),
  SOC_SINGLE_TLV("DJBUM L ADC 2 Capture Volume", DJBUM_REG_ADC_VOL(2),   0, 0xff, 1, djbu_dig_tlv),
  SOC_SINGLE_TLV("DJBUM R ADC 2 Capture Volume", DJBUM_REG_ADC_VOL(2)+1, 0, 0xff, 1, djbu_dig_tlv),
  SOC_SINGLE_TLV("DJBUM L ADC 3 Capture Volume", DJBUM_REG_ADC_VOL(3),   0, 0xff, 1, djbu_dig_tlv),
  SOC_SINGLE_TLV("DJBUM R ADC 3 Capture Volume", DJBUM_REG_ADC_VOL(3)+1, 0, 0xff, 1, djbu_dig_tlv),
  SOC_SINGLE("DJBUM L ADC 0 Capture Switch", DJBUM_REG_ADC_MUTE,   0, 1, 1),
  SOC_SINGLE("DJBUM R ADC 0 Capture Switch", DJBUM_REG_ADC_MUTE+1, 0, 1, 1),
  SOC_SINGLE("DJBUM L ADC 1 Capture Switch", DJBUM_REG_ADC_MUTE,   1, 1, 1),
  SOC_SINGLE("DJBUM R ADC 1 Capture Switch", DJBUM_REG_ADC_MUTE+1, 1, 1, 1),
  SOC_SINGLE("DJBUM L ADC 2 Capture Switch", DJBUM_REG_ADC_MUTE,   2, 1, 1),
  SOC_SINGLE("DJBUM R ADC 2 Capture Switch", DJBUM_REG_ADC_MUTE+1, 2, 1, 1),
  SOC_SINGLE("DJBUM L ADC 3 Capture Switch", DJBUM_REG_ADC_MUTE,   3, 1, 1),
  SOC_SINGLE("DJBUM R ADC 3 Capture Switch", DJBUM_REG_ADC_MUTE+1, 3, 1, 1),
  //ASRCOs source
  SOC_ENUM("DJBUM L ASRCO 0 Source", djbum_l_asrco0source_enum),
  SOC_ENUM("DJBUM R ASRCO 0 Source", djbum_r_asrco0source_enum),
  SOC_ENUM("DJBUM L ASRCO 1 Source", djbum_l_asrco1source_enum),
  SOC_ENUM("DJBUM R ASRCO 1 Source", djbum_r_asrco1source_enum),
  SOC_ENUM("DJBUM L ASRCO 2 Source", djbum_l_asrco2source_enum),
  SOC_ENUM("DJBUM R ASRCO 2 Source", djbum_r_asrco2source_enum),
  SOC_ENUM("DJBUM L ASRCO 3 Source", djbum_l_asrco3source_enum),
  SOC_ENUM("DJBUM R ASRCO 3 Source", djbum_r_asrco3source_enum),
};

//DJBUS controls
static const struct snd_kcontrol_new djbus_controls[] = {
  //Biases
  SOC_ENUM("DJBUS L DAC 0+1 Bias", djbus_l_bias_dac_enum),
  SOC_ENUM("DJBUS R DAC 0+1 Bias", djbus_r_bias_dac_enum),
  SOC_ENUM("DJBUS L Headphone Bias", djbus_l_bias_hp_enum),
  SOC_ENUM("DJBUS R Headphone Bias", djbus_r_bias_hp_enum),
  //DACs
  SOC_SINGLE_TLV("DJBUS L DAC 0 Playback Volume", DJBUS_REG_DAC_VOL(0),   0, 0xff, 1, djbu_dig_tlv),
  SOC_SINGLE_TLV("DJBUS R DAC 0 Playback Volume", DJBUS_REG_DAC_VOL(0)+1, 0, 0xff, 1, djbu_dig_tlv),
  SOC_SINGLE_TLV("DJBUS L DAC 1 Playback Volume", DJBUS_REG_DAC_VOL(1),   0, 0xff, 1, djbu_dig_tlv),
  SOC_SINGLE_TLV("DJBUS R DAC 1 Playback Volume", DJBUS_REG_DAC_VOL(1)+1, 0, 0xff, 1, djbu_dig_tlv),
  SOC_SINGLE("DJBUS L DAC 0 Playback Switch", DJBUS_REG_DAC_MUTE,   0, 1, 1),
  SOC_SINGLE("DJBUS R DAC 0 Playback Switch", DJBUS_REG_DAC_MUTE+1, 0, 1, 1),
  SOC_SINGLE("DJBUS L DAC 1 Playback Switch", DJBUS_REG_DAC_MUTE,   1, 1, 1),
  SOC_SINGLE("DJBUS R DAC 1 Playback Switch", DJBUS_REG_DAC_MUTE+1, 1, 1, 1),
  //DAC sources
  SOC_ENUM("DJBUS L DAC 0 Source", djbus_l_dac0source_enum),
  SOC_ENUM("DJBUS R DAC 0 Source", djbus_r_dac0source_enum),
  SOC_ENUM("DJBUS L DAC 1 Source", djbus_l_dac1source_enum),
  SOC_ENUM("DJBUS R DAC 1 Source", djbus_r_dac1source_enum),
  //DAC invert
  SOC_ENUM("DJBUS All DACs Invert", djbus_dacinvert_enum),
};

/*******************************************************************************
 DJBUM codec DAPM widgets & routes
*******************************************************************************/

//We just define the minimal routing for the 4 virtual inputs and outputs so we
//get a bit write when capture/playback begins. The actual routing is partly
//hardcoded and partly determined by explicit ALSA controls.

static const struct snd_soc_dapm_widget djbum_dapm_widgets[] = {
  SND_SOC_DAPM_INPUT("DJBUM Virtual AIN 0"),
  SND_SOC_DAPM_INPUT("DJBUM Virtual AIN 1"),
  SND_SOC_DAPM_INPUT("DJBUM Virtual AIN 2"),
  SND_SOC_DAPM_INPUT("DJBUM Virtual AIN 3"),

  SND_SOC_DAPM_ADC("DJBUM Virtual ADC 0", NULL, DJBUM_REG_CAP_PWR, 0, 0),
  SND_SOC_DAPM_ADC("DJBUM Virtual ADC 1", NULL, DJBUM_REG_CAP_PWR, 1, 0),
  SND_SOC_DAPM_ADC("DJBUM Virtual ADC 2", NULL, DJBUM_REG_CAP_PWR, 2, 0),
  SND_SOC_DAPM_ADC("DJBUM Virtual ADC 3", NULL, DJBUM_REG_CAP_PWR, 3, 0),

  SND_SOC_DAPM_AIF_OUT("DJBUM Virtual Serial Output 0", NULL, 0, SND_SOC_NOPM, 0, 0),
  SND_SOC_DAPM_AIF_OUT("DJBUM Virtual Serial Output 1", NULL, 1, SND_SOC_NOPM, 0, 0),
  SND_SOC_DAPM_AIF_OUT("DJBUM Virtual Serial Output 2", NULL, 2, SND_SOC_NOPM, 0, 0),
  SND_SOC_DAPM_AIF_OUT("DJBUM Virtual Serial Output 3", NULL, 3, SND_SOC_NOPM, 0, 0),
};

static const struct snd_soc_dapm_route djbum_dapm_routes[] = {
  {"DJBUM Virtual ADC 0", NULL, "DJBUM Virtual AIN 0"},
  {"DJBUM Virtual ADC 1", NULL, "DJBUM Virtual AIN 1"},
  {"DJBUM Virtual ADC 2", NULL, "DJBUM Virtual AIN 2"},
  {"DJBUM Virtual ADC 3", NULL, "DJBUM Virtual AIN 3"},

  {"DJBUM Virtual Serial Output 0", NULL, "DJBUM Virtual ADC 0"},
  {"DJBUM Virtual Serial Output 1", NULL, "DJBUM Virtual ADC 1"},
  {"DJBUM Virtual Serial Output 2", NULL, "DJBUM Virtual ADC 2"},
  {"DJBUM Virtual Serial Output 3", NULL, "DJBUM Virtual ADC 3"},

  {"Capture", NULL, "DJBUM Virtual Serial Output 0"},
  {"Capture", NULL, "DJBUM Virtual Serial Output 1"},
  {"Capture", NULL, "DJBUM Virtual Serial Output 2"},
  {"Capture", NULL, "DJBUM Virtual Serial Output 3"},
};

static const struct snd_soc_dapm_widget djbus_dapm_widgets[] = {
  SND_SOC_DAPM_AIF_IN("DJBUS Virtual Serial Input 0", NULL, 0, SND_SOC_NOPM, 0, 0),
  SND_SOC_DAPM_AIF_IN("DJBUS Virtual Serial Input 1", NULL, 1, SND_SOC_NOPM, 0, 0),
  SND_SOC_DAPM_AIF_IN("DJBUS Virtual Serial Input 2", NULL, 2, SND_SOC_NOPM, 0, 0),
  SND_SOC_DAPM_AIF_IN("DJBUS Virtual Serial Input 3", NULL, 3, SND_SOC_NOPM, 0, 0),

  SND_SOC_DAPM_DAC("DJBUS Virtual DAC 0", NULL, DJBUS_REG_PLY_PWR, 0, 0),
  SND_SOC_DAPM_DAC("DJBUS Virtual DAC 1", NULL, DJBUS_REG_PLY_PWR, 1, 0),
  SND_SOC_DAPM_DAC("DJBUS Virtual DAC 2", NULL, DJBUS_REG_PLY_PWR, 2, 0),
  SND_SOC_DAPM_DAC("DJBUS Virtual DAC 3", NULL, DJBUS_REG_PLY_PWR, 3, 0),

  SND_SOC_DAPM_OUTPUT("DJBUS Virtual AOUT 0"),
  SND_SOC_DAPM_OUTPUT("DJBUS Virtual AOUT 1"),
  SND_SOC_DAPM_OUTPUT("DJBUS Virtual AOUT 2"),
  SND_SOC_DAPM_OUTPUT("DJBUS Virtual AOUT 3"),
};

static const struct snd_soc_dapm_route djbus_dapm_routes[] = {
  {"DJBUS Virtual Serial Input 0", NULL, "Playback"},
  {"DJBUS Virtual Serial Input 1", NULL, "Playback"},
  {"DJBUS Virtual Serial Input 2", NULL, "Playback"},
  {"DJBUS Virtual Serial Input 3", NULL, "Playback"},

  {"DJBUS Virtual DAC 0", NULL, "DJBUS Virtual Serial Input 0"},
  {"DJBUS Virtual DAC 1", NULL, "DJBUS Virtual Serial Input 1"},
  {"DJBUS Virtual DAC 2", NULL, "DJBUS Virtual Serial Input 2"},
  {"DJBUS Virtual DAC 3", NULL, "DJBUS Virtual Serial Input 3"},

  {"DJBUS Virtual AOUT 0", NULL, "DJBUS Virtual DAC 0"},
  {"DJBUS Virtual AOUT 1", NULL, "DJBUS Virtual DAC 1"},
  {"DJBUS Virtual AOUT 2", NULL, "DJBUS Virtual DAC 2"},
  {"DJBUS Virtual AOUT 3", NULL, "DJBUS Virtual DAC 3"},
};

/*******************************************************************************
 DJBUM codec translation functions
*******************************************************************************/

static void djbu_setrun(bool running)
{
  if(djbu.running == running) return;
  if(running){
    //Enable everything for playback/capture and do hardcoded routing
    //ASRCOs to SDO hardcoded
    regmap_write(djbu.regmap_l, DJBU_LR_REG_SOUT_SOURCE_0_1, 0x54);
    regmap_write(djbu.regmap_r, DJBU_LR_REG_SOUT_SOURCE_0_1, 0x54);
    regmap_write(djbu.regmap_l, DJBU_LR_REG_SOUT_SOURCE_2_3, 0x76);
    regmap_write(djbu.regmap_r, DJBU_LR_REG_SOUT_SOURCE_2_3, 0x76);
    regmap_write(djbu.regmap_l, DJBU_LR_REG_SOUT_SOURCE_4_5, 0x54);
    regmap_write(djbu.regmap_r, DJBU_LR_REG_SOUT_SOURCE_4_5, 0x54);
    regmap_write(djbu.regmap_l, DJBU_LR_REG_SOUT_SOURCE_6_7, 0x76);
    regmap_write(djbu.regmap_r, DJBU_LR_REG_SOUT_SOURCE_6_7, 0x76);
    //SDO to SDATA0/1 hardcoded
    regmap_write(djbu.regmap_l, DJBU_LR_REG_ADC_SDATA_CH, 0x00);
    regmap_write(djbu.regmap_r, DJBU_LR_REG_ADC_SDATA_CH, 0x00);
    //SDI to ASRCIs hardcoded, plus ASRCI/ASRCO enables
    regmap_write(djbu.regmap_l, DJBU_LR_REG_ASRC_MODE, 0x03);
    regmap_write(djbu.regmap_r, DJBU_LR_REG_ASRC_MODE, 0x03);
    //Enable ADCs and ensure decimator captures from ADC not DMIC
    regmap_update_bits(djbu.regmap_l, DJBU_LR_REG_ADC_CTRL2, 0x07, 0x03);
    regmap_update_bits(djbu.regmap_r, DJBU_LR_REG_ADC_CTRL2, 0x07, 0x03);
    regmap_update_bits(djbu.regmap_l, DJBU_LR_REG_ADC_CTRL3, 0x07, 0x03);
    regmap_update_bits(djbu.regmap_r, DJBU_LR_REG_ADC_CTRL3, 0x07, 0x03);
    //Enable PGAs
    regmap_update_bits(djbu.regmap_l, DJBU_LR_REG_PGA_CTRL(0), 0x80, 0x80);
    regmap_update_bits(djbu.regmap_r, DJBU_LR_REG_PGA_CTRL(0), 0x80, 0x80);
    regmap_update_bits(djbu.regmap_l, DJBU_LR_REG_PGA_CTRL(1), 0x80, 0x80);
    regmap_update_bits(djbu.regmap_r, DJBU_LR_REG_PGA_CTRL(1), 0x80, 0x80);
    regmap_update_bits(djbu.regmap_l, DJBU_LR_REG_PGA_CTRL(2), 0x80, 0x80);
    regmap_update_bits(djbu.regmap_r, DJBU_LR_REG_PGA_CTRL(2), 0x80, 0x80);
    regmap_update_bits(djbu.regmap_l, DJBU_LR_REG_PGA_CTRL(3), 0x80, 0x80);
    regmap_update_bits(djbu.regmap_r, DJBU_LR_REG_PGA_CTRL(3), 0x80, 0x80);
    //Enable pop suppression
    regmap_write(djbu.regmap_l, DJBU_LR_REG_POP_SUPPRESS, 0x00);
    regmap_write(djbu.regmap_r, DJBU_LR_REG_POP_SUPPRESS, 0x00);
    //Enable DACs
    regmap_update_bits(djbu.regmap_l, DJBU_LR_REG_DAC_CTRL, 0x03, 0x03);
    regmap_update_bits(djbu.regmap_r, DJBU_LR_REG_DAC_CTRL, 0x03, 0x03);
    //Unmute headphone out pins
    regmap_write(djbu.regmap_l, DJBU_LR_REG_OP_STAGE_MUTE, 0x00);
    regmap_write(djbu.regmap_r, DJBU_LR_REG_OP_STAGE_MUTE, 0x00);
    //Power up headphone output stages and enable headphone out mode
    regmap_write(djbu.regmap_l, DJBU_LR_REG_OP_STAGE_CTRL, 0x30);
    regmap_write(djbu.regmap_r, DJBU_LR_REG_OP_STAGE_CTRL, 0x30);
    //Power up decimators, ADC filters, DAC modulators, and interpolators
    regmap_write(djbu.regmap_l, DJBU_LR_REG_DECIM_PWR, 0xFF);
    regmap_write(djbu.regmap_r, DJBU_LR_REG_DECIM_PWR, 0xFF);
    regmap_write(djbu.regmap_l, DJBU_LR_REG_INTERP_PWR, 0x0F);
    regmap_write(djbu.regmap_r, DJBU_LR_REG_INTERP_PWR, 0x0F);
    //High drive strength on output or I/O pins
    regmap_write(djbu.regmap_l, DJBU_LR_REG_PAD_CONTROL4, 0x06);
    regmap_write(djbu.regmap_r, DJBU_LR_REG_PAD_CONTROL4, 0x06);
    regmap_write(djbu.regmap_l, DJBU_LR_REG_PAD_CONTROL5, 0x0C);
    regmap_write(djbu.regmap_r, DJBU_LR_REG_PAD_CONTROL5, 0x0C);
    //
    printk(KERN_DEBUG "DJBU now running\n");
  }else{
    //Disable everything for playback/capture
    //Disable ASRCIs/ASRCOs
    regmap_update_bits(djbu.regmap_l, DJBU_LR_REG_ASRC_MODE, 0x03, 0x00);
    regmap_update_bits(djbu.regmap_r, DJBU_LR_REG_ASRC_MODE, 0x03, 0x00);
    //Disable ADCs
    regmap_update_bits(djbu.regmap_l, DJBU_LR_REG_ADC_CTRL2, 0x03, 0x00);
    regmap_update_bits(djbu.regmap_r, DJBU_LR_REG_ADC_CTRL2, 0x03, 0x00);
    regmap_update_bits(djbu.regmap_l, DJBU_LR_REG_ADC_CTRL3, 0x03, 0x00);
    regmap_update_bits(djbu.regmap_r, DJBU_LR_REG_ADC_CTRL3, 0x03, 0x00);
    //Disable PGAs
    regmap_update_bits(djbu.regmap_l, DJBU_LR_REG_PGA_CTRL(0), 0x80, 0x00);
    regmap_update_bits(djbu.regmap_r, DJBU_LR_REG_PGA_CTRL(0), 0x80, 0x00);
    regmap_update_bits(djbu.regmap_l, DJBU_LR_REG_PGA_CTRL(1), 0x80, 0x00);
    regmap_update_bits(djbu.regmap_r, DJBU_LR_REG_PGA_CTRL(1), 0x80, 0x00);
    regmap_update_bits(djbu.regmap_l, DJBU_LR_REG_PGA_CTRL(2), 0x80, 0x00);
    regmap_update_bits(djbu.regmap_r, DJBU_LR_REG_PGA_CTRL(2), 0x80, 0x00);
    regmap_update_bits(djbu.regmap_l, DJBU_LR_REG_PGA_CTRL(3), 0x80, 0x00);
    regmap_update_bits(djbu.regmap_r, DJBU_LR_REG_PGA_CTRL(3), 0x80, 0x00);
    //Disable DACs
    regmap_update_bits(djbu.regmap_l, DJBU_LR_REG_DAC_CTRL, 0x03, 0x00);
    regmap_update_bits(djbu.regmap_r, DJBU_LR_REG_DAC_CTRL, 0x03, 0x00);
    //Mute headphone out pins
    regmap_write(djbu.regmap_l, DJBU_LR_REG_OP_STAGE_MUTE, 0x0F);
    regmap_write(djbu.regmap_r, DJBU_LR_REG_OP_STAGE_MUTE, 0x0F);
    //Power down headphone output stages
    regmap_update_bits(djbu.regmap_l, DJBU_LR_REG_OP_STAGE_CTRL, 0x0F, 0x0F);
    regmap_update_bits(djbu.regmap_r, DJBU_LR_REG_OP_STAGE_CTRL, 0x0F, 0x0F);
    //Power down decimators, ADC filters, DAC modulators, and interpolators
    regmap_write(djbu.regmap_l, DJBU_LR_REG_DECIM_PWR, 0x00);
    regmap_write(djbu.regmap_r, DJBU_LR_REG_DECIM_PWR, 0x00);
    regmap_write(djbu.regmap_l, DJBU_LR_REG_INTERP_PWR, 0x00);
    regmap_write(djbu.regmap_r, DJBU_LR_REG_INTERP_PWR, 0x00);
    //
    printk(KERN_DEBUG "DJBU now stopped\n");
  }
  djbu.running = running;
}

static int djbum_write(struct snd_soc_component *component, unsigned int reg, unsigned int val)
{
  unsigned int ear_r = reg & 1;
  unsigned int reg_main = reg & ~1;
  struct regmap *map = ear_r ? djbu.regmap_r : djbu.regmap_l;
  //printk(KERN_DEBUG "djbum_write @%02X = %02X\n", reg, val);
  switch(reg_main){
    case DJBUM_REG_BIAS_MIC:
      regmap_update_bits(map, DJBU_LR_REG_BIAS_CTRL1, 0x0C, (val & 0x3) << 2);
      break;
    case DJBUM_REG_BIAS_AFE01:
      regmap_update_bits(map, DJBU_LR_REG_BIAS_CTRL0, 0x30, (val & 0x3) << 4);
      break;
    case DJBUM_REG_BIAS_AFE23:
      regmap_update_bits(map, DJBU_LR_REG_BIAS_CTRL1, 0x30, (val & 0x3) << 4);
      break;
    case DJBUM_REG_BIAS_ADC01:
      regmap_update_bits(map, DJBU_LR_REG_BIAS_CTRL0, 0x03, (val & 0x3) << 0);
      break;
    case DJBUM_REG_BIAS_ADC23:
      regmap_update_bits(map, DJBU_LR_REG_BIAS_CTRL0, 0x0C, (val & 0x3) << 2);
      break;
    case DJBUM_REG_HPF_ADC01:
      regmap_update_bits(map, DJBU_LR_REG_ADC_CTRL2,  0x60, (val & 0x3) << 5);
      break;
    case DJBUM_REG_HPF_ADC23:
      regmap_update_bits(map, DJBU_LR_REG_ADC_CTRL3,  0x60, (val & 0x3) << 5);
      break;
    case DJBUM_REG_PGA_VOL(0):
    case DJBUM_REG_PGA_VOL(1):
    case DJBUM_REG_PGA_VOL(2):
    case DJBUM_REG_PGA_VOL(3):
      regmap_update_bits(map, DJBU_LR_REG_PGA_CTRL((reg_main - DJBUM_REG_PGA_VOL(0)) >> 1),
        0x3F, (val & 0x3F) << 0);
      break;
    case DJBUM_REG_PGA_MUTE:
      regmap_update_bits(map, DJBU_LR_REG_PGA_CTRL(0), 0x40, ((val >> 0) & 1) << 6);
      regmap_update_bits(map, DJBU_LR_REG_PGA_CTRL(1), 0x40, ((val >> 1) & 1) << 6);
      regmap_update_bits(map, DJBU_LR_REG_PGA_CTRL(2), 0x40, ((val >> 2) & 1) << 6);
      regmap_update_bits(map, DJBU_LR_REG_PGA_CTRL(3), 0x40, ((val >> 3) & 1) << 6);
      break;
    case DJBUM_REG_PGA_BOOST:
      regmap_write(map, DJBU_LR_REG_PGA_BOOST, val & 0xF);
      break;
    case DJBUM_REG_ADC_VOL(0):
    case DJBUM_REG_ADC_VOL(1):
    case DJBUM_REG_ADC_VOL(2):
    case DJBUM_REG_ADC_VOL(3):
      regmap_write(map, DJBU_LR_REG_ADC_VOL((reg_main - DJBUM_REG_ADC_VOL(0)) >> 1), val);
      break;
    case DJBUM_REG_ADC_MUTE:
      regmap_update_bits(map, DJBU_LR_REG_ADC_CTRL0, 0x18, ((val >> 0) & 3) << 3);
      regmap_update_bits(map, DJBU_LR_REG_ADC_CTRL1, 0x18, ((val >> 2) & 3) << 3);
      break;
    case DJBUM_REG_ASRCO01_SRC:
      regmap_write(map, DJBU_LR_REG_ASRCO_SOURCE_0_1, val);
      break;
    case DJBUM_REG_ASRCO23_SRC:
      regmap_write(map, DJBU_LR_REG_ASRCO_SOURCE_2_3, val);
      break;
    case DJBUM_REG_CAP_PWR:
      if(ear_r){
        printk(KERN_INFO "--Invalid right ear for DJBUM_REG_CAP_PWR!\n");
        return -EINVAL;
      }
      djbu.reg_cap_state = val;
      djbu_setrun(djbu.reg_cap_state != 0 || djbu.reg_ply_state != 0);
      break;
    default:
      printk(KERN_INFO "--Invalid register!\n");
      return -EINVAL;
  }
  return 0;
}

static unsigned int djbum_read(struct snd_soc_component *component, unsigned int reg)
{
  unsigned int val = 0;
  unsigned int ear_r = reg & 1;
  unsigned int reg_main = reg & ~1;
  unsigned int i;
  struct regmap *map = ear_r ? djbu.regmap_r : djbu.regmap_l;
  //printk(KERN_DEBUG "djbum_read @%02X\n", reg);
  switch(reg_main){
    case DJBUM_REG_BIAS_MIC:
      regmap_read(map, DJBU_LR_REG_BIAS_CTRL1, &val);
      val = (val >> 2) & 0x3;
      break;
    case DJBUM_REG_BIAS_AFE01:
      regmap_read(map, DJBU_LR_REG_BIAS_CTRL0, &val);
      val = (val >> 4) & 0x3;
      break;
    case DJBUM_REG_BIAS_AFE23:
      regmap_read(map, DJBU_LR_REG_BIAS_CTRL1, &val);
      val = (val >> 4) & 0x3;
      break;
    case DJBUM_REG_BIAS_ADC01:
      regmap_read(map, DJBU_LR_REG_BIAS_CTRL0, &val);
      val = (val >> 0) & 0x3;
      break;
    case DJBUM_REG_BIAS_ADC23:
      regmap_read(map, DJBU_LR_REG_BIAS_CTRL0, &val);
      val = (val >> 2) & 0x3;
      break;
    case DJBUM_REG_HPF_ADC01:
      regmap_read(map, DJBU_LR_REG_ADC_CTRL2, &val);
      val = (val >> 5) & 0x3;
      break;
    case DJBUM_REG_HPF_ADC23:
      regmap_read(map, DJBU_LR_REG_ADC_CTRL3, &val);
      val = (val >> 5) & 0x3;
      break;
    case DJBUM_REG_PGA_VOL(0):
    case DJBUM_REG_PGA_VOL(1):
    case DJBUM_REG_PGA_VOL(2):
    case DJBUM_REG_PGA_VOL(3):
      regmap_read(map, DJBU_LR_REG_PGA_CTRL((reg_main - DJBUM_REG_PGA_VOL(0)) >> 1), &val);
      val = (val >> 0) & 0x3F;
      break;
    case DJBUM_REG_PGA_MUTE:
      regmap_read(map, DJBU_LR_REG_PGA_CTRL(0), &i);
      val |= ((i >> 6) & 1) << 0;
      regmap_read(map, DJBU_LR_REG_PGA_CTRL(1), &i);
      val |= ((i >> 6) & 1) << 1;
      regmap_read(map, DJBU_LR_REG_PGA_CTRL(2), &i);
      val |= ((i >> 6) & 1) << 2;
      regmap_read(map, DJBU_LR_REG_PGA_CTRL(3), &i);
      val |= ((i >> 6) & 1) << 3;
      break;
    case DJBUM_REG_PGA_BOOST:
      regmap_read(map, DJBU_LR_REG_PGA_BOOST, &val);
      val &= 0xF;
      break;
    case DJBUM_REG_ADC_VOL(0):
    case DJBUM_REG_ADC_VOL(1):
    case DJBUM_REG_ADC_VOL(2):
    case DJBUM_REG_ADC_VOL(3):
      regmap_read(map, DJBU_LR_REG_ADC_VOL((reg_main - DJBUM_REG_ADC_VOL(0)) >> 1), &val);
      break;
    case DJBUM_REG_ADC_MUTE:
      regmap_read(map, DJBU_LR_REG_ADC_CTRL0, &i);
      val |= ((i >> 3) & 3) << 0;
      regmap_read(map, DJBU_LR_REG_ADC_CTRL1, &i);
      val |= ((i >> 3) & 3) << 2;
      break;
    case DJBUM_REG_ASRCO01_SRC:
      regmap_read(map, DJBU_LR_REG_ASRCO_SOURCE_0_1, &val);
      break;
    case DJBUM_REG_ASRCO23_SRC:
      regmap_read(map, DJBU_LR_REG_ASRCO_SOURCE_2_3, &val);
      break;
    case DJBUM_REG_CAP_PWR:
      if(ear_r){
        printk(KERN_INFO "--Invalid right ear for DJBUM_REG_CAP_PWR!\n");
        break;
      }
      val = djbu.reg_cap_state;
      break;
    default:
      printk(KERN_INFO "--Invalid register!\n");
  }
  return val;
}

static int djbus_write(struct snd_soc_component *component, unsigned int reg, unsigned int val)
{
  unsigned int ear_r = reg & 1;
  unsigned int reg_main = reg & ~1;
  struct regmap *map = ear_r ? djbu.regmap_r : djbu.regmap_l;
  //printk(KERN_DEBUG "djbus_write @%02X = %02X\n", reg, val);
  switch(reg_main){
    case DJBUS_REG_BIAS_DAC:
      regmap_update_bits(map, DJBU_LR_REG_BIAS_CTRL1, 0x03, (val & 0x3) << 0);
      break;
    case DJBUS_REG_BIAS_HP:
      regmap_update_bits(map, DJBU_LR_REG_BIAS_CTRL0, 0xC0, (val & 0x3) << 6);
      break;
    case DJBUS_REG_DAC_VOL(0):
    case DJBUS_REG_DAC_VOL(1):
      regmap_write(map, DJBU_LR_REG_DAC_VOL((reg_main - DJBUS_REG_DAC_VOL(0)) >> 1), val);
      break;
    case DJBUS_REG_DAC_MUTE:
      regmap_update_bits(map, DJBU_LR_REG_DAC_CTRL, 0x18, (val & 0x3) << 3);
      break;
    case DJBUS_REG_DAC_SRC:
      regmap_write(map, DJBU_LR_REG_DAC_SOURCE, val);
      break;
    case DJBUS_REG_DACINVERT:
      if(ear_r){
        printk(KERN_INFO "--Invalid right ear for DJBUS_REG_DACINVERT!\n");
        return -EINVAL;
      }
      regmap_update_bits(djbu.regmap_l, DJBU_LR_REG_DAC_CTRL, 0x20, (val & 0x1) << 5);
      regmap_update_bits(djbu.regmap_r, DJBU_LR_REG_DAC_CTRL, 0x20, (val & 0x1) << 5);
      break;
    case DJBUS_REG_PLY_PWR:
      if(ear_r){
        printk(KERN_INFO "--Invalid right ear for DJBUS_REG_PLY_PWR!\n");
        return -EINVAL;
      }
      djbu.reg_ply_state = val;
      djbu_setrun(djbu.reg_cap_state != 0 || djbu.reg_ply_state != 0);
      break;
    default:
      printk(KERN_INFO "--Invalid register!\n");
      return -EINVAL;
  }
  return 0;
}

static unsigned int djbus_read(struct snd_soc_component *component, unsigned int reg)
{
  unsigned int val = 0;
  unsigned int ear_r = reg & 1;
  unsigned int reg_main = reg & ~1;
  unsigned int i;
  struct regmap *map = ear_r ? djbu.regmap_r : djbu.regmap_l;
  //printk(KERN_DEBUG "djbus_read @%02X\n", reg);
  switch(reg_main){
  case DJBUS_REG_BIAS_DAC:
      regmap_read(map, DJBU_LR_REG_BIAS_CTRL1, &val);
      val = (val >> 0) & 0x3;
      break;
    case DJBUS_REG_BIAS_HP:
      regmap_read(map, DJBU_LR_REG_BIAS_CTRL0, &val);
      val = (val >> 6) & 0x3;
      break;
    case DJBUS_REG_DAC_VOL(0):
    case DJBUS_REG_DAC_VOL(1):
      regmap_read(map, DJBU_LR_REG_DAC_VOL((reg_main - DJBUS_REG_DAC_VOL(0)) >> 1), &val);
      break;
    case DJBUS_REG_DAC_MUTE:
      regmap_read(map, DJBU_LR_REG_DAC_CTRL, &val);
      val = (val >> 3) & 0x3;
      break;
    case DJBUS_REG_DAC_SRC:
      regmap_read(map, DJBU_LR_REG_DAC_SOURCE, &val);
      break;
    case DJBUS_REG_DACINVERT:
      if(ear_r){
        printk(KERN_INFO "--Invalid right ear for DJBUS_REG_DACINVERT!\n");
        break;
      }
      regmap_read(djbu.regmap_l, DJBU_LR_REG_DAC_CTRL, &val);
      val = (val >> 5) & 0x1;
      regmap_read(djbu.regmap_r, DJBU_LR_REG_DAC_CTRL, &i);
      i = (i >> 5) & 0x1;
      if(i != val){
        printk(KERN_INFO "--Both ears didn't have the same value for DJBUS_REG_DACINVERT!\n");
      }
      break;
    case DJBUS_REG_PLY_PWR:
      if(ear_r){
        printk(KERN_INFO "--Invalid right ear for DJBUS_REG_PLY_PWR!\n");
        break;
      }
      val = djbu.reg_ply_state;
      break;
    default:
      printk(KERN_INFO "--Invalid register!\n");
  }
  return val;
}

/*******************************************************************************
 Codec (DJBUM/DJBUS) DAI/codec functions
*******************************************************************************/

static int djbu_ms_set_power(enum snd_soc_bias_level level, bool spkr)
{
  bool flag = false;
  int readreg;
  switch (level) {
    case SND_SOC_BIAS_ON:
      //printk(KERN_INFO "djbu_ms_set_power ON, doing nothing\n");
      return 0; //Ignored in original codec driver
    case SND_SOC_BIAS_PREPARE:
      //printk(KERN_INFO "djbu_ms_set_power PREPARE, doing nothing\n");
      return 0; //Ignored in original codec driver
    case SND_SOC_BIAS_STANDBY:
      if(djbu.enabled_m || djbu.enabled_s){
        //printk(KERN_INFO "djbu_ms_set_power STANDBY, was already enabled (ok)\n");
        flag = true;
      }
      if(spkr) djbu.enabled_s = true;
      if(!spkr) djbu.enabled_m = true;
      if(flag) return 0;
      //printk(KERN_INFO "djbu_ms_set_power: turning on codecs\n");
      //Switch to SPI mode if necessary
      if(djbu.switch_mode_l) djbu.switch_mode_l(djbu.dev_l);
      if(djbu.switch_mode_r) djbu.switch_mode_r(djbu.dev_r);
      //Unfreeze register cache
      regcache_cache_only(djbu.regmap_l, false);
      regcache_cache_only(djbu.regmap_r, false);
      //Turn on master clock
      regmap_update_bits(djbu.regmap_l, DJBU_LR_REG_CLK_CTRL,
          DJBU_LR_CLK_CTRL_MCLK_EN, DJBU_LR_CLK_CTRL_MCLK_EN);
      regmap_update_bits(djbu.regmap_r, DJBU_LR_REG_CLK_CTRL,
          DJBU_LR_CLK_CTRL_MCLK_EN, DJBU_LR_CLK_CTRL_MCLK_EN);
      //Ensure all registers up to date
      regcache_mark_dirty(djbu.regmap_l);
      regcache_mark_dirty(djbu.regmap_r);
      regcache_sync(djbu.regmap_l);
      regcache_sync(djbu.regmap_r);
      //Read back a register from each of the codecs to check if present
      regcache_cache_bypass(djbu.regmap_l, true);
      regcache_cache_bypass(djbu.regmap_r, true);
      regmap_read(djbu.regmap_l, DJBU_LR_REG_CLK_CTRL, &readreg);
      if(readreg == (DJBU_LR_CLK_CTRL_STATE | DJBU_LR_CLK_CTRL_MCLK_EN)){
        printk(KERN_INFO "DJBUL connected and responding properly\n");
      }else{
        printk(KERN_INFO "DJBUL disconnected or not responding\n");
      }
      //printk(KERN_INFO "DJBUL DJBU_LR_REG_CLK_CTRL = 0x%02X (should be 0x03)\n", readreg);
      regmap_read(djbu.regmap_r, DJBU_LR_REG_CLK_CTRL, &readreg);
      if(readreg == (DJBU_LR_CLK_CTRL_STATE | DJBU_LR_CLK_CTRL_MCLK_EN)){
        printk(KERN_INFO "DJBUR connected and responding properly\n");
      }else{
        printk(KERN_INFO "DJBUR disconnected or not responding\n");
      }
      //printk(KERN_INFO "DJBUR DJBU_LR_REG_CLK_CTRL = 0x%02X (should be 0x03)\n", readreg);
      regcache_cache_bypass(djbu.regmap_l, false);
      regcache_cache_bypass(djbu.regmap_r, false);
      break;
    case SND_SOC_BIAS_OFF:
      if(spkr) djbu.enabled_s = false;
      if(!spkr) djbu.enabled_m = false;
      if(djbu.enabled_m || djbu.enabled_s){
        //printk(KERN_INFO "djbu_ms_set_power OFF, but other operation still on, doing nothing\n");
        return 0;
      }
      //printk(KERN_INFO "djbu_ms_set_power: turning off codecs\n");
      //Turn off master clock (and PLL if it was on)
      regmap_update_bits(djbu.regmap_l, DJBU_LR_REG_CLK_CTRL,
          DJBU_LR_CLK_CTRL_MCLK_EN | DJBU_LR_CLK_CTRL_PLL_EN, 0);
      regmap_update_bits(djbu.regmap_r, DJBU_LR_REG_CLK_CTRL,
          DJBU_LR_CLK_CTRL_MCLK_EN | DJBU_LR_CLK_CTRL_PLL_EN, 0);
      //Freeze register cache
      regcache_cache_only(djbu.regmap_l, true);
      regcache_cache_only(djbu.regmap_r, true);
      break;
  }
  return 0;
}

static int djbum_set_bias_level(struct snd_soc_component *component,
    enum snd_soc_bias_level level)
{
  //printk(KERN_INFO "djbum_set_bias_level level %d\n", (int)level);
  return djbu_ms_set_power(level, false);
}

static int djbus_set_bias_level(struct snd_soc_component *component,
    enum snd_soc_bias_level level)
{
  //printk(KERN_INFO "djbus_set_bias_level level %d\n", (int)level);
  return djbu_ms_set_power(level, true);
}

static int djbu_ms_set_dai_fmt(unsigned int fmt, bool spkr)
{
  //Check capablities of requested format
  if((fmt & SND_SOC_DAIFMT_MASTER_MASK) != SND_SOC_DAIFMT_CBS_CFS){
    printk(KERN_INFO "--Requested codec to be master, not supported in DJBs!\n");
    return -EINVAL;
  }
  if((fmt & SND_SOC_DAIFMT_INV_MASK) != SND_SOC_DAIFMT_NB_NF){
    //Both inversions are fully supported by the codecs, but not expected to be
    //supported by the I2S units, so we should make sure it's what we expected
    printk(KERN_INFO "--Requested non-standard BCLK/LRCLK inversion!\n");
    return -EINVAL;
  }
  if((fmt & SND_SOC_DAIFMT_FORMAT_MASK) != SND_SOC_DAIFMT_I2S){
    //Other formats are supported by codec and will be used in 4-mic-per-ear
    printk(KERN_INFO "--Requested non-standard DAI format!\n");
    return -EINVAL;
  }
  //Set up known format
  //printk(KERN_INFO "--Setting up standard format\n");
  regmap_update_bits(djbu.regmap_l, DJBU_LR_REG_SAI0,
      DJBU_LR_SAI0_DELAY_MASK | DJBU_LR_SAI0_SAI_MASK,
      DJBU_LR_SAI0_DELAY1     | DJBU_LR_SAI0_SAI_I2S);
  regmap_update_bits(djbu.regmap_r, DJBU_LR_REG_SAI0,
      DJBU_LR_SAI0_DELAY_MASK | DJBU_LR_SAI0_SAI_MASK,
      DJBU_LR_SAI0_DELAY1     | DJBU_LR_SAI0_SAI_I2S);
  regmap_update_bits(djbu.regmap_l, DJBU_LR_REG_SAI1, 0xFF, 0);
  regmap_update_bits(djbu.regmap_r, DJBU_LR_REG_SAI1, 0xFF, 0);
  return 0;
}

static int djbum_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
  //printk(KERN_INFO "djbum_set_dai_fmt format 0x%X\n", fmt);
  return djbu_ms_set_dai_fmt(fmt, false);
}

static int djbus_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
  //printk(KERN_INFO "djbum_set_dai_fmt format 0x%X\n", fmt);
  return djbu_ms_set_dai_fmt(fmt, true);
}

static int djbu_ms_set_tdm_slot(unsigned int tx_mask,
    unsigned int rx_mask, int slots, int width, bool spkr)
{
  if(width != 32){
    printk(KERN_INFO "--Only 32-bit signed little endian supported!\n");
    return -EINVAL;
  }
  if(spkr){
    if(rx_mask != 0){
      printk(KERN_INFO "--DJBUS can't receive!\n");
      return -EINVAL;
    }
    if(tx_mask != 0x3 && tx_mask != 0xF){ //2 or 4 channels
      printk(KERN_INFO "--DJBUS must have channel mask 0b11 (2-channel) or 0b1111 (4-channel)!\n");
      return -EINVAL;
    }
  }else{
    if(tx_mask != 0){
      printk(KERN_INFO "--DJBUM can't transmit!\n");
      return -EINVAL;
    }
    if(tx_mask != 0xF){ //4 channels
      printk(KERN_INFO "--DJBUM must have channel mask 0b1111 (4-channel)!\n");
      return -EINVAL;
    }
  }
  //Don't know exactly what to do with slots
  return 0;
}

static int djbum_set_tdm_slot(struct snd_soc_dai *dai, unsigned int tx_mask,
    unsigned int rx_mask, int slots, int width)
{
  //printk(KERN_INFO "djbum_set_tdm_slot: tx_mask 0x%X, rx_mask 0x%X, slots %d, width %d\n", tx_mask, rx_mask, slots, width);
  return djbu_ms_set_tdm_slot(tx_mask, rx_mask, slots, width, false);
}

static int djbus_set_tdm_slot(struct snd_soc_dai *dai, unsigned int tx_mask,
    unsigned int rx_mask, int slots, int width)
{
  //printk(KERN_INFO "djbus_set_tdm_slot: tx_mask 0x%X, rx_mask 0x%X, slots %d, width %d\n", tx_mask, rx_mask, slots, width);
  return djbu_ms_set_tdm_slot(tx_mask, rx_mask, slots, width, true);
}

static int djbu_ms_set_tristate(int tristate, bool spkr)
{
  if(tristate){
    printk(KERN_INFO "--Tristate not supported!\n");
    return -EINVAL;
  }
  return 0;
}

static int djbum_set_tristate(struct snd_soc_dai *dai, int tristate)
{
  //printk(KERN_INFO "djbum_set_tristate: %d\n", tristate);
  return djbu_ms_set_tristate(tristate, false);
}

static int djbus_set_tristate(struct snd_soc_dai *dai, int tristate)
{
  //printk(KERN_INFO "djbus_set_tristate: %d\n", tristate);
  return djbu_ms_set_tristate(tristate, true);
}

static int djbu_ms_hw_params(struct snd_pcm_hw_params *params, bool spkr)
{
  unsigned int rate = params_rate(params);
  unsigned int width = params_width(params);
  unsigned int ri;
  //printk(KERN_INFO "--rate = %d, width = %d\n", rate, width);
  //Check width
  if(width != 32){
    printk(KERN_INFO "--Width must be 32!\n");
    return -EINVAL;
  }
  //Check rate
  for(ri=0; ri<ARRAY_SIZE(djbu_ms_rates); ++ri){
    if(rate == djbu_ms_rates[ri]) break;
  }
  if(ri == ARRAY_SIZE(djbu_ms_rates)){
    printk(KERN_INFO "--Unsupported sample rate!\n");
    return -EINVAL;
  }
  if(djbu.running){
    //Make sure the rate is the same one in use
    regmap_read(djbu.regmap_l, DJBU_LR_REG_SAI0, &rate);
    rate &= DJBU_LR_SAI0_FS_MASK;
    if(rate != ri){
      printk(KERN_INFO "--DJBU_L already running at rate %d!\n", rate >= ARRAY_SIZE(djbu_lr_rates) ? -1 : djbu_ms_rates[rate]);
      return -EINVAL;
    }
    regmap_read(djbu.regmap_r, DJBU_LR_REG_SAI0, &rate);
    rate &= DJBU_LR_SAI0_FS_MASK;
    if(rate != ri){
      printk(KERN_INFO "--DJBU_R already running at rate %d!\n", rate >= ARRAY_SIZE(djbu_lr_rates) ? -1 : djbu_ms_rates[rate]);
      return -EINVAL;
    }
  }else{
    //Set the rate
    //printk(KERN_INFO "--Setting rate\n");
    regmap_update_bits(djbu.regmap_l, DJBU_LR_REG_SAI0, DJBU_LR_SAI0_FS_MASK, ri);
    regmap_update_bits(djbu.regmap_r, DJBU_LR_REG_SAI0, DJBU_LR_SAI0_FS_MASK, ri);
  }
  return 0;
}

static int djbum_hw_params(struct snd_pcm_substream *substream,
    struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
  //printk(KERN_INFO "djbum_hw_params\n");
  return djbu_ms_hw_params(params, false);
}

static int djbus_hw_params(struct snd_pcm_substream *substream,
    struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
  //printk(KERN_INFO "djbus_hw_params\n");
  return djbu_ms_hw_params(params, true);
}

static int djbu_ms_startup(struct snd_pcm_substream *substream, bool spkr)
{
  snd_pcm_hw_constraint_list(substream->runtime, 0,
      SNDRV_PCM_HW_PARAM_RATE, &djbu.rate_constraints);

  return 0;
}

static int djbum_startup(struct snd_pcm_substream *substream,
    struct snd_soc_dai *dai)
{
  //printk(KERN_INFO "djbum_startup\n");
  return djbu_ms_startup(substream, false);
}

static int djbus_startup(struct snd_pcm_substream *substream,
    struct snd_soc_dai *dai)
{
  //printk(KERN_INFO "djbus_startup\n");
  return djbu_ms_startup(substream, true);
}

/*******************************************************************************
 Codec (DJBUM/DJBUS) driver definitions
*******************************************************************************/

static struct snd_soc_component_driver djbum_codec_driver = {
/* .get_regmap = NULL, switch to snd_soc_component_init_regmap. ref commit c6766aae8e */
  .write		= djbum_write,
  .read			= djbum_read,
  .controls		= djbum_controls,
  .num_controls		= ARRAY_SIZE(djbum_controls),
  .dapm_widgets		= djbum_dapm_widgets,
  .num_dapm_widgets	= ARRAY_SIZE(djbum_dapm_widgets),
  .dapm_routes		= djbum_dapm_routes,
  .num_dapm_routes	= ARRAY_SIZE(djbum_dapm_routes),
  .set_bias_level	= djbum_set_bias_level,
  .idle_bias_on		= 0,
  .endianness		= 1,
  .non_legacy_dai_naming	= 1,
};

static struct snd_soc_component_driver djbus_codec_driver = {
/* .get_regmap = NULL, switch to snd_soc_component_init_regmap. ref commit c6766aae8e */
  .write		= djbus_write,
  .read			= djbus_read,
  .set_bias_level	= djbus_set_bias_level,
  .controls		= djbus_controls,
  .num_controls		= ARRAY_SIZE(djbus_controls),
  .dapm_widgets		= djbus_dapm_widgets,
  .num_dapm_widgets	= ARRAY_SIZE(djbus_dapm_widgets),
  .dapm_routes		= djbus_dapm_routes,
  .num_dapm_routes	= ARRAY_SIZE(djbus_dapm_routes),
  .idle_bias_on		= 0,
  .endianness		= 1,
  .non_legacy_dai_naming	= 1,
};

static const struct snd_soc_dai_ops djbum_dai_ops = {
  .set_fmt      = djbum_set_dai_fmt,
  .set_tdm_slot = djbum_set_tdm_slot,
  .set_tristate = djbum_set_tristate,
  .hw_params    = djbum_hw_params,
  .startup      = djbum_startup,
};

static const struct snd_soc_dai_ops djbus_dai_ops = {
  .set_fmt      = djbus_set_dai_fmt,
  .set_tdm_slot = djbus_set_tdm_slot,
  .set_tristate = djbus_set_tristate,
  .hw_params    = djbus_hw_params,
  .startup      = djbus_startup,
};

static struct snd_soc_dai_driver djbum_dai_driver = {
  .name = "djbum",
  .capture = {
    .stream_name = "Capture",
    .channels_min = 4,
    .channels_max = 4,
    .rates = DJBU_MS_RATES,
    .formats = DJBU_MS_FORMATS,
    .sig_bits = 32,
  },
  .ops = &djbum_dai_ops,
};

static struct snd_soc_dai_driver djbus_dai_driver = {
  .name = "djbus",
  .playback = {
    .stream_name = "Playback",
    .channels_min = 2,
    .channels_max = 4,
    .rates = DJBU_MS_RATES,
    .formats = DJBU_MS_FORMATS,
    .sig_bits = 32,
  },
  .ops = &djbus_dai_ops,
};

/*******************************************************************************
 Device (DJBUL/DJBUR) probe/remove
*******************************************************************************/

int djbu_probe(struct device *dev, struct regmap *regmap,
  void (*switch_mode)(struct device *dev), bool ear_r)
{
  int ret;
  printk(KERN_INFO "DJBU_%c: djbu_probe\n", ear_r ? 'R' : 'L');

  if((ear_r && djbu.dev_r != NULL) || (!ear_r && djbu.dev_l != NULL)){
    printk(KERN_INFO "--Already have DJBU_%c registered!\n", ear_r ? 'R' : 'L');
    return -EINVAL;
  }

  //Ensure register map valid
  if(IS_ERR(regmap)){
    printk(KERN_INFO "--Given invalid register map!\n");
    return PTR_ERR(regmap);
  }

  //Store device info in driver data
  if(ear_r){
    djbu.dev_r = dev;
    djbu.regmap_r = regmap;
    djbu.switch_mode_r = switch_mode;
  }else{
    djbu.dev_l = dev;
    djbu.regmap_l = regmap;
    djbu.switch_mode_l = switch_mode;
  }

  //Store driver data in device info
  dev_set_drvdata(dev, &djbu);

  //Set register map to cache only (don't update device)
  regcache_cache_only(regmap, true);

  //Manual clock setup: External 12.288 MHz clock source connected
  regmap_update_bits(regmap, DJBU_LR_REG_CLK_CTRL,
      DJBU_LR_CLK_CTRL_STATE, DJBU_LR_CLK_CTRL_STATE);

  //Set up multipurpose pins:
  regmap_write(regmap, DJBU_LR_REG_MODE_MP(0), 0x00); //Serial Input 0
  regmap_write(regmap, DJBU_LR_REG_MODE_MP(1), 0x00); //Serial Output 0
  regmap_write(regmap, DJBU_LR_REG_MODE_MP(4), 0x00); //DMIC In 0/1, not used by harmless
  regmap_write(regmap, DJBU_LR_REG_MODE_MP(5), 0x00); //DMIC In 2/3, not used by harmless
  regmap_write(regmap, DJBU_LR_REG_MODE_MP(6), 0x00); //Serial Output 1

  //Other setup:
  regmap_write(regmap, DJBU_LR_REG_OP_STAGE_MUTE, 0x0); //Unmute all HP drivers
  //regmap_write(regmap, 0x7, 0x01); //Set clock out to divide by 1

  if(djbu.dev_l != NULL && djbu.dev_r != NULL){
    //One-time setup
    djbu.enabled_m = false;
    djbu.enabled_s = false;
    djbu.running = false;
    djbu.reg_cap_state = 0;
    djbu.reg_ply_state = 0;
    djbu.rate_constraints.list = djbu_ms_rates;
    djbu.rate_constraints.count = ARRAY_SIZE(djbu_ms_rates);
    djbu.rate_constraints.mask = DJBU_MS_RATE_MASK;

    //Register codec
    printk(KERN_INFO "--Have both L and R, registering codecs\n");
    //Have to specify a device, using DJBUL -> DJBUM and DJBUR -> DJBUS but hope this doesn't cause problems; if changed, make sure to update snd_soc_unregister_codec
    ret = devm_snd_soc_register_component(djbu.dev_l, &djbum_codec_driver,
        &djbum_dai_driver, 1);
    ret = devm_snd_soc_register_component(djbu.dev_r, &djbus_codec_driver,
        &djbus_dai_driver, 1);
  }else{
    ret = 0;
  }
  printk(KERN_INFO "--Returning %d\n", ret);
  return ret;
}
EXPORT_SYMBOL(djbu_probe);

int djbu_remove(struct device *dev, bool ear_r)
{
  printk(KERN_INFO "DJBU_%c: djbu_remove\n", ear_r ? 'R' : 'L');
  if(djbu.dev_l != NULL && djbu.dev_r != NULL){
    printk(KERN_INFO "--Unregistering codecs\n");
    /* deprecated
    snd_soc_unregister_codec(djbu.dev_l);
    snd_soc_unregister_codec(djbu.dev_r);
    */
  }
  if(ear_r){
    if(djbu.dev_r == NULL){
      printk(KERN_INFO "--Tried to remove R, but already nonexistent\n");
      return -1;
    }
    djbu.dev_r = NULL;
  }else{
    if(djbu.dev_l == NULL){
      printk(KERN_INFO "--Tried to remove L, but already nonexistent\n");
      return -1;
    }
    djbu.dev_l = NULL;
  }
  return 0;
}
EXPORT_SYMBOL(djbu_remove);



MODULE_DESCRIPTION("ASoC DJBU CODEC driver");
MODULE_AUTHOR("Louis Pisha <lpisha@eng.ucsd.edu>");
MODULE_LICENSE("GPL v3");

/*
//Unused PLL and MCLK clock functions
static void djbu_enable_pll(struct regmap *regmap)
{
  unsigned int val, timeout = 0;
  int ret;

  regmap_update_bits(regmap,
      DJBL_REG_CLK_CTRL, DJBL_CLK_CTRL_PLL_EN,
      DJBL_CLK_CTRL_PLL_EN);
  do {
    //Takes about 1ms to lock
    usleep_range(1000, 2000);
    ret = regmap_read(regmap, DJBL_REG_PLL(5), &val);
    if (ret)
      break;
    timeout++;
  } while (!(val & 1) && timeout < 3);

  if (ret < 0 || !(val & 1)) {
    dev_err(djbu.dev_l, "Failed to lock PLL\n");
  }
}
static int djbu_setup_pll(struct regmap *regmap, unsigned int rate)
{
  uint8_t regs[5];
  unsigned int i;
  int ret;

  ret = adau_calc_pll_cfg(rate, 49152000, regs);
  if (ret < 0)
    return ret;

  for (i = 0; i < ARRAY_SIZE(regs); i++)
    regmap_write(regmap, DJBU_REG_PLL(i), regs[i]);

  return 0;
}
//Clock setup from _probe
unsigned int clk_ctrl;
unsigned long rate;
djbu.clk = devm_clk_get(dev, "mclk");
if (IS_ERR(djbu.clk)) {
  ret =  PTR_ERR(djbu.clk);
  return ret;
}
rate = clk_get_rate(djbu.clk);
switch (rate) {
  case 12288000:
    clk_ctrl = DJBU_CLK_CTRL_CC_MDIV;
    break;
  case 24576000:
    clk_ctrl = 0;
    break;
  default:
    clk_ctrl = 0;

    ret = djbu_setup_pll(djbu, rate);
    if (ret < 0) {
      return ret;
    }
    djbu.use_pll = true;
    break;
}
regmap_update_bits(regmap, DJBU_REG_CLK_CTRL,
    DJBU_CLK_CTRL_CC_MDIV, clk_ctrl);
//Clock enabling from _set_power
unsigned int clk_ctrl = DJBL_CLK_CTRL_MCLK_EN;
clk_prepare_enable(djbu.mclk);

//Clocks needs to be enabled before any other register can be accessed.
if (djbu.use_pll) {
  djbu_enable_pll();
  clk_ctrl |= DJBL_CLK_CTRL_CLKSRC;
}

regmap_update_bits(regmap, DJBL_REG_CLK_CTRL,
    DJBL_CLK_CTRL_MCLK_EN | DJBL_CLK_CTRL_CLKSRC,
    clk_ctrl);


clk_disable_unprepare(djbu.mclk);
*/
