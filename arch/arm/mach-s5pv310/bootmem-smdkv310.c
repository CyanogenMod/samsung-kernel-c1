/* linux/arch/arm/mach-s5pv310/bootmem-smdkv310.c
 *
 * Copyright (c) 2009 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Bootmem helper functions for smdkv310
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/mm.h>
#include <linux/bootmem.h>
#include <linux/swap.h>
#include <asm/setup.h>
#include <linux/io.h>
#include <mach/memory.h>
#include <plat/media.h>
#include <mach/media.h>

struct s5p_media_device media_devs[] = {
	/* Will be populated later */
#ifdef CONFIG_S5P_PMEM_MEMSIZE_PMEM
	{
		.id = S5P_MDEV_PMEM,
		.name = "pmem",
		.bank = 1,
		.memsize = CONFIG_S5P_PMEM_MEMSIZE_PMEM * SZ_1K,
		.paddr = 0,
	},
#endif
};

int nr_media_devs = (sizeof(media_devs) / sizeof(media_devs[0]));
