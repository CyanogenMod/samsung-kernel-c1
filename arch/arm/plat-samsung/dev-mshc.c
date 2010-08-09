/* linux/arch/arm/plat-samsung/dev-mshc.c
 *
 * Copyright (c) 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * Based on arch/arm/plat-samsung/dev-hsmmc1.c
 *
 * Device definition for mshc devices
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/mmc/host.h>

#include <mach/map.h>
//#include <plat/sdhci.h>
#include <plat/mshci.h>
#include <plat/devs.h>
//#include <plat/cpu.h>

#define S5P6450_SZ_HSMMC	(0x1000)

static struct resource s3c_mshci_resource[] = {
	[0] = {
		.start = S5P6450_PA_SDMMC44,
		.end   = S5P6450_PA_SDMMC44 + S5P6450_SZ_HSMMC - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_SDMMC44,
		.end   = IRQ_SDMMC44,
		.flags = IORESOURCE_IRQ,
	}
};

static u64 s3c_device_hsmmc_dmamask = 0xffffffffUL;

struct s3c_mshci_platdata s3c_mshci_def_platdata = {
	.max_width	= 4,
	.host_caps	= (MMC_CAP_4_BIT_DATA |
			   MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED),
};

struct platform_device s3c_device_mshci = {
	.name		= "s3c-mshci",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s3c_mshci_resource),
	.resource	= s3c_mshci_resource,
	.dev		= {
		.dma_mask		= &s3c_device_hsmmc_dmamask,
		.coherent_dma_mask	= 0xffffffffUL,
		.platform_data		= &s3c_mshci_def_platdata,
	},
};

void s3c_mshci_set_platdata(struct s3c_mshci_platdata *pd)
{
	struct s3c_mshci_platdata *set = &s3c_mshci_def_platdata;

	set->max_width = pd->max_width;

	if (pd->cfg_gpio)
		set->cfg_gpio = pd->cfg_gpio;
	if (pd->cfg_card)
		set->cfg_card = pd->cfg_card;
}
