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

enum disp_status {
	DECODING_ONLY = 0,
	DECODING_DISPLAY = 1,
	DISPLAY_ONLY = 2,
	DECODING_EMPTY = 3,
};

enum frame_type {
	N_FRAME = 0,
	I_FRAME = 1,
	P_FRAME = 2,
	B_FRAME = 3,
	OTHER_FRAME = 4,
};

enum pixel_cache_decoder {
	PIXEL_CACHE_ONLY_P	= 0,
	PIXEL_CACHE_ONLY_B	= 1,
	PIXEL_CACHE_BOTH_P_B	= 2,
	PIXEL_CACHE_DISABLE	= 3,
};

/*
struct mfc_codec_buffer {
	unsigned int offset;
	unsigned int size;
};
*/

struct mfc_dec_ctx {
	/*
	unsigned int crc;
	*/
	enum pixel_cache_decoder pixelcache;

	/*
	unsigned int slice;
	*/

	unsigned long streamaddr;
	unsigned int streamsize;
	unsigned int consumed;		/* R */

	unsigned int nummindpb;
	unsigned int numextradpb;
	unsigned int numtotaldpb;

	unsigned int lastframe;

	unsigned int packedpb;		/* only for MPEG4 compatible */

	enum frame_type frametype;

	/*struct list_head cb_list;*/	/* Codec buffer list */

	unsigned int lumasize;
	unsigned int chromasize;
	unsigned int mvsize;
	/* struct list_head dpb_list;*/	/* DPB buffer list */

	unsigned int level;
	unsigned int profile;

	void *d_priv;
};

int mfc_init_decoding(struct mfc_inst_ctx *ctx, union mfc_args *args);
int mfc_exec_decoding(struct mfc_inst_ctx *ctx, union mfc_args *args);

/*---------------------------------------------------------------------------*/

struct mfc_dec_info {
	struct list_head list;
	const char *name;
	enum codec_type codectype;
	int codecid;

	const struct codec_operations c_ops;
};

void mfc_init_decoders(void);

#endif /* __MFC_CMD_H */
