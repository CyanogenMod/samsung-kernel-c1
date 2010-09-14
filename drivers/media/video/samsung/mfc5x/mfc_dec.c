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

#if 0
#define MPEG4_START_CODE_PREFIX_SIZE	3
#define MPEG4_START_CODE_PREFIX		0x000001
#define MPEG4_START_CODE_MASK		0x000000FF
static int find_mpeg4_startcode(unsigned long addr, unsigned int size)
{
	unsigned char *data;
	unsigned int i = 0;

	/* FIXME: optimize cache operation size */
	mfc_mem_cache_inv((void *)addr, size);

	/* FIXME: optimize matching algorithm */
	data = (unsigned char *)addr;

	for (i = 0; i < (size - MPEG4_START_CODE_PREFIX_SIZE); i++) {
		if ((data[i] == 0x00) && (data[i + 1] == 0x00) && (data[i + 2] == 0x01))
			return i;
	}

	return -1;
}

static int check_vcl(unsigned long addr, unsigned int size)
{
	return -1;
}
#endif

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

/*
 * alloc_ctx_buf() implementations
 */
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

/*
 * alloc_desc_buf() implementations
 */
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

/*
 * set_codec_bufs() implementations
 */
static int set_codec_bufs(struct mfc_inst_ctx *ctx)
{
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

static int mpeg4_set_codec_bufs(struct mfc_inst_ctx *ctx)
{
	struct mfc_alloc_buffer *alloc;

	alloc = _mfc_alloc_buf(ctx, MFC_CODEC_NBDCAC_SIZE, ALIGN_2KB, PORT_A);
	if (alloc == NULL) {
		mfc_err("failed alloc codec buffer\n");

		return -1;
	}
	write_reg(mfc_mem_base_ofs(alloc->real) >> 11, MFC_NB_DCAC_ADR);

	alloc = _mfc_alloc_buf(ctx, MFC_CODEC_UPNBNV_SIZE, ALIGN_2KB, PORT_A);
	if (alloc == NULL) {
		mfc_err("failed alloc codec buffer\n");

		return -1;
	}
	write_reg(mfc_mem_base_ofs(alloc->real) >> 11, MFC_UP_NB_MV_ADR);

	alloc = _mfc_alloc_buf(ctx, MFC_CODEC_SAMV_SIZE, ALIGN_2KB, PORT_A);
	if (alloc == NULL) {
		mfc_err("failed alloc codec buffer\n");

		return -1;
	}
	write_reg(mfc_mem_base_ofs(alloc->real) >> 11, MFC_SA_MV_ADR);

	alloc = _mfc_alloc_buf(ctx, MFC_CODEC_OTLINE_SIZE, ALIGN_2KB, PORT_A);
	if (alloc == NULL) {
		mfc_err("failed alloc codec buffer\n");

		return -1;
	}
	write_reg(mfc_mem_base_ofs(alloc->real) >> 11, MFC_OT_LINE_ADR);

	alloc = _mfc_alloc_buf(ctx, MFC_CODEC_SYNPAR_SIZE, ALIGN_2KB, PORT_A);
	if (alloc == NULL) {
		mfc_err("failed alloc codec buffer\n");

		return -1;
	}
	write_reg(mfc_mem_base_ofs(alloc->real) >> 11, MFC_SP_ADR);

	return 0;
}

/*
 * set_dpbs() implementations
 */
static int set_dpbs(struct mfc_inst_ctx *ctx)
{
	struct mfc_alloc_buffer *alloc;
	struct mfc_dec_ctx *dec_ctx;
	int i;
	unsigned int reg;

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

	/* set DPB number */
	reg = read_reg(MFC_SI_CH1_DPB_CONF_CTRL);
	reg &= ~(0x3FFF);
	reg |= dec_ctx->numtotaldpb;
	write_reg(reg, MFC_SI_CH1_DPB_CONF_CTRL);

	return 0;
}

static int h264_set_dpbs(struct mfc_inst_ctx *ctx)
{
	struct mfc_alloc_buffer *alloc;
	struct mfc_dec_ctx *dec_ctx;
	int i;
	unsigned int reg;

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

	/* set DPB number */
	reg = read_reg(MFC_SI_CH1_DPB_CONF_CTRL);
	reg &= ~(0x3FFF);
	reg |= dec_ctx->numtotaldpb;
	write_reg(reg, MFC_SI_CH1_DPB_CONF_CTRL);

	return 0;
}

/*
 * pre_seq_start() implementations
 */
static int pre_seq_start(struct mfc_inst_ctx *ctx)
{
	struct mfc_dec_ctx *dec_ctx = (struct mfc_dec_ctx *)ctx->c_priv;
	unsigned reg;

	/* slice interface */
	reg = read_reg(MFC_SI_CH1_DPB_CONF_CTRL);
	if (dec_ctx->slice)
		reg |= (1 << 31);
	else
		reg &= ~(1 << 31);
	write_reg(reg, MFC_SI_CH1_DPB_CONF_CTRL);

	return 0;
}

static int h264_pre_seq_start(struct mfc_inst_ctx *ctx)
{
	struct mfc_dec_ctx *dec_ctx = (struct mfc_dec_ctx *)ctx->c_priv;
	struct mfc_dec_h264 *h264 = (struct mfc_dec_h264 *)dec_ctx->d_priv;
	unsigned int reg;

	/*
	pre_seq_start(ctx);
	*/
	/* slice interface */
	reg = read_reg(MFC_SI_CH1_DPB_CONF_CTRL);
	if (dec_ctx->slice)
		reg |= (1 << 31);
	else
		reg &= ~(1 << 31);

	/* display delay */
	if (h264->dispdelay >= 0) {
		/* enable */
		reg |= (1 << 30);
		/* value */
		reg &= ~(0x3FFF << 16);
		reg |= ((h264->dispdelay & 0x3FFF) << 16);
	} else {
		/* disable & value clear */
		reg &= ~(0x7FFF << 16);
	}
	write_reg(reg, MFC_SI_CH1_DPB_CONF_CTRL);

	return 0;
}

static int mpeg4_pre_seq_start(struct mfc_inst_ctx *ctx)
{
	struct mfc_dec_ctx *dec_ctx = (struct mfc_dec_ctx *)ctx->c_priv;
	struct mfc_dec_mpeg4 *mpeg4 = (struct mfc_dec_mpeg4 *)dec_ctx->d_priv;
	unsigned int reg;

	/*
	pre_seq_start(ctx);
	*/
	/* slice interface */
	reg = read_reg(MFC_SI_CH1_DPB_CONF_CTRL);
	if (dec_ctx->slice)
		reg |= (1 << 31);
	else
		reg &= ~(1 << 31);
	write_reg(reg, MFC_SI_CH1_DPB_CONF_CTRL);

	/* loop filter, this register can be used by both decoders & encoders */
	reg = read_reg(MFC_ENC_LF_CTRL);
	if (mpeg4->postfilter)
		reg |= (1 << 0);
	else
		reg &= ~(1 << 0);
	write_reg(reg, MFC_ENC_LF_CTRL);

	return 0;
}

static int fimv1_pre_seq_start(struct mfc_inst_ctx *ctx)
{
	struct mfc_dec_ctx *dec_ctx = (struct mfc_dec_ctx *)ctx->c_priv;
	struct mfc_dec_fimv1 *fimv1 = (struct mfc_dec_fimv1 *)dec_ctx->d_priv;
	unsigned int reg;

	/*
	pre_seq_start(ctx);
	*/
	/* slice interface */
	reg = read_reg(MFC_SI_CH1_DPB_CONF_CTRL);
	if (dec_ctx->slice)
		reg |= (1 << 31);
	else
		reg &= ~(1 << 31);
	write_reg(reg, MFC_SI_CH1_DPB_CONF_CTRL);

	/* set width, height for FIMV1 */
	write_reg(fimv1->width, MFC_SI_FIMV1_HRESOL);
	write_reg(fimv1->height, MFC_SI_FIMV1_VRESOL);

	return 0;
}

/*
 * post_seq_start() implementations
 */
static int post_seq_start(struct mfc_inst_ctx *ctx)
{
	struct mfc_dec_ctx *dec_ctx = (struct mfc_dec_ctx *)ctx->c_priv;
	unsigned int shm;

	/* CHKME: case of FIMV1 */
	ctx->width = read_reg(MFC_SI_HRESOL);
	ctx->height = read_reg(MFC_SI_VRESOL);

	dec_ctx->nummindpb = read_reg(MFC_SI_BUF_NUMBER);
	dec_ctx->numtotaldpb = dec_ctx->nummindpb + dec_ctx->numextradpb;

	shm = read_shm(ctx, DISP_PIC_PROFILE);
	dec_ctx->level = (shm >> 8) & 0xFF;
	dec_ctx->profile = shm & 0x1F;

	return 0;
}

static int h264_post_seq_start(struct mfc_inst_ctx *ctx)
{
	struct mfc_dec_ctx *dec_ctx = (struct mfc_dec_ctx *)ctx->c_priv;
	struct mfc_dec_h264 *h264 = (struct mfc_dec_h264 *)dec_ctx->d_priv;
	unsigned int shm;

	/*
	post_seq_start(ctx);
	*/
	ctx->width = read_reg(MFC_SI_HRESOL);
	ctx->height = read_reg(MFC_SI_VRESOL);

	dec_ctx->nummindpb = read_reg(MFC_SI_BUF_NUMBER);
	dec_ctx->numtotaldpb = dec_ctx->nummindpb + dec_ctx->numextradpb;

	/* FIXME: consider it */
	/*
	if (dec_ctx->numtotaldpb < h264->dispdelay)
		dec_ctx->numtotaldpb = h264->dispdelay;
	*/

	shm = read_shm(ctx, DISP_PIC_PROFILE);
	dec_ctx->level = (shm >> 8) & 0xFF;
	dec_ctx->profile = shm & 0x1F;

	/* FIXME: which one will be use? */
	dec_ctx->crop_r_ofs = read_shm(ctx, CROP_INFO1) >> 16;
	dec_ctx->crop_l_ofs = read_shm(ctx, CROP_INFO1);
	dec_ctx->crop_b_ofs = read_shm(ctx, CROP_INFO2) >> 16;
	dec_ctx->crop_t_ofs = read_shm(ctx, CROP_INFO2);

	h264->crop_r_ofs = read_shm(ctx, CROP_INFO1) >> 16;
	h264->crop_l_ofs = read_shm(ctx, CROP_INFO1);
	h264->crop_b_ofs = read_shm(ctx, CROP_INFO2) >> 16;
	h264->crop_t_ofs = read_shm(ctx, CROP_INFO2);

	return 0;
}

static int mpeg4_post_seq_start(struct mfc_inst_ctx *ctx)
{
	struct mfc_dec_ctx *dec_ctx = (struct mfc_dec_ctx *)ctx->c_priv;
	struct mfc_dec_mpeg4 *mpeg4 = (struct mfc_dec_mpeg4 *)dec_ctx->d_priv;
	unsigned int shm;

	/*
	post_seq_start(ctx);
	*/
	ctx->width = read_reg(MFC_SI_HRESOL);
	ctx->height = read_reg(MFC_SI_VRESOL);

	dec_ctx->nummindpb = read_reg(MFC_SI_BUF_NUMBER);
	dec_ctx->numtotaldpb = dec_ctx->nummindpb + dec_ctx->numextradpb;

	shm = read_shm(ctx, DISP_PIC_PROFILE);
	dec_ctx->level = (shm >> 8) & 0xFF;
	dec_ctx->profile = shm & 0x1F;

	/* FIXME: which one will be use? */
	dec_ctx->aspect_ratio = read_shm(ctx, ASPECT_RATIO_INFO) & 0xF;
	if (dec_ctx->aspect_ratio == 15) {
		shm = read_shm(ctx, EXTENDED_PAR);
		dec_ctx->ext_par_width = (shm >> 16) & 0xFFFF;
		dec_ctx->ext_par_height = shm & 0xFFFF;
	}

	mpeg4->aspect_ratio = read_shm(ctx, ASPECT_RATIO_INFO) & 0xF;
	if (mpeg4->aspect_ratio == 15) {
		shm = read_shm(ctx, EXTENDED_PAR);
		mpeg4->ext_par_width = (shm >> 16) & 0xFFFF;
		mpeg4->ext_par_height = shm & 0xFFFF;
	}

	return 0;
}

/*
 * pre_frame_start() implementations
 */
static int pre_frame_start(struct mfc_inst_ctx *ctx)
{
	return 0;
}

/*
 * post_frame_start() implementations
 */
static int post_frame_start(struct mfc_inst_ctx *ctx)
{
	return 0;
}

/*
 * multi_frame_start() implementations
 */
static int multi_data_frame(struct mfc_inst_ctx *ctx)
{
	return 0;
}

static int mpeg4_multi_data_frame(struct mfc_inst_ctx *ctx)
{
	struct mfc_dec_ctx *dec_ctx = (struct mfc_dec_ctx *)ctx->c_priv;

	if (!dec_ctx->packedpb)
		return 0;

	/* FIXME: I_FRAME is valid? */
	if ((dec_ctx->frametype == DEC_FRM_I) || (dec_ctx->frametype == DEC_FRM_P)) {

	}

	return 0;
}

static struct mfc_dec_info default_dec = {
	.name		= "DEFAULT",
	.codectype	= UNKNOWN,
	.codecid	= -1,
	.d_priv_size	= 0,
	.c_ops		= {
		.alloc_ctx_buf		= alloc_ctx_buf,
		.alloc_desc_buf 	= alloc_desc_buf,
		.set_codec_bufs		= set_codec_bufs,
		.set_dpbs		= set_dpbs,
		.pre_seq_start 		= pre_seq_start,
		.post_seq_start 	= post_seq_start,
		.pre_frame_start	= pre_frame_start,
		.post_frame_start	= post_frame_start,
		.multi_data_frame	= multi_data_frame,
	},
};

static struct mfc_dec_info h264_dec = {
	.name		= "H264",
	.codectype	= H264_DEC,
	.codecid	= 0,
	.d_priv_size	= sizeof(struct mfc_dec_h264),
	.c_ops		= {
		.alloc_ctx_buf		= h264_alloc_ctx_buf,
		.alloc_desc_buf 	= alloc_desc_buf,
		.set_codec_bufs		= h264_set_codec_bufs,
		.set_dpbs		= h264_set_dpbs,
		.pre_seq_start 		= h264_pre_seq_start,
		.post_seq_start 	= h264_post_seq_start,
		.pre_frame_start	= NULL,
		.post_frame_start	= NULL,
		.multi_data_frame	= NULL,
	},
};

static struct mfc_dec_info mpeg4_dec = {
	.name		= "MPEG4",
	.codectype	= MPEG4_DEC,
	.codecid	= 2,
	.d_priv_size	= sizeof(struct mfc_dec_mpeg4),
	.c_ops		= {
		.alloc_ctx_buf		= alloc_ctx_buf,
		.alloc_desc_buf 	= alloc_desc_buf,
		.set_codec_bufs		= mpeg4_set_codec_bufs,
		.set_dpbs		= set_dpbs,
		.pre_seq_start 		= mpeg4_pre_seq_start,
		.post_seq_start 	= mpeg4_post_seq_start,
		.pre_frame_start	= NULL,
		.post_frame_start	= NULL,
		.multi_data_frame	= NULL,
	},
};

static struct mfc_dec_info fimv1_dec = {
	.name		= "FIMV1",
	.codectype	= FIMV1_DEC,
	.codecid	= 6,
	.d_priv_size	= sizeof(struct mfc_dec_fimv1),
	.c_ops		= {
		.alloc_ctx_buf		= alloc_ctx_buf,
		.alloc_desc_buf 	= alloc_desc_buf,
		.set_codec_bufs		= mpeg4_set_codec_bufs,
		.set_dpbs		= set_dpbs,
		.pre_seq_start 		= fimv1_pre_seq_start,
		.post_seq_start 	= mpeg4_post_seq_start,	/* FIMXE */
		.pre_frame_start	= NULL,
		.post_frame_start	= NULL,
		.multi_data_frame	= mpeg4_multi_data_frame,
	},
};

void mfc_init_decoders(void)
{
	list_add_tail(&default_dec.list, &mfc_decoders);

	list_add_tail(&h264_dec.list, &mfc_decoders);
	list_add_tail(&mpeg4_dec.list, &mfc_decoders);
	list_add_tail(&fimv1_dec.list, &mfc_decoders);
}

static int mfc_set_decoder(struct mfc_inst_ctx *ctx, enum codec_type codectype)
{
	struct list_head *pos;
	struct mfc_dec_info *decoder;
	int codecid = -1;
	struct mfc_dec_ctx *dec_ctx;

	/* find and set codec private */
	list_for_each(pos, &mfc_decoders) {
		decoder = list_entry(pos, struct mfc_dec_info, list);

		if (decoder->codectype == codectype) {
			if (decoder->codecid < 0)
				break;

			ctx->type = DECODER;
			ctx->c_ops = (struct codec_operations *)&decoder->c_ops;

			/* FIXME: free below when decoding finish or instance destory */
			dec_ctx = kzalloc(sizeof(struct mfc_dec_ctx), GFP_KERNEL);
			if (!dec_ctx) {
				mfc_err("failed to allocate codec private\n");
				return -ENOMEM;
			}
			ctx->c_priv = dec_ctx;

			/* FIXME: free below when decoding finish or instance destory */
			dec_ctx->d_priv = kzalloc(decoder->d_priv_size, GFP_KERNEL);
			if (!dec_ctx->d_priv) {
				mfc_err("failed to allocate decoder private\n");
				kfree(dec_ctx);
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

	/* FIXME: fill the value */
	dec_ctx->crc = 0;
	dec_ctx->pixelcache = 0;
	dec_ctx->packedpb = 0;
	dec_ctx->numextradpb = 0;
	dec_ctx->packedpb = init_arg->in_packed_PB;

	dec_ctx->streamaddr = init_arg->in_strm_buf;
	dec_ctx->streamsize = init_arg->in_strm_size;

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

	mfc_set_inst_state(ctx, INST_STATE_DEC_INIT);

	if (init_shm(ctx) < 0)
		return MFC_DEC_INIT_FAIL;

	/*
	 * allocate descriptor buffer
	 */
	if (ctx->c_ops->alloc_desc_buf) {
		if (ctx->c_ops->alloc_desc_buf(ctx) < 0)
			return MFC_DEC_INIT_FAIL;
	}

	/*
	 * execute pre sequence start operation
	 */
	if (ctx->c_ops->pre_seq_start) {
		if (ctx->c_ops->pre_seq_start(ctx) < 0)
			return MFC_DEC_INIT_FAIL;
	}

#ifdef DUMP_STREAM
	dump_stream(dec_ctx->streamaddr, dec_ctx->streamsize);
#endif
	/* FIXME: postion */
	mfc_set_stream_info(ctx, mfc_mem_base_ofs(dec_ctx->streamaddr) >> 11,
		dec_ctx->streamsize, 0);

	ret = mfc_cmd_seq_start(ctx);
	if (ret < 0)
		return ret;

	if (ctx->c_ops->post_seq_start) {
		if (ctx->c_ops->post_seq_start(ctx) < 0)
			return MFC_DEC_INIT_FAIL;
	}

	/* FIXME: */
	init_arg->out_img_width = ctx->width;
	init_arg->out_img_height = ctx->height;
	init_arg->out_buf_width = ALIGN(ctx->width, 128);
	init_arg->out_buf_height = ALIGN(ctx->height, 32);
	init_arg->out_dpb_cnt = dec_ctx->numtotaldpb;

	/* FIXME: add decoder specific data (MPEG4, H.264) */

	/* FIXME: */
	init_arg->out_crop_right_offset = dec_ctx->crop_r_ofs;
	init_arg->out_crop_left_offset = dec_ctx->crop_l_ofs;
	init_arg->out_crop_bottom_offset = dec_ctx->crop_b_ofs;
	init_arg->out_crop_top_offset = dec_ctx->crop_t_ofs;
	/*
	h264->crop_r_ofs, h264->crop_l_ofs, h264->crop_b_ofs, h264->crop_t_ofs
	*/

	mfc_dbg("H: %d, W: %d, DPB_Count: %d", ctx->width, ctx->height,
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

	ret = mfc_cmd_init_buffers(ctx);
	if (ret < 0)
		return ret;

	return MFC_OK;
}

static int mfc_decoding_frame(struct mfc_inst_ctx *ctx, struct mfc_dec_exe_arg *exe_arg, int *consumed)
{
	int start_ofs;
	int out_display_Y_addr;
	int out_display_C_addr;

	int ret;

	struct mfc_dec_ctx *dec_ctx = (struct mfc_dec_ctx *)ctx->c_priv;

	/* FIXME: set, get frame tag, picture time */

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
	/* FIXME: postion */
	mfc_set_stream_info(ctx, mfc_mem_base_ofs(exe_arg->in_strm_buf) >> 11,
		exe_arg->in_strm_size, start_ofs);

	ret = mfc_cmd_frame_start(ctx);
	if (ret < 0)
		return ret;

	/*
	if ((read_reg(MFC_SI_DISPLAY_STATUS) & 0x3) == DECODING_EMPTY)
		exe_arg->out_display_status = 0;
	else if ((read_reg(MFC_SI_DISPLAY_STATUS) & 0x3) == DECODING_DISPLAY)
		exe_arg->out_display_status = 1;
	else if ((read_reg(MFC_SI_DISPLAY_STATUS) & 0x3) == DISPLAY_ONLY)
		exe_arg->out_display_status = 2;
	else
		exe_arg->out_display_status = 3;
	*/
	/* FIXME: add resolution change information */
	dec_ctx->disp_status = read_reg(MFC_SI_DISPLAY_STATUS) & DISP_S_MASK;

	dec_ctx->frametype = read_reg(MFC_SI_FRAME_TYPE) & DEC_FRM_T_MASK;

	if (dec_ctx->disp_status == DISP_S_DECODING) {
		out_display_Y_addr = 0;
		out_display_C_addr = 0;
		mfc_dbg("DECODING_ONLY frame decoded\n");
	} else {
		#if 0
		if (mfc_ctx->IsPackedPB) {
			if ((mfc_ctx->FrameType == MFC_RET_FRAME_P_FRAME) ||
			    (mfc_ctx->FrameType == MFC_RET_FRAME_I_FRAME)) {
				out_display_Y_addr = read_reg(MFC_SI_DISPLAY_Y_ADR);
				out_display_C_addr = read_reg(MFC_SI_DISPLAY_C_ADR);
			} else {
				out_display_Y_addr = mfc_ctx->pre_display_Y_addr;
				out_display_C_addr = mfc_ctx->pre_display_C_addr;
			}
			/* save the display addr */
			mfc_ctx->pre_display_Y_addr = read_reg(MFC_SI_DISPLAY_Y_ADR);
			mfc_ctx->pre_display_C_addr = read_reg(MFC_SI_DISPLAY_C_ADR);
			mfc_dbg("(pre_Y_ADDR : 0x%08x  pre_C_ADDR : 0x%08x)\r\n",
				(mfc_ctx->pre_display_Y_addr << 11),
				(mfc_ctx->pre_display_C_addr << 11));
		} else {
			out_display_Y_addr = read_reg(MFC_SI_DISPLAY_Y_ADR);
			out_display_C_addr = read_reg(MFC_SI_DISPLAY_C_ADR);
		}
		#else
		out_display_Y_addr = read_reg(MFC_SI_DISPLAY_Y_ADR);
		out_display_C_addr = read_reg(MFC_SI_DISPLAY_C_ADR);
		#endif
	}

	exe_arg->out_display_status = dec_ctx->disp_status;

	exe_arg->out_display_Y_addr = (out_display_Y_addr << 11);
	exe_arg->out_display_C_addr = (out_display_C_addr << 11);

	exe_arg->out_y_offset = mfc_mem_data_ofs(out_display_Y_addr << 11, 1);
	exe_arg->out_c_offset = mfc_mem_data_ofs(out_display_C_addr << 11, 1);

#ifdef CONFIG_S5P_VMEM
	mfc_dbg("cookie Y: 0x%08x, C:0x%08x\n", s5p_getcookie((void *)(out_display_Y_addr << 11)),
		s5p_getcookie((void *)(out_display_C_addr << 11)));

	exe_arg->out_y_cookie = s5p_getcookie((void *)(out_display_Y_addr << 11));
	exe_arg->out_c_cookie = s5p_getcookie((void *)(out_display_C_addr << 11));
#endif

	exe_arg->out_pic_time_top = read_shm(ctx, PIC_TIME_TOP);
	exe_arg->out_pic_time_bottom = read_shm(ctx, PIC_TIME_BOT);

	exe_arg->out_consumed_byte = read_reg(MFC_SI_FRM_COUNT);

	//if (ctx->codecid == H264_DEC) {
		exe_arg->out_crop_right_offset = read_shm(ctx, CROP_INFO1) >> 16;
		exe_arg->out_crop_left_offset = read_shm(ctx, CROP_INFO1);
		exe_arg->out_crop_bottom_offset = read_shm(ctx, CROP_INFO2) >> 16;
		exe_arg->out_crop_top_offset = read_shm(ctx, CROP_INFO2);
	//}

	mfc_dbg("frame type: %d\n", dec_ctx->frametype);
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

	/*
	if (dec_ctx->packedpb) &&
		(dec_ctx->frametype == P_FRAME) &&
		(exe_arg->in_frm_size) {

	}

	if (ctx->c_ops->set_dpbs) {
		if (ctx->c_ops->set_dpbs(ctx) < 0)
			return MFC_DEC_INIT_FAIL;
	}
	*/

	return ret;
}

