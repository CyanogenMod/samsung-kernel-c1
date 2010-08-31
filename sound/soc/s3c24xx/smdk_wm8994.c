/*
 *  smdk_wm8994.c
 *
 *  Copyright (c) 2009 Samsung Electronics Co. Ltd
 *  Author: boojin,kim  <boojin.kim@samsung.com>
 *
 *  This program is free software; you can redistribute  it and/or  modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <mach/regs-clock.h>
#include "../codecs/wm8994.h"
#include "s3c-dma.h"
#include "s3c-pcm.h"
#include "s3c64xx-i2s.h"

/* smdkv310 has a 16.9344MHZ crystal attached to wm8994 */
#define SMDK_WM8994_OSC_FREQ 16934400

static int set_epll_rate(unsigned long rate)
{
	struct clk *fout_epll;

	fout_epll = clk_get(NULL, "fout_epll");
	if (IS_ERR(fout_epll)) {
		printk(KERN_ERR "%s: failed to get fout_epll\n", __func__);
		return -ENOENT;
	}

	if (rate == clk_get_rate(fout_epll))
		goto out;

	clk_disable(fout_epll);
	clk_set_rate(fout_epll, rate);
	clk_enable(fout_epll);
out:
	clk_put(fout_epll);

	return 0;
}

static int smdk_hifi_hw_params(struct snd_pcm_substream *substream,
			      struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->dai->codec_dai;
	int ret;

	/* Set the codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S
				| SND_SOC_DAIFMT_NB_NF
				| SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_sysclk(codec_dai, WM8994_SYSCLK_FLL1,
				params_rate(params)*256,
				SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_pll(codec_dai, WM8994_FLL1,
				WM8994_FLL_SRC_MCLK1,
				SMDK_WM8994_OSC_FREQ,
				params_rate(params)*256);
	if (ret < 0)
		return ret;

	/* Set the cpu DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S
				| SND_SOC_DAIFMT_NB_NF
				| SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C64XX_CLKSRC_CDCLK,
				0, SND_SOC_CLOCK_OUT);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C64XX_CLKSRC_MUX,
				0, SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	return 0;
}

static int smdk_voice_hw_params(struct snd_pcm_substream *substream,
			      struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->dai->codec_dai;
	int rfs, ret;
	unsigned long epll_out_rate;

	switch (params_rate(params)) {
	case 8000:
	case 12000:
	case 16000:
	case 24000:
	case 32000:
	case 48000:
	case 64000:
	case 96000:
		epll_out_rate = 49152000;
		break;
	case 11025:
	case 22050:
	case 44100:
	case 88200:
		epll_out_rate = 67737600;
		break;
	default:
		printk(KERN_ERR "%s:%d Sampling Rate %u not supported!\n",
			__func__, __LINE__, params_rate(params));
		return -EINVAL;
	}

	switch (params_rate(params)) {
	case 16000:
	case 22050:
	case 22025:
	case 32000:
	case 44100:
	case 48000:
	case 96000:
	case 24000:
		rfs = 256;
		break;
	case 64000:
		rfs = 384;
		break;
	case 8000:
	case 11025:
	case 12000:
		rfs = 512;
		break;
	case 88200:
		rfs = 128;
		break;
	default:
		printk(KERN_ERR "%s:%d Sampling Rate %u not supported!\n",
			__func__, __LINE__, params_rate(params));
		return -EINVAL;
	}

	/* Set the codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_DSP_B
				| SND_SOC_DAIFMT_IB_NF
				| SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_sysclk(codec_dai, WM8994_SYSCLK_MCLK1,
				SMDK_WM8994_OSC_FREQ,
				SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_pll(codec_dai, WM8994_FLL1,
				WM8994_FLL_SRC_MCLK1,
				SMDK_WM8994_OSC_FREQ,
				params_rate(params)*256);
	if (ret < 0)
		return ret;

	/* Set the cpu DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_DSP_B
				| SND_SOC_DAIFMT_IB_NF
				| SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	/* Set EPLL clock rate */
	ret = set_epll_rate(epll_out_rate);
	if (ret < 0)
		return ret;

	/* Set SCLK_DIV for making bclk */
	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_PCM_SCLK_PER_FS, rfs);
	if (ret < 0)
		return ret;

	return 0;
}

/* SMDK Audio  extra controls */
#define SMDK_AUDIO_OFF	0
#define SMDK_HEADSET_OUT	1
#define SMDK_MIC_IN		2
#define SMDK_LINE_IN	3

static int smdk_aud_mode;

static const char *smdk_aud_scenario[] = {
	[SMDK_AUDIO_OFF] = "Off",
	[SMDK_HEADSET_OUT] = "Playback Headphones",
	[SMDK_MIC_IN] = "Capture Mic",
	[SMDK_LINE_IN] = "Capture Line",
};

static void smdk_aud_ext_control(struct snd_soc_codec *codec)
{
	switch (smdk_aud_mode) {
	case SMDK_HEADSET_OUT:
		snd_soc_dapm_enable_pin(codec, "Headset Out");
		snd_soc_dapm_disable_pin(codec, "Mic In");
		snd_soc_dapm_disable_pin(codec, "Line In");
		break;
	case SMDK_MIC_IN:
		snd_soc_dapm_disable_pin(codec, "Headset Out");
		snd_soc_dapm_enable_pin(codec, "Mic In");
		snd_soc_dapm_disable_pin(codec, "Line In");
		break;
	case SMDK_LINE_IN:
		snd_soc_dapm_disable_pin(codec, "Headset Out");
		snd_soc_dapm_disable_pin(codec, "Mic In");
		snd_soc_dapm_enable_pin(codec, "Line In");
		break;
	case SMDK_AUDIO_OFF:
	default:
		snd_soc_dapm_disable_pin(codec, "Headset Out");
		snd_soc_dapm_disable_pin(codec, "Mic In");
		snd_soc_dapm_disable_pin(codec, "Line In");
		break;
	}

	snd_soc_dapm_sync(codec);
}

static int smdk_wm8994_getp(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = smdk_aud_mode;
	return 0;
}

static int smdk_wm8994_setp(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	if (smdk_aud_mode == ucontrol->value.integer.value[0])
		return 0;

	smdk_aud_mode = ucontrol->value.integer.value[0];
	smdk_aud_ext_control(codec);

	return 1;
}

static const struct snd_soc_dapm_widget smdk_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headset Out", NULL),
	SND_SOC_DAPM_MIC("Mic In", NULL),
	SND_SOC_DAPM_LINE("Line In", NULL),
};

static const struct soc_enum smdk_aud_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(smdk_aud_scenario),
			    smdk_aud_scenario),
};

static const struct snd_kcontrol_new smdk_aud_controls[] = {
	SOC_ENUM_EXT("Smdk Audio Mode", smdk_aud_enum[0],
		     smdk_wm8994_getp, smdk_wm8994_setp),
};

static const struct snd_soc_dapm_route smdk_dapm_routes[] = {
	/* Connections to Headset */
	{"Headset Out", NULL, "HPOUT1L"},
	{"Headset Out", NULL, "HPOUT1R"},
	/* Connections to Mics */
	{"IN1LN", NULL, "Mic In"},
	{"IN1RN", NULL, "Mic In"},
	/* Connections to Line In */
	{"IN2LN", NULL, "Line In"},
	{"IN2RN", NULL, "Line In"},
};

static int smdk_hifiaudio_init(struct snd_soc_codec *codec)
{
	int err;

	/* set up NC codec pins */
	snd_soc_dapm_nc_pin(codec, "SPKOUTLP");
	snd_soc_dapm_nc_pin(codec, "SPKOUTLN");
	snd_soc_dapm_nc_pin(codec, "SPKOUTRP");
	snd_soc_dapm_nc_pin(codec, "SPKOUTRN");
	snd_soc_dapm_nc_pin(codec, "HPOUT2P");
	snd_soc_dapm_nc_pin(codec, "HPOUT2N");
	snd_soc_dapm_nc_pin(codec, "LINEOUT1P");
	snd_soc_dapm_nc_pin(codec, "LINEOUT1N");
	snd_soc_dapm_nc_pin(codec, "LINEOUT2P");
	snd_soc_dapm_nc_pin(codec, "LINEOUT2N");

	/* Add smdk specific widgets */
	err = snd_soc_dapm_new_controls(codec, smdk_dapm_widgets,
					ARRAY_SIZE(smdk_dapm_widgets));
	if (err < 0)
		return err;

	/* Add smdk specific controls */
	err = snd_soc_add_controls(codec, smdk_aud_controls,
				   ARRAY_SIZE(smdk_aud_controls));
	if (err < 0)
		return err;

	/* Set up smdk specific audio routes */
	err = snd_soc_dapm_add_routes(codec, smdk_dapm_routes,
				      ARRAY_SIZE(smdk_dapm_routes));
	if (err < 0)
		return err;

	/* Set up default in/out pins */
	smdk_aud_mode = SMDK_HEADSET_OUT;
	smdk_aud_ext_control(codec);

	err = snd_soc_dapm_sync(codec);
	if (err < 0)
		return err;

	return 0;
}

/*
 * smdk wm8994 DAI operations.
 */
static struct snd_soc_ops smdk_hifi_ops = {
	.hw_params = smdk_hifi_hw_params,
};

static struct snd_soc_ops smdk_voice_ops = {
	.hw_params = smdk_voice_hw_params,
};

static struct snd_soc_dai_link smdk_dai[] = {
	{
	 .name = "wm8994 AIF1",
	 .stream_name = "hifiaudio",
	 .cpu_dai = &s3c64xx_i2s_v4_dai,
	 .codec_dai = &wm8994_dai[0],
	 .init = smdk_hifiaudio_init,
	 .ops = &smdk_hifi_ops,
	 },
	 {
	 .name = "wm8994 AIF2 pcm",
	 .stream_name = "voice",
	 .cpu_dai = &s3c_pcm_dai[1],
	 .codec_dai = &wm8994_dai[1],
	 .ops = &smdk_voice_ops,
	 },
};

static struct snd_soc_card smdk = {
	.name = "smdk",
	.platform = &s3c24xx_soc_platform,
	.dai_link = smdk_dai,
	.num_links = ARRAY_SIZE(smdk_dai),
};

static struct snd_soc_device smdk_snd_devdata = {
	.card = &smdk,
	.codec_dev = &soc_codec_dev_wm8994,
};

static struct platform_device *smdk_snd_device;

static int __init smdk_audio_init(void)
{
	int ret;

	smdk_snd_device = platform_device_alloc("soc-audio", -1);
	if (!smdk_snd_device)
		return -ENOMEM;

	platform_set_drvdata(smdk_snd_device, &smdk_snd_devdata);
	smdk_snd_devdata.dev = &smdk_snd_device->dev;

	ret = platform_device_add(smdk_snd_device);
	if (ret)
		platform_device_put(smdk_snd_device);
	return ret;
}

static void __exit smdk_audio_exit(void)
{
	platform_device_unregister(smdk_snd_device);
}

module_init(smdk_audio_init);

MODULE_AUTHOR("Boojin Kim, boojin.kim@samsung.com");
MODULE_DESCRIPTION("ALSA SoC smdk wm8994");
MODULE_LICENSE("GPL");
