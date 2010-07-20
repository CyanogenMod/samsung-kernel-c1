/* linux/sound/soc/s3c24xx/smdk2416_wm8580.c
 * ASoC audio for SMDK2416-WM8580
 *
 * Copyright (c) 2010 Samsung Electronics Co. Ltd
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <sound/soc-dapm.h>
#include <plat/regs-iis.h>

#include "../codecs/wm8580.h"
#include "s3c-dma.h"
#include "s3c24xx-i2s.h"

static int smdk_hifi_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;
	int ret = 0;
	unsigned int prescaler = 0;

	switch (params_rate(params)) {
	case 8000:
		prescaler = 24;
		break;
	case 11025:
		prescaler = 32;
		break;
	case 16000:
		prescaler = 12;
		break;
	case 22050:
		prescaler = 16;
		break;
	case 32000:
		prescaler = 6;
		break;
	case 44100:
		prescaler = 8;
		break;
	case 48000:
	case 64000:
	case 88200:
		prescaler = 4;
		break;
	case 96000:
		prescaler = 2;
		break;
	}

	/* set cpu DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai,
		SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
		SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	/* set MCLK division for sample rate */
	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C24XX_DIV_MCLK,
		S3C2410_IISMOD_32FS);
	if (ret < 0)
		return ret;

	/* Enable and set prescaler division for sample rate */
	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C24XX_DIV_PRESCALER,
		(((prescaler - 1) << 0x8) | (0x1 << 15)));
	if (ret < 0)
		return ret;

	return 0;
}

static int smdk_hifi_hw_free(struct snd_pcm_substream *substream)
{
	return 0;
}

/* SMDK2450-WM8580 HiFi DAI opserations. */
static struct snd_soc_ops smdk2416_hifi_ops = {
	.hw_params = smdk_hifi_hw_params,
	.hw_free = smdk_hifi_hw_free,
};

static const struct snd_soc_dapm_widget wm8580_dapm_widgets_rx[] = {
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
};

static const struct snd_soc_dapm_widget wm8580_dapm_widgets_tx[] = {
	SND_SOC_DAPM_MIC("Mic1 Jack", NULL),
	SND_SOC_DAPM_LINE("Line In Jack", NULL),
};

/* example machine audio_map RX connections */
static const struct snd_soc_dapm_route audio_map_rx[] = {
	{"Headphone Jack", NULL, "LOUT1"},
	{"Headphone Jack", NULL, "ROUT1"},

	/* Connect the ALC pins */
	{"ACIN", NULL, "ACOP"},
};

/* example machine audio_map TX connections */
static const struct snd_soc_dapm_route audio_map_tx[] = {
	/* mic is connected to mic1 - with bias */
	{"MIC1", NULL, "Mic1 Jack"},

	{"LINE1", NULL, "Line In Jack"},
	{"LINE2", NULL, "Line In Jack"},

	/* Connect the ALC pins */
	{"ACIN", NULL, "ACOP"},
};

/* This is an example machine initialisation for a wm8580 connected to a
 * smdk2416. It is missing logic to detect hp/mic insertions and logic
 * to re-route the audio in such an event.
 */
static int smdk2416_wm8580_init_rx(struct snd_soc_codec *codec)
{
	int err;

	/* Add smdk2416 specific widgets */
	snd_soc_dapm_new_controls(codec, wm8580_dapm_widgets_rx,
				ARRAY_SIZE(wm8580_dapm_widgets_rx));

	/* set up smdk2416 specific audio path audio_mapnects */
	err = snd_soc_dapm_add_routes(codec, audio_map_rx,
					ARRAY_SIZE(audio_map_rx));
	if (err < 0)
		return err;

	/* disable as not required*/
	snd_soc_dapm_disable_pin(codec, "Mic1 Jack");
	snd_soc_dapm_disable_pin(codec, "Line In Jack");

	snd_soc_dapm_sync(codec);
	return 0;
}

static int smdk2416_wm8580_init_tx(struct snd_soc_codec *codec)
{
	int err;

	/* Add smdk2416 specific widgets */
	snd_soc_dapm_new_controls(codec, wm8580_dapm_widgets_tx,
					ARRAY_SIZE(wm8580_dapm_widgets_tx));

	/* set up smdk2416 specific audio path audio_mapnects */
	err = snd_soc_dapm_add_routes(codec, audio_map_tx,
						ARRAY_SIZE(audio_map_tx));

	snd_soc_dapm_sync(codec);
	return 0;
}


static struct snd_soc_dai_link smdk2416_dai[] = {
	{
		/* Primary Playback i/f */
		.name = "WM8580 PAIF RX",
		.stream_name = "Playback",
		.cpu_dai = &s3c24xx_i2s_dai,
		.codec_dai = &wm8580_dai[WM8580_DAI_PAIFRX],
		.init = smdk2416_wm8580_init_rx,
		.ops = &smdk2416_hifi_ops,
	}, {
		/* Primary Capture i/f */
		.name = "WM8580 PAIF TX",
		.stream_name = "Capture",
		.cpu_dai = &s3c24xx_i2s_dai,
		.codec_dai = &wm8580_dai[WM8580_DAI_PAIFTX],
		.init = smdk2416_wm8580_init_tx,
		.ops = &smdk2416_hifi_ops,
	},
};

static struct snd_soc_card smdk2416 = {
	.name = "smdk2416",
	.dai_link = smdk2416_dai,
	.platform = &s3c24xx_soc_platform,
	.num_links = ARRAY_SIZE(smdk2416_dai),
};

static struct snd_soc_device smdk_snd_devdata = {
	.card = &smdk2416,
	.codec_dev = &soc_codec_dev_wm8580,
};

static struct platform_device *smdk_snd_device;

static int __init s3c_init(void)
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

static void __exit s3c_exit(void)
{
	platform_device_unregister(smdk_snd_device);
}

module_init(s3c_init);
module_exit(s3c_exit);

/* Module information */
MODULE_AUTHOR("Ryu Euiyoul");
MODULE_DESCRIPTION("ALSA SoC WM8580 SMDK2416");
