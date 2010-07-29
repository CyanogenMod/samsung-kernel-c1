/* linux/drivers/media/video/samsung/mfc50/s3c_mfc_memory.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Function for base address of each memory
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/sizes.h>
#include <asm/memory.h>
#include <plat/media.h>
#include <mach/media.h>

#include "s3c_mfc_memory.h"
#include "s3c_mfc_logmsg.h"
#include "s3c_mfc_interface.h"

volatile unsigned char *s3c_mfc_get_fw_buf_virt_addr()
{
	volatile unsigned char *virt_addr;

	virt_addr = s3c_mfc_virt_buf;

	return virt_addr;
}

volatile unsigned char *s3c_mfc_get_data_buf_virt_addr()
{
	volatile unsigned char *virt_addr;

	virt_addr =
	    s3c_mfc_virt_buf + FIRMWARE_CODE_SIZE + MFC_FW_TOTAL_BUF_SIZE +
	    (MFC_MAX_INSTANCE_NUM * RISC_BUF_SIZE);

	return virt_addr;
}

volatile unsigned char *s3c_mfc_get_dpb_luma_buf_virt_addr()
{
	volatile unsigned char *virt_addr;

	virt_addr = s3c_mfc_virt_dpb_luma_buf;

	return virt_addr;
}

unsigned int s3c_mfc_get_fw_buf_phys_addr()
{
	unsigned int phys_addr;

	phys_addr = s3c_mfc_phys_buf;

	return phys_addr;
}

/* Buf for MFC fw 9/30 buf for each instance */
unsigned int s3c_mfc_get_fw_context_phys_addr(int inst_no)
{
	unsigned int phys_addr;

	phys_addr =
	    s3c_mfc_phys_buf + FIRMWARE_CODE_SIZE + MFC_FW_SYSTEM_SIZE +
	    (inst_no * MFC_FW_BUF_SIZE);

	return phys_addr;
}

unsigned int s3c_mfc_get_fw_context_phys_size(int inst_no)
{
	return FIRMWARE_CODE_SIZE + MFC_FW_SYSTEM_SIZE +
	    (inst_no * MFC_FW_BUF_SIZE);
}

/* Buf for desc, motion vector, bitplane0/1/2, etc */
unsigned int s3c_mfc_get_risc_buf_phys_addr(int inst_no)
{
	unsigned int phys_addr;

	phys_addr =
	    s3c_mfc_phys_buf + FIRMWARE_CODE_SIZE + MFC_FW_TOTAL_BUF_SIZE +
	    (inst_no * RISC_BUF_SIZE);

	return phys_addr;
}

unsigned int s3c_mfc_get_risc_buf_phys_size(int inst_no)
{
	return FIRMWARE_CODE_SIZE + MFC_FW_TOTAL_BUF_SIZE +
	    (inst_no * RISC_BUF_SIZE);
}

unsigned int s3c_mfc_get_data_buf_phys_addr()
{
	unsigned int phys_addr;

	phys_addr =
	    s3c_mfc_get_risc_buf_phys_addr(MFC_MAX_INSTANCE_NUM);
	phys_addr = ALIGN(phys_addr, (4 * BUF_L_UNIT));

	return phys_addr;
}

unsigned int s3c_mfc_get_data_buf_phys_size(void)
{
	unsigned long offset;

	offset = s3c_mfc_get_data_buf_phys_addr() - s3c_mfc_phys_buf;

	return (s3c_mfc_buf_size - offset);
}

unsigned int s3c_mfc_get_dpb_luma_buf_phys_addr()
{
	unsigned int phys_addr;

	/*
	 * Luma buffer in the port 1 already aligned to 128KB boundary.
	 * We do not have to align to 4KB.
	 */
	phys_addr = s3c_mfc_phys_dpb_luma_buf;

	return phys_addr;
}

unsigned int s3c_mfc_get_dpb_luma_buf_phys_size(void)
{
	return s3c_mfc_dpb_luma_buf_size;
}

void s3c_mfc_get_reserved_memory(struct s3c_platform_mfc *npd)
{
#if (defined(CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC) || defined(CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC0))
	npd->buf_phy_base[0] = s5p_get_media_memory_bank(S5P_MDEV_MFC, 0);
	npd->buf_phy_size[0] = s5p_get_media_memsize_bank(S5P_MDEV_MFC, 0);
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC1
	npd->buf_phy_base[1] = s5p_get_media_memory_bank(S5P_MDEV_MFC, 1);
	npd->buf_phy_size[1] = s5p_get_media_memsize_bank(S5P_MDEV_MFC, 1);
#endif
}
