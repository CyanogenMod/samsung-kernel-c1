/* linux/drivers/media/video/samsung/mfc50/s3c_mfc_common.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Definition of memory related function
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __S3C_MFC_COMMON_H
#define __S3C_MFC_COMMON_H __FILE__

#include <mach/regs-mfc.h>

#include "s3c_mfc_interface.h"

#define BUF_L_UNIT (1024)
#define BUF_S_UNIT (32)

#define S3C_MFC_CLK_NAME	"mfc"
#define MFC_ERR_START_NO	1
#define MFC_WARN_START_NO	150

/* Aspect ratio VUI is enabled in H.264 encoding */
#define ASPECT_RATIO_VUI_ENABLE	(1<<15)
/* Sequence header is generated on both SEQ_START and the first FRAME_START */
#define SEQ_HEADER_CONTROL		(1<<3)

/* Frame skip is enabled using maximum buffer size defined by level */
#define FRAME_SKIP_ENABLE		(1<<1)

/* Frame skip is enabled using VBV_BUFFER_SIZE defined by HOST */
/* #define  FRAME_SKIP_ENABLE		(1<<2) */

/* Header extension code (HEC) is enabled */
#define HEC_ENABLE			(1<<0)

enum s3c_mfc_inst_state {
	MFCINST_STATE_NULL = 0,

	/* Instance is created */
	MFCINST_STATE_OPENED = 10,

	/* channel_set and init_codec is completed */
	MFCINST_STATE_DEC_INITIALIZE = 20,

	MFCINST_STATE_DEC_EXE = 30,
	MFCINST_STATE_DEC_EXE_DONE,

	/* Instance is initialized for encoding */
	MFCINST_STATE_ENC_INITIALIZE = 40,
	MFCINST_STATE_ENC_EXE,
	MFCINST_STATE_ENC_EXE_DONE
};

enum s3c_mfc_mem_type {
	MEM_STRUCT_LINEAR = 0,
	MEM_STRUCT_TILE_ENC = 3	/* 64x32 */
};

enum s3c_mfc_inst_no_type {
	MEMORY = 0,
	CONTEXT = 1
};

enum s3c_mfc_dec_type {
	SEQ_HEADER = 1,
	FRAME = 2,
	LAST_FRAME = 3,
	INIT_BUFFER = 4,
	FRAME_REALLOC = 5
};

enum s3c_mfc_facade_cmd {
	H2R_CMD_EMPTY = 0,
	H2R_CMD_OPEN_INSTANCE = 1,
	H2R_CMD_CLOSE_INSTANCE = 2,
	H2R_CMD_SYS_INIT = 3,
	H2R_CMD_SLEEP = 5,
	H2R_CMD_WAKEUP = 6
};

enum s3c_mfc_wait_done_type {
	R2H_CMD_EMPTY = 0,
	R2H_CMD_OPEN_INSTANCE_RET = 1,
	R2H_CMD_CLOSE_INSTANCE_RET = 2,
	R2H_CMD_SEQ_DONE_RET = 4,
	R2H_CMD_FRAME_DONE_RET = 5,
	R2H_CMD_SLICE_DONE_RET = 6,
	R2H_CMD_SYS_INIT_RET = 8,
	R2H_CMD_FW_STATUS_RET = 9,
	R2H_CMD_SLEEP_RET = 10,
	R2H_CMD_WAKEUP_RET = 11,
	R2H_CMD_INIT_BUFFERS_RET = 15,
	R2H_CMD_EDFU_INIT_RET = 16,
	R2H_CMD_ERR_RET = 32,
	R2H_CMD_NONE_RET = 0xFF
};

enum s3c_mfc_display_status {
	DECODING_ONLY = 0,
	DECODING_DISPLAY = 1,
	DISPLAY_ONLY = 2,
	DECODING_EMPTY = 3
};

enum s3c_mfc_resolution_status {
	RES_INCREASED = 1,
	REG_DECREASED = 2
};

enum s3c_mfc_resolution_chagne_status {
	RESOLUTION_NO_CHANGE = 0,
	RESOLUTION_SET_CHANGE = 1,
	RESOLUTION_WAIT_FRMAE_DONE = 2
};

/* In case of decoder */
enum s3c_mfc_frame_type {
	MFC_RET_FRAME_NOT_SET = 0,
	MFC_RET_FRAME_I_FRAME = 1,
	MFC_RET_FRAME_P_FRAME = 2,
	MFC_RET_FRAME_B_FRAME = 3,
	MFC_RET_FRAME_OTHERS = 7,
};

enum s3c_mfc_port_type {
	PORTA = 0,
	PORTB = 1
};

struct s3c_mfc_inst_ctx {
	int InstNo;
	int resoultion_status;
	unsigned char *input_strm_buf;		
	unsigned int mapped_addr;
	unsigned int DPBCnt;
	unsigned int totalDPBCnt;
	unsigned int extraDPB;
	unsigned int displayDelay;
	unsigned int postEnable;
	unsigned int sliceEnable;
	unsigned int crcEnable;
	unsigned int frameSkipEnable;
	unsigned int endOfFrame;
	unsigned int forceSetFrameType;
	unsigned int dynamic_framerate;
	unsigned int dynamic_bitrate;
	unsigned int img_width;
	unsigned int img_height;
	unsigned int buf_width;
	unsigned int buf_height;
	unsigned int dwAccess;	/* for Power Management. */
	unsigned int IsPackedPB;
	unsigned int interlace_mode;
	unsigned int h264_i_period_enable;
	unsigned int h264_i_period;
	int mem_inst_no;
	int port_no;
	enum s3c_mfc_frame_type FrameType;
	unsigned int vui_enable;
	struct s3c_mfc_enc_vui_info vui_info;
	struct s3c_mfc_dec_fimv1_info fimv1_info;
	unsigned int hier_p_enable;
	struct s3c_mfc_enc_hier_p_qp hier_p_qp;
	enum SSBSIP_MFC_CODEC_TYPE MfcCodecType;
	int pre_display_Y_addr;
	int pre_display_C_addr;
	enum s3c_mfc_inst_state MfcState;
	unsigned int shared_mem_phy_addr;
	volatile unsigned char *shared_mem_vir_addr;
};

struct s3c_mfc_ctrl {
	char clk_name[16];
	struct clk *clock;
};

struct s3c_mfc_frame_buf_arg_t s3c_mfc_get_frame_buf_size(struct s3c_mfc_inst_ctx *mfc_ctx,
						   union s3c_mfc_args *args);
enum SSBSIP_MFC_ERROR_CODE s3c_mfc_allocate_frame_buf(struct s3c_mfc_inst_ctx *mfc_ctx,
						 union s3c_mfc_args *args,
						 struct s3c_mfc_frame_buf_arg_t
						 buf_size);
enum SSBSIP_MFC_ERROR_CODE s3c_mfc_allocate_stream_ref_buf(struct s3c_mfc_inst_ctx *
						      mfc_ctx,
						      union s3c_mfc_args *args);
enum SSBSIP_MFC_ERROR_CODE s3c_mfc_return_inst_no(int inst_no,
					     enum SSBSIP_MFC_CODEC_TYPE codec_type);
bool s3c_mfc_is_running(void);
int s3c_mfc_set_state(struct s3c_mfc_inst_ctx *ctx, enum s3c_mfc_inst_state state);
void s3c_mfc_init_mem_inst_no(void);
int s3c_mfc_get_mem_inst_no(enum s3c_mfc_inst_no_type type);
void s3c_mfc_return_mem_inst_no(int inst_no);

#endif /* __S3C_MFC_COMMON_H */
