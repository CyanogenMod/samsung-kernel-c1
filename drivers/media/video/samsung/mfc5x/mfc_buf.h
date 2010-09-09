/*
 * linux/drivers/media/video/samsung/mfc5x/mfc_buf.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Buffer manager for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MFC_BUF_H_
#define __MFC_BUF_H_ __FILE__

#include <linux/list.h>

#include "mfc.h"
#include "mfc_inst.h"
#include "mfc_user.h"

/* FIXME */
#define ALIGN_4B	(1 <<  2)
#define ALIGN_2KB	(1 << 11)
#define ALIGN_4KB	(1 << 12)
#define ALIGN_8KB	(1 << 13)
#define ALIGN_128KB	(1 << 17)

/* System */					/* Size, Port, Align */
#define MFC_FW_SYSTEM_SIZE	(0x200000)	/* 2MB, A, N(4KB for VMEM) */

/* Instance */
#define MFC_CONTEXT_SIZE_L	(0x96000)	/* 600KB, N, 2KB, H.264 Decoding only */
#define MFC_CONTEXT_SIZE	(0x2800)	/* 10KB, N, 2KB */
#define MFC_SHM_SIZE		(0x400)		/* 1KB, N, 4B */

/* Decoding */
#define MFC_CPB_SIZE		(0x400000)	/* Max.4MB, A, 2KB */
#define MFC_DESC_SIZE		(0x20000)	/* Max.128KB, A, 2KB */

#define MFC_CODEC_NBMV_SIZE	(0x4000)	/* 16KB, ?, ? */
#define MFC_CODEC_NBIP_SIZE	(0x8000)	/* 32KB, ?, ? */
#define MFC_CODEC_NBDCAC_SIZE	(0x4000)	/* 16KB, ?, ? */
#define MFC_CODEC_UPNBNV_SIZE	(0x11000)	/* 68KB, ?, ? */
#define MFC_CODEC_SAMV_SIZE	(0x22000)	/* 136KB, ?, ? */
#define MFC_CODEC_OTLINE_SIZE	(0x8000)	/* 32KB, ?, ? */
#define MFC_CODEC_SYNPAR_SIZE	(0x11000)	/* 68KB, ?, ? */

/* Encoding */


#define MFC_LUMA_ALIGN		ALIGN_8KB
#define MFC_CHROMA_ALIGN	ALIGN_8KB
#define MFC_MV_ALIGN		ALIGN_8KB	/* H.264 Decoding only */

#define PORT_A			0
#define PORT_B			1

/* FIXME: MFC Buffer Type add as allocation parameter */
#define MBT_CONTEXT		(0x01 << 16)
#define MBT_DESC		(0x02 << 16)
#define MBT_CODEC		(0x04 << 16)
#define MBT_CPB			(0x08 << 16)
#define MBT_DPB			(0x10 << 16)

struct mfc_alloc_buffer {
	struct list_head list;
	unsigned long real;	/* phys. or virt. addr for MFC	*/
	unsigned int size;	/* allocation size		*/
	unsigned char *addr;	/* kernel virtual address space */
	unsigned char *user;	/* uer virtual address space	*/
	unsigned int type;	/* buffer type			*/
	int owner;		/* instance context id		*/
#ifdef CONFIG_S5P_VMEM
	unsigned int vmem_cookie;
	unsigned long vmem_addr;
	size_t vmem_size;
#endif
};

struct mfc_free_buffer {
	struct list_head list;
	unsigned long real;	/* phys. or virt. addr for MFC	*/
	unsigned int size;
};

int mfc_init_buf(void);
void mfc_merge_buf(void);
struct mfc_alloc_buffer *_mfc_alloc_buf(
	struct mfc_inst_ctx *ctx, int size, int align, int port);
int mfc_alloc_buf(
	struct mfc_inst_ctx *ctx, struct mfc_buf_alloc_arg* args, int port);
int _mfc_free_buf(unsigned char *user);
int mfc_free_buf(struct mfc_inst_ctx *ctx, unsigned char *user);
void mfc_free_buf_inst(int owner);
unsigned long mfc_get_buf_real(int owner, unsigned char *user);
unsigned char *mfc_get_buf_addr(int owner, unsigned char *user);
unsigned char *_mfc_get_buf_addr(int owner, unsigned char *user);

#endif /* __MFC_BUF_H_ */
