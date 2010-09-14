/*
 * linux/drivers/media/video/samsung/mfc5x/mfc_dec.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Decoder interface for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MFC_DEC_H
#define __MFC_DEC_H __FILE__

#include <linux/list.h>

#include "mfc.h"
#include "mfc_user.h"
#include "mfc_inst.h"

/* display status */
/* resolution change */
#define RC_MASK		0x3
#define RC_SHIFT	4
#define RC_NO		0
#define RC_INC		1
#define RC_DEC		2

/* progressive/interface */
#define PI_MASK		0x1
#define PI_SHIFT	3
#define PI_PROGRESSIVE	0
#define PI_INTERFACE	1

#define DISP_S_MASK	0x3	/* FIXME: 0x7 */
enum disp_status {
	DISP_S_DECODING = 0,
	DISP_S_DD = 1,
	DISP_S_DISPLAY = 2,
	DISP_S_FINISH = 3,
};

/* decoding status */
/* CRC */
#define CRC_G_MASK	0x1
#define CRC_G_SHIFT	5

#define CRC_N_MASK	0x1
#define CRC_N_SHIFT	4
#define CRC_TWO		0
#define CRC_FOUR	1

#define DEC_S_MASK	0x7
enum dec_status {
	DEC_S_DECODING = 0,
	DEC_S_DD = 1,
	DEC_S_DISPLAY = 2,
	DEC_S_FINISH = 3,
	DEC_S_NO = 4,
};

#define DEC_FRM_T_MASK	0x7
enum dec_frame_type {
	DEC_FRM_N = 0,
	DEC_FRM_I = 1,
	DEC_FRM_P = 2,
	DEC_FRM_B = 3,
	DEC_FRM_OTHER = 4,
};

#define DISP_FRM_T_MASK	0x3
enum disp_frame_type {
	DISP_FRM_N = 0,
	DISP_FRM_I = 1,
	DISP_FRM_P = 2,
	DISP_FRM_B = 3,
};

enum pixel_cache_decoder {
	PIXEL_CACHE_ONLY_P	= 0,
	PIXEL_CACHE_ONLY_B	= 1,
	PIXEL_CACHE_BOTH_P_B	= 2,
	PIXEL_CACHE_DISABLE	= 3,
};

struct mfc_dec_ctx {
	unsigned int crc;		/* I */
	enum pixel_cache_decoder pixelcache;	/* I */

	unsigned int slice;		/* I */

	unsigned long streamaddr;
	unsigned int streamsize;	/* I */
	unsigned int consumed;

	unsigned int nummindpb;
	unsigned int numextradpb;	/* I */
	unsigned int numtotaldpb;

	unsigned int lastframe;		/* I */

	unsigned int packedpb;		/* MPEG4 compatible */

	enum dec_frame_type frametype;	/* O */

	unsigned int lumasize;
	unsigned int chromasize;
	unsigned int mvsize;

	unsigned int level;		/* O */
	unsigned int profile;		/* O */

	/* FIXME: remove duplicated members */
	unsigned int crop_r_ofs;	/* O, for H.264 */
	unsigned int crop_l_ofs;	/* O, for H.264 */
	unsigned int crop_b_ofs;	/* O, for H.264 */
	unsigned int crop_t_ofs;	/* O, for H.264 */

	/* FIXME: remove duplicated members */
	unsigned int aspect_ratio;	/* O */
	unsigned int ext_par_width;	/* O */
	unsigned int ext_par_height;	/* O */

	unsigned int disp_status;
	unsigned int dec_status;

	void *d_priv;
};

/* decoder private data */
struct mfc_dec_h264 {
	int dispdelay;			/* I,  == -1: disable, >= 0: enable */

	unsigned int crop_r_ofs;	/* O */
	unsigned int crop_l_ofs;	/* O */
	unsigned int crop_b_ofs;	/* O */
	unsigned int crop_t_ofs;	/* O */
};

struct mfc_dec_mpeg4 {
	unsigned int postfilter;	/* I */

	unsigned int aspect_ratio;	/* O */
	unsigned int ext_par_width;	/* O */
	unsigned int ext_par_height;	/* O */
};

struct mfc_dec_fimv1 {
	unsigned int width;		/* I */
	unsigned int height;		/* I */
};

int mfc_init_decoding(struct mfc_inst_ctx *ctx, union mfc_args *args);
/*
int mfc_init_decoding(struct mfc_inst_ctx *ctx, struct mfc_dec_init_arg *init_arg);
*/
int mfc_exec_decoding(struct mfc_inst_ctx *ctx, union mfc_args *args);
/*
int mfc_exec_decoding(struct mfc_inst_ctx *ctx, struct mfc_dec_exe_arg *exe_arg);
*/

/*---------------------------------------------------------------------------*/

struct mfc_dec_info {
	struct list_head list;
	const char *name;
	enum codec_type codectype;
	int codecid;
	unsigned int d_priv_size;

	const struct codec_operations c_ops;
};

void mfc_init_decoders(void);

#endif /* __MFC_CMD_H */
