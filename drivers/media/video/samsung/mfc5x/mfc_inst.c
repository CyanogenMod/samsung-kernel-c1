/*
 * linux/drivers/media/video/samsung/mfc5x/mfc_inst.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Instance manager for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/slab.h>
#include <linux/mm.h>

#include "mfc_inst.h"
#include "mfc_log.h"
#include "mfc_buf.h"
#include "mfc_dec.h"

#ifdef SYSMMU_MFC_ON
#include <linux/interrupt.h>
#endif

struct mfc_inst_ctx *mfc_create_inst(void)
{
	struct mfc_inst_ctx *ctx;

	ctx = kzalloc(sizeof(struct mfc_inst_ctx), GFP_KERNEL);
	if (!ctx) {
		mfc_err("failed to create instance\n");
		return NULL;
	}

	/* FIXME: set default values */
	ctx->state = INST_STATE_CREATED;

#ifdef SYSMMU_MFC_ON
	//ctx->pgd = __pa(current->mm->pgd);
	ctx->pgd = __pa(swapper_pg_dir);
#endif

	return ctx;
}

int mfc_destroy_inst(struct mfc_inst_ctx* ctx)
{
	if (!ctx)
		kfree(ctx);

	return 0;
}

int mfc_set_inst_state(struct mfc_inst_ctx *ctx, enum instance_state state)
{
	if (ctx->state > state)
		return -1;

	ctx->state = state;

	return 0;
}

int mfc_chk_inst_state(struct mfc_inst_ctx *ctx, enum instance_state state)
{
	if (ctx->state != state)
		return -1;
	else
		return 0;
}

int mfc_set_inst_cfg(struct mfc_inst_ctx *ctx, unsigned int type, int *value)
{
	struct mfc_dec_ctx *dec_ctx;

	dec_ctx = (struct mfc_dec_ctx *)ctx->c_priv;

	switch (type) {
	case MFC_DEC_SETCONF_IS_LAST_FRAME:
		if (ctx->type != DECODER) {
			mfc_err("instance type is nod decoder\n");
			return MFC_SET_CONF_FAIL;
		}

		/* FIXME: */
		//if (ctx->state < INST_STATE_DEC_INIT)
		mfc_dbg("last frame\n");
		dec_ctx->lastframe = 1;

		return MFC_OK;

		break;

	default:

		mfc_err("invalid set config type: %d\n", type);
		/* FIXME */
		//return MFC_SET_CONF_FAIL;
		//break;

		return MFC_OK;
		break;
	}
}

