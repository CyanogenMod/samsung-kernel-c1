/*
 *  smdkv2xx_wm8580pcm.c
 *
 *  Copyright (c) 2010 Samsung Electronics Co. Ltd
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include <mach/map.h>
#include <mach/regs-clock.h>

#include "../codecs/wm8580.h"
#include "s3c-dma.h"
#include "s3c-pcm.h"

#define SMDK_WM8580_XTI_FREQ	24000000
#define SMDK_WM8580_PCM_PORT	0

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

/* Note that PCM works __only__ in AP-Master mode */
static int smdk_socpcm_hw_params(struct snd_pcm_substream *substream,
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
		epll_out_rate = 67738000;
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

	/* Set the Codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_DSP_B
					 | SND_SOC_DAIFMT_IB_NF
					 | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	/* Set the AP DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_DSP_B
					 | SND_SOC_DAIFMT_IB_NF
					 | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	/* Set MUX for PCM clock source to audio-bus */
	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C_PCM_CLKSRC_MUX,
					0, SND_SOC_CLOCK_OUT);
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

	/* Set WM8580 to drive MCLK from PLLA */
	ret = snd_soc_dai_set_clkdiv(codec_dai, WM8580_MCLK,
					WM8580_CLKSRC_PLLA);
	if (ret < 0)
		return ret;

	/* Explicitly set WM8580-DAC to source from MCLK */
	ret = snd_soc_dai_set_clkdiv(codec_dai, WM8580_DAC_CLKSEL,
					WM8580_CLKSRC_MCLK);
	if (ret < 0)
		return ret;

	/* Explicitly set WM8580-ADC to source from MCLK */
	ret = snd_soc_dai_set_clkdiv(codec_dai, WM8580_ADC_CLKSEL,
					WM8580_CLKSRC_ADCMCLK);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_pll(codec_dai, WM8580_PLLA, 0,
					SMDK_WM8580_XTI_FREQ,
					params_rate(params) * rfs);
	if (ret < 0)
		return ret;

	return 0;
}

/*
 * SMDK WM8580 DAI operations.
 */
static struct snd_soc_ops smdk_pcm_ops = {
	.hw_params = smdk_socpcm_hw_params,
};

/* Playback widgets */
static const struct snd_soc_dapm_widget wm8580_dapm_widgets_pbk[] = {
	SND_SOC_DAPM_HP("Front-L/R", NULL),
};

/* Capture widgets */
static const struct snd_soc_dapm_widget wm8580_dapm_widgets_cpt[] = {
	SND_SOC_DAPM_MIC("MicIn", NULL),
	SND_SOC_DAPM_LINE("LineIn", NULL),
};

/* TX connections */
static const struct snd_soc_dapm_route audio_map_tx[] = {
	/* MicIn feeds AINL */
	{"AINL", NULL, "MicIn"},

	/* LineIn feeds AINL/R */
	{"AINL", NULL, "LineIn"},
	{"AINR", NULL, "LineIn"},
};

/* RX connections */
static const struct snd_soc_dapm_route audio_map_rx[] = {
	/* Front Left/Right are fed VOUT1L/R */
	{"Front-L/R", NULL, "VOUT1L"},
	{"Front-L/R", NULL, "VOUT1R"},
};

static int smdk_wm8580_pcm_init(struct snd_soc_codec *codec)
{
	/* Add smdk specific Playback widgets */
	snd_soc_dapm_new_controls(codec, wm8580_dapm_widgets_pbk,
				  ARRAY_SIZE(wm8580_dapm_widgets_pbk));

	/* Add smdk specific Capture widgets */
	snd_soc_dapm_new_controls(codec, wm8580_dapm_widgets_cpt,
				  ARRAY_SIZE(wm8580_dapm_widgets_cpt));

	/* Set up TX audio path */
	snd_soc_dapm_add_routes(codec, audio_map_tx, ARRAY_SIZE(audio_map_tx));

	/* Set up RX audio path */
	snd_soc_dapm_add_routes(codec, audio_map_rx, ARRAY_SIZE(audio_map_rx));

	/* All enabled by default */
	snd_soc_dapm_enable_pin(codec, "Front-L/R");
	snd_soc_dapm_enable_pin(codec, "MicIn");
	snd_soc_dapm_enable_pin(codec, "LineIn");

	/* signal a DAPM event */
	snd_soc_dapm_sync(codec);

	return 0;
}

static struct snd_soc_dai_link smdk_dai[] = {
{
	.name = "WM8580 PCM PAIF RX",
	.stream_name = "Playback",
	.cpu_dai = &s3c_pcm_dai[SMDK_WM8580_PCM_PORT],
	.codec_dai = &wm8580_dai[WM8580_DAI_PAIFRX],
	.init = smdk_wm8580_pcm_init,
	.ops = &smdk_pcm_ops,
},
{
	.name = "WM8580 PCM PAIF TX",
	.stream_name = "Capture",
	.cpu_dai = &s3c_pcm_dai[SMDK_WM8580_PCM_PORT],
	.codec_dai = &wm8580_dai[WM8580_DAI_PAIFTX],
	.init = smdk_wm8580_pcm_init,
	.ops = &smdk_pcm_ops,
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
	.codec_dev = &soc_codec_dev_wm8580,
};

static struct platform_device *smdk_snd_device;

static int __init smdk_pcm_audio_init(void)
{
	int ret;
	u32 reg;

	/* Set PM MISC register to set CLK_OUT to use XUSBXTI
	 * as it's source clock.
	 */
	reg = __raw_readl(S5P_OTHERS);
	reg &= ~(0x3<<8);
	reg |= (0x3<<8);
	__raw_writel(reg, S5P_OTHERS);

	/* Set CLK_OUT clock selection register to select DOUT
	 * and disable internal mux to avoid interference.
	 */
	reg = __raw_readl(S5P_CLK_OUT);
	reg &= ~(0xf<<20);
	reg |= (19<<20);
	__raw_writel(reg, S5P_CLK_OUT);

	reg = __raw_readl(S5P_CLK_OUT);
	reg &= ~(0x1<<0);
	reg |= (0x0<<0);
	__raw_writel(reg, S5P_CLK_OUT);

	/* Set Platform device for PCM audio */
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

static void __exit smdk_pcm_audio_exit(void)
{
	platform_device_unregister(smdk_snd_device);
}

module_init(smdk_pcm_audio_init);
module_exit(smdk_pcm_audio_exit);

MODULE_AUTHOR("Seungwhan Youn <sw.youn@samsung.com>");
MODULE_DESCRIPTION("ALSA SoC SMDK WM8580 with PCM");
MODULE_LICENSE("GPL");
