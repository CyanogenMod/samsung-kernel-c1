/*
 * s3c-dma-wrapper.c  --  S3C DMA Platform Wrapper Driver
 *
 * Copyright (c) 2010 Samsung Electronics Co. Ltd
 *	Jaswinder Singh <jassi.brar@samsung.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <sound/soc.h>

extern struct snd_soc_platform idma_soc_platform;
extern struct snd_soc_platform s3c24xx_soc_platform;
extern struct snd_soc_dai i2s_sec_fifo_dai;

static int s3c_wrpdma_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai_link *dai_link = rtd->dai;
	struct snd_soc_dai *cpu_dai = dai_link->cpu_dai;
	struct snd_soc_platform *platform;

	if (cpu_dai->use_idma)
		platform = &idma_soc_platform;
	else
		platform = &s3c24xx_soc_platform;

	if (platform->pcm_ops->hw_params)
		return platform->pcm_ops->hw_params(substream, params);
	else
		return 0;
}

static int s3c_wrpdma_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai_link *dai_link = rtd->dai;
	struct snd_soc_dai *cpu_dai = dai_link->cpu_dai;
	struct snd_soc_platform *platform;

	if (cpu_dai->use_idma)
		platform = &idma_soc_platform;
	else
		platform = &s3c24xx_soc_platform;

	if (platform->pcm_ops->hw_free)
		return platform->pcm_ops->hw_free(substream);
	else
		return 0;
}

static int s3c_wrpdma_prepare(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai_link *dai_link = rtd->dai;
	struct snd_soc_dai *cpu_dai = dai_link->cpu_dai;
	struct snd_soc_platform *platform;

	if (cpu_dai->use_idma)
		platform = &idma_soc_platform;
	else
		platform = &s3c24xx_soc_platform;

	if (platform->pcm_ops->prepare)
		return platform->pcm_ops->prepare(substream);
	else
		return 0;
}

static int s3c_wrpdma_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai_link *dai_link = rtd->dai;
	struct snd_soc_dai *cpu_dai = dai_link->cpu_dai;
	struct snd_soc_platform *platform;

	if (cpu_dai->use_idma)
		platform = &idma_soc_platform;
	else
		platform = &s3c24xx_soc_platform;

	if (platform->pcm_ops->trigger)
		return platform->pcm_ops->trigger(substream, cmd);
	else
		return 0;
}

static snd_pcm_uframes_t s3c_wrpdma_pointer(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai_link *dai_link = rtd->dai;
	struct snd_soc_dai *cpu_dai = dai_link->cpu_dai;
	struct snd_soc_platform *platform;

	if (cpu_dai->use_idma)
		platform = &idma_soc_platform;
	else
		platform = &s3c24xx_soc_platform;

	if (platform->pcm_ops->pointer)
		return platform->pcm_ops->pointer(substream);
	else
		return 0;
}

static int s3c_wrpdma_open(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai_link *dai_link = rtd->dai;
	struct snd_soc_dai *cpu_dai = dai_link->cpu_dai;
	struct snd_soc_platform *platform;

	if (cpu_dai->use_idma)
		platform = &idma_soc_platform;
	else
		platform = &s3c24xx_soc_platform;

	if (platform->pcm_ops->open)
		return platform->pcm_ops->open(substream);
	else
		return 0;
}

static int s3c_wrpdma_close(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai_link *dai_link = rtd->dai;
	struct snd_soc_dai *cpu_dai = dai_link->cpu_dai;
	struct snd_soc_platform *platform;

	if (cpu_dai->use_idma)
		platform = &idma_soc_platform;
	else
		platform = &s3c24xx_soc_platform;

	if (platform->pcm_ops->close)
		return platform->pcm_ops->close(substream);
	else
		return 0;
}

static int s3c_wrpdma_ioctl(struct snd_pcm_substream *substream,
		unsigned int cmd, void *arg)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai_link *dai_link = rtd->dai;
	struct snd_soc_dai *cpu_dai = dai_link->cpu_dai;
	struct snd_soc_platform *platform;

	if (cpu_dai->use_idma)
		platform = &idma_soc_platform;
	else
		platform = &s3c24xx_soc_platform;

	if (platform->pcm_ops->ioctl)
		return platform->pcm_ops->ioctl(substream, cmd, arg);
	else
		return 0;
}

static int s3c_wrpdma_mmap(struct snd_pcm_substream *substream,
		struct vm_area_struct *vma)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai_link *dai_link = rtd->dai;
	struct snd_soc_dai *cpu_dai = dai_link->cpu_dai;
	struct snd_soc_platform *platform;

	if (cpu_dai->use_idma)
		platform = &idma_soc_platform;
	else
		platform = &s3c24xx_soc_platform;

	if (platform->pcm_ops->mmap)
		return platform->pcm_ops->mmap(substream, vma);
	else
		return 0;
}

static struct snd_pcm_ops s3c_wrpdma_ops = {
	.open		= s3c_wrpdma_open,
	.close		= s3c_wrpdma_close,
	.ioctl		= s3c_wrpdma_ioctl,
	.hw_params	= s3c_wrpdma_hw_params,
	.hw_free	= s3c_wrpdma_hw_free,
	.prepare	= s3c_wrpdma_prepare,
	.trigger	= s3c_wrpdma_trigger,
	.pointer	= s3c_wrpdma_pointer,
	.mmap		= s3c_wrpdma_mmap,
};

static void s3c_wrpdma_pcm_free(struct snd_pcm *pcm)
{
	struct snd_soc_pcm_runtime *rtd = pcm->private_data;
	struct snd_soc_dai_link *dai_link = rtd->dai;
	struct snd_soc_dai *cpu_dai = dai_link->cpu_dai;
	struct snd_soc_platform *platform;

	if (cpu_dai == &i2s_sec_fifo_dai)
		platform = &idma_soc_platform;
	else
		platform = &s3c24xx_soc_platform;

	if (platform->pcm_free)
		platform->pcm_free(pcm);
}

static int s3c_wrpdma_pcm_new(struct snd_card *card,
		struct snd_soc_dai *dai, struct snd_pcm *pcm)
{
	struct snd_soc_pcm_runtime *rtd = pcm->private_data;
	struct snd_soc_dai_link *dai_link = rtd->dai;
	struct snd_soc_dai *cpu_dai = dai_link->cpu_dai;
	struct snd_soc_platform *platform;

	/* sec_fifo i/f always use internal h/w buffers
	 * irrespective of the xfer method (iDMA or SysDMA)
	 */
	if (cpu_dai == &i2s_sec_fifo_dai)
		platform = &idma_soc_platform;
	else
		platform = &s3c24xx_soc_platform;

	if (platform->pcm_new)
		return platform->pcm_new(card, dai, pcm);
	else
		return 0;
}

struct snd_soc_platform s3c_dma_wrapper = {
	.name		= "samsung-audio",
	.pcm_ops	= &s3c_wrpdma_ops,
	.pcm_new	= s3c_wrpdma_pcm_new,
	.pcm_free	= s3c_wrpdma_pcm_free,
};
EXPORT_SYMBOL_GPL(s3c_dma_wrapper);

static int __init s3c_dma_wrapper_init(void)
{
	return snd_soc_register_platform(&s3c_dma_wrapper);
}
module_init(s3c_dma_wrapper_init);

static void __exit s3c_dma_wrapper_exit(void)
{
	snd_soc_unregister_platform(&s3c_dma_wrapper);
}
module_exit(s3c_dma_wrapper_exit);

MODULE_AUTHOR("Jaswinder Singh, <jassi.brar@samsung.com>");
MODULE_DESCRIPTION("Audio DMA wrapper module");
MODULE_LICENSE("GPL");
