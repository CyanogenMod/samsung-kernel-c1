/*
 * linux/drivers/media/video/samsung/mfc5x/mfc_user.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Definition used commonly between application and driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MFC_USER_H
#define __MFC_USER_H __FILE__

#include "mfc_errno.h"

#define IOCTL_MFC_DEC_INIT		(0x00800001)
#define IOCTL_MFC_ENC_INIT		(0x00800002)
#define IOCTL_MFC_DEC_EXE		(0x00800003)
#define IOCTL_MFC_ENC_EXE		(0x00800004)

#define IOCTL_MFC_GET_IN_BUF		(0x00800010)
#define IOCTL_MFC_FREE_BUF		(0x00800011)
#define IOCTL_MFC_GET_PHYS_ADDR		(0x00800012)
#define IOCTL_MFC_GET_MMAP_SIZE		(0x00800014)

#define IOCTL_MFC_SET_CONFIG		(0x00800101)
#define IOCTL_MFC_GET_CONFIG		(0x00800102)

/* MFC H/W support maximum 32 extra DPB. */
#define MFC_MAX_EXTRA_DPB		(5)

enum codec_type {
	UNKNOWN,

	H264_DEC = 0x80,
	MPEG4_DEC,
	XVID_DEC,
	H263_DEC,

	FIMV1_DEC,
	FIMV2_DEC,
	FIMV3_DEC,
	FIMV4_DEC,

	VC1_DEC,		/* VC1 advaced Profile decoding  */
	VC1RCV_DEC,		/* VC1 simple/main profile decoding  */

	MPEG1_DEC,
	MPEG2_DEC,

	MPEG4_ENC = 0x100,
	H263_ENC,
	H264_ENC,
};

enum inst_type {
	DECODING = 0x0,
	ENCODING = 0x1
};

/*
enum SSBSIP_MFC_FORCE_SET_FRAME_TYPE {
	DONT_CARE = 0,
	I_FRAME = 1,
	NOT_CODED = 2,
};
*/

enum SSBSIP_MFC_DEC_CONF {
	MFC_DEC_SETCONF_POST_ENABLE = 1,
	MFC_DEC_SETCONF_EXTRA_BUFFER_NUM,
	MFC_DEC_SETCONF_DISPLAY_DELAY,
	MFC_DEC_SETCONF_IS_LAST_FRAME,
	MFC_DEC_SETCONF_SLICE_ENABLE,
	MFC_DEC_SETCONF_CRC_ENABLE,
	MFC_DEC_SETCONF_FIMV1_WIDTH_HEIGHT,
	MFC_DEC_SETCONF_FRAME_TAG,
	MFC_DEC_GETCONF_CRC_DATA,
	MFC_DEC_GETCONF_BUF_WIDTH_HEIGHT, /* MFC_DEC_GETCONF_IMG_RESOLUTION */
	MFC_DEC_GETCONF_FRAME_TAG,
};

enum SSBSIP_MFC_ENC_CONF {
	MFC_ENC_SETCONF_FRAME_TYPE = 100,
	MFC_ENC_SETCONF_CHANGE_FRAME_RATE,
	MFC_ENC_SETCONF_CHANGE_BIT_RATE,
	MFC_ENC_SETCONF_FRAME_TAG,
	MFC_ENC_SETCONF_ALLOW_FRAME_SKIP,
	MFC_ENC_SETCONF_VUI_INFO,
	MFC_ENC_SETCONF_I_PERIOD,
	MFC_ENC_GETCONF_FRAME_TAG,
	MFC_ENC_SETCONF_HIER_P,
};

struct mfc_strm_ref_buf_arg {
	unsigned int strm_ref_y;
	unsigned int mv_ref_yc;
};

struct mfc_frame_buf_arg {
	unsigned int luma;
	unsigned int chroma;
};

/* but, due to lack of memory, MFC driver use 5 as maximum */
struct mfc_enc_init_mpeg4_arg {
	enum codec_type in_codec_type;	/* [IN]  codec type */
	int in_width;		/* [IN] width of YUV420 frame to be encoded */
	int in_height;		/* [IN] height of YUV420 frame to be encoded */
	int in_profile_level;	/* [IN] profile & level */
	int in_gop_num;		/* [IN] GOP Number (interval of I-frame) */
	int in_vop_quant;	/* [IN] VOP quant */
	int in_vop_quant_p;	/* [IN] VOP quant for P frame */
	int in_vop_quant_b;	/* [IN] VOP quant for B frame */

	/* [IN]  RC enable */
	/* [IN] RC enable (0:disable, 1:frame level RC) */
	int in_RC_frm_enable;
	int in_RC_framerate;	/* [IN]  RC parameter (framerate) */
	int in_RC_bitrate;	/* [IN]  RC parameter (bitrate in kbps) */
	int in_RC_qbound;	/* [IN]  RC parameter (Q bound) */
	int in_RC_rpara;	/* [IN]  RC parameter (Reaction Coefficient) */

	/* [IN] Multi-slice mode (0:single, 1:multiple) */
	int in_MS_mode;
	/* [IN] Multi-slice size (in num. of mb or byte) */
	int in_MS_size;

	int in_mb_refresh;	/* [IN]  Macroblock refresh */
	/* [IN] interlace mode(0:progressive, 1:interlace) */
	int in_interlace_mode;

	/* [IN]  B frame number */
	int in_BframeNum;

	/* [IN] Enable (1) / Disable (0) padding with the specified values */
	int in_pad_ctrl_on;

	/* [IN] pad value if pad_ctrl_on is Enable */
	int in_luma_pad_val;
	int in_cb_pad_val;
	int in_cr_pad_val;

	unsigned int in_mapped_addr;
	struct mfc_strm_ref_buf_arg out_u_addr;
	struct mfc_strm_ref_buf_arg out_p_addr;
	struct mfc_strm_ref_buf_arg out_buf_size;
	unsigned int out_header_size;

	/*
	 * MPEG4 Only
	 */

	/* [IN] Quarter-pel MC enable (1:enabled, 0:disabled) */
	int in_qpelME_enable;
	int in_TimeIncreamentRes;	/* [IN] VOP time resolution */
	int in_VopTimeIncreament;	/* [IN] Frame delta */
};

/* FIXME:
struct mfc_enc_init_mpeg4_arg mfc_enc_init_h263_arg;
*/

struct mfc_enc_init_h264_arg {
	enum codec_type in_codec_type;	/* [IN] codec type */
	int in_width;		/* [IN] width  of YUV420 frame to be encoded */
	int in_height;		/* [IN] height of YUV420 frame to be encoded */
	int in_profile_level;	/* [IN] profile & level */
	int in_gop_num;		/* [IN] GOP Number (interval of I-frame) */
	int in_vop_quant;	/* [IN] VOP quant */
	int in_vop_quant_p;	/* [IN]  VOP quant for P frame */
	int in_vop_quant_b;	/* [IN]  VOP quant for B frame */

	/* [IN]  RC enable */
	/* [IN] RC enable (0:disable, 1:frame level RC) */
	int in_RC_frm_enable;
	int in_RC_framerate;	/* [IN]  RC parameter (framerate) */
	int in_RC_bitrate;	/* [IN]  RC parameter (bitrate in kbps) */
	int in_RC_qbound;	/* [IN]  RC parameter (Q bound) */
	int in_RC_rpara;	/* [IN]  RC parameter (Reaction Coefficient) */

	/* [IN] Multi-slice mode (0:single, 1:multiple) */
	int in_MS_mode;
	/* [IN] Multi-slice size (in num. of mb or byte) */
	int in_MS_size;

	int in_mb_refresh;	/* [IN] Macroblock refresh */
	/* [IN] interlace mode(0:progressive, 1:interlace */
	int in_interlace_mode;

	/* [IN]  B frame number */
	int in_BframeNum;

	/* [IN] Enable (1) / Disable (0) padding with the specified values */
	int in_pad_ctrl_on;
	/* [IN] pad value if pad_ctrl_on is Enable */
	int in_luma_pad_val;
	int in_cb_pad_val;
	int in_cr_pad_val;

	unsigned int in_mapped_addr;
	struct mfc_strm_ref_buf_arg out_u_addr;
	struct mfc_strm_ref_buf_arg out_p_addr;
	struct mfc_strm_ref_buf_arg out_buf_size;
	unsigned int out_header_size;

	/* H264 Only
	 */
	int in_RC_mb_enable;	/* [IN] RC enable (0:disable, 1:MB level RC) */

	/* [IN]  reference number */
	int in_reference_num;
	/* [IN]  reference number of P frame */
	int in_ref_num_p;

	/* [IN] MB level rate control dark region adaptive feature */
	int in_RC_mb_dark_disable;	/* (0:enable, 1:disable) */
	/* [IN] MB level rate control smooth region adaptive feature */
	int in_RC_mb_smooth_disable;	/* (0:enable, 1:disable) */
	/* [IN] MB level rate control static region adaptive feature */
	int in_RC_mb_static_disable;	/* (0:enable, 1:disable) */
	/* [IN] MB level rate control activity region adaptive feature */
	int in_RC_mb_activity_disable;	/* (0:enable, 1:disable) */

	/* [IN]  disable deblocking filter idc */
	int in_deblock_filt;	/* (0: enable,1: disable) */
	/* [IN]  slice alpha C0 offset of deblocking filter */
	int in_deblock_alpha_C0;
	/* [IN]  slice beta offset of deblocking filter */
	int in_deblock_beta;

	/* [IN]  ( 0 : CAVLC, 1 : CABAC ) */
	int in_symbolmode;
	/* [IN] (0: only 4x4 transform, 1: allow using 8x8 transform) */
	int in_transform8x8_mode;
	/* [IN] Inter weighted parameter for mode decision */
	int in_md_interweight_pps;
	/* [IN] Intra weighted parameter for mode decision */
	int in_md_intraweight_pps;
};

struct mfc_enc_exe_arg {
	enum codec_type in_codec_type;	/* [IN] codec type */
	unsigned int in_Y_addr;		/*[IN]In-buffer addr of Y component */
	/*[IN]In-buffer addr of CbCr component */
	unsigned int in_CbCr_addr;
	unsigned int in_Y_addr_vir;	/*[IN]In-buffer addr of Y component */
	/*[IN]In-buffer addr of CbCr component */
	unsigned int in_CbCr_addr_vir;
	/*[IN]Out-buffer start addr of encoded strm */
	unsigned int in_strm_st;
	/*[IN]Out-buffer end addr of encoded strm */
	unsigned int in_strm_end;
	unsigned int out_frame_type;	/* [OUT] frame type  */
	int out_encoded_size;	/* [OUT] Length of Encoded video stream */
	/*[OUT]Out-buffer addr of encoded Y component */
	unsigned int out_Y_addr;
	/*[OUT]Out-buffer addr of encoded CbCr component */
	unsigned int out_CbCr_addr;
};

struct mfc_dec_init_arg {
	enum codec_type in_codec_type;	/* [IN] codec type */
	int in_strm_buf;	/* [IN] the physical address of STRM_BUF */
	/* [IN] size of video stream filled in STRM_BUF */
	int in_strm_size;
	/* [IN]  Is packed PB frame or not, 1: packedPB  0: unpacked */
	int in_packed_PB;

	int out_img_width;	/* [OUT] width  of YUV420 frame */
	int out_img_height;	/* [OUT] height of YUV420 frame */
	int out_buf_width;	/* [OUT] width  of YUV420 frame */
	int out_buf_height;	/* [OUT] height of YUV420 frame */
	/* [OUT] the number of buffers which is nessary during decoding. */
	int out_dpb_cnt;

	/* [IN] the address of dpb FRAME_BUF */
	struct mfc_frame_buf_arg in_frm_buf;
	/* [IN] size of dpb FRAME_BUF */
	struct mfc_frame_buf_arg in_frm_size;
	unsigned int in_mapped_addr;

	struct mfc_frame_buf_arg out_u_addr;
	struct mfc_frame_buf_arg out_p_addr;
	struct mfc_frame_buf_arg out_frame_buf_size;

	/* FIXME: are they need? */
	int out_crop_right_offset;
	int out_crop_left_offset;
	int out_crop_bottom_offset;
	int out_crop_top_offset;
};

struct mfc_dec_exe_arg {
	enum codec_type in_codec_type;	/* [IN]  codec type */
	int in_strm_buf;	/* [IN]  the physical address of STRM_BUF */
	/* [IN]  Size of video stream filled in STRM_BUF */
	int in_strm_size;
	/* [IN] the address of dpb FRAME_BUF */
	struct mfc_frame_buf_arg in_frm_buf;
	/* [IN] size of dpb FRAME_BUF */
	struct mfc_frame_buf_arg in_frm_size;
	/* [OUT]  the physical address of display buf */
	int out_display_Y_addr;
	/* [OUT]  the physical address of display buf */
	int out_display_C_addr;
	int out_display_status;
	int out_pic_time_top;
	int out_pic_time_bottom;
	int out_consumed_byte;
	int out_crop_right_offset;
	int out_crop_left_offset;
	int out_crop_bottom_offset;
	int out_crop_top_offset;

	/* in new driver, each buffer offset must be return to the user */
	int out_y_offset;
	int out_c_offset;

#ifdef CONFIG_S5P_VMEM
	unsigned int out_y_cookie;
	unsigned int out_c_cookie;
#endif
};

struct mfc_get_config_arg {
	int in_config_param;	/* [IN] Configurable parameter type */

	/* [IN] Values to get for the configurable parameter. */
	int out_config_value[4];
	/* Maximum two integer values can be obtained; */
};

struct mfc_set_config_arg {
	int in_config_param;	/* [IN] Configurable parameter type */

	/* [IN]  Values to be set for the configurable parameter. */
	int in_config_value[3];
	/* Maximum two integer values can be set. */
};

struct mfc_get_phys_addr_arg {
	unsigned int u_addr;
	unsigned int p_addr;
};

struct mfc_buf_alloc_arg {
	enum inst_type type;
	int size;
	/*
	unsigned int mapped;
	*/
	unsigned int align;

	unsigned int addr;
	unsigned int user;
	unsigned int phys;
#ifdef CONFIG_S5P_VMEM
	unsigned int cookie;
#endif
};

struct mfc_buf_free_arg {
	unsigned int addr;
};


/* RMVME */
enum MFC_DEC_ENC_TYPE {
	DECODER = 0x0,
	ENCODER = 0x1
};

struct mfc_mem_alloc_arg {
	enum MFC_DEC_ENC_TYPE dec_enc_type;
	int buff_size;
	unsigned int mapped_addr;
	unsigned int out_addr;
#ifdef CONFIG_S5P_VMEM
	unsigned int cookie;
#endif
};

struct mfc_mem_free_arg {
	unsigned int u_addr;
};
/* RMVME */

union mfc_args {
	struct mfc_enc_init_mpeg4_arg enc_init_mpeg4;
	struct mfc_enc_init_mpeg4_arg enc_init_h263;
	struct mfc_enc_init_h264_arg enc_init_h264;
	struct mfc_enc_exe_arg enc_exe;

	struct mfc_dec_init_arg dec_init;
	struct mfc_dec_exe_arg dec_exe;

	struct mfc_get_config_arg get_config;
	struct mfc_set_config_arg set_config;

	struct mfc_buf_alloc_arg buf_alloc;
	struct mfc_buf_free_arg buf_free;
	struct mfc_get_phys_addr_arg get_phys_addr;

	/* RMVME */
	struct mfc_mem_alloc_arg mem_alloc;
	struct mfc_mem_free_arg mem_free;
	/* RMVME */
};

struct mfc_common_args {
	enum mfc_ret_code ret_code;	/* [OUT] error code */
	union mfc_args args;
};

struct mfc_enc_vui_info {
	int aspect_ratio_idc;
};

struct mfc_dec_fimv1_info {
	int width;
	int height;
};

struct mfc_enc_hier_p_qp {
	int t0_frame_qp;
	int t2_frame_qp;
	int t3_frame_qp;
};

#define ENC_PROFILE_LEVEL(profile, level)      ((profile) | ((level) << 8))
#define ENC_RC_QBOUND(min_qp, max_qp)          ((min_qp) | ((max_qp) << 8))

#endif /* __MFC_USER_H */
