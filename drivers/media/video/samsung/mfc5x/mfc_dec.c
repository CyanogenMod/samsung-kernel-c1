/*
 * linux/drivers/media/video/samsung/mfc5x/mfc_dec.c
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

#include <linux/kernel.h>
#include <linux/slab.h>

#ifdef CONFIG_ARCH_S5PV310
#include <mach/regs-mfc.h>
#elif CONFIG_ARCH_S5PV210
#include <plat/regs-mfc.h>
#endif

#include "mfc_dec.h"
#include "mfc_cmd.h"
#include "mfc_log.h"

#include "mfc_shm.h"
#include "mfc_reg.h"
#include "mfc_mem.h"
#include "mfc_buf.h"

#undef DUMP_STREAM

static LIST_HEAD(mfc_decoders);

#ifdef DUMP_STREAM
static void dump_stream(unsigned long addr, unsigned int size)
{
	int i, j;

	mfc_mem_cache_inv((void *)addr, size);

	mfc_dbg("---- stream dump ---- \n");
	mfc_dbg("size: 0x%04x\n", size);
	for (i = 0; i < size; i += 16) {
		mfc_dbg("0x%04x: ", i);

		if ((size - i) >= 16) {
			for (j = 0; j < 16; j++)
				mfc_dbg("0x%02x ", (u8)(*(u8 *)(addr + i + j)));
		} else {
			for (j = 0; j < (size - i); j++)
				mfc_dbg("0x%02x ", (u8)(*(u8 *)(addr + i + j)));
		}
		mfc_dbg("\n");
	}
}
#endif

/*
 * FIXME: when _mfc_alloc_buf() fail, all allocation functions must be care
 * already allocated buffers or caller funtion takes care of that
 */

static int h264_alloc_ctx_buf(struct mfc_inst_ctx *ctx)
{
	struct mfc_alloc_buffer *alloc;

	alloc = _mfc_alloc_buf(ctx, MFC_CONTEXT_SIZE_L, ALIGN_2KB, PORT_A);
	if (alloc == NULL) {
		mfc_err("failed alloc context buffer\n");

		return -1;
	}

	ctx->ctxbufofs = mfc_mem_base_ofs(alloc->real) >> 11;
	ctx->ctxbufsize = alloc->size;

	memset((void *)alloc->addr, 0, alloc->size);

	mfc_mem_cache_clean((void *)alloc->addr, alloc->size);

	return 0;
}

static int alloc_ctx_buf(struct mfc_inst_ctx *ctx)
{
	struct mfc_alloc_buffer *alloc;

	alloc = _mfc_alloc_buf(ctx, MFC_CONTEXT_SIZE, ALIGN_2KB, PORT_A);
	if (alloc == NULL) {
		mfc_err("failed alloc context buffer\n");

		return -1;
	}

	ctx->ctxbufofs = mfc_mem_base_ofs(alloc->real) >> 11;
	ctx->ctxbufsize = alloc->size;

	memset((void *)alloc->addr, 0, alloc->size);

	mfc_mem_cache_clean((void *)alloc->addr, alloc->size);

	return 0;
}

static int alloc_desc_buf(struct mfc_inst_ctx *ctx)
{
	struct mfc_alloc_buffer *alloc;

	/* FIXME: size fixed? */
	alloc = _mfc_alloc_buf(ctx, MFC_DESC_SIZE, ALIGN_2KB, PORT_A);
	if (alloc == NULL) {
		mfc_err("failed alloc descriptor buffer\n");

		return -1;
	}

	ctx->descbufofs = mfc_mem_base_ofs(alloc->real) >> 11;
	/* FIXME: size fixed? */
	ctx->descbufsize = MFC_DESC_SIZE;

	return 0;
}

static int h264_set_codec_bufs(struct mfc_inst_ctx *ctx)
{
	struct mfc_alloc_buffer *alloc;

	alloc = _mfc_alloc_buf(ctx, MFC_CODEC_NBMV_SIZE, ALIGN_2KB, PORT_A);
	if (alloc == NULL) {
		mfc_err("failed alloc codec buffer\n");

		return -1;
	}
	write_reg(mfc_mem_base_ofs(alloc->real) >> 11, MFC_VERT_NB_MV_ADR);

	alloc = _mfc_alloc_buf(ctx, MFC_CODEC_NBIP_SIZE, ALIGN_2KB, PORT_A);
	if (alloc == NULL) {
		mfc_err("failed alloc codec buffer\n");

		return -1;
	}
	write_reg(mfc_mem_base_ofs(alloc->real) >> 11, MFC_VERT_NB_IP_ADR);

	return 0;
}

static int h264_set_dpbs(struct mfc_inst_ctx *ctx)
{
	struct mfc_alloc_buffer *alloc;
	struct mfc_dec_ctx *dec_ctx;
	int i;

	dec_ctx = (struct mfc_dec_ctx *)ctx->c_priv;

	/* width: 128B align, height: 32B align, size: 8KB align */
	dec_ctx->lumasize = ALIGN(ctx->width, 128) * ALIGN(ctx->height, 32);
	dec_ctx->lumasize = ALIGN(dec_ctx->lumasize, ALIGN_8KB);
	dec_ctx->chromasize = ALIGN(ctx->width, 128) * ALIGN(ctx->height >> 1, 32);
	dec_ctx->chromasize = ALIGN(dec_ctx->chromasize, ALIGN_8KB);
	dec_ctx->mvsize = ALIGN(ctx->width, 128) * ALIGN(ctx->height >> 2, 32);
	dec_ctx->mvsize = ALIGN(dec_ctx->mvsize, ALIGN_8KB);

	for (i = 0; i < dec_ctx->numtotaldpb; i++) {
		/*
		 * allocate chroma buffer
		 */
		alloc = _mfc_alloc_buf(ctx, dec_ctx->chromasize, ALIGN_2KB, PORT_A);
		if (alloc == NULL) {
			mfc_err("failed alloc dpb buffer\n");

			return -1;
		}
		/*
		 * set chroma buffer address
		 */
		write_reg(mfc_mem_base_ofs(alloc->real) >> 11, MFC_CHROMA_ADR + (4 * i));

		/*
		 * allocate luma & mv buffer
		 */
		alloc = _mfc_alloc_buf(ctx,
			dec_ctx->lumasize + dec_ctx->mvsize, ALIGN_2KB, PORT_B);
		if (alloc == NULL) {
			mfc_err("failed alloc dpb buffer\n");

			return -1;
		}
		/*
		 * set luma & mv buffer address
		 */
		write_reg(mfc_mem_base_ofs(alloc->real) >> 11, MFC_LUMA_ADR + (4 * i));
		write_reg(mfc_mem_base_ofs(alloc->real + dec_ctx->lumasize) >> 11,
			MFC_MV_ADR + (4 * i));
	}

	write_shm(ctx, dec_ctx->lumasize, ALLOCATED_LUMA_DPB_SIZE);
	write_shm(ctx, dec_ctx->chromasize, ALLOCATED_CHROMA_DPB_SIZE);
	write_shm(ctx, dec_ctx->mvsize, ALLOCATED_MV_SIZE);

	return 0;
}

static int set_dpbs(struct mfc_inst_ctx *ctx)
{
	struct mfc_alloc_buffer *alloc;
	struct mfc_dec_ctx *dec_ctx;
	int i;

	dec_ctx = (struct mfc_dec_ctx *)ctx->c_priv;

	/* width: 128B align, height: 32B align, size: 8KB align */
	dec_ctx->lumasize = ALIGN(ctx->width, 128) * ALIGN(ctx->height, 32);
	dec_ctx->lumasize = ALIGN(dec_ctx->lumasize, ALIGN_8KB);
	dec_ctx->chromasize = ALIGN(ctx->width, 128) * ALIGN(ctx->height >> 1, 32);
	dec_ctx->chromasize = ALIGN(dec_ctx->chromasize, ALIGN_8KB);
	dec_ctx->mvsize = 0;

	for (i = 0; i < dec_ctx->numtotaldpb; i++) {
		/*
		 * allocate chroma buffer
		 */
		alloc = _mfc_alloc_buf(ctx, dec_ctx->chromasize, ALIGN_2KB, PORT_A);
		if (alloc == NULL) {
			mfc_err("failed alloc dpb buffer\n");

			return -1;
		}
		/*
		 * set chroma buffer address
		 */
		write_reg(mfc_mem_base_ofs(alloc->real) >> 11, MFC_CHROMA_ADR + (4 * i));

		/*
		 * allocate luma & mv buffer
		 */
		alloc = _mfc_alloc_buf(ctx, dec_ctx->lumasize, ALIGN_2KB, PORT_B);
		if (alloc == NULL) {
			mfc_err("failed alloc dpb buffer\n");

			return -1;
		}
		/*
		 * set luma buffer address
		 */
		write_reg(mfc_mem_base_ofs(alloc->real) >> 11, MFC_LUMA_ADR + (4 * i));
	}

	write_shm(ctx, dec_ctx->lumasize, ALLOCATED_LUMA_DPB_SIZE);
	write_shm(ctx, dec_ctx->chromasize, ALLOCATED_CHROMA_DPB_SIZE);
	write_shm(ctx, dec_ctx->mvsize, ALLOCATED_MV_SIZE);

	return 0;
}

static struct mfc_dec_info h264_dec = {
	.name		= "H264_DEC",
	.codectype	= H264_DEC,
	.codecid	= 0,
	.c_ops		= {
		.alloc_ctx_buf	= h264_alloc_ctx_buf,
		.alloc_desc_buf = alloc_desc_buf,
		.set_codec_bufs	= h264_set_codec_bufs,
		.set_dpbs	= h264_set_dpbs,
		.pre_exec	= NULL,
		.post_exec	= NULL,
		.multi_exec	= NULL,
	},
};

void mfc_init_decoders(void)
{
	list_add_tail(&h264_dec.list, &mfc_decoders);
}

static int mfc_set_decoder(struct mfc_inst_ctx *ctx, enum codec_type codectype)
{
	struct list_head *pos;
	struct mfc_dec_info *decoder;
	int codecid = -1;

	list_for_each(pos, &mfc_decoders) {
		decoder = list_entry(pos, struct mfc_dec_info, list);

		if (decoder->codectype == codectype) {
			ctx->type = DECODER;
			ctx->c_ops = (struct codec_operations *)&decoder->c_ops;

			ctx->c_priv = kzalloc(sizeof(struct mfc_dec_ctx), GFP_KERNEL);
			if (!ctx->c_priv) {
				mfc_err("failed to allocate codec private.\n");
				return -ENOMEM;
			}

			codecid = decoder->codecid;

			break;
		}
	}

	return codecid;
}

static void mfc_set_stream_info(
	struct mfc_inst_ctx *ctx,
	unsigned int addr,
	unsigned int size,
	unsigned int ofs)
{
	write_reg(addr, MFC_SI_CH1_SB_ST_ADR);
	write_reg(size, MFC_SI_CH1_SB_FRM_SIZE);

	/* FIXME: IOCTL_MFC_GET_IN_BUF size */
	write_reg(MFC_CPB_SIZE, MFC_SI_CH1_CPB_SIZE);

	write_reg(ctx->descbufofs, MFC_SI_CH1_DESC_ADR);
	write_reg(ctx->descbufsize, MFC_SI_CH1_DESC_SIZE);

	/* FIXME: right position */
	write_shm(ctx, ofs, START_BYTE_NUM);
}

int mfc_init_decoding(struct mfc_inst_ctx *ctx, union mfc_args *args)
{
	struct mfc_dec_init_arg *init_arg = (struct mfc_dec_init_arg *)args;
	struct mfc_dec_ctx *dec_ctx;
	int ret;

	ctx->codecid = mfc_set_decoder(ctx, init_arg->in_codec_type);
	if (ctx->codecid < 0)
		return MFC_DEC_INIT_FAIL;

	dec_ctx = (struct mfc_dec_ctx *)ctx->c_priv;

	dec_ctx->packedpb = init_arg->in_packed_PB;

	/* FIXME: fill the value */
	//dec_ctx->crc = 0;
	dec_ctx->pixelcache = 0;
	dec_ctx->packedpb = 0;

	/*
	 * allocate context buffer
	 */
	if (ctx->c_ops->alloc_ctx_buf) {
		if (ctx->c_ops->alloc_ctx_buf(ctx) < 0)
			return MFC_DEC_INIT_FAIL;
	}

	ret = mfc_cmd_inst_open(ctx);
	if (ret < 0)
		return ret;

	if (init_shm(ctx) < 0)
		return MFC_DEC_INIT_FAIL;

	/*
	 * allocate descriptor buffer
	 */
	if (ctx->c_ops->alloc_desc_buf) {
		if (ctx->c_ops->alloc_desc_buf(ctx) < 0)
			return MFC_DEC_INIT_FAIL;
	}

#ifdef DUMP_STREAM
	dump_stream(init_arg->in_strm_buf, init_arg->in_strm_size);
#endif

	mfc_set_stream_info(ctx, mfc_mem_base_ofs(init_arg->in_strm_buf) >> 11,
		init_arg->in_strm_size, 0);

	ret = mfc_cmd_seq_start(ctx);
	if (ret < 0)
		return ret;

	ctx->width = read_reg(MFC_SI_HRESOL);
	ctx->height = read_reg(MFC_SI_VRESOL);

	init_arg->out_img_width = ctx->width;
	init_arg->out_img_height = ctx->height;
	init_arg->out_buf_width = ALIGN(ctx->width, 128);
	init_arg->out_buf_height = ALIGN(ctx->height, 32);

	dec_ctx->nummindpb = read_reg(MFC_SI_BUF_NUMBER);
	dec_ctx->numtotaldpb = dec_ctx->nummindpb + dec_ctx->numextradpb;
	init_arg->out_dpb_cnt = dec_ctx->numtotaldpb;

	mfc_dbg("H: %d, W: %d, DPB_Count: %d",
		ctx->width,
		ctx->height,
		dec_ctx->numtotaldpb);

	/*
	 * allocate & set codec buffers
	 */
	if (ctx->c_ops->set_codec_bufs) {
		if (ctx->c_ops->set_codec_bufs(ctx) < 0)
			return MFC_DEC_INIT_FAIL;
	}

	/*
	 * allocate & set DPBs
	 */
	if (ctx->c_ops->set_dpbs) {
		if (ctx->c_ops->set_dpbs(ctx) < 0)
			return MFC_DEC_INIT_FAIL;
	}

	/* FIXME: move to right position, fill right value */
	write_reg(dec_ctx->numtotaldpb, MFC_SI_CH1_DPB_CONF_CTRL);

	ret = mfc_cmd_init_buffers(ctx);
	if (ret < 0)
		return ret;

	return MFC_OK;
}

static int mfc_decoding_frame(struct mfc_inst_ctx *ctx, struct mfc_dec_exe_arg *exe_arg, int *consumed)
{
	unsigned int frame_type;
	int start_ofs;
	int out_display_Y_addr;
	int out_display_C_addr;

	int ret;

	/* FIXME: */
	write_reg(0xFFFFFFFF, MFC_SI_CH1_RELEASE_BUF);

	/* FIXME: is need, ALIGN_2KB */
	if (*consumed)
		start_ofs = (int)(*consumed - (ALIGN(*consumed, ALIGN_4KB) - ALIGN_4KB));
	else
		start_ofs = 0;

#ifdef DUMP_STREAM
	dump_stream(exe_arg->in_strm_buf, exe_arg->in_strm_size);
#endif

	mfc_set_stream_info(ctx, mfc_mem_base_ofs(exe_arg->in_strm_buf) >> 11,
		exe_arg->in_strm_size, start_ofs);

	ret = mfc_cmd_frame_start(ctx);
	if (ret < 0)
		return ret;

	if ((read_reg(MFC_SI_DISPLAY_STATUS) & 0x3) == DECODING_EMPTY)
		exe_arg->out_display_status = 0;
	else if ((read_reg(MFC_SI_DISPLAY_STATUS) & 0x3) == DECODING_DISPLAY)
		exe_arg->out_display_status = 1;
	else if ((read_reg(MFC_SI_DISPLAY_STATUS) & 0x3) == DISPLAY_ONLY)
		exe_arg->out_display_status = 2;
	else
		exe_arg->out_display_status = 3;

	frame_type = read_reg(MFC_SI_FRAME_TYPE);
	//mfc_ctx->FrameType = (enum s3c_mfc_frame_type) (frame_type & 0x7);

	if ((read_reg(MFC_SI_DISPLAY_STATUS) & 0x3) == DECODING_ONLY) {
		out_display_Y_addr = 0;
		out_display_C_addr = 0;
		mfc_dbg("DECODING_ONLY frame decoded \n");
	} else {
		out_display_Y_addr = read_reg(MFC_SI_DISPLAY_Y_ADR);
		out_display_C_addr = read_reg(MFC_SI_DISPLAY_C_ADR);
	}

	exe_arg->out_display_Y_addr = (out_display_Y_addr << 11);
	exe_arg->out_display_C_addr = (out_display_C_addr << 11);

	exe_arg->out_y_offset = mfc_mem_data_ofs(out_display_Y_addr << 11, 1);
	exe_arg->out_c_offset = mfc_mem_data_ofs(out_display_C_addr << 11, 1);

#ifdef CONFIG_S5P_VMEM
	mfc_dbg("cookie Y: 0x%08x, C:0x%08x\n", 
		s5p_getcookie((void *)(out_display_Y_addr << 11)),
		s5p_getcookie((void *)(out_display_C_addr << 11)));

	exe_arg->out_y_cookie = s5p_getcookie((void *)(out_display_Y_addr << 11));
	exe_arg->out_c_cookie = s5p_getcookie((void *)(out_display_C_addr << 11));
#endif

	exe_arg->out_pic_time_top = read_shm(ctx, PIC_TIME_TOP);
	exe_arg->out_pic_time_bottom = read_shm(ctx, PIC_TIME_BOT);

	exe_arg->out_consumed_byte = read_reg(MFC_SI_FRM_COUNT);

	exe_arg->out_crop_right_offset = read_shm(ctx, CROP_INFO1) >> 16;
	exe_arg->out_crop_left_offset = read_shm(ctx, CROP_INFO1);
	exe_arg->out_crop_bottom_offset = read_shm(ctx, CROP_INFO2) >> 16;
	exe_arg->out_crop_top_offset = read_shm(ctx, CROP_INFO2);

	mfc_dbg("frame_type: %d\n", frame_type);
	mfc_dbg("y: 0x%08x  c: 0x%08x\n",
		  exe_arg->out_display_Y_addr, exe_arg->out_display_C_addr);

	*consumed = read_reg(MFC_SI_FRM_COUNT);
	mfc_dbg("stream size: %d  consumed: %d\n",
		  exe_arg->in_strm_size, *consumed);

	return MFC_OK;
}

int mfc_exec_decoding(struct mfc_inst_ctx *ctx, union mfc_args *args)
{
	struct mfc_dec_exe_arg *exe_arg;
	int ret;
	int consumed = 0;
	struct mfc_dec_ctx *dec_ctx = (struct mfc_dec_ctx *)ctx->c_priv;

	exe_arg = (struct mfc_dec_exe_arg *)args;

	/* set pre-decoding informations */
	dec_ctx->streamaddr = exe_arg->in_strm_buf;
	dec_ctx->streamsize = exe_arg->in_strm_size;

	ret = mfc_decoding_frame(ctx, exe_arg, &consumed);

	return ret;
}

