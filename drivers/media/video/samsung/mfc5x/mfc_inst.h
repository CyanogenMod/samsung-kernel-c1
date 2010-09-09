/*
 * linux/drivers/media/video/samsung/mfc5x/mfc_inst.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Instance manager file for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MFC_INST_H
#define __MFC_INST_H __FILE__

#include "mfc.h"
#include "mfc_user.h"

enum instance_state {
	INST_STATE_NULL		= 0,

	INST_STATE_CREATED	= 10,

	INST_STATE_DEC_INIT	= 20,
	INST_STATE_DEC_EXE,
	INST_STATE_DEC_EXE_DONE,

	INST_STATE_ENC_INIT	= 40,
	INST_STATE_ENC_EXE,
	INST_STATE_ENC_EXE_DONE,
};

struct mfc_inst_ctx;

struct codec_operations {
	/*
	int (*codec_init)(struct mfc_inst_ctx *);
	int (*codec_exec)(struct mfc_inst_ctx *);
	*/
	int (*alloc_ctx_buf) (struct mfc_inst_ctx *ctx);
	int (*alloc_desc_buf) (struct mfc_inst_ctx *ctx);
	int (*set_codec_bufs) (struct mfc_inst_ctx *ctx);
	int (*set_dpbs)  (struct mfc_inst_ctx *ctx);
	int (*pre_exec) (struct mfc_inst_ctx *ctx);
	int (*post_exec) (struct mfc_inst_ctx *ctx);
	int (*multi_exec) (struct mfc_inst_ctx *ctx);
};

struct mfc_dec_init_cfg {
	/*
	type:
	  init
	  runtime
	  init + runtime
	*/

	/* init */
	unsigned int postfilter;	/* MPEG4 */
	unsigned int dispdelay;		/* H.264 */
	unsigned int numextradpb;
	unsigned int slice;
	unsigned int width;		/* FIMV1 */
	unsigned int height;		/* FIMV1 */
	unsigned int crc;

	/* runtime ? */
	#if 0
	unsigned int dpbflush;
	unsigned int lastframe;
	#endif
};

struct mfc_enc_init_cfg {
	/*
	type:
	  init
	  runtime
	  init + runtime
	*/

	/* init */
	unsigned int frameskip;
	unsigned int frammode;
	unsigned int hier_p;

	/* runtime ? */
	#if 0
	unsigned int frametype;
	unsigned int fraemrate;
	unsigned int bitrate;
	unsigned int vui;		/* H.264 */
	unsigned int hec;		/* MPEG4 */
	unsigned int seqhdrctrl;

	unsigned int i_period;
	#endif
};

struct mfc_inst_ctx {
	int id;
	int codecid;
	unsigned int type;
	enum instance_state state;
	unsigned int width;
	unsigned int height;
	volatile unsigned char *shm;
	unsigned int shmofs;
	unsigned int ctxbufofs;
	unsigned int ctxbufsize;
	unsigned int descbufofs;
	unsigned int descbufsize;
	unsigned long userbase;

	struct mfc_dec_init_cfg deccfg;
	struct mfc_enc_init_cfg enccfg;

	void *c_priv;
	struct codec_operations *c_ops;
	struct mfc_dev *dev;
#ifdef SYSMMU_MFC_ON
	unsigned long pgd;
#endif
};

struct mfc_inst_ctx *mfc_create_inst(void);
int mfc_destroy_inst(struct mfc_inst_ctx* ctx);
int mfc_set_inst_state(struct mfc_inst_ctx *ctx, enum instance_state state);
int mfc_chk_inst_state(struct mfc_inst_ctx *ctx, enum instance_state state);
int mfc_set_inst_cfg(struct mfc_inst_ctx *ctx, unsigned int type, int *value);

#endif /* __MFC_INST_H */
