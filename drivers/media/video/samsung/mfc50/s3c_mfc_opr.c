/* linux/drivers/media/video/samsung/mfc50/s3c_mfc_opr.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Operation for MFC HW setting from driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <mach/regs-mfc.h>
#include <linux/dma-mapping.h>
#include <asm/cacheflush.h>
#include <linux/slab.h>

#include "s3c_mfc_common.h"
#include "s3c_mfc_opr.h"
#include "s3c_mfc_logmsg.h"
#include "s3c_mfc_memory.h"
#include "s3c_mfc_buffer_manager.h"
#include "s3c_mfc_interface.h"
#include "s3c_mfc_intr.h"

static int s3c_mfc_init_count;

#define READL(offset)		readl(s3c_mfc_sfr_virt_base + (offset))
#define WRITEL(data, offset)	writel((data), s3c_mfc_sfr_virt_base + (offset))

/*
 * DMA_FROM_DEVICE : invalidate only
 * DMA_TO_DEVICE   : writeback only
 * DMA_BIDIRECTIONAL : writeback and invalidate
 */
#define WRITEL_SHARED_MEM(data, address) \
	{ writel(data, address); \
	dmac_unmap_area((void *)address, 4, DMA_TO_DEVICE); }
#define READL_SHARED_MEM(address) \
	{ dmac_map_area((void *)address, 4, DMA_FROM_DEVICE); \
	readl(address); }
#define READL_STREAM_BUF(address) READL_SHARED_MEM(address)

static void s3c_mfc_cmd_reset(void);
static void s3c_mfc_set_encode_init_param(int inst_no,
					  enum SSBSIP_MFC_CODEC_TYPE mfc_codec_type,
					  union s3c_mfc_args *args);

static int s3c_mfc_get_inst_no(enum SSBSIP_MFC_CODEC_TYPE codec_type,
			       unsigned int crc_enable);

static enum SSBSIP_MFC_ERROR_CODE s3c_mfc_set_dec_stream_buffer(struct s3c_mfc_inst_ctx *mfc_ctx,
							   int buf_addr,
							   unsigned int
							   start_byte_num,
							   unsigned int
							   buf_size);
static enum SSBSIP_MFC_ERROR_CODE s3c_mfc_set_shared_mem_buffer(struct s3c_mfc_inst_ctx *mfc_ctx);
static enum SSBSIP_MFC_ERROR_CODE s3c_mfc_set_risc_buffer(enum SSBSIP_MFC_CODEC_TYPE
						     codec_type, int inst_no);
static enum SSBSIP_MFC_ERROR_CODE s3c_mfc_decode_one_frame(struct s3c_mfc_inst_ctx *
						      mfc_ctx,
						      struct s3c_mfc_dec_exe_arg_t *
						      dec_arg,
						      int *consumed_strm_size);
static void get_byte(int buff, int *code);
static bool is_vcl(struct s3c_mfc_dec_exe_arg_t *dec_arg, int consumed_strm_size);
void s3c_mfc_check_change_resolution(struct s3c_mfc_inst_ctx *mfc_ctx,
						struct s3c_mfc_dec_exe_arg_t *args);


static void s3c_mfc_cmd_reset(void)
{
	unsigned int mc_status;

	/* Stop procedure */
	WRITEL(0x3f7, S3C_FIMV_SW_RESET);	/*  reset VI */
	WRITEL(0x3f6, S3C_FIMV_SW_RESET);	/*  reset RISC */
	WRITEL(0x3e2, S3C_FIMV_SW_RESET);	/*  All reset except for MC */
	mdelay(10);

	/* Check MC status */
	do {
		mc_status = READL(S3C_FIMV_MC_STATUS);
	} while (mc_status & 0x3);

	WRITEL(0x0, S3C_FIMV_SW_RESET);

	WRITEL(0x3fe, S3C_FIMV_SW_RESET);
}

static void s3c_mfc_cmd_host2risc(enum s3c_mfc_facade_cmd cmd, int arg1, int arg2)
{
	enum s3c_mfc_facade_cmd cur_cmd;
	unsigned int fw_phybuf, context_base_addr;

	/* wait until host to risc command register becomes 'H2R_CMD_EMPTY' */
	do {
		cur_cmd = READL(S3C_FIMV_HOST2RISC_CMD);
	} while (cur_cmd != H2R_CMD_EMPTY);

	WRITEL(arg1, S3C_FIMV_HOST2RISC_ARG1);

	if (cmd == H2R_CMD_OPEN_INSTANCE) {
		fw_phybuf =
		    ALIGN(s3c_mfc_get_fw_buf_phys_addr(), 128 * BUF_L_UNIT);
		context_base_addr =
		    s3c_mfc_get_fw_context_phys_addr(s3c_mfc_init_count);

		/* pixel cache enable(0)/disable(3)
		   WRITEL(3, S3C_FIMV_HOST2RISC_ARG2); */

		/* Enable CRC data */
		WRITEL(arg2 << 31, S3C_FIMV_HOST2RISC_ARG2);

		WRITEL((context_base_addr - fw_phybuf) >> 11,
		       S3C_FIMV_HOST2RISC_ARG3);
		WRITEL(MFC_FW_BUF_SIZE, S3C_FIMV_HOST2RISC_ARG4);

		mfc_debug
		    ("s3c_mfc_init_count : %d, fw_phybuf : 0x%08x, \
		     context_base_addr : 0x%08x\n",
		     s3c_mfc_init_count, fw_phybuf, context_base_addr);
	}

	WRITEL(cmd, S3C_FIMV_HOST2RISC_CMD);
}

static enum SSBSIP_MFC_ERROR_CODE s3c_mfc_set_dec_stream_buffer(struct s3c_mfc_inst_ctx *mfc_ctx,
							   int buf_addr,
							   unsigned int
							   start_byte_num,
							   unsigned int
							   buf_size)
{
	unsigned int fw_phybuf;
	unsigned int risc_phy_buf, aligned_risc_phy_buf;

	mfc_debug
	    ("inst_no : %d, buf_addr : 0x%08x, start_byte_num : 0x%08x, \
	     buf_size : 0x%08x\n",
	     mfc_ctx->InstNo, buf_addr, start_byte_num, buf_size);

	fw_phybuf = ALIGN(s3c_mfc_get_fw_buf_phys_addr(), 128 * BUF_L_UNIT);

	risc_phy_buf = s3c_mfc_get_risc_buf_phys_addr(mfc_ctx->InstNo);
	aligned_risc_phy_buf = ALIGN(risc_phy_buf, 2 * BUF_L_UNIT);

	/* Buf_size is '0' when last frame */
	WRITEL(buf_size, S3C_FIMV_SI_CH1_SB_FRM_SIZE);
	WRITEL((aligned_risc_phy_buf - fw_phybuf) >> 11,
	       S3C_FIMV_SI_CH1_DESC_ADR);
	WRITEL(CPB_BUF_SIZE, S3C_FIMV_SI_CH1_CPB_SIZE);
	WRITEL(DESC_BUF_SIZE, S3C_FIMV_SI_CH1_DESC_SIZE);
	WRITEL((buf_addr - fw_phybuf) >> 11, S3C_FIMV_SI_CH1_SB_ST_ADR);

	WRITEL_SHARED_MEM(start_byte_num, mfc_ctx->shared_mem_vir_addr + 0x18);

	mfc_debug("risc_phy_buf : 0x%08x, aligned_risc_phy_buf : 0x%08x\n",
		  risc_phy_buf, aligned_risc_phy_buf);

	return MFC_RET_OK;
}

enum SSBSIP_MFC_ERROR_CODE s3c_mfc_set_dec_frame_buffer(struct s3c_mfc_inst_ctx *mfc_ctx,
						   union s3c_mfc_args *args)
{
	unsigned int width, height, frame_size;
	unsigned int chroma_base, luma_base;
	unsigned int i;
	struct s3c_mfc_frame_buf_arg_t dpb_buf_addr;
	unsigned int aligned_width, aligned_ch_height, aligned_mv_height;
	struct s3c_mfc_dec_init_arg_t *init_arg;
	unsigned int slice_en_display_en;
	unsigned int luma_size, chroma_size, mv_size;

	init_arg = (struct s3c_mfc_dec_init_arg_t *) args;

	mfc_debug("luma_buf_addr : 0x%08x  luma_buf_size : %d\n",
		  init_arg->out_p_addr.luma, init_arg->out_frame_buf_size.luma);
	mfc_debug("chroma_buf_addr : 0x%08x  chroma_buf_size : %d\n",
		  init_arg->out_p_addr.chroma,
		  init_arg->out_frame_buf_size.chroma);

	/*
	 * CHKME: jtpark
	 * fw_phybuf = align(s3c_mfc_get_fw_buf_phys_addr(), 128*BUF_L_UNIT);
	 */
	chroma_base = s3c_mfc_get_fw_buf_phys_addr();
#if CONFIG_VIDEO_MFC_MEM_PORT_COUNT == 1
	luma_base = chroma_base;
#else
	luma_base = s3c_mfc_get_dpb_luma_buf_phys_addr();
#endif

	width = ALIGN(mfc_ctx->img_width, BUF_S_UNIT / 2);
	height = ALIGN(mfc_ctx->img_height, BUF_S_UNIT);
	frame_size = (width * height * 3) >> 1;

	mfc_debug
	    ("width : %d height : %d frame_size : %d mfc_ctx->DPBCnt :%d\n",
	     width, height, frame_size, mfc_ctx->DPBCnt);

	if ((init_arg->out_frame_buf_size.luma +
	     init_arg->out_frame_buf_size.chroma) <
	    frame_size * mfc_ctx->totalDPBCnt) {
		mfc_err("MFC_RET_FRM_BUF_SIZE_FAIL\n");
		return MFC_RET_FRM_BUF_SIZE_FAIL;
	}

	aligned_width = ALIGN(mfc_ctx->img_width, 4 * BUF_S_UNIT);	/* 128B align */
	aligned_ch_height = ALIGN(mfc_ctx->img_height / 2, BUF_S_UNIT);	/* 32B align */
	aligned_mv_height = ALIGN(mfc_ctx->img_height / 4, BUF_S_UNIT);	/* 2B align */

	dpb_buf_addr.luma = ALIGN(init_arg->out_p_addr.luma, 2 * BUF_S_UNIT);
	dpb_buf_addr.chroma =
	    ALIGN(init_arg->out_p_addr.chroma, 2 * BUF_S_UNIT);

	mfc_debug("DEC_LUMA_DPB_START_ADR : 0x%08x\n", dpb_buf_addr.luma);
	mfc_debug("DEC_CHROMA_DPB_START_ADR : 0x%08x\n", dpb_buf_addr.chroma);

	if (mfc_ctx->MfcCodecType == H264_DEC) {
		for (i = 0; i < mfc_ctx->totalDPBCnt; i++) {

			/* set Luma address */
			WRITEL((dpb_buf_addr.luma - luma_base) >> 11,
			       S3C_FIMV_LUMA_ADR + (4 * i));
			dpb_buf_addr.luma +=
			    ALIGN(aligned_width * height, 8 * BUF_L_UNIT);

			/* set MV address */
			WRITEL((dpb_buf_addr.luma - luma_base) >> 11,
			       S3C_FIMV_MV_ADR + (4 * i));
			dpb_buf_addr.luma +=
			    ALIGN(aligned_width * aligned_mv_height,
				  8 * BUF_L_UNIT);

			/* set Chroma address */
			WRITEL((dpb_buf_addr.chroma - chroma_base) >> 11,
			       S3C_FIMV_CHROMA_ADR + (4 * i));
			dpb_buf_addr.chroma +=
			    ALIGN(aligned_width * aligned_ch_height,
				  8 * BUF_L_UNIT);
		}
	} else {
		for (i = 0; i < mfc_ctx->totalDPBCnt; i++) {

			/* set Luma address */
			WRITEL((dpb_buf_addr.luma - luma_base) >> 11,
			       S3C_FIMV_LUMA_ADR + (4 * i));
			dpb_buf_addr.luma +=
			    ALIGN(aligned_width * height, 8 * BUF_L_UNIT);

			/* set Chroma address */
			WRITEL((dpb_buf_addr.chroma - chroma_base) >> 11,
			       S3C_FIMV_CHROMA_ADR + (4 * i));
			dpb_buf_addr.chroma +=
			    ALIGN(aligned_width * aligned_ch_height,
				  8 * BUF_L_UNIT);
		}
	}

	/* Set DPB number */
	slice_en_display_en = READL(S3C_FIMV_SI_CH1_DPB_CONF_CTRL) & 0xffff0000;
	WRITEL(init_arg->out_dpb_cnt | slice_en_display_en,
	       S3C_FIMV_SI_CH1_DPB_CONF_CTRL);

	/* MFC fw 9/30 */

	/* set the luma DPB size */
	luma_size = ALIGN(aligned_width * height, 64 * BUF_L_UNIT);
	WRITEL_SHARED_MEM(luma_size, mfc_ctx->shared_mem_vir_addr + 0x64);

	/* set the chroma DPB size */
	chroma_size = ALIGN(aligned_width * aligned_ch_height, 64 * BUF_L_UNIT);
	WRITEL_SHARED_MEM(chroma_size, mfc_ctx->shared_mem_vir_addr + 0x68);
	if (mfc_ctx->MfcCodecType == H264_DEC) {
		/* set the mv DPB size */
		mv_size =
		    ALIGN(aligned_width * aligned_mv_height, 64 * BUF_L_UNIT);
		WRITEL_SHARED_MEM(mv_size, mfc_ctx->shared_mem_vir_addr + 0x6c);
	}

	/* MFC fw 8/7 */
	WRITEL((INIT_BUFFER << 16 & 0x70000) | (mfc_ctx->InstNo),
	       S3C_FIMV_SI_CH1_INST_ID);
	if (s3c_mfc_wait_for_done(R2H_CMD_INIT_BUFFERS_RET) == 0) {
		mfc_err("MFC_RET_DEC_INIT_BUF_FAIL\n");
		return MFC_RET_DEC_INIT_BUF_FAIL;
	}

	mfc_debug("img_width : %d img_height : %d\n",
		  init_arg->out_img_width, init_arg->out_img_height);
	mfc_debug("DEC_LUMA_DPB_END_ADR : 0x%08x\n", dpb_buf_addr.luma);
	mfc_debug("DEC_CHROMA_DPB_END_ADR : 0x%08x\n", dpb_buf_addr.chroma);

	return MFC_RET_OK;
}

enum SSBSIP_MFC_ERROR_CODE s3c_mfc_set_enc_ref_buffer(struct s3c_mfc_inst_ctx *mfc_ctx,
						 union s3c_mfc_args *args)
{
	struct s3c_mfc_enc_init_mpeg4_arg_t *init_arg;
	unsigned int width, height, frame_size;
	unsigned int y01_base, y23_c_ip_base;
	unsigned int i;
	unsigned int aligned_width, aligned_height;

	unsigned int ref_y, mv_ref_yc;

	init_arg = (struct s3c_mfc_enc_init_mpeg4_arg_t *) args;

	mfc_debug("strm_ref_y_buf_addr : 0x%08x  strm_ref_y_buf_size : %d\n",
		  init_arg->out_p_addr.strm_ref_y,
		  init_arg->out_buf_size.strm_ref_y);

	mfc_debug("mv_ref_yc_buf_addr : 0x%08x  mv_ref_yc_buf_size : %d\n",
		  init_arg->out_p_addr.mv_ref_yc,
		  init_arg->out_buf_size.mv_ref_yc);

	/*
	 * CHKME: jtpark
	 * fw_phybuf = align(s3c_mfc_get_fw_buf_phys_addr(), 128*BUF_L_UNIT);
	 */
	y01_base = s3c_mfc_get_fw_buf_phys_addr();
#if CONFIG_VIDEO_MFC_MEM_PORT_COUNT == 1
	y23_c_ip_base = y01_base;
#else
	y23_c_ip_base = s3c_mfc_get_dpb_luma_buf_phys_addr();
#endif

	width = ALIGN(mfc_ctx->img_width, BUF_S_UNIT / 2);
	height = ALIGN(mfc_ctx->img_height, BUF_S_UNIT);
	frame_size = (width * height * 3) >> 1;

	mfc_debug("width : %d height : %d frame_size : %d\n", width, height,
		  frame_size);

	aligned_width = ALIGN(mfc_ctx->img_width, 4 * BUF_S_UNIT);	/* 128B align */
	aligned_height = ALIGN(mfc_ctx->img_height, BUF_S_UNIT);	/* 32B align */

	ref_y = init_arg->out_p_addr.strm_ref_y + STREAM_BUF_SIZE;
	ref_y = ALIGN(ref_y, 64 * BUF_L_UNIT);
	mv_ref_yc = init_arg->out_p_addr.mv_ref_yc;

	mv_ref_yc = ALIGN(mv_ref_yc, 64 * BUF_L_UNIT);

	for (i = 0; i < 2; i++) {
		/* Set refY0,Y1 */
		WRITEL((ref_y - y01_base) >> 11,
		       S3C_FIMV_ENC_REF0_LUMA_ADR + (4 * i));
		ref_y += ALIGN(aligned_width * aligned_height, 64 * BUF_L_UNIT);

		/* Set refY2,Y3 */
		WRITEL((mv_ref_yc - y23_c_ip_base) >> 11,
		       S3C_FIMV_ENC_REF2_LUMA_ADR + (4 * i));
		mv_ref_yc +=
		    ALIGN(aligned_width * aligned_height, 64 * BUF_L_UNIT);
	}

/* MFC fw 10/30, EVT0 */
#if defined(CONFIG_CPU_S5PV210_EVT0)
	WRITEL((ref_y - y01_base) >> 11, S3C_FIMV_ENC_REF_B_LUMA_ADR);
	ref_y += ALIGN(aligned_width * aligned_height, 64 * BUF_L_UNIT);
	WRITEL((ref_y - y01_base) >> 11, S3C_FIMV_ENC_REF_B_CHROMA_ADR);
	ref_y += ALIGN(aligned_width * aligned_height / 2, 64 * BUF_L_UNIT);	/* Align size should be checked */
#endif

	for (i = 0; i < 4; i++) {
		/* Set refC0~C3 */
		WRITEL((mv_ref_yc - y23_c_ip_base) >> 11,
		       S3C_FIMV_ENC_REF0_CHROMA_ADR + (4 * i));
		mv_ref_yc += ALIGN(aligned_width * aligned_height / 2, 64 * BUF_L_UNIT);	/* Align size should be checked */
	}

	/* for pixel cache */
	WRITEL((mv_ref_yc - y23_c_ip_base) >> 11,
	       S3C_FIMV_ENC_UP_INTRA_PRED_ADR);
	mv_ref_yc += 64 * BUF_L_UNIT;

	mfc_debug("ENC_REFY01_END_ADR : 0x%08x\n", ref_y);
	mfc_debug("ENC_REFYC_MV_END_ADR : 0x%08x\n", mv_ref_yc);

	return MFC_RET_OK;
}

static enum SSBSIP_MFC_ERROR_CODE s3c_mfc_set_shared_mem_buffer(struct s3c_mfc_inst_ctx *mfc_ctx)
{
	unsigned int fw_phybuf;
	unsigned int i;

	fw_phybuf = ALIGN(s3c_mfc_get_fw_buf_phys_addr(), 128 * BUF_L_UNIT);

	mfc_ctx->shared_mem_phy_addr =
	    s3c_mfc_get_risc_buf_phys_addr(mfc_ctx->InstNo) + (RISC_BUF_SIZE -
						       SHARED_MEM_SIZE);
	mfc_ctx->shared_mem_phy_addr = ALIGN(mfc_ctx->shared_mem_phy_addr, 2 * BUF_L_UNIT);

	mfc_ctx->shared_mem_vir_addr = phys_to_virt(mfc_ctx->shared_mem_phy_addr);
	/* It should be cache op for shared memory init. */
	for (i = 0; i < SHARED_MEM_SIZE; i += 4)
		WRITEL_SHARED_MEM(0, mfc_ctx->shared_mem_vir_addr+i)
	WRITEL((mfc_ctx->shared_mem_phy_addr - fw_phybuf), S3C_FIMV_SI_CH1_HOST_WR_ADR);

	mfc_debug
	    ("inst_no : %d, shared_mem_phy_addr : 0x%08x, shared_mem_vir_addr : 0x%08x\r\n",
	     mfc_ctx->InstNo, mfc_ctx->shared_mem_phy_addr, (unsigned int)mfc_ctx->shared_mem_vir_addr);

	return MFC_RET_OK;
}

/* Set the desc, motion vector, overlap, bitplane0/1/2, etc */
static enum SSBSIP_MFC_ERROR_CODE s3c_mfc_set_risc_buffer(enum SSBSIP_MFC_CODEC_TYPE
						     codec_type, int inst_no)
{
	unsigned int fw_phybuf;
	unsigned int risc_phy_buf, aligned_risc_phy_buf;

	fw_phybuf = ALIGN(s3c_mfc_get_fw_buf_phys_addr(), 128 * BUF_L_UNIT);

	risc_phy_buf = s3c_mfc_get_risc_buf_phys_addr(inst_no);
	aligned_risc_phy_buf =
	    ALIGN(risc_phy_buf, 2 * BUF_L_UNIT) + DESC_BUF_SIZE;

	mfc_debug("inst_no : %d, risc_buf_start : 0x%08x\n", inst_no,
		  risc_phy_buf);

	switch (codec_type) {
	case H264_DEC:
		WRITEL((aligned_risc_phy_buf - fw_phybuf) >> 11,
		       S3C_FIMV_VERT_NB_MV_ADR);
		aligned_risc_phy_buf += 16 * BUF_L_UNIT;
		WRITEL((aligned_risc_phy_buf - fw_phybuf) >> 11,
		       S3C_FIMV_VERT_NB_IP_ADR);
		aligned_risc_phy_buf += 32 * BUF_L_UNIT;
		break;

	case MPEG4_DEC:
	case XVID_DEC:
	case H263_DEC:
	case FIMV1_DEC:
	case FIMV2_DEC:
	case FIMV3_DEC:
	case FIMV4_DEC:
		WRITEL((aligned_risc_phy_buf - fw_phybuf) >> 11,
		       S3C_FIMV_NB_DCAC_ADR);
		aligned_risc_phy_buf += 16 * BUF_L_UNIT;
		WRITEL((aligned_risc_phy_buf - fw_phybuf) >> 11,
		       S3C_FIMV_UP_NB_MV_ADR);
		aligned_risc_phy_buf += 68 * BUF_L_UNIT;
		WRITEL((aligned_risc_phy_buf - fw_phybuf) >> 11,
		       S3C_FIMV_SA_MV_ADR);
		aligned_risc_phy_buf += 136 * BUF_L_UNIT;
		WRITEL((aligned_risc_phy_buf - fw_phybuf) >> 11,
		       S3C_FIMV_OT_LINE_ADR);
		aligned_risc_phy_buf += 32 * BUF_L_UNIT;
		WRITEL((aligned_risc_phy_buf - fw_phybuf) >> 11,
		       S3C_FIMV_SP_ADR);
		aligned_risc_phy_buf += 68 * BUF_L_UNIT;
		break;

	case VC1_DEC:
	case VC1RCV_DEC:
		WRITEL((aligned_risc_phy_buf - fw_phybuf) >> 11,
		       S3C_FIMV_NB_DCAC_ADR);
		aligned_risc_phy_buf += 16 * BUF_L_UNIT;
		WRITEL((aligned_risc_phy_buf - fw_phybuf) >> 11,
		       S3C_FIMV_UP_NB_MV_ADR);
		aligned_risc_phy_buf += 68 * BUF_L_UNIT;
		WRITEL((aligned_risc_phy_buf - fw_phybuf) >> 11,
		       S3C_FIMV_SA_MV_ADR);
		aligned_risc_phy_buf += 136 * BUF_L_UNIT;
		WRITEL((aligned_risc_phy_buf - fw_phybuf) >> 11,
		       S3C_FIMV_OT_LINE_ADR);
		aligned_risc_phy_buf += 32 * BUF_L_UNIT;
		WRITEL((aligned_risc_phy_buf - fw_phybuf) >> 11,
		       S3C_FIMV_BITPLANE3_ADR);
		aligned_risc_phy_buf += 2 * BUF_L_UNIT;
		WRITEL((aligned_risc_phy_buf - fw_phybuf) >> 11,
		       S3C_FIMV_BITPLANE2_ADR);
		aligned_risc_phy_buf += 2 * BUF_L_UNIT;
		WRITEL((aligned_risc_phy_buf - fw_phybuf) >> 11,
		       S3C_FIMV_BITPLANE1_ADR);
		aligned_risc_phy_buf += 2 * BUF_L_UNIT;
		break;

	case MPEG1_DEC:
	case MPEG2_DEC:
		break;

	case H264_ENC:
	case MPEG4_ENC:
	case H263_ENC:
		aligned_risc_phy_buf = ALIGN(risc_phy_buf, 64 * BUF_L_UNIT);
		WRITEL((aligned_risc_phy_buf - fw_phybuf) >> 11,
		       S3C_FIMV_ENC_UP_MV_ADR);
		aligned_risc_phy_buf += 64 * BUF_L_UNIT;
		WRITEL((aligned_risc_phy_buf - fw_phybuf) >> 11,
		       S3C_FIMV_ENC_COZERO_FLAG_ADR);
		aligned_risc_phy_buf += 64 * BUF_L_UNIT;
		WRITEL((aligned_risc_phy_buf - fw_phybuf) >> 11,
		       S3C_FIMV_ENC_UP_INTRA_MD_ADR);
		aligned_risc_phy_buf += 64 * BUF_L_UNIT;
		WRITEL((aligned_risc_phy_buf - fw_phybuf) >> 11,
		       S3C_FIMV_ENC_NB_DCAC_ADR);
		aligned_risc_phy_buf += 64 * BUF_L_UNIT;
		break;

	default:
		break;

	}

	mfc_debug("inst_no : %d, risc_buf_end : 0x%08x\n", inst_no,
		  aligned_risc_phy_buf);

	return MFC_RET_OK;
}

/* This function sets the MFC SFR values according to the input arguments. */
static void s3c_mfc_set_encode_init_param(int inst_no,
					  enum SSBSIP_MFC_CODEC_TYPE mfc_codec_type,
					  union s3c_mfc_args *args)
{
	unsigned int ms_size;
	struct s3c_mfc_enc_init_mpeg4_arg_t *enc_init_mpeg4_arg;
	struct s3c_mfc_enc_init_h264_arg_t *enc_init_h264_arg;

	enc_init_mpeg4_arg = (struct s3c_mfc_enc_init_mpeg4_arg_t *) args;
	enc_init_h264_arg = (struct s3c_mfc_enc_init_h264_arg_t *) args;

	mfc_debug("mfc_codec_type : %d\n", mfc_codec_type);

	/* Set image size */
	WRITEL(enc_init_mpeg4_arg->in_width, S3C_FIMV_ENC_HSIZE_PX);
	if ((mfc_codec_type == H264_ENC)
	    && (enc_init_mpeg4_arg->in_interlace_mode)) {
		WRITEL(enc_init_mpeg4_arg->in_height >> 1,
		       S3C_FIMV_ENC_VSIZE_PX);
		WRITEL(enc_init_mpeg4_arg->in_interlace_mode,
		       S3C_FIMV_ENC_PIC_STRUCT);
	} else {
		WRITEL(enc_init_mpeg4_arg->in_height, S3C_FIMV_ENC_VSIZE_PX);
		WRITEL(enc_init_mpeg4_arg->in_interlace_mode,
		       S3C_FIMV_ENC_PIC_STRUCT);
	}

	WRITEL(1, S3C_FIMV_ENC_BF_MODE_CTRL);	/* stream buf frame mode */

/* MFC fw 10/30, EVT0 */
#if defined(CONFIG_CPU_S5PV210_EVT0)
	WRITEL(1, S3C_FIMV_ENC_B_RECON_WRITE_ON);
#endif

	WRITEL(1, S3C_FIMV_ENC_SF_EPB_ON_CTRL);	/* Auto EPB insertion on, only for h264 */
	WRITEL((1 << 18) | (enc_init_mpeg4_arg->in_BframeNum << 16) |
	       enc_init_mpeg4_arg->in_gop_num, S3C_FIMV_ENC_PIC_TYPE_CTRL);

	/* Multi-slice options */
	if (enc_init_mpeg4_arg->in_MS_mode) {
		ms_size =
		    (mfc_codec_type ==
		     H263_ENC) ? 0 : enc_init_mpeg4_arg->in_MS_size;
		switch (enc_init_mpeg4_arg->in_MS_mode) {
		case 1:
			WRITEL((0 << 1) | 0x1, S3C_FIMV_ENC_MSLICE_CTRL);
			WRITEL(ms_size, S3C_FIMV_ENC_MSLICE_MB);
			break;
		case 2:
			WRITEL((1 << 1) | 0x1, S3C_FIMV_ENC_MSLICE_CTRL);
			WRITEL(ms_size, S3C_FIMV_ENC_MSLICE_BYTE);
			break;
		case 4:
			WRITEL(0x5, S3C_FIMV_ENC_MSLICE_CTRL);
			break;
		default:
			mfc_err("Invalid Multi-slice mode type\n");
			break;
		}
	} else {
		WRITEL(0, S3C_FIMV_ENC_MSLICE_CTRL);
	}

	/* Set circular intra refresh MB count */
	WRITEL(enc_init_mpeg4_arg->in_mb_refresh, S3C_FIMV_ENC_CIR_CTRL);
	WRITEL(MEM_STRUCT_LINEAR, S3C_FIMV_ENC_MAP_FOR_CUR);	/* linear mode */

	/* Set padding control */
	WRITEL((enc_init_mpeg4_arg->in_pad_ctrl_on << 31) |
	       (enc_init_mpeg4_arg->in_cr_pad_val << 16) | (enc_init_mpeg4_arg->
							    in_cb_pad_val << 8)
	       | (enc_init_mpeg4_arg->in_luma_pad_val << 0),
	       S3C_FIMV_ENC_PADDING_CTRL);
	WRITEL(0, S3C_FIMV_ENC_INT_MASK);	/* mask interrupt */

	/* Set Rate Control */
	/* for MFC fw 2010/03/08 */
	if ((mfc_codec_type == MPEG4_ENC) &&
	    (enc_init_mpeg4_arg->in_VopTimeIncreament > 0)) {
		enc_init_mpeg4_arg->in_RC_framerate =
			enc_init_mpeg4_arg->in_TimeIncreamentRes /
			enc_init_mpeg4_arg->in_VopTimeIncreament;
		/* for MFC fw 2010/04/09 */
		WRITEL(enc_init_mpeg4_arg->in_RC_framerate*SCALE_NUM,
		       S3C_FIMV_ENC_RC_FRAME_RATE);
	} else if (enc_init_mpeg4_arg->in_RC_framerate != 0) {
		/* for MFC fw 2010/04/09 */
		WRITEL(enc_init_mpeg4_arg->in_RC_framerate*SCALE_NUM,
		       S3C_FIMV_ENC_RC_FRAME_RATE);
	}

	if (enc_init_mpeg4_arg->in_RC_bitrate != 0)
		WRITEL(enc_init_mpeg4_arg->in_RC_bitrate,
		       S3C_FIMV_ENC_RC_BIT_RATE);
	WRITEL(enc_init_mpeg4_arg->in_RC_qbound, S3C_FIMV_ENC_RC_QBOUND);

	/* if in_RC_frm_enable is '1' */
	/* if (READL(S3C_FIMV_ENC_RC_CONFIG) & 0x0200) */
	if (enc_init_mpeg4_arg->in_RC_frm_enable)
		WRITEL(enc_init_mpeg4_arg->in_RC_rpara, S3C_FIMV_ENC_RC_RPARA);

	switch (mfc_codec_type) {
	case H264_ENC:
		/* if in_RC_mb_enable is '1' */
		WRITEL((enc_init_mpeg4_arg->in_RC_frm_enable << 9) |
		       (enc_init_h264_arg->in_RC_mb_enable << 8) |
		       (enc_init_mpeg4_arg->in_vop_quant & 0x3F),
		       S3C_FIMV_ENC_RC_CONFIG);

		if (READL(S3C_FIMV_ENC_RC_CONFIG) & 0x0100) {
			WRITEL((enc_init_h264_arg->in_RC_mb_dark_disable << 3) |
			       (enc_init_h264_arg->in_RC_mb_smooth_disable << 2)
			       | (enc_init_h264_arg->in_RC_mb_static_disable <<
				  1) |
			       (enc_init_h264_arg->in_RC_mb_activity_disable <<
				0), S3C_FIMV_ENC_RC_MB_CTRL);
		}
		WRITEL((enc_init_mpeg4_arg->in_profile_level),
		       S3C_FIMV_ENC_PROFILE);
		WRITEL((enc_init_h264_arg->in_transform8x8_mode),
		       S3C_FIMV_ENC_H264_TRANS_FLAG);
		WRITEL((enc_init_h264_arg->in_symbolmode & 0x1),
		       S3C_FIMV_ENC_ENTRP_MODE);
		WRITEL((enc_init_h264_arg->in_deblock_filt & 0x3),
		       S3C_FIMV_ENC_LF_CTRL);
		WRITEL((enc_init_h264_arg->in_deblock_alpha_C0 & 0x1f) * 2,
		       S3C_FIMV_ENC_ALPHA_OFF);
		WRITEL((enc_init_h264_arg->in_deblock_beta & 0x1f) * 2,
		       S3C_FIMV_ENC_BETA_OFF);

		WRITEL((enc_init_h264_arg->in_ref_num_p << 5) |
		       (enc_init_h264_arg->in_reference_num),
		       S3C_FIMV_ENC_H264_NUM_OF_REF);

		WRITEL(enc_init_h264_arg->in_md_interweight_pps,
		       S3C_FIMV_ENC_H264_MDINTER_WGT);
		WRITEL(enc_init_h264_arg->in_md_intraweight_pps,
		       S3C_FIMV_ENC_H264_MDINTRA_WGT);
		break;

	case MPEG4_ENC:
		WRITEL((enc_init_mpeg4_arg->in_RC_frm_enable << 9) |
		       (enc_init_mpeg4_arg->in_vop_quant & 0x3F),
		       S3C_FIMV_ENC_RC_CONFIG);
		WRITEL(enc_init_mpeg4_arg->in_profile_level,
		       S3C_FIMV_ENC_PROFILE);
		WRITEL(enc_init_mpeg4_arg->in_qpelME_enable,
		       S3C_FIMV_ENC_MPEG4_QUART_PXL);
		break;

	case H263_ENC:
		WRITEL((enc_init_mpeg4_arg->in_RC_frm_enable << 9) |
		       (enc_init_mpeg4_arg->in_vop_quant & 0x3F),
		       S3C_FIMV_ENC_RC_CONFIG);
		WRITEL(0x20, S3C_FIMV_ENC_PROFILE);
		break;

	default:
		mfc_err("Invalid MFC codec type\n");
	}

}

enum SSBSIP_MFC_ERROR_CODE s3c_mfc_init_hw()
{
	unsigned int dram0_base;
	unsigned int dram1_base;
	int fw_buf_size;
	unsigned int fw_version;

	mfc_debug("s3c_mfc_init_hw++\n");

	/*
	 * CHKME: jtpark
	 * fw_phybuf = align(s3c_mfc_get_fw_buf_phys_addr(), 128*BUF_L_UNIT);
	 */
	dram0_base = s3c_mfc_get_fw_buf_phys_addr();
	dram1_base = s3c_mfc_get_dpb_luma_buf_phys_addr();

	/*
	 * 0. MFC reset
	 */
	s3c_mfc_cmd_reset();

	/*
	 * 1. Set DRAM base Addr
	 */
	WRITEL(dram0_base, S3C_FIMV_MC_DRAMBASE_ADR_A);		/* Channel A, Port 0 */
#if CONFIG_VIDEO_MFC_MEM_PORT_COUNT == 1
	WRITEL(dram0_base, S3C_FIMV_MC_DRAMBASE_ADR_B);		/* Channel B, Port 0 */
#else
	WRITEL(dram1_base, S3C_FIMV_MC_DRAMBASE_ADR_B);		/* Channel B, Port 1 */
#endif
	WRITEL(0, S3C_FIMV_MC_RS_IBASE);			/* FW location sel : 0->A, 1->B */
#if CONFIG_VIDEO_MFC_MEM_PORT_COUNT == 1
	WRITEL(0, S3C_FIMV_NUM_MASTER);				/* 0 -> 1 Master */
#else
	WRITEL(1, S3C_FIMV_NUM_MASTER);				/* 1 -> 2 Master */
#endif

	/*
	 * 2. Initialize registers of stream I/F for decoder
	 */
	WRITEL(0xffff, S3C_FIMV_SI_CH1_INST_ID);
	WRITEL(0xffff, S3C_FIMV_SI_CH2_INST_ID);

	WRITEL(0, S3C_FIMV_RISC2HOST_CMD);
	WRITEL(0, S3C_FIMV_HOST2RISC_CMD);

	/*
	 * 3. Release reset signal to the RISC.
	 */
	WRITEL(0x3ff, S3C_FIMV_SW_RESET);

	if (s3c_mfc_wait_for_done(R2H_CMD_FW_STATUS_RET) == 0) {
		mfc_err("MFC_RET_FW_LOAD_FAIL\n");
		return MFC_RET_FW_LOAD_FAIL;
	}

	/*
	 * 4. Initialize firmware
	 */

	/* for MFC fw 9/30 */
	fw_buf_size = MFC_FW_SYSTEM_SIZE;
	s3c_mfc_cmd_host2risc(H2R_CMD_SYS_INIT, fw_buf_size, 0);

	if (s3c_mfc_wait_for_done(R2H_CMD_SYS_INIT_RET) == 0) {
		mfc_err("R2H_CMD_SYS_INIT_RET FAIL\n");
		return MFC_RET_FW_INIT_FAIL;
	}

	fw_version = READL(S3C_FIMV_FW_VERSION);
	mfc_info("MFC FW version : %02xyy, %02xmm, %02xdd\n",
		 (fw_version >> 16) & 0xff, (fw_version >> 8) & 0xff,
		 (fw_version) & 0xff);

	mfc_debug("FW_PHY_BUFFER : 0x%08x\n",
		  READL(S3C_FIMV_MC_DRAMBASE_ADR_A));
	mfc_debug("DRAM1_START_BUFFER : 0x%08x\n",
		  READL(S3C_FIMV_MC_DRAMBASE_ADR_B));
	mfc_debug("s3c_mfc_init_hw--\n");

	return MFC_RET_OK;
}

static unsigned int s3c_mfc_get_codec_arg(enum SSBSIP_MFC_CODEC_TYPE codec_type)
{
	unsigned int codec_no = 99;

	switch (codec_type) {
	case H264_DEC:
		codec_no = 0;
		break;
	case VC1_DEC:
		codec_no = 1;
		break;
	case MPEG4_DEC:
	case XVID_DEC:
		codec_no = 2;
		break;
	case MPEG1_DEC:
	case MPEG2_DEC:
		codec_no = 3;
		break;
	case H263_DEC:
		codec_no = 4;
		break;
	case VC1RCV_DEC:
		codec_no = 5;
		break;
	case FIMV1_DEC:
		codec_no = 6;
		break;
	case FIMV2_DEC:
		codec_no = 7;
		break;
	case FIMV3_DEC:
		codec_no = 8;
		break;
	case FIMV4_DEC:
		codec_no = 9;
		break;
	case H264_ENC:
		codec_no = 16;
		break;
	case MPEG4_ENC:
		codec_no = 17;
		break;
	case H263_ENC:
		codec_no = 18;
		break;
	default:
		break;
	}

	return codec_no;
}

static int s3c_mfc_get_inst_no(enum SSBSIP_MFC_CODEC_TYPE codec_type,
			       unsigned int crc_enable)
{
	unsigned int codec_no;
	int inst_no;

	codec_no = (unsigned int)s3c_mfc_get_codec_arg(codec_type);

	s3c_mfc_cmd_host2risc(H2R_CMD_OPEN_INSTANCE, codec_no, crc_enable);

	if (s3c_mfc_wait_for_done(R2H_CMD_OPEN_INSTANCE_RET) == 0) {
		mfc_err("R2H_CMD_OPEN_INSTANCE_RET FAIL\n");
		return MFC_RET_OPEN_FAIL;
	}

	inst_no = READL(S3C_FIMV_RISC2HOST_ARG1);

	mfc_debug("INSTANCE NO : %d, CODEC_TYPE : %d --\n", inst_no, codec_no);

	if (inst_no >= MFC_MAX_INSTANCE_NUM)
		return -1;
	else
		return inst_no;	/* Get the instance number */
}

enum SSBSIP_MFC_ERROR_CODE s3c_mfc_return_inst_no(int inst_no,
					     enum SSBSIP_MFC_CODEC_TYPE codec_type)
{
	unsigned int codec_no;

	codec_no = (unsigned int)s3c_mfc_get_codec_arg(codec_type);

	s3c_mfc_cmd_host2risc(H2R_CMD_CLOSE_INSTANCE, inst_no, 0);

	if (s3c_mfc_wait_for_done(R2H_CMD_CLOSE_INSTANCE_RET) == 0) {
		mfc_err("R2H_CMD_CLOSE_INSTANCE_RET FAIL\n");
		return MFC_RET_CLOSE_FAIL;
	}

	mfc_debug("INSTANCE NO : %d, CODEC_TYPE : %d --\n", inst_no, codec_no);

	return MFC_RET_OK;

}

enum SSBSIP_MFC_ERROR_CODE s3c_mfc_init_encode(struct s3c_mfc_inst_ctx *mfc_ctx,
					  union s3c_mfc_args *args)
{
	struct s3c_mfc_enc_init_mpeg4_arg_t *enc_init_mpeg4_arg;

	enc_init_mpeg4_arg = (struct s3c_mfc_enc_init_mpeg4_arg_t *) args;

	mfc_debug("++\n");
	mfc_ctx->MfcCodecType = enc_init_mpeg4_arg->in_codec_type;
	mfc_ctx->img_width = (unsigned int)enc_init_mpeg4_arg->in_width;
	mfc_ctx->img_height = (unsigned int)enc_init_mpeg4_arg->in_height;
	mfc_ctx->interlace_mode = enc_init_mpeg4_arg->in_interlace_mode;

	/*
	 * OPEN CHANNEL
	 *      - set open instance using codec_type
	 *      - get the instance no
	 */

	s3c_mfc_init_count = s3c_mfc_get_mem_inst_no(CONTEXT);
	mfc_ctx->InstNo = s3c_mfc_get_inst_no(mfc_ctx->MfcCodecType, 0);
	if (mfc_ctx->InstNo < 0) {
		kfree(mfc_ctx);
		mfc_err("MFC_RET_INST_NUM_EXCEEDED_FAIL\n");
		return MFC_RET_INST_NUM_EXCEEDED_FAIL;
	}

	/*
	 * INIT CODEC
	 *  - set init parameter
	 *  - set init sequence done command
	 *  - set input risc buffer
	 */

	s3c_mfc_set_encode_init_param(mfc_ctx->InstNo, mfc_ctx->MfcCodecType,
				      args);

	s3c_mfc_set_shared_mem_buffer(mfc_ctx);

	s3c_mfc_set_risc_buffer(mfc_ctx->MfcCodecType, mfc_ctx->InstNo);

	mfc_debug("--\n");

	return MFC_RET_OK;
}

enum SSBSIP_MFC_ERROR_CODE s3c_mfc_encode_header(struct s3c_mfc_inst_ctx *mfc_ctx,
					    union s3c_mfc_args *args)
{
	struct s3c_mfc_enc_init_mpeg4_arg_t *init_arg;
	unsigned int fw_phybuf;

	init_arg = (struct s3c_mfc_enc_init_mpeg4_arg_t *) args;

	mfc_debug("++ strm start addr : 0x%08x  \r\n",
		  init_arg->out_p_addr.strm_ref_y);

	fw_phybuf = ALIGN(s3c_mfc_get_fw_buf_phys_addr(), 128 * BUF_L_UNIT);

	/* MFC fw 9/30, set the QP for P/B */
	if (mfc_ctx->MfcCodecType == H263_ENC)
		init_arg->in_vop_quant_b = 0;
	WRITEL_SHARED_MEM((init_arg->
			   in_vop_quant_p) | (init_arg->in_vop_quant_b << 6),
			  mfc_ctx->shared_mem_vir_addr + 0x70);

	/* MFC fw 11/10 */
	if (mfc_ctx->MfcCodecType == H264_ENC) {
		WRITEL_SHARED_MEM((mfc_ctx->vui_enable << 15) |
				  (mfc_ctx->hier_p_enable << 4) |
				  (mfc_ctx->frameSkipEnable << 1),
				  mfc_ctx->shared_mem_vir_addr + 0x28);
		if (mfc_ctx->vui_enable)
			WRITEL_SHARED_MEM((mfc_ctx->
					   vui_info.aspect_ratio_idc & 0xff),
					  mfc_ctx->shared_mem_vir_addr + 0x74);
		/* MFC fw 2010/04/09 */
		if (mfc_ctx->hier_p_enable)
			WRITEL_SHARED_MEM((mfc_ctx->hier_p_qp.t3_frame_qp << 12) |
					  (mfc_ctx->hier_p_qp.t2_frame_qp << 6) |
					  (mfc_ctx->hier_p_qp.t0_frame_qp),
					  mfc_ctx->shared_mem_vir_addr + 0xe0);
	} else
		WRITEL_SHARED_MEM((mfc_ctx->frameSkipEnable << 1),
				  mfc_ctx->shared_mem_vir_addr + 0x28);

	/* MFC fw 10/30, set vop_time_resolution, frame_delta */
	if (mfc_ctx->MfcCodecType == MPEG4_ENC)
		WRITEL_SHARED_MEM((1 << 31) |
				  (init_arg->in_TimeIncreamentRes << 16) |
				  (init_arg->in_VopTimeIncreament),
				  mfc_ctx->shared_mem_vir_addr + 0x30);

	if ((mfc_ctx->MfcCodecType == H264_ENC)
	    && (mfc_ctx->h264_i_period_enable)) {
		WRITEL_SHARED_MEM((1 << 16) | (mfc_ctx->h264_i_period),
				  mfc_ctx->shared_mem_vir_addr + 0x9c);
	}

	/* Set stream buffer addr */
	WRITEL((init_arg->out_p_addr.strm_ref_y - fw_phybuf) >> 11,
	       S3C_FIMV_ENC_SI_CH1_SB_U_ADR);
	WRITEL((init_arg->out_p_addr.strm_ref_y - fw_phybuf) >> 11,
	       S3C_FIMV_ENC_SI_CH1_SB_L_ADR);
	WRITEL(STREAM_BUF_SIZE, S3C_FIMV_ENC_SI_CH1_SB_SIZE);

	WRITEL(1, S3C_FIMV_ENC_STR_BF_U_EMPTY);
	WRITEL(1, S3C_FIMV_ENC_STR_BF_L_EMPTY);

	/* buf reset command if stream buffer is frame mode */
	WRITEL(0x1 << 1, S3C_FIMV_ENC_SF_BUF_CTRL);

	/* for MFC fw 8/7 */
	WRITEL((SEQ_HEADER << 16 & 0x70000) | (mfc_ctx->InstNo),
	       S3C_FIMV_SI_CH1_INST_ID);

	if (s3c_mfc_wait_for_done(R2H_CMD_SEQ_DONE_RET) == 0) {
		mfc_err("MFC_RET_ENC_HEADER_FAIL\n");
		return MFC_RET_ENC_HEADER_FAIL;
	}

	init_arg->out_header_size = READL(S3C_FIMV_ENC_SI_STRM_SIZE);

	mfc_debug("-- encoded header size(%d)\r\n", init_arg->out_header_size);

	return MFC_RET_OK;

}

static enum SSBSIP_MFC_ERROR_CODE s3c_mfc_encode_one_frame(struct s3c_mfc_inst_ctx *
						      mfc_ctx,
						      struct s3c_mfc_enc_exe_arg *
						      args)
{
	struct s3c_mfc_enc_exe_arg *enc_arg;
	unsigned int strm_base, cur_frm_base;
	int interrupt_flag;
	unsigned int time_increment_res;
	unsigned int fw_phybuf;

	enc_arg = (struct s3c_mfc_enc_exe_arg *) args;

	mfc_debug
	    ("++ enc_arg->in_strm_st : 0x%08x enc_arg->in_strm_end :0x%08x \r\n",
	     enc_arg->in_strm_st, enc_arg->in_strm_end);
	mfc_debug
	    ("enc_arg->in_Y_addr : 0x%08x enc_arg->in_CbCr_addr :0x%08x \r\n",
	     enc_arg->in_Y_addr, enc_arg->in_CbCr_addr);

	/*
	 * CHKME: jtpark
	 * fw_phybuf = align(s3c_mfc_get_fw_buf_phys_addr(), 128*BUF_L_UNIT);
	 */
	strm_base = s3c_mfc_get_fw_buf_phys_addr();
#if CONFIG_VIDEO_MFC_MEM_PORT_COUNT == 1
	cur_frm_base = strm_base;
#else
	cur_frm_base = s3c_mfc_get_dpb_luma_buf_phys_addr();
#endif
	fw_phybuf = ALIGN(s3c_mfc_get_fw_buf_phys_addr(), 128 * BUF_L_UNIT);
	WRITEL((mfc_ctx->shared_mem_phy_addr - fw_phybuf), S3C_FIMV_SI_CH1_HOST_WR_ADR);

	/* Set stream buffer addr */
	WRITEL((enc_arg->in_strm_st - strm_base) >> 11,
	       S3C_FIMV_ENC_SI_CH1_SB_U_ADR);
	WRITEL((enc_arg->in_strm_st - strm_base) >> 11,
	       S3C_FIMV_ENC_SI_CH1_SB_L_ADR);
	WRITEL(STREAM_BUF_SIZE, S3C_FIMV_ENC_SI_CH1_SB_SIZE);

	/* force I frame or Not-coded frame */
	if (mfc_ctx->forceSetFrameType == I_FRAME)
		WRITEL(1, S3C_FIMV_ENC_SI_CH1_FRAME_INS);
	else if (mfc_ctx->forceSetFrameType == NOT_CODED)
		WRITEL(1 << 1, S3C_FIMV_ENC_SI_CH1_FRAME_INS);

	if (mfc_ctx->dynamic_framerate != 0) {
		WRITEL_SHARED_MEM((1 << 1), mfc_ctx->shared_mem_vir_addr + 0x2c);
		/* MFC fw 2010/04/09 */
		WRITEL_SHARED_MEM(mfc_ctx->dynamic_framerate*SCALE_NUM,
				  mfc_ctx->shared_mem_vir_addr + 0x94);
		if (mfc_ctx->MfcCodecType == MPEG4_ENC) {
			time_increment_res = mfc_ctx->dynamic_framerate *
						MPEG4_TIME_RES;
			WRITEL_SHARED_MEM((1 << 31) |
				  (time_increment_res << 16) |
				  (MPEG4_TIME_RES),
				  mfc_ctx->shared_mem_vir_addr + 0x30);
		}
	}

	if (mfc_ctx->dynamic_bitrate != 0) {
		WRITEL_SHARED_MEM((1 << 2), mfc_ctx->shared_mem_vir_addr + 0x2c);
		WRITEL_SHARED_MEM(mfc_ctx->dynamic_bitrate,
				  mfc_ctx->shared_mem_vir_addr + 0x90);
	}

	/* Set current frame buffer addr */
	WRITEL((enc_arg->in_Y_addr - cur_frm_base) >> 11,
	       S3C_FIMV_ENC_SI_CH1_CUR_Y_ADR);
	WRITEL((enc_arg->in_CbCr_addr - cur_frm_base) >> 11,
	       S3C_FIMV_ENC_SI_CH1_CUR_C_ADR);

	WRITEL(1, S3C_FIMV_ENC_STR_BF_U_EMPTY);
	WRITEL(1, S3C_FIMV_ENC_STR_BF_L_EMPTY);

	/* buf reset command if stream buffer is frame mode */
	WRITEL(0x1 << 1, S3C_FIMV_ENC_SF_BUF_CTRL);

	WRITEL((FRAME << 16 & 0x70000) | (mfc_ctx->InstNo),
	       S3C_FIMV_SI_CH1_INST_ID);

	interrupt_flag = s3c_mfc_wait_for_done(R2H_CMD_FRAME_DONE_RET);
	if (interrupt_flag == 0) {
		mfc_err("MFC_RET_ENC_EXE_TIME_OUT\n");
		return MFC_RET_ENC_EXE_TIME_OUT;
	}
	if ((interrupt_flag == R2H_CMD_ERR_RET) &&
	    (s3c_mfc_err_type >= MFC_ERR_START_NO) &&
	    (s3c_mfc_err_type < MFC_WARN_START_NO)) {
		mfc_err("MFC_RET_ENC_EXE_ERR\n");
		return MFC_RET_ENC_EXE_ERR;
	}

	enc_arg->out_frame_type = READL(S3C_FIMV_ENC_SI_SLICE_TYPE);
	enc_arg->out_encoded_size = READL(S3C_FIMV_ENC_SI_STRM_SIZE);

	/* MFC fw 9/30 */
	enc_arg->out_Y_addr =
	    cur_frm_base + (READL(S3C_FIMV_ENCODED_Y_ADDR) << 11);
	enc_arg->out_CbCr_addr =
	    cur_frm_base + (READL(S3C_FIMV_ENCODED_C_ADDR) << 11);

	WRITEL(0, S3C_FIMV_ENC_SI_CH1_FRAME_INS);
	mfc_ctx->forceSetFrameType = 0;

	WRITEL_SHARED_MEM(0, mfc_ctx->shared_mem_vir_addr + 0x2c);
	mfc_ctx->dynamic_framerate = 0;
	mfc_ctx->dynamic_bitrate = 0;

	mfc_debug
	    ("-- frame type(%d) encoded frame size(%d) encoded Y_addr(0x%08x)/C_addr(0x%08x)\r\n",
	     enc_arg->out_frame_type, enc_arg->out_encoded_size,
	     enc_arg->out_Y_addr, enc_arg->out_CbCr_addr);

	return MFC_RET_OK;
}

enum SSBSIP_MFC_ERROR_CODE s3c_mfc_exe_encode(struct s3c_mfc_inst_ctx *mfc_ctx,
					 union s3c_mfc_args *args)
{
	enum SSBSIP_MFC_ERROR_CODE ret;
	struct s3c_mfc_enc_exe_arg *enc_arg;

	/*
	 * 5. Encode Frame
	 */

	enc_arg = (struct s3c_mfc_enc_exe_arg *) args;
	ret = s3c_mfc_encode_one_frame(mfc_ctx, enc_arg);

	mfc_debug("--\n");

	return ret;
}

enum SSBSIP_MFC_ERROR_CODE s3c_mfc_init_decode(struct s3c_mfc_inst_ctx *mfc_ctx,
					  union s3c_mfc_args *args)
{
	struct s3c_mfc_dec_init_arg_t *init_arg;
	unsigned int start_byte_num = 0;

	mfc_debug("++\n");
	init_arg = (struct s3c_mfc_dec_init_arg_t *) args;

	/* Context setting from input param */
	mfc_ctx->MfcCodecType = init_arg->in_codec_type;
	mfc_ctx->IsPackedPB = init_arg->in_packed_PB;

	/* OPEN CHANNEL
	 *      - set open instance using codec_type
	 *      - get the instance no
	 */
	if (mfc_ctx->resoultion_status != RESOLUTION_WAIT_FRMAE_DONE) {
		s3c_mfc_init_count = s3c_mfc_get_mem_inst_no(CONTEXT);
		mfc_ctx->InstNo =
		    s3c_mfc_get_inst_no(mfc_ctx->MfcCodecType, mfc_ctx->crcEnable);
		mfc_ctx->InstNo = s3c_mfc_init_count;
		if (mfc_ctx->InstNo < 0) {
			kfree(mfc_ctx);
			mfc_err("MFC_RET_INST_NUM_EXCEEDED_FAIL\n");
			return MFC_RET_INST_NUM_EXCEEDED_FAIL;
		}
	}
	s3c_mfc_set_shared_mem_buffer(mfc_ctx);

	/*
	 * INIT CODEC
	 *      - set input stream buffer
	 *      - set sequence done command
	 *      - set input risc buffer
	 *      - set NUM_EXTRA_DPB
	 */

	s3c_mfc_set_dec_stream_buffer(mfc_ctx, init_arg->in_strm_buf,
				      start_byte_num, init_arg->in_strm_size);

	if (mfc_ctx->MfcCodecType == MPEG4_DEC)
		WRITEL(mfc_ctx->postEnable, S3C_FIMV_ENC_LF_CTRL);

	if (mfc_ctx->MfcCodecType == FIMV1_DEC) {
		WRITEL(mfc_ctx->fimv1_info.width, S3C_FIMV_SI_FIMV1_HRESOL);
		WRITEL(mfc_ctx->fimv1_info.height, S3C_FIMV_SI_FIMV1_VRESOL);
	}

	/* Set the slice interface(31th bit), display delay(30th bit + delay count) */
	if (mfc_ctx->displayDelay != 9999)
		WRITEL((mfc_ctx->
			sliceEnable << 31) | (1 << 30) | (mfc_ctx->displayDelay
							  << 16),
		       S3C_FIMV_SI_CH1_DPB_CONF_CTRL);
	else
		WRITEL((mfc_ctx->sliceEnable << 31),
		       S3C_FIMV_SI_CH1_DPB_CONF_CTRL);

	WRITEL((SEQ_HEADER << 16 & 0x70000) | (mfc_ctx->InstNo),
	       S3C_FIMV_SI_CH1_INST_ID);

	if (s3c_mfc_wait_for_done(R2H_CMD_SEQ_DONE_RET) == 0) {
		mfc_err("MFC_RET_DEC_HEADER_FAIL\n");
		return MFC_RET_DEC_HEADER_FAIL;
	}

	s3c_mfc_set_risc_buffer(mfc_ctx->MfcCodecType, mfc_ctx->InstNo);

	if (mfc_ctx->MfcCodecType == FIMV1_DEC) {
		mfc_ctx->img_width = READL(S3C_FIMV_SI_FIMV1_HRESOL);
		mfc_ctx->img_height = READL(S3C_FIMV_SI_FIMV1_VRESOL);

		init_arg->out_img_width = READL(S3C_FIMV_SI_FIMV1_HRESOL);
		init_arg->out_img_height = READL(S3C_FIMV_SI_FIMV1_VRESOL);

		init_arg->out_buf_width =
		    (READL(S3C_FIMV_SI_FIMV1_HRESOL) + 127) / 128 * 128;
		init_arg->out_buf_height =
		    (READL(S3C_FIMV_SI_FIMV1_VRESOL) + 31) / 32 * 32;

		mfc_ctx->buf_width =
		    (READL(S3C_FIMV_SI_FIMV1_HRESOL) + 127) / 128 * 128;

		mfc_ctx->buf_height =
		    (READL(S3C_FIMV_SI_FIMV1_VRESOL) + 31) / 32 * 32;
	} else {
		/* out param & context setting from header decoding result */
		mfc_ctx->img_width = READL(S3C_FIMV_SI_HRESOL);
		mfc_ctx->img_height = READL(S3C_FIMV_SI_VRESOL);

		init_arg->out_img_width = READL(S3C_FIMV_SI_HRESOL);
		init_arg->out_img_height = READL(S3C_FIMV_SI_VRESOL);

		/*
		 * In the tiled mode output of MFC, width will be the multiple of 128
		 * height will be the mupltiple of 32
		 * for 8/7 MFC fw
		 */
		init_arg->out_buf_width =
		    (READL(S3C_FIMV_SI_HRESOL) + 127) / 128 * 128;
		init_arg->out_buf_height =
		    (READL(S3C_FIMV_SI_VRESOL) + 31) / 32 * 32;
		mfc_ctx->buf_width =
		    (READL(S3C_FIMV_SI_HRESOL) + 127) / 128 * 128;
		mfc_ctx->buf_height =
		    (READL(S3C_FIMV_SI_VRESOL) + 31) / 32 * 32;
	}

	/* It should have extraDPB to protect tearing in the display */
	mfc_ctx->DPBCnt = READL(S3C_FIMV_SI_BUF_NUMBER);
	mfc_ctx->totalDPBCnt = mfc_ctx->DPBCnt + mfc_ctx->extraDPB;
	init_arg->out_dpb_cnt = mfc_ctx->totalDPBCnt;

	mfc_debug
	    ("buf_width : %d buf_height : %d out_dpb_cnt : %d mfc_ctx->DPBCnt : %d\n",
	     init_arg->out_buf_width, init_arg->out_buf_height,
	     init_arg->out_dpb_cnt, mfc_ctx->DPBCnt);
	mfc_debug("img_width : %d img_height : %d\n", init_arg->out_img_width,
		  init_arg->out_img_height);

	mfc_debug("--\n");

	return MFC_RET_OK;
}

static enum SSBSIP_MFC_ERROR_CODE s3c_mfc_decode_one_frame(struct s3c_mfc_inst_ctx *
						      mfc_ctx,
						      struct s3c_mfc_dec_exe_arg_t *
						      dec_arg,
						      int *consumed_strm_size)
{
	unsigned int frame_type;
	int start_byte_num;
	int interrupt_flag;
	int out_display_Y_addr;
	int out_display_C_addr;
	unsigned int fw_phybuf;

	mfc_debug("++ InstNo%d \r\n", mfc_ctx->InstNo);

	WRITEL(0xffffffff, S3C_FIMV_SI_CH1_RELEASE_BUF);	/* MFC fw 8/7 */

	if ((*consumed_strm_size)) {
		start_byte_num = (int)((*consumed_strm_size) -
				       (ALIGN
					(*consumed_strm_size,
					 4 * BUF_L_UNIT) - 4 * BUF_L_UNIT));
	} else {
		start_byte_num = 0;
	}

	fw_phybuf = ALIGN(s3c_mfc_get_fw_buf_phys_addr(), 128 * BUF_L_UNIT);
	WRITEL((mfc_ctx->shared_mem_phy_addr - fw_phybuf), S3C_FIMV_SI_CH1_HOST_WR_ADR);

	s3c_mfc_set_dec_stream_buffer(mfc_ctx, dec_arg->in_strm_buf,
				      start_byte_num, dec_arg->in_strm_size);

	if (mfc_ctx->endOfFrame) {
		WRITEL((LAST_FRAME << 16 & 0x70000) | (mfc_ctx->InstNo),
		       S3C_FIMV_SI_CH1_INST_ID);
		mfc_ctx->endOfFrame = 0;
	} else {
		if (mfc_ctx->resoultion_status == RESOLUTION_SET_CHANGE) {
			WRITEL((FRAME_REALLOC << 16 & 0x70000) | (mfc_ctx->InstNo),
				S3C_FIMV_SI_CH1_INST_ID);
			mfc_ctx->resoultion_status = RESOLUTION_WAIT_FRMAE_DONE;
		}
		else
			WRITEL((FRAME << 16 & 0x70000) | (mfc_ctx->InstNo),
		       		S3C_FIMV_SI_CH1_INST_ID);
	}

	interrupt_flag = s3c_mfc_wait_for_done(R2H_CMD_FRAME_DONE_RET);

	if (interrupt_flag == 0) {
		mfc_err("MFC_RET_DEC_EXE_TIME_OUT\n");
		return MFC_RET_DEC_EXE_TIME_OUT;
	}

	if ((interrupt_flag == R2H_CMD_ERR_RET) &&
	    (s3c_mfc_err_type >= MFC_ERR_START_NO) &&
	    (s3c_mfc_err_type < MFC_WARN_START_NO)) {
		mfc_err("MFC_RET_DEC_EXE_ERR\n");
		return MFC_RET_DEC_EXE_ERR;
	}

	if ((READL(S3C_FIMV_SI_DISPLAY_STATUS) & 0x3) == DECODING_EMPTY) {
		dec_arg->out_display_status = 0;
	} else if ((READL(S3C_FIMV_SI_DISPLAY_STATUS) & 0x3) ==
		   DECODING_DISPLAY) {
		dec_arg->out_display_status = 1;
	} else if ((READL(S3C_FIMV_SI_DISPLAY_STATUS) & 0x3) == DISPLAY_ONLY) {
		dec_arg->out_display_status = 2;
	} else
		dec_arg->out_display_status = 3;
	if ((mfc_ctx->resoultion_status == RESOLUTION_WAIT_FRMAE_DONE) &&
	     (dec_arg->out_display_status == 0)) {
		dec_arg->out_display_status = 4;
		s3c_mfc_chnage_resloution(mfc_ctx, dec_arg);
		mfc_ctx->resoultion_status = RESOLUTION_NO_CHANGE;
	}

	frame_type = READL(S3C_FIMV_SI_FRAME_TYPE);
	mfc_ctx->FrameType = (enum s3c_mfc_frame_type) (frame_type & 0x7);

	if ((READL(S3C_FIMV_SI_DISPLAY_STATUS) & 0x3) == DECODING_ONLY) {
		out_display_Y_addr = 0;
		out_display_C_addr = 0;
		mfc_debug("DECODING_ONLY frame decoded \n");
	} else {
		#if 1
		if (mfc_ctx->IsPackedPB) {
			if ((mfc_ctx->FrameType == MFC_RET_FRAME_P_FRAME) ||
			    (mfc_ctx->FrameType == MFC_RET_FRAME_I_FRAME)) {
				out_display_Y_addr = READL(S3C_FIMV_SI_DISPLAY_Y_ADR);
				out_display_C_addr = READL(S3C_FIMV_SI_DISPLAY_C_ADR);
			} else {
				out_display_Y_addr = mfc_ctx->pre_display_Y_addr;
				out_display_C_addr = mfc_ctx->pre_display_C_addr;
			}
			/* save the display addr */
			mfc_ctx->pre_display_Y_addr = READL(S3C_FIMV_SI_DISPLAY_Y_ADR);
			mfc_ctx->pre_display_C_addr = READL(S3C_FIMV_SI_DISPLAY_C_ADR);
			mfc_debug("(pre_Y_ADDR : 0x%08x  pre_C_ADDR : 0x%08x)\r\n",
				(mfc_ctx->pre_display_Y_addr << 11),
				(mfc_ctx->pre_display_C_addr << 11));
		} else {
			out_display_Y_addr = READL(S3C_FIMV_SI_DISPLAY_Y_ADR);
			out_display_C_addr = READL(S3C_FIMV_SI_DISPLAY_C_ADR);
		}
		#else
		out_display_Y_addr = READL(S3C_FIMV_SI_DISPLAY_Y_ADR);
		out_display_C_addr = READL(S3C_FIMV_SI_DISPLAY_C_ADR);
		#endif
	}

	dec_arg->out_display_Y_addr = (out_display_Y_addr << 11);
	dec_arg->out_display_C_addr = (out_display_C_addr << 11);

	dec_arg->out_pic_time_top =
	    (int)(READL_SHARED_MEM(mfc_ctx->shared_mem_vir_addr + 0x10));
	dec_arg->out_pic_time_bottom =
	    (int)(READL_SHARED_MEM(mfc_ctx->shared_mem_vir_addr + 0x14));
	dec_arg->out_consumed_byte = (int)READL(S3C_FIMV_SI_FRM_COUNT);

	if (mfc_ctx->MfcCodecType == H264_DEC) {
		dec_arg->out_crop_right_offset =
		    (int)((READL_SHARED_MEM(mfc_ctx->shared_mem_vir_addr + 0x20)) >> 16)
		    & 0xffff;
		dec_arg->out_crop_left_offset =
		    (int)(READL_SHARED_MEM(mfc_ctx->shared_mem_vir_addr + 0x20)) &
		    0xffff;
		dec_arg->out_crop_bottom_offset =
		    (int)((READL_SHARED_MEM(mfc_ctx->shared_mem_vir_addr + 0x24)) >> 16)
		    & 0xffff;
		dec_arg->out_crop_top_offset =
		    (int)(READL_SHARED_MEM(mfc_ctx->shared_mem_vir_addr + 0x24)) &
		    0xffff;
	}

	mfc_debug("(FrameType : %d)\r\n", mfc_ctx->FrameType);
	mfc_debug("(Y_ADDR : 0x%08x  C_ADDR : 0x%08x)\r\n",
		  dec_arg->out_display_Y_addr, dec_arg->out_display_C_addr);

	/* MFC fw 10/30 */
	*consumed_strm_size = READL(S3C_FIMV_SI_FRM_COUNT);
	mfc_debug("(in_strmsize : 0x%08x  consumed byte : 0x%08x)\r\n",
		  dec_arg->in_strm_size, *consumed_strm_size);

	return MFC_RET_OK;
}

static void get_byte(int buff, int *code)
{
	int byte;

	*code = (*code << 8);
	byte = (buff & 0xFF);
	*code |= byte;
}

static bool is_vcl(struct s3c_mfc_dec_exe_arg_t *dec_arg, int consumed_strm_size)
{
	volatile unsigned char *strm_buf_vir_addr;
	unsigned char *strm_buf;
	int start_code = 0xffffffff;
	int remained_size;

	strm_buf_vir_addr = phys_to_virt((unsigned long)dec_arg->in_strm_buf);
	remained_size = dec_arg->in_strm_size - consumed_strm_size;

	mfc_debug("(strm_buf_vir_addr : 0x%08x) \n",
		  (unsigned int)strm_buf_vir_addr);

	strm_buf = (unsigned char *)(strm_buf_vir_addr + consumed_strm_size);
	while (remained_size) {
		while (remained_size &&
		       ((start_code & 0xffffff) != NAL_START_CODE)) {
			get_byte((int)(READL_STREAM_BUF(strm_buf)),
				 &start_code);
			strm_buf++;
			remained_size--;
		}
		if (!remained_size)
			return false;

		mfc_debug("(start_code : 0x%08x  remained_size : 0x%08x)\r\n",
			  start_code, remained_size);

		get_byte((int)(READL_STREAM_BUF(strm_buf)), &start_code);
		strm_buf++;
		remained_size--;
		if ((start_code & 0x0f) < 0x6) {	/* In case of VCL */
			mfc_debug("(start_code : 0x%08x) \n", start_code);
			return true;
		}
		mfc_debug("(start_code : 0x%08x) \n", start_code);
	}

	return false;

}

enum SSBSIP_MFC_ERROR_CODE s3c_mfc_exe_decode(struct s3c_mfc_inst_ctx *mfc_ctx,
					 union s3c_mfc_args *args)
{
	enum SSBSIP_MFC_ERROR_CODE ret;
	struct s3c_mfc_dec_exe_arg_t *dec_arg;
	int consumed_strm_size = 0;

	/* 6. Decode Frame */
	mfc_debug("++\n");

	dec_arg = (struct s3c_mfc_dec_exe_arg_t *) args;
	ret = s3c_mfc_decode_one_frame(mfc_ctx, dec_arg, &consumed_strm_size);
	if (ret == MFC_RET_OK) {
		s3c_mfc_check_change_resolution(mfc_ctx, dec_arg);
	}	
#if 1
	/* MFC fw 10/30 */
	if (((mfc_ctx->IsPackedPB) &&
	     (mfc_ctx->FrameType == MFC_RET_FRAME_P_FRAME) &&
	     (dec_arg->in_strm_size - consumed_strm_size > STUFF_BYTE_SIZE)) ||
	/* MFC fw 11/30 */
	    ((mfc_ctx->MfcCodecType == H264_DEC) &&
	     (mfc_ctx->FrameType >= MFC_RET_FRAME_I_FRAME) &&
	     (mfc_ctx->FrameType <= MFC_RET_FRAME_B_FRAME) &&
	     (dec_arg->in_strm_size - consumed_strm_size > STUFF_BYTE_SIZE) &&
	     (is_vcl(dec_arg, consumed_strm_size)))) {
		if (ret == MFC_RET_OK) {
			mfc_debug
			("In case that two samples exist in the \
			 one stream buffer\n");
			dec_arg->in_strm_buf +=
			ALIGN(consumed_strm_size, 4 * BUF_L_UNIT) -
				4 * BUF_L_UNIT;
			dec_arg->in_strm_size -= consumed_strm_size;
			ret = s3c_mfc_decode_one_frame(mfc_ctx, dec_arg,
					     &consumed_strm_size);
		}
	}
#endif
	mfc_debug("--\n");

	return ret;
}

enum SSBSIP_MFC_ERROR_CODE s3c_mfc_deinit_hw(struct s3c_mfc_inst_ctx *mfc_ctx)
{
	return MFC_RET_OK;
}

enum SSBSIP_MFC_ERROR_CODE s3c_mfc_get_config(struct s3c_mfc_inst_ctx *mfc_ctx,
					 union s3c_mfc_args *args)
{
	struct s3c_mfc_get_config_arg_t *get_cnf_arg;
	get_cnf_arg = (struct s3c_mfc_get_config_arg_t *) args;

	switch (get_cnf_arg->in_config_param) {
	case MFC_DEC_GETCONF_CRC_DATA:
		if (mfc_ctx->MfcState != MFCINST_STATE_DEC_EXE) {
			mfc_err
			    ("MFC_DEC_GETCONF_CRC_DATA : state is invalid\n");
			return MFC_RET_STATE_INVALID;
		}
		get_cnf_arg->out_config_value[0] = READL(S3C_FIMV_CRC_LUMA0);
		get_cnf_arg->out_config_value[1] = READL(S3C_FIMV_CRC_CHROMA0);

		break;

	default:
		mfc_err("invalid config param\n");
		return MFC_RET_GET_CONF_FAIL;
	}

	return MFC_RET_OK;
}

enum SSBSIP_MFC_ERROR_CODE s3c_mfc_set_config(struct s3c_mfc_inst_ctx *mfc_ctx,
					 union s3c_mfc_args *args)
{
	struct s3c_mfc_set_config_arg_t *set_cnf_arg;
	set_cnf_arg = (struct s3c_mfc_set_config_arg_t *) args;

	switch (set_cnf_arg->in_config_param) {
	case MFC_DEC_SETCONF_POST_ENABLE:
		if (mfc_ctx->MfcState >= MFCINST_STATE_DEC_INITIALIZE) {
			mfc_err
			    ("MFC_DEC_SETCONF_POST_ENABLE : state is invalid\n");
			return MFC_RET_STATE_INVALID;
		}

		if ((set_cnf_arg->in_config_value[0] == 0)
		    || (set_cnf_arg->in_config_value[0] == 1))
			mfc_ctx->postEnable = set_cnf_arg->in_config_value[0];
		else {
			mfc_warn("POST_ENABLE should be 0 or 1\n");
			mfc_ctx->postEnable = 0;
		}
		break;

	case MFC_DEC_SETCONF_SLICE_ENABLE:
		if (mfc_ctx->MfcState >= MFCINST_STATE_DEC_INITIALIZE) {
			mfc_err
			    ("MFC_DEC_SETCONF_SLICE_ENABLE : state is invalid\n");
			return MFC_RET_STATE_INVALID;
		}

		if ((set_cnf_arg->in_config_value[0] == 0)
		    || (set_cnf_arg->in_config_value[0] == 1))
			mfc_ctx->sliceEnable = set_cnf_arg->in_config_value[0];
		else {
			mfc_warn("SLICE_ENABLE should be 0 or 1\n");
			mfc_ctx->sliceEnable = 0;
		}
		break;

	case MFC_DEC_SETCONF_EXTRA_BUFFER_NUM:
		if (mfc_ctx->MfcState >= MFCINST_STATE_DEC_INITIALIZE) {
			mfc_err
			    ("MFC_DEC_SETCONF_EXTRA_BUFFER_NUM : state is invalid\n");
			return MFC_RET_STATE_INVALID;
		}
		if ((set_cnf_arg->in_config_value[0] >= 0)
		    || (set_cnf_arg->in_config_value[0] <= MFC_MAX_EXTRA_DPB))
			mfc_ctx->extraDPB = set_cnf_arg->in_config_value[0];
		else {
			mfc_warn
			    ("EXTRA_BUFFER_NUM should be between 0 and 5...It will be set 5 by default\n");
			mfc_ctx->extraDPB = MFC_MAX_EXTRA_DPB;
		}
		break;

	case MFC_DEC_SETCONF_DISPLAY_DELAY:
		if (mfc_ctx->MfcState >= MFCINST_STATE_DEC_INITIALIZE) {
			mfc_err
			    ("MFC_DEC_SETCONF_DISPLAY_DELAY : state is invalid\n");
			return MFC_RET_STATE_INVALID;
		}
		if ((set_cnf_arg->in_config_value[0] >= 0)
		    || (set_cnf_arg->in_config_value[0] < 16))
			mfc_ctx->displayDelay = set_cnf_arg->in_config_value[0];
		else {
			mfc_warn("DISPLAY_DELAY should be between 0 and 16\n");
			mfc_ctx->displayDelay = 15;
		}

		break;

	case MFC_DEC_SETCONF_IS_LAST_FRAME:
		if (mfc_ctx->MfcState != MFCINST_STATE_DEC_EXE) {
			mfc_err
			    ("MFC_DEC_SETCONF_IS_LAST_FRAME : state is invalid\n");
			return MFC_RET_STATE_INVALID;
		}

		if ((set_cnf_arg->in_config_value[0] == 0)
		    || (set_cnf_arg->in_config_value[0] == 1))
			mfc_ctx->endOfFrame = set_cnf_arg->in_config_value[0];
		else {
			mfc_warn("IS_LAST_FRAME should be 0 or 1\n");
			mfc_ctx->endOfFrame = 0;
		}
		break;

	case MFC_DEC_SETCONF_CRC_ENABLE:
		if (mfc_ctx->MfcState >= MFCINST_STATE_DEC_INITIALIZE) {
			mfc_err
			    ("MFC_DEC_SETCONF_CRC_ENABLE : state is invalid\n");
			return MFC_RET_STATE_INVALID;
		}

		if ((set_cnf_arg->in_config_value[0] == 0)
		    || (set_cnf_arg->in_config_value[0] == 1))
			mfc_ctx->crcEnable = set_cnf_arg->in_config_value[0];
		else {
			mfc_warn("CRC ENABLE should be 0 or 1\n");
			mfc_ctx->crcEnable = 0;
		}
		break;

	case MFC_DEC_SETCONF_FIMV1_WIDTH_HEIGHT:
		if (mfc_ctx->MfcState >= MFCINST_STATE_DEC_INITIALIZE) {
			mfc_err
			    ("MFC_DEC_SETCONF_FIMV1_WIDTH_HEIGHT : state is invalid\n");
			return MFC_RET_STATE_INVALID;
		}

		mfc_ctx->fimv1_info.width = set_cnf_arg->in_config_value[0];
		mfc_ctx->fimv1_info.height = set_cnf_arg->in_config_value[1];

		break;

	case MFC_ENC_SETCONF_FRAME_TYPE:
		if (mfc_ctx->MfcState < MFCINST_STATE_ENC_INITIALIZE) {
			mfc_err
			    ("MFC_ENC_SETCONF_FRAME_TYPE : state is invalid\n");
			return MFC_RET_STATE_INVALID;
		}

		if ((set_cnf_arg->in_config_value[0] > DONT_CARE)
		    && (set_cnf_arg->in_config_value[0] <= NOT_CODED))
			mfc_ctx->forceSetFrameType =
			    set_cnf_arg->in_config_value[0];
		else {
			mfc_warn("FRAME_TYPE should be 1 and 2\n");
			mfc_ctx->forceSetFrameType = DONT_CARE;
		}
		break;

	case MFC_ENC_SETCONF_CHANGE_FRAME_RATE:
		if (mfc_ctx->MfcState < MFCINST_STATE_ENC_INITIALIZE) {
			mfc_err
			    ("MFC_ENC_SETCONF_CHANGE_FRAME_RATE : state is invalid\n");
			return MFC_RET_STATE_INVALID;
		}
		mfc_ctx->dynamic_framerate = set_cnf_arg->in_config_value[0];

		break;

	case MFC_ENC_SETCONF_CHANGE_BIT_RATE:
		if (mfc_ctx->MfcState < MFCINST_STATE_ENC_INITIALIZE) {
			mfc_err
			    ("MFC_ENC_SETCONF_CHANGE_BIT_RATE : state is invalid\n");
			return MFC_RET_STATE_INVALID;
		}
		mfc_ctx->dynamic_bitrate = set_cnf_arg->in_config_value[0];

		break;

	case MFC_ENC_SETCONF_ALLOW_FRAME_SKIP:
		if (mfc_ctx->MfcState >= MFCINST_STATE_ENC_INITIALIZE) {
			mfc_err
			    ("MFC_ENC_SETCONF_ALLOW_FRAME_SKIP : state is invalid\n");
			return MFC_RET_STATE_INVALID;
		}
		if ((set_cnf_arg->in_config_value[0] == 0)
		    || (set_cnf_arg->in_config_value[0] == 1))
			mfc_ctx->frameSkipEnable =
			    set_cnf_arg->in_config_value[0];
		else {
			mfc_warn("FRAME SKIP ENABLE should be 0 or 1\n");
			mfc_ctx->frameSkipEnable = 0;
		}
		break;

	case MFC_ENC_SETCONF_VUI_INFO:
		if (mfc_ctx->MfcState >= MFCINST_STATE_ENC_INITIALIZE) {
			mfc_err
			    ("MFC_ENC_SETCONF_VUI_INFO : state is invalid\n");
			return MFC_RET_STATE_INVALID;
		}

		mfc_ctx->vui_enable = 1;
		mfc_ctx->vui_info.aspect_ratio_idc =
		    set_cnf_arg->in_config_value[0];

		break;

	case MFC_ENC_SETCONF_I_PERIOD:
		if (mfc_ctx->MfcState >= MFCINST_STATE_ENC_INITIALIZE) {
			mfc_err
			    ("MFC_ENC_SETCONF_I_PERIOD : state is invalid\n");
			return MFC_RET_STATE_INVALID;
		}

		mfc_ctx->h264_i_period_enable = 1;
		mfc_ctx->h264_i_period = set_cnf_arg->in_config_value[0];

		break;
	/* MFC fw 2010/04/09 */
	case MFC_ENC_SETCONF_HIER_P:
		if (mfc_ctx->MfcState >= MFCINST_STATE_ENC_INITIALIZE) {
			mfc_err
			    ("MFC_ENC_SETCONF_HIER_P : state is invalid\n");
			return MFC_RET_STATE_INVALID;
		}

		mfc_ctx->hier_p_enable = 1;
		mfc_ctx->hier_p_qp.t0_frame_qp =
		    set_cnf_arg->in_config_value[0];
		mfc_ctx->hier_p_qp.t2_frame_qp =
		    set_cnf_arg->in_config_value[1];
		mfc_ctx->hier_p_qp.t3_frame_qp =
		    set_cnf_arg->in_config_value[2];
		break;

	default:
		mfc_err("invalid config param\n");
		return MFC_RET_SET_CONF_FAIL;
	}

	return MFC_RET_OK;
}

enum SSBSIP_MFC_ERROR_CODE s3c_mfc_set_sleep(void)
{
	s3c_mfc_cmd_host2risc(H2R_CMD_SLEEP, 0, 0);

	if (s3c_mfc_wait_for_done(R2H_CMD_SLEEP_RET) == 0) {
		mfc_err("R2H_CMD_SLEEP_RET FAIL\n");
		return MFC_RET_FAIL;
	}

	return MFC_RET_OK;

}

enum SSBSIP_MFC_ERROR_CODE s3c_mfc_set_wakeup(void)
{
	unsigned int dram0_base, dram1_base;

	/*
	 * CHKME: jtpark
	 * fw_phybuf = align(s3c_mfc_get_fw_buf_phys_addr(), 128*BUF_L_UNIT);
	 */
	dram0_base = s3c_mfc_get_fw_buf_phys_addr();
	dram1_base = s3c_mfc_get_dpb_luma_buf_phys_addr();

	s3c_mfc_cmd_reset();

	/*
	 * 1. Set DRAM base Addr
	 */
	WRITEL(dram0_base, S3C_FIMV_MC_DRAMBASE_ADR_A);		/* Channel A, Port 0 */
#if CONFIG_VIDEO_MFC_MEM_PORT_COUNT == 1
	WRITEL(dram0_base, S3C_FIMV_MC_DRAMBASE_ADR_B);		/* Channel B, Port 0 */
#else
	WRITEL(dram1_base, S3C_FIMV_MC_DRAMBASE_ADR_B);		/* Channel B, Port 1 */
#endif
	WRITEL(0, S3C_FIMV_MC_RS_IBASE);			/* FW location sel : 0->A, 1->B */
#if CONFIG_VIDEO_MFC_MEM_PORT_COUNT == 1
	WRITEL(0, S3C_FIMV_NUM_MASTER);				/* 0 -> 1 Master */
#else
	WRITEL(1, S3C_FIMV_NUM_MASTER);				/* 1 -> 2 Master */
#endif

	s3c_mfc_cmd_host2risc(H2R_CMD_WAKEUP, 0, 0);

	/*
	 * 2. Release reset signal to the RISC.
	 */
	WRITEL(0x3ff, S3C_FIMV_SW_RESET);

	if (s3c_mfc_wait_for_done(R2H_CMD_WAKEUP_RET) == 0) {
		mfc_err("R2H_CMD_WAKEUP_RET FAIL\n");
		return MFC_RET_FAIL;
	}

	return MFC_RET_OK;

}

void s3c_mfc_clear_int(void)
{
	WRITEL(0, S3C_FIMV_RISC_HOST_INT);
	WRITEL(0, S3C_FIMV_RISC2HOST_CMD);

}

void s3c_mfc_clear_ch_id(unsigned int int_type)
{

	if (((int_type & R2H_CMD_SEQ_DONE_RET) == R2H_CMD_SEQ_DONE_RET) ||
	    ((int_type & R2H_CMD_FRAME_DONE_RET) == R2H_CMD_FRAME_DONE_RET) ||
	    ((int_type & R2H_CMD_ERR_RET) == R2H_CMD_ERR_RET)) {

	    WRITEL(0xffff, S3C_FIMV_SI_RTN_CHID);

	}

}

void s3c_mfc_check_change_resolution(struct s3c_mfc_inst_ctx *mfc_ctx,
						struct s3c_mfc_dec_exe_arg_t *args)
{
	if ((READL(S3C_FIMV_SI_DISPLAY_STATUS)&0x3) == DECODING_ONLY ) {
		if((READL(S3C_FIMV_SI_DISPLAY_STATUS)&0x30) == (RES_INCREASED<<4) || 
		    (READL(S3C_FIMV_SI_DISPLAY_STATUS)&0x30) == (REG_DECREASED<<4)) {
			mfc_ctx->resoultion_status = RESOLUTION_SET_CHANGE;
			mfc_ctx->input_strm_buf = 
				(unsigned char*)__phys_to_virt(args->in_strm_buf);
			mfc_debug("\n**MFC Change resolution %d**\n",(READL(S3C_FIMV_SI_DISPLAY_STATUS)&0x30));
		}
	}

}
void s3c_mfc_chnage_resloution(struct s3c_mfc_inst_ctx *mfc_ctx,
						struct s3c_mfc_dec_exe_arg_t *args)
{
	struct s3c_mfc_dec_init_arg_t init_arg;
	struct s3c_mfc_frame_buf_arg_t frame_buf_size;
	struct s3c_mfc_alloc_mem_t *node, *tmp_node;
	int port_no = 0;
	enum SSBSIP_MFC_ERROR_CODE ret;

	mfc_debug("\n**s3c_mfc_chnage_resloution called**\n");

	/* Free current MFC buffer */
	for (port_no = 0; port_no < MFC_MAX_PORT_NUM; port_no++) {
		for (node = s3c_mfc_get_alloc_mem_head(port_no);
		     node != s3c_mfc_get_alloc_mem_tail(port_no);
		     node = node->next) {
			if ((node->inst_no == mfc_ctx->mem_inst_no) &&
			    (node->v_addr != mfc_ctx->input_strm_buf))
			{
				tmp_node = node;
				node = node->prev;
				mfc_ctx->port_no = port_no;
				s3c_mfc_release_alloc_mem(mfc_ctx, tmp_node);
			}
		}
	}
	s3c_mfc_merge_frag(mfc_ctx->mem_inst_no);



	mfc_debug("MFC_DEC_INIT\n");

	/* MFC decode init */
	init_arg.in_codec_type = args->in_codec_type;
	init_arg.in_strm_buf = args->in_strm_buf;
	init_arg.in_strm_size = args->in_strm_size;
	init_arg.in_packed_PB = mfc_ctx->IsPackedPB;
	init_arg.in_mapped_addr = mfc_ctx->mapped_addr;
	ret = s3c_mfc_init_decode(mfc_ctx,(union s3c_mfc_args *)&init_arg);
	if (ret < 0) {
		mfc_err("resolution change : init failure =%d\n",ret);
		return;
	}

	if (init_arg.out_dpb_cnt <= 0) {
		mfc_err("MFC out_dpb_cnt error\n");
		return;
	}
//	s3c_mfc_backup_decode_init(mfc_ctx,(union s3c_mfc_args *)&init_arg);

	/* Get frame buf size */
	frame_buf_size =
	    s3c_mfc_get_frame_buf_size(mfc_ctx,(union s3c_mfc_args *)&init_arg);

	/* Allocate MFC buffer(Y, C, MV) */
	ret = s3c_mfc_allocate_frame_buf(mfc_ctx,(union s3c_mfc_args *)&init_arg,
					frame_buf_size);
	if (ret < 0) {
		mfc_err("resolution change : alloc failure =%d\n",ret);
		return;
	}

	/* Set DPB buffer */
	ret = s3c_mfc_set_dec_frame_buffer(mfc_ctx, (union s3c_mfc_args *)&init_arg);
	if (ret < 0) {
		mfc_err("resolution change : set_dec_frame failure =%d\n",ret);
		return;
	}
//	s3c_mfc_backup_decode_frame_buf(mfc_ctx,(union s3c_mfc_args *)&init_arg);

	args->out_u_addr.chroma = init_arg.out_u_addr.chroma;
	args->out_u_addr.luma = init_arg.out_u_addr.luma;
	args->out_p_addr.chroma = init_arg.out_p_addr.chroma;
	args->out_p_addr.luma = init_arg.out_p_addr.luma;
	args->out_frame_buf_size.chroma = init_arg.out_frame_buf_size.chroma;
	args->out_frame_buf_size.luma = init_arg.out_frame_buf_size.luma;
	args->out_img_width = init_arg.out_img_width;
	args->out_img_height = init_arg.out_img_height;
	args->out_buf_width = init_arg.out_buf_width;
	args->out_buf_height = init_arg.out_buf_height;
}

