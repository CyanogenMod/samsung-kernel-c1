/* 
 * linux/drivers/media/video/samsung/mfc5x/mfc_dev.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Driver interface for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MFC_DEV_H
#define __MFC_DEV_H __FILE__

#include <linux/mutex.h>
#include <linux/firmware.h>

#include "mfc_inst.h"

#define MFC_DEV_NAME	"mfc"

struct mfc_reg {
	resource_size_t	rsrc_start;
	resource_size_t	rsrc_len;
	void __iomem	*base;
};

struct mfc_pm {
	char		pd_name[16];
	char		clk_name[16];
	struct clk	*clock;
};

struct mfc_mem {
	unsigned long	base;	/* phys. or virt. addr for MFC	*/
	size_t		size;	/* total size			*/
	unsigned char	*addr;	/* kernel virtual address space */
	void		*vmalloc_addr;	/* not aligned vmalloc alloc. addr */
};

struct mfc_dev {
	char			name[16];
	struct mfc_reg		reg;
	int			irq;
	struct mfc_pm		pm;
	int			mem_ports;
	struct mfc_mem		mem_infos[2];

	atomic_t		inst_cnt;
	struct mfc_inst_ctx	inst_ctx[16];

	struct mutex		lock;
	wait_queue_head_t	wait_sys;
	int			irq_sys;
	wait_queue_head_t	wait_codec[2];
	int			irq_codec[2];

	struct firmware		fw_info;
	int			fw_state;
	int			fw_ver;
	int 			fw_vmem_cookie;

	struct device		*device;
};

#endif /* __MFC_DEV_H */
