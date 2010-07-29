/* linux/drivers/media/video/samsung/mfc50/s3c_mfc_common.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Memory related function
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/mm.h>

#include "s3c_mfc_buffer_manager.h"
#include "s3c_mfc_common.h"
#include "s3c_mfc_interface.h"
#include "s3c_mfc_memory.h"
#include "s3c_mfc_logmsg.h"
#include "s3c_mfc_errorno.h"

static int s3c_mfc_mem_inst_no[MFC_MAX_INSTANCE_NUM];
static int s3c_mfc_context_inst_no[MFC_MAX_INSTANCE_NUM];

/* Allocate buffers for decoder */
enum SSBSIP_MFC_ERROR_CODE s3c_mfc_allocate_frame_buf(struct s3c_mfc_inst_ctx *mfc_ctx,
						 union s3c_mfc_args *args,
						 struct s3c_mfc_frame_buf_arg_t
						 buf_size)
{
	struct s3c_mfc_dec_init_arg_t *init_arg;
	union s3c_mfc_args local_param;
	unsigned int luma_size, chroma_size;
	enum SSBSIP_MFC_ERROR_CODE ret_code = MFC_RET_OK;

	init_arg = (struct s3c_mfc_dec_init_arg_t *) args;
	luma_size = ALIGN(buf_size.luma, 2 * BUF_L_UNIT) * mfc_ctx->totalDPBCnt;
	chroma_size =
	    ALIGN(buf_size.chroma, 2 * BUF_L_UNIT) * mfc_ctx->totalDPBCnt;

	/*
	 * Allocate chroma buf
	 */
	init_arg->out_frame_buf_size.chroma = chroma_size;

	memset(&local_param, 0, sizeof(local_param));
	local_param.mem_alloc.buff_size = ALIGN(chroma_size, 2 * BUF_S_UNIT);
	local_param.mem_alloc.mapped_addr = init_arg->in_mapped_addr;
	mfc_ctx->mapped_addr = init_arg->in_mapped_addr;

	mfc_ctx->port_no = 0;
	ret_code = s3c_mfc_get_virt_addr(mfc_ctx, &(local_param));
	if (ret_code < 0)
		return ret_code;
	init_arg->out_u_addr.chroma = local_param.mem_alloc.out_addr;

	memset(&local_param, 0, sizeof(local_param));
	local_param.get_phys_addr.u_addr = init_arg->out_u_addr.chroma;

	ret_code = s3c_mfc_get_phys_addr(mfc_ctx, &local_param);
	if (ret_code < 0)
		return ret_code;

	init_arg->out_p_addr.chroma = local_param.get_phys_addr.p_addr;

	/*
	 * Allocate luma buf & (Mv in case of H264) buf
	 */
	init_arg->out_frame_buf_size.luma = luma_size;

	/* It chould be checked in related to luma cases */
	memset(&local_param, 0, sizeof(local_param));
	local_param.mem_alloc.buff_size = ALIGN(luma_size, 2 * BUF_S_UNIT);
	local_param.mem_alloc.mapped_addr = init_arg->in_mapped_addr;

#if CONFIG_VIDEO_MFC_MEM_PORT_COUNT == 1
	mfc_ctx->port_no = 0;
#else
	mfc_ctx->port_no = 1;
#endif
	ret_code = s3c_mfc_get_virt_addr(mfc_ctx, &(local_param));
	if (ret_code < 0)
		return ret_code;
	init_arg->out_u_addr.luma = local_param.mem_alloc.out_addr;

	memset(&local_param, 0, sizeof(local_param));
	local_param.get_phys_addr.u_addr = init_arg->out_u_addr.luma;

	ret_code = s3c_mfc_get_phys_addr(mfc_ctx, &(local_param));
	if (ret_code < 0)
		return ret_code;

	init_arg->out_p_addr.luma = local_param.get_phys_addr.p_addr;

	return ret_code;
}

struct s3c_mfc_frame_buf_arg_t s3c_mfc_get_frame_buf_size(struct s3c_mfc_inst_ctx *mfc_ctx,
						   union s3c_mfc_args *args)
{
	unsigned int luma_plane_sz, chroma_plane_sz, mv_plane_sz;
	struct s3c_mfc_frame_buf_arg_t buf_size;
	struct s3c_mfc_dec_init_arg_t *init_arg;

	init_arg = (struct s3c_mfc_dec_init_arg_t *) args;

	/* frameBufSize is sizes for LumaPlane+MvPlane, ChromaPlane */
	luma_plane_sz = ALIGN(ALIGN(init_arg->out_img_width, 4 * BUF_S_UNIT)
			      * ALIGN(init_arg->out_img_height, BUF_S_UNIT),
			      8 * BUF_L_UNIT);
	chroma_plane_sz = ALIGN(ALIGN(init_arg->out_img_width, 4 * BUF_S_UNIT)
				* ALIGN(init_arg->out_img_height / 2,
					BUF_S_UNIT), 8 * BUF_L_UNIT);
	buf_size.luma = luma_plane_sz;
	buf_size.chroma = chroma_plane_sz;

	if (mfc_ctx->MfcCodecType == H264_DEC) {
		mv_plane_sz =
		    ALIGN(ALIGN(init_arg->out_img_width, 4 * BUF_S_UNIT)
			  * ALIGN(init_arg->out_img_height / 4, BUF_S_UNIT),
			  8 * BUF_L_UNIT);
		buf_size.luma += mv_plane_sz;
	}

	return buf_size;
}

/* Allocate buffers for encoder */
enum SSBSIP_MFC_ERROR_CODE s3c_mfc_allocate_stream_ref_buf(struct s3c_mfc_inst_ctx *
						      mfc_ctx,
						      union s3c_mfc_args *args)
{
	struct s3c_mfc_enc_init_mpeg4_arg_t *init_arg;
	union s3c_mfc_args local_param;
	unsigned int buf_width, buf_height;
	enum SSBSIP_MFC_ERROR_CODE ret_code = MFC_RET_OK;

	init_arg = (struct s3c_mfc_enc_init_mpeg4_arg_t *) args;

	/*
	 * Allocate stream & ref Y0, Y2 buf
	 */
	buf_width = (mfc_ctx->img_width + 15) / 16 * 16;
	buf_height = (mfc_ctx->img_height + 31) / 32 * 32;

/* MFC fw 10/30, EVT0 */
#if defined(CONFIG_CPU_S5PV210_EVT0)
	init_arg->out_buf_size.strm_ref_y =
	    STREAM_BUF_SIZE + ALIGN(buf_width * buf_height,
				    64 * BUF_L_UNIT) * 3 +
	    ALIGN(buf_width * buf_height / 2, 64 * BUF_L_UNIT);
#else
	init_arg->out_buf_size.strm_ref_y =
	    STREAM_BUF_SIZE + ALIGN(buf_width * buf_height,
				    64 * BUF_L_UNIT) * 2;
#endif

	memset(&local_param, 0, sizeof(local_param));
	local_param.mem_alloc.buff_size =
	    ALIGN(init_arg->out_buf_size.strm_ref_y, 2 * BUF_S_UNIT);
	local_param.mem_alloc.mapped_addr = init_arg->in_mapped_addr;

	mfc_ctx->port_no = 0;
	ret_code = s3c_mfc_get_virt_addr(mfc_ctx, &(local_param));
	if (ret_code < 0)
		return ret_code;
	init_arg->out_u_addr.strm_ref_y = local_param.mem_alloc.out_addr;

	memset(&local_param, 0, sizeof(local_param));
	local_param.get_phys_addr.u_addr = init_arg->out_u_addr.strm_ref_y;

	ret_code = s3c_mfc_get_phys_addr(mfc_ctx, &local_param);
	if (ret_code < 0)
		return ret_code;

	init_arg->out_p_addr.strm_ref_y = local_param.get_phys_addr.p_addr;

	/*
	 * Allocate ref C0,C2,YC1,YC3 & MV buf
	 */
	init_arg->out_buf_size.mv_ref_yc =
	    ENC_UP_INTRA_PRED_SIZE + ALIGN(buf_width * buf_height,
					   64 * BUF_L_UNIT) * 2 +
	    ALIGN(buf_width * buf_height / 2, 64 * BUF_L_UNIT) * 4;

	memset(&local_param, 0, sizeof(local_param));
	/* In IOCTL_MFC_GET_IN_BUF(),
	 * Cur Y/C buf start addr should be 64KB aligned
	 */
	local_param.mem_alloc.buff_size =
	    ALIGN(init_arg->out_buf_size.mv_ref_yc, 64 * BUF_L_UNIT);
	local_param.mem_alloc.mapped_addr = init_arg->in_mapped_addr;

#if CONFIG_VIDEO_MFC_MEM_PORT_COUNT == 1
	mfc_ctx->port_no = 0;
#else
	mfc_ctx->port_no = 1;
#endif
	ret_code = s3c_mfc_get_virt_addr(mfc_ctx, &(local_param));
	if (ret_code < 0)
		return ret_code;
	init_arg->out_u_addr.mv_ref_yc = local_param.mem_alloc.out_addr;

	memset(&local_param, 0, sizeof(local_param));
	local_param.get_phys_addr.u_addr = init_arg->out_u_addr.mv_ref_yc;

	ret_code = s3c_mfc_get_phys_addr(mfc_ctx, &(local_param));
	if (ret_code < 0)
		return ret_code;

	init_arg->out_p_addr.mv_ref_yc = local_param.get_phys_addr.p_addr;

	return ret_code;

}

void s3c_mfc_init_mem_inst_no(void)
{
	memset(&s3c_mfc_mem_inst_no, 0x00, sizeof(s3c_mfc_mem_inst_no));
	memset(&s3c_mfc_context_inst_no, 0x00, sizeof(s3c_mfc_context_inst_no));
}

int s3c_mfc_get_mem_inst_no(enum s3c_mfc_inst_no_type type)
{
	unsigned int i;

	if (type == MEMORY) {
		for (i = 0; i < MFC_MAX_INSTANCE_NUM; i++)
			if (s3c_mfc_mem_inst_no[i] == 0) {
				s3c_mfc_mem_inst_no[i] = 1;
				return i;
			}
	} else {
		for (i = 0; i < MFC_MAX_INSTANCE_NUM; i++)
			if (s3c_mfc_context_inst_no[i] == 0) {
				s3c_mfc_context_inst_no[i] = 1;
				return i;
			}
	}

	return -1;
}

void s3c_mfc_return_mem_inst_no(int inst_no)
{
	if ((inst_no >= 0) && (inst_no < MFC_MAX_INSTANCE_NUM)) {
		s3c_mfc_mem_inst_no[inst_no] = 0;
		s3c_mfc_context_inst_no[inst_no] = 0;
	}
}

bool s3c_mfc_is_running(void)
{
	unsigned int i;
	bool ret = false;

	for (i = 0; i < MFC_MAX_INSTANCE_NUM; i++) {
		if (s3c_mfc_mem_inst_no[i] == 1)
			ret = true;
	}

	return ret;
}

int s3c_mfc_set_state(struct s3c_mfc_inst_ctx *ctx, enum s3c_mfc_inst_state state)
{

	if (ctx->MfcState > state)
		return -1;

	ctx->MfcState = state;
	return 0;

}
