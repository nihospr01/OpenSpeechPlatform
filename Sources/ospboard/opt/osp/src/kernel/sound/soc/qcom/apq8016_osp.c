// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2015 The Linux Foundation. All rights reserved.
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/jack.h>
#include <sound/soc.h>
#include <uapi/linux/input-event-codes.h>
#include <dt-bindings/sound/apq8016-lpass.h>
#include "common.h"

struct apq8016_sbc_data {
	struct snd_soc_card card;
	void __iomem *mic_iomux;
	void __iomem *spkr_iomux;
	struct snd_soc_jack jack;
	bool jack_setup;
};

// LPASS_CSR_GP_IO_MUX_MIC_CTL
#define MIC_CTL_TLMM_CODEC_SLK_OUT_SEL			BIT(0)
#define MIC_CTL_TLMM_SCLK_EN					BIT(1)
#define MIC_CTL_TLMM_DATA1_EN					BIT(2)
#define MIC_CTL_TLMM_DATA0_EN					BIT(3)
#define MIC_CTL_TLMM_DATA1_EN_SEL				BIT(4)
#define MIC_CTL_TLMM_DATA0_EN_SEL				BIT(5)
#define MIC_CTL_TLMM_DATAIN1_SEL				BIT(6)
#define MIC_CTL_TLMM_DATAIN0_SEL				BIT(7)
#define MIC_CTL_TLMM_WS_EN_VAL					BIT(8)
#define MIC_CTL_TLMM_WS_EN_SEL_01				BIT(9)
#define MIC_CTL_TLMM_WS_EN_SEL_10				BIT(10)
#define MIC_CTL_TLMM_WS_EN_SEL_11				(BIT(9) | BIT(10))
#define MIC_CTL_TLMM_WS_OUT_SEL_01				BIT(11)
#define MIC_CTL_TLMM_WS_OUT_SEL_10				BIT(12)
#define MIC_CTL_TLMM_WS_OUT_SEL_11				(BIT(11) | BIT(12))
#define MIC_CTL_QUA_DATAIN1_SEL					BIT(13)
#define MIC_CTL_QUA_DATAIN0_SEL					BIT(14)
#define MIC_CTL_QUA_SCLK_SEL					BIT(15)
#define MIC_CTL_QUA_WS_SLAVE_SEL_01				BIT(16)
#define MIC_CTL_QUA_WS_SLAVE_SEL_10				BIT(17)
#define MIC_CTL_QUA_WS_SLAVE_SEL_11				(BIT(16) | BIT(17))
#define MIC_CTL_TER_DATAIN1_SEL					BIT(18)
#define MIC_CTL_TER_DATAIN0_SEL					BIT(19)
#define MIC_CTL_CODEC_WS_SLAVE_SEL				BIT(20)
#define MIC_CTL_TER_WS_SLAVE_SEL				BIT(21)
#define MIC_CTL_MI2S_AUX_EXT_CLK_SEL			BIT(22)
#define MIC_CTL_CODEC_MIC_MI2S_EXT_CLK_SEL		BIT(23)
#define MIC_CTL_AUX_I2S_SCLK_BIT_EXT_CLK_SEL	BIT(24)

// LPASS_CSR_GP_IO_MUX_SPKR_CTL
#define SPKR_CTL_TLMM_CODEC_SLK_OUT_SEL			BIT(0)
#define SPKR_CTL_TLMM_MCLK_EN					BIT(1)
#define SPKR_CTL_TLMM_SCLK_EN					BIT(2)
#define SPKR_CTL_TLMM_DATA1_EN					BIT(3)
#define SPKR_CTL_TLMM_DATA0_EN					BIT(4)
#define SPKR_CTL_TLMM_WS_EN_VAL					BIT(5)
#define SPKR_CTL_TLMM_WS_OUT_SEL_01				BIT(6)
#define SPKR_CTL_TLMM_WS_OUT_SEL_10				BIT(7)
#define SPKR_CTL_TLMM_WS_OUT_SEL_11				(BIT(6) | BIT(7))
#define SPKR_CTL_SEC_WS_SLAVE_SEL_01			BIT(8)
#define SPKR_CTL_SEC_WS_SLAVE_SEL_10			BIT(9)
#define SPKR_CTL_SEC_WS_SLAVE_SEL_11			(BIT(8) | BIT(9))
#define SPKR_CTL_CODEC_DATAIN1_SEL				BIT(10)
#define SPKR_CTL_PRI_SEC_DATAOUT1_SEL			BIT(11)
#define SPKR_CTL_CODEC_DATAIN0_SEL				BIT(12)
#define SPKR_CTL_PRI_DATAOUT1_SEC_DATAOUT0_SEL	BIT(13)
#define SPKR_CTL_CODEC_WS_SLAVE_SEL_01			BIT(14)
#define SPKR_CTL_CODEC_WS_SLAVE_SEL_10			BIT(15)
#define SPKR_CTL_CODEC_WS_SLAVE_SEL_11			(BIT(14) | BIT(15))
#define SPKR_CTL_PRI_WS_SLAVE_SEL_01			BIT(16)
#define SPKR_CTL_PRI_WS_SLAVE_SEL_10			BIT(17)
#define SPKR_CTL_PRI_WS_SLAVE_SEL_11			(BIT(16) | BIT(17))
#define SPKR_CTL_TLMM_WS_EN_SEL_01				BIT(18)
#define SPKR_CTL_TLMM_WS_EN_SEL_10				BIT(19)
#define SPKR_CTL_TLMM_WS_EN_SEL_11				(BIT(18) | BIT(19))
#define SPKR_CTL_CODEC_SPKR_MI2S_EXT_CLK_SEL	BIT(20)

#define MIC_CTRL_TER_WS_SLAVE_SEL	BIT(21)
#define MIC_CTRL_QUA_WS_SLAVE_SEL_10	BIT(17)
#define MIC_CTRL_TLMM_SCLK_EN		BIT(1)
//#define	SPKR_CTL_PRI_WS_SLAVE_SEL_11	(BIT(17) | BIT(16))
#define DEFAULT_MCLK_RATE		9600000

static int apq8016_sbc_dai_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_dai *cpu_dai = asoc_rtd_to_cpu(rtd, 0);
	struct snd_soc_dai *codec_dai;
	struct snd_soc_component *component;
	struct snd_soc_card *card = rtd->card;
	struct apq8016_sbc_data *pdata = snd_soc_card_get_drvdata(card);
	int i, rval;

	uint32_t before_value_iomic = readl(pdata->mic_iomux);
	uint32_t before_value_iospkr = readl(pdata->spkr_iomux);
	dev_info(card->dev, "%s():  mic_iomux startup: 0x%08X\n", __func__, before_value_iomic);
	dev_info(card->dev, "%s(): spkr_iomux startup: 0x%08X\n", __func__, before_value_iospkr);

#if 0
	switch (cpu_dai->id) {
	case MI2S_PRIMARY:
		writel(readl(pdata->spkr_iomux) | SPKR_CTL_PRI_WS_SLAVE_SEL_11,
			pdata->spkr_iomux);
		break;

	case MI2S_QUATERNARY:
		/* Configure the Quat MI2S to TLMM */
		writel(readl(pdata->mic_iomux) | MIC_CTRL_QUA_WS_SLAVE_SEL_10 |
			MIC_CTRL_TLMM_SCLK_EN,
			pdata->mic_iomux);
		break;
	case MI2S_TERTIARY:
		writel(readl(pdata->mic_iomux) | MIC_CTRL_TER_WS_SLAVE_SEL |
			MIC_CTRL_TLMM_SCLK_EN,
			pdata->mic_iomux);

		break;

	default:
		dev_err(card->dev, "unsupported cpu dai configuration\n");
		return -EINVAL;

	}
#endif

	/*
	//Config for DJBs without FPGA
	switch (cpu_dai->id) {
	case MI2S_PRIMARY:
		// Config for transmit on external primary from PRI
		writel(	readl(pdata->spkr_iomux) | SPKR_CTL_TLMM_DATA1_EN |
						SPKR_CTL_TLMM_DATA0_EN | SPKR_CTL_PRI_WS_SLAVE_SEL_10 |
						SPKR_CTL_TLMM_WS_EN_SEL_10 | SPKR_CTL_CODEC_SPKR_MI2S_EXT_CLK_SEL,
				pdata->spkr_iomux);
		break;
	case MI2S_QUATERNARY:
		// Config for receive on external secondary from QUA
		writel(	readl(pdata->mic_iomux) | MIC_CTL_TLMM_SCLK_EN |
						MIC_CTL_TLMM_DATA1_EN_SEL | MIC_CTL_TLMM_DATA0_EN_SEL |
						MIC_CTL_TLMM_WS_EN_VAL | MIC_CTL_TLMM_WS_EN_SEL_10,
				pdata->mic_iomux);

		break;
	case MI2S_TERTIARY:
		writel(readl(pdata->mic_iomux) | MIC_CTL_TER_WS_SLAVE_SEL |
			MIC_CTL_TLMM_SCLK_EN,
			pdata->mic_iomux);
		break;
	default:
		dev_err(card->dev, "unsupported cpu dai configuration\n");
		rval = -EINVAL;
		break;
	}
	*/

	//Config for DJBs with FPGA
	switch (cpu_dai->id) {
	case MI2S_PRIMARY:
		// Config for transmit on external primary from PRI
		// with external clock and WS
		writel(	(readl(pdata->spkr_iomux) & ~(
		                //Bits cleared to 0
		                SPKR_CTL_TLMM_SCLK_EN | //Ext Pri SCLK Output Enable = 0: Input
		                SPKR_CTL_TLMM_WS_EN_VAL | //Override Ext Pri WS Dir = 0: Input
		                SPKR_CTL_PRI_SEC_DATAOUT1_SEL | // Ext Pri D1 Out = 0: PRI D1 Out
		                SPKR_CTL_PRI_DATAOUT1_SEC_DATAOUT0_SEL | // Ext Pri D1 Out = 0: PRI D1 Out
		                SPKR_CTL_PRI_WS_SLAVE_SEL_11 | //PRI WS In = (clear existing value)
		                SPKR_CTL_TLMM_WS_EN_SEL_11 //Ext Pri WS Dir = (clear existing value)
		        )) |
	                    //Bits set to 1
		                SPKR_CTL_TLMM_DATA1_EN | //Ext Pri D1 Output Enable = 1: Output
		                SPKR_CTL_TLMM_DATA0_EN | //Ext Pri D0 Output Enable = 1: Output
		                SPKR_CTL_PRI_WS_SLAVE_SEL_10 | //PRI WS In = 2: Ext Pri WS In
		                SPKR_CTL_TLMM_WS_EN_SEL_10 | //Ext Pri WS Dir = 2: Override
		                SPKR_CTL_CODEC_SPKR_MI2S_EXT_CLK_SEL //Local Main Clock <= 1: Ext Pri SCLK
		        , pdata->spkr_iomux); //Other bits don't care
		break;
	case MI2S_QUATERNARY:
		// Config for receive from external secondary on QUA
		// with external clock and WS
		writel(	(readl(pdata->mic_iomux) & ~(
		                //Bits cleared to 0
	                    MIC_CTL_TLMM_SCLK_EN | //Ext Sec SCLK = 0: Input
	                    MIC_CTL_TLMM_DATA1_EN | //Override Ext Sec D1 Dir = 0: Input
	                    MIC_CTL_TLMM_DATA0_EN | //Override Ext Sec D0 Dir = 0: Input
	                    MIC_CTL_TLMM_WS_EN_VAL | //Override Ext Sec WS Dir = 0: Input
	                    MIC_CTL_TLMM_WS_EN_SEL_11 | //Ext Sec WS Dir = (clear existing value)
	                    MIC_CTL_QUA_DATAIN0_SEL | //QUA D0 In = 0: Ext Sec D0 In
	                    MIC_CTL_QUA_SCLK_SEL | //QUA SCLK = 0: Local Aux Clock
	                    MIC_CTL_QUA_WS_SLAVE_SEL_11 //QUA WS In = (clear existing value)
	            )) |
	                    //Bits set to 1
	                    MIC_CTL_TLMM_DATA1_EN_SEL | //Ext Sec D1 Dir = 1: Override
	                    MIC_CTL_TLMM_DATA0_EN_SEL | //Ext Sec D0 Dir = 1: Override
	                    MIC_CTL_TLMM_WS_EN_SEL_10 | //Ext Sec WS Dir = 2: Override
			    MIC_CTL_QUA_WS_SLAVE_SEL_10 | //QUA WS In = 2: Ext Sec WS In
			    MIC_CTL_MI2S_AUX_EXT_CLK_SEL //Local Aux Clock = 1: Ext Sec SCLK In
	            , pdata->mic_iomux); //Other bits don't care
		break;
	case MI2S_TERTIARY:
		writel((readl(pdata->mic_iomux) & ~(
			//Bits cleared to 0
			MIC_CTL_TLMM_SCLK_EN | //Ext Sec SCLK = 0: Input
			MIC_CTL_TLMM_WS_EN_VAL | //Override Ext Sec WS Dir = 0: Input
			MIC_CTL_TLMM_WS_EN_SEL_11 | //Ext Sec WS Dir = (clear existing value)
			MIC_CTL_TER_DATAIN1_SEL | //TER D1 In = 0: CODEC D1 Out
			MIC_CTL_TER_DATAIN0_SEL //TER D0 In = 0: CODEC D0 Out
			)) |
			//Bits set to 1
			MIC_CTL_TLMM_WS_EN_SEL_10 | //Ext Sec WS Dir = 2: Override
			MIC_CTL_TER_WS_SLAVE_SEL | //TER WS In = 1: Ext Sec WS In
			MIC_CTL_CODEC_WS_SLAVE_SEL | //CODEC WS In = 1: Ext Sec WS In
			MIC_CTL_CODEC_MIC_MI2S_EXT_CLK_SEL //Local Main Clock <= 1: Ext Sec SCLK In
			, pdata->mic_iomux); //Other bits don't care
	    /*
	    // Config for receive from CODEC on TER
	    // with internal clock and WS
	    // TER must be configured to use EXTERNAL WS--in this case, external = from CODEC
	    // MI2S_TERTIARY is clocked from GCC_ULTAUDIO_LPAIF_SEC_I2S_CLK
	    // This sort-of works: when recording is done separately from playback,
	    // the recording is noisy but undistorted, BUT recording 10 seconds of
	    // audio takes about 8 seconds and the result (when played back at the
	    // supposed original rate) is pitch-shifted down. That is, the effective
	    // sampling rate is higher than it should be. As best I can tell the
	    // clock is being generated correctly, but perhaps CODEC is generating
	    // WS incorrectly (i.e. swapping it after 25 clocks instead of 32)?
	    // If capture and playback (to the DJBs) are occurring simultaneously,
	    // it runs in a distorted mode for a short time and then devolves into
	    // buffer-repeating noise (buzzing).
	    writel(	(readl(pdata->mic_iomux) & ~(
		                //Bits cleared to 0
	                    MIC_CTL_TER_DATAIN1_SEL | //TER D1 In = 0: CODEC D1 Out
	                    MIC_CTL_TER_DATAIN0_SEL | //TER D0 In = 0: CODEC D0 Out
	                    MIC_CTL_CODEC_WS_SLAVE_SEL | //CODEC WS In = 0: TER WS Out
	                    MIC_CTL_TER_WS_SLAVE_SEL | //TER WS In = 0: CODEC WS Out
	                    MIC_CTL_CODEC_MIC_MI2S_EXT_CLK_SEL //Local Main Clock = 0: Main Clock
	            ))
	            , pdata->mic_iomux); //Other bits don't care
        */
        /*
        // Config for receive from CODEC on TER
	    // with external (FPGA) clock and WS
	    // TER must be configured to use external WS--in this case, external = from FPGA
	    // This works correctly, though there is noise.
	    writel(	(readl(pdata->mic_iomux) & ~(
		                //Bits cleared to 0
		                MIC_CTL_TLMM_SCLK_EN | //Ext Sec SCLK = 0: Input
	                    MIC_CTL_TLMM_WS_EN_VAL | //Override Ext Sec WS Dir = 0: Input
	                    MIC_CTL_TLMM_WS_EN_SEL_11 | //Ext Sec WS Dir = (clear existing value)
	                    MIC_CTL_TER_DATAIN1_SEL | //TER D1 In = 0: CODEC D1 Out
	                    MIC_CTL_TER_DATAIN0_SEL //TER D0 In = 0: CODEC D0 Out
	            )) |
	                    //Bits set to 1
	                    MIC_CTL_TLMM_WS_EN_SEL_10 | //Ext Sec WS Dir = 2: Override
	                    MIC_CTL_CODEC_WS_SLAVE_SEL | //CODEC WS In = 1: Ext Sec WS In
	                    MIC_CTL_TER_WS_SLAVE_SEL | //TER WS In = 1: Ext Sec WS In
	                    MIC_CTL_CODEC_MIC_MI2S_EXT_CLK_SEL //Local Main Clock <= 1: Ext Sec SCLK In
	            , pdata->mic_iomux); //Other bits don't care
        */
        /*
        // Config for receive from CODEC on TER
	    // with external (FPGA) clock but internal WS
	    // TER must be configured to use EXTERNAL WS--in this case, external = from CODEC
	    // This has a similar problem to the case of the internal clock, except there are no
	    // buffer underruns, it just continues with a pitch-shifted effect forever.
	    // (The pitch-shifting is as if each buffer is being captured too fast
	    // but then there is blank space until each buffer is ready.)
	    writel(	(readl(pdata->mic_iomux) & ~(
		                //Bits cleared to 0
		                MIC_CTL_TLMM_SCLK_EN | //Ext Sec SCLK = 0: Input
	                    MIC_CTL_TER_DATAIN1_SEL | //TER D1 In = 0: CODEC D1 Out
	                    MIC_CTL_TER_DATAIN0_SEL | //TER D0 In = 0: CODEC D0 Out
	                    MIC_CTL_CODEC_WS_SLAVE_SEL | //CODEC WS In = 0: TER WS Out
	                    MIC_CTL_TER_WS_SLAVE_SEL //TER WS In = 0: CODEC WS Out
	            )) |
	                    //Bits set to 1
	                    MIC_CTL_CODEC_MIC_MI2S_EXT_CLK_SEL //Local Main Clock <= 1: Ext Sec SCLK In
	            , pdata->mic_iomux); //Other bits don't care
        */
		break;
	case MI2S_SECONDARY:
	    //MI2S_SECONDARY is not supported for two reasons:
	    //1)
        //In the SPKR_CTL mux block, there is only one local clock used for both
        //PRI and SEC. We have this set to Ext Pri SCLK so that PRI can receive
        //the external 48/96kHz clock from the FPGA. But, the codec requires the
        //clock to be 48kHz, so if the FPGA is configured for 96kHz this will break.
        //2)
        //Given that we are having PRI connect to both Ext Pri D0 and Ext Pri D1,
        //there is no way to get SEC to connect to either CODEC data input.
        //This configuration is ultimately not mandatory, since we can have the
        //same data line going to both DJBs (this is up to the CAR FPGA code),
        //with the left and right receiver data in the left and right channels.
        //In that case, SEC can be connected to Ext Pri D1 Out which can be used
        //for the DJBs, and PRI can be used for the codec.
        //
        //Currently, we will use MI2S_SECONDARY for the dummy codec playback,
        //but opening that device and sending audio over it will go nowhere.
        //(It is being clocked so it shouldn't hang.)
	    dev_err(card->dev, "MI2S_SECONDARY not supported, see comments in apq8016_sbc.c\n");
		break;
	default:
		dev_err(card->dev, "unsupported cpu dai configuration\n");
		rval = -EINVAL;
		break;
	}

	before_value_iomic = readl(pdata->mic_iomux);
	before_value_iospkr = readl(pdata->spkr_iomux);
	dev_info(card->dev, "%s():  mic_iomux after setup: 0x%08X\n", __func__, before_value_iomic);
	dev_info(card->dev, "%s(): spkr_iomux after setup: 0x%08X\n", __func__, before_value_iospkr);

	if (!pdata->jack_setup) {
		struct snd_jack *jack;

		rval = snd_soc_card_jack_new(card, "Headset Jack",
					     SND_JACK_HEADSET |
					     SND_JACK_HEADPHONE |
					     SND_JACK_BTN_0 | SND_JACK_BTN_1 |
					     SND_JACK_BTN_2 | SND_JACK_BTN_3 |
					     SND_JACK_BTN_4,
					     &pdata->jack, NULL, 0);

		if (rval < 0) {
			dev_err(card->dev, "Unable to add Headphone Jack\n");
			return rval;
		}

		jack = pdata->jack.jack;

		snd_jack_set_key(jack, SND_JACK_BTN_0, KEY_PLAYPAUSE);
		snd_jack_set_key(jack, SND_JACK_BTN_1, KEY_VOICECOMMAND);
		snd_jack_set_key(jack, SND_JACK_BTN_2, KEY_VOLUMEUP);
		snd_jack_set_key(jack, SND_JACK_BTN_3, KEY_VOLUMEDOWN);
		pdata->jack_setup = true;
	}

	for_each_rtd_codec_dais(rtd, i, codec_dai) {

		component = codec_dai->component;
		/* Set default mclk for internal codec */
		rval = snd_soc_component_set_sysclk(component, 0, 0, DEFAULT_MCLK_RATE,
				       SND_SOC_CLOCK_IN);
		if (rval != 0 && rval != -ENOTSUPP) {
			dev_warn(card->dev, "Failed to set mclk: %d\n", rval);
			return rval;
		}
		rval = snd_soc_component_set_jack(component, &pdata->jack, NULL);
		if (rval != 0 && rval != -ENOTSUPP) {
			dev_warn(card->dev, "Failed to set jack: %d\n", rval);
			return rval;
		}
	}

	return 0;
}

static void apq8016_sbc_add_ops(struct snd_soc_card *card)
{
	struct snd_soc_dai_link *link;
	int i;

	for_each_card_prelinks(card, i, link)
		link->init = apq8016_sbc_dai_init;
}

static const struct snd_soc_dapm_widget apq8016_sbc_dapm_widgets[] = {

	SND_SOC_DAPM_MIC("Handset Mic", NULL),
	SND_SOC_DAPM_MIC("Headset Mic", NULL),
	SND_SOC_DAPM_MIC("Secondary Mic", NULL),
	SND_SOC_DAPM_MIC("Digital Mic1", NULL),
	SND_SOC_DAPM_MIC("Digital Mic2", NULL),
};

static int apq8016_sbc_platform_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct snd_soc_card *card;
	struct apq8016_sbc_data *data;
	struct resource *res;
	int ret;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	card = &data->card;
	card->dev = dev;
	card->owner = THIS_MODULE;
	card->dapm_widgets = apq8016_sbc_dapm_widgets;
	card->num_dapm_widgets = ARRAY_SIZE(apq8016_sbc_dapm_widgets);

	ret = qcom_snd_parse_of(card);
	if (ret)
		return ret;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "mic-iomux");
	data->mic_iomux = devm_ioremap_resource(dev, res);
	if (IS_ERR(data->mic_iomux))
		return PTR_ERR(data->mic_iomux);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "spkr-iomux");
	data->spkr_iomux = devm_ioremap_resource(dev, res);
	if (IS_ERR(data->spkr_iomux))
		return PTR_ERR(data->spkr_iomux);

	snd_soc_card_set_drvdata(card, data);

	apq8016_sbc_add_ops(card);
	return devm_snd_soc_register_card(&pdev->dev, card);
}

static const struct of_device_id apq8016_sbc_device_id[] __maybe_unused = {
	{ .compatible = "qcom,apq8016-osp-sndcard" },
	{},
};
MODULE_DEVICE_TABLE(of, apq8016_sbc_device_id);

static struct platform_driver apq8016_sbc_platform_driver = {
	.driver = {
		.name = "qcom-apq8016-osp",
		.of_match_table = of_match_ptr(apq8016_sbc_device_id),
	},
	.probe = apq8016_sbc_platform_probe,
};
module_platform_driver(apq8016_sbc_platform_driver);

MODULE_AUTHOR("Srinivas Kandagatla <srinivas.kandagatla@linaro.org");
MODULE_DESCRIPTION("APQ8016 ASoC Machine Driver");
MODULE_LICENSE("GPL v2");
