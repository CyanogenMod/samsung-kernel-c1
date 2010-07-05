/* linux/arch/arm/plat-s5p/include/plat/media.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Samsung Media device descriptions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _S5P_MEDIA_H
#define _S5P_MEDIA_H

#include <linux/types.h>
#include <asm/setup.h>

#define S3C_MDEV_MFC		1

struct s3c_media_device {
	u32		id;
	const char	*name;
	u32		bank;
	size_t		memsize;
	dma_addr_t	paddr;
};

extern struct meminfo meminfo;
extern dma_addr_t s3c_get_media_memory_bank(int dev_id, int bank);
extern size_t s3c_get_media_memsize_bank(int dev_id, int bank);

#endif

