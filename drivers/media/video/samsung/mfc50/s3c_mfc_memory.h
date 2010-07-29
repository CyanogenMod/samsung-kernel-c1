/* linux/drivers/media/video/samsung/mfc50/s3c_mfc_memory.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Definition for size of each memory operation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __S3C_MFC_MEMORY_H
#define __S3C_MFC_MEMORY_H __FILE__

#include "s3c_mfc_common.h"

#ifdef CONFIG_VIDEO_MFC_MAX_INSTANCE
#define MFC_MAX_INSTANCE_NUM (CONFIG_VIDEO_MFC_MAX_INSTANCE)
#endif

/* DRAM memory start address */
/* mDDR, 0x4000_0000 ~ 0x4800_0000 (128MB) */
#define MFC_DRAM1_START		(0x40000000)

/* All buffer size have to be aligned to 64K */
/* 0x306c0(198,336 byte) ~ 0x60000(393,216) */
#define FIRMWARE_CODE_SIZE	(0x60000)
#define MFC_FW_SYSTEM_SIZE	(0x100000)	/* 1MB : 1x1024x1024 */
/* 600KB : 600x1024 size per instance */
#define MFC_FW_BUF_SIZE		(0x96000)
#define MFC_FW_TOTAL_BUF_SIZE	\
	(MFC_FW_SYSTEM_SIZE + MFC_MAX_INSTANCE_NUM * MFC_FW_BUF_SIZE)

#define DESC_BUF_SIZE		(0x20000)	/* 128KB : 128x1024 */
/* 512KB : 512x1024 size per instance */
#define RISC_BUF_SIZE		(0x80000)
#define SHARED_MEM_SIZE		(0x1000)	/* 4KB   : 4x1024 size */
/* 4MB : 4x1024x1024 for decoder */
#define CPB_BUF_SIZE		(0x400000)
/* 2MB : 2x1024x1024 for encoder */
#define STREAM_BUF_SIZE		(0x200000)
/* 64KB : 64x1024 for encoder */
#define ENC_UP_INTRA_PRED_SIZE	(0x10000)

struct s3c_platform_mfc {
	unsigned int buf_phy_base[2];
	unsigned int buf_phy_size[2];
};


/* port1 */
extern volatile unsigned char *s3c_mfc_virt_buf;
/* port 2 */
extern volatile unsigned char *s3c_mfc_virt_dpb_luma_buf;

extern unsigned int s3c_mfc_phys_buf, s3c_mfc_phys_dpb_luma_buf;
extern unsigned int s3c_mfc_buf_size, s3c_mfc_dpb_luma_buf_size;

volatile unsigned char *s3c_mfc_get_fw_buf_virt_addr(void);
volatile unsigned char *s3c_mfc_get_data_buf_virt_addr(void);
volatile unsigned char *s3c_mfc_get_dpb_luma_buf_virt_addr(void);

unsigned int s3c_mfc_get_fw_buf_phys_addr(void);
unsigned int s3c_mfc_get_fw_context_phys_addr(int inst_no);
unsigned int s3c_mfc_get_fw_context_phys_size(int inst_no);
unsigned int s3c_mfc_get_risc_buf_phys_addr(int inst_no);
unsigned int s3c_mfc_get_risc_buf_phys_size(int inst_no);
unsigned int s3c_mfc_get_data_buf_phys_addr(void);
unsigned int s3c_mfc_get_data_buf_phys_size(void);
unsigned int s3c_mfc_get_dpb_luma_buf_phys_addr(void);
unsigned int s3c_mfc_get_dpb_luma_buf_phys_size(void);
void s3c_mfc_get_reserved_memory(struct s3c_platform_mfc *npd);
#endif /* __S3C_MFC_MEMORY_H */
