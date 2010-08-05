/* linux/arch/arm/mach-s5pv210/dev-audio.c
 *
 * Copyright (c) 2010 Samsung Electronics Co. Ltd
 *	Jaswinder Singh <jassi.brar@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/dma-mapping.h>

#include <plat/gpio-cfg.h>
#include <plat/audio.h>

#include <mach/gpio.h>
#include <mach/map.h>
#include <mach/dma.h>
#include <mach/irqs.h>

static int s5p6450_cfg_i2s(struct platform_device *pdev)
{
	/* configure GPIO for i2s port */
	switch (pdev->id) {
	case -1:
		s3c_gpio_cfgpin(S5P6450_GPB(4), S3C_GPIO_SFN(5));
		s3c_gpio_cfgpin(S5P6450_GPR(4), S3C_GPIO_SFN(5));
		s3c_gpio_cfgpin(S5P6450_GPR(5), S3C_GPIO_SFN(5));
		s3c_gpio_cfgpin(S5P6450_GPR(6), S3C_GPIO_SFN(5));
		s3c_gpio_cfgpin(S5P6450_GPR(7), S3C_GPIO_SFN(5));
		s3c_gpio_cfgpin(S5P6450_GPR(8), S3C_GPIO_SFN(5));
		s3c_gpio_cfgpin(S5P6450_GPR(13), S3C_GPIO_SFN(5));
		s3c_gpio_cfgpin(S5P6450_GPR(14), S3C_GPIO_SFN(5));
		break;

	default:
		printk(KERN_ERR "Invalid Device %d\n", pdev->id);
		return -EINVAL;
	}

	return 0;
}

static struct s3c_audio_pdata s3c_i2s_pdata = {
	.cfg_gpio = s5p6450_cfg_i2s,
};

static struct resource s5p6450_iis0_resource[] = {
	[0] = {
		.start = S5P6450_PA_I2S,
		.end   = S5P6450_PA_I2S + 0x100 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = DMACH_I2S0_TX,
		.end   = DMACH_I2S0_TX,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.start = DMACH_I2S0_RX,
		.end   = DMACH_I2S0_RX,
		.flags = IORESOURCE_DMA,
	},
};

struct platform_device s5p6450_device_iis0 = {
	.name		  = "s3c64xx-iis-v4",
	.id		  = -1,
	.num_resources	  = ARRAY_SIZE(s5p6450_iis0_resource),
	.resource	  = s5p6450_iis0_resource,
	.dev = {
		.platform_data = &s3c_i2s_pdata,
	},
};
