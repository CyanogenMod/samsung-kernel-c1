/* linux/drivers/media/video/samsung/mfc50/s3c_mfc_opr.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Definition for operation of MFC HW setting from driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __S3C_MFC_OPR_H
#define __S3C_MFC_OPR_H __FILE__

#include "s3c_mfc_errorno.h"
#include "s3c_mfc_interface.h"

#define     INT_MFC_FW_DONE		(0x1 << 5)
#define     INT_MFC_DMA_DONE		(0x1 << 7)
#define     INT_MFC_FRAME_DONE		(0x1 << 8)
/* Interrupt on/off (0x500) */
#define     INT_ENABLE_BIT		(0 << 0)
#define     INT_DISABLE_BIT		(1 << 0)
/* Interrupt mode (0x504) */
#define     INT_LEVEL_BIT		(0 << 0)
#define     INT_PULSE_BIT		(1 << 0)

/* Command Types */
#define		MFC_CHANNEL_SET		0
#define		MFC_CHANNEL_READ	1
#define		MFC_CHANNEL_END		2
#define		MFC_INIT_CODEC		3
#define		MFC_FRAME_RUN		4
#define		MFC_SLEEP		6
#define		MFC_WAKEUP		7

/* DPB Count */
#define		NUM_MPEG4_DPB		(2)
#define		NUM_POST_DPB		(3)
#define		NUM_VC1_DPB		(4)

#define		STUFF_BYTE_SIZE		(4)
#define 	NAL_START_CODE		(0x00000001)

#define MPEG4_TIME_RES	1000
#define SCALE_NUM	MPEG4_TIME_RES

extern void __iomem *s3c_mfc_sfr_virt_base;

enum SSBSIP_MFC_ERROR_CODE s3c_mfc_init_hw(void);
enum SSBSIP_MFC_ERROR_CODE s3c_mfc_init_encode(struct s3c_mfc_inst_ctx *mfc_ctx,
					  union s3c_mfc_args *args);
enum SSBSIP_MFC_ERROR_CODE s3c_mfc_encode_header(struct s3c_mfc_inst_ctx *mfc_ctx,
					    union s3c_mfc_args *args);
enum SSBSIP_MFC_ERROR_CODE s3c_mfc_set_enc_ref_buffer(struct s3c_mfc_inst_ctx *mfc_ctx,
						 union s3c_mfc_args *args);
enum SSBSIP_MFC_ERROR_CODE s3c_mfc_exe_encode(struct s3c_mfc_inst_ctx *mfc_ctx,
					 union s3c_mfc_args *args);
enum SSBSIP_MFC_ERROR_CODE s3c_mfc_init_decode(struct s3c_mfc_inst_ctx *mfc_ctx,
					  union s3c_mfc_args *args);
enum SSBSIP_MFC_ERROR_CODE s3c_mfc_set_dec_frame_buffer(struct s3c_mfc_inst_ctx *mfc_ctx,
						   union s3c_mfc_args *args);
enum SSBSIP_MFC_ERROR_CODE s3c_mfc_exe_decode(struct s3c_mfc_inst_ctx *mfc_ctx,
					 union s3c_mfc_args *args);
enum SSBSIP_MFC_ERROR_CODE s3c_mfc_get_config(struct s3c_mfc_inst_ctx *mfc_ctx,
					 union s3c_mfc_args *args);
enum SSBSIP_MFC_ERROR_CODE s3c_mfc_set_config(struct s3c_mfc_inst_ctx *mfc_ctx,
					 union s3c_mfc_args *args);
enum SSBSIP_MFC_ERROR_CODE s3c_mfc_deinit_hw(struct s3c_mfc_inst_ctx *mfc_ctx);
enum SSBSIP_MFC_ERROR_CODE s3c_mfc_set_sleep(void);
enum SSBSIP_MFC_ERROR_CODE s3c_mfc_set_wakeup(void);
void s3c_mfc_clear_int(void);
void s3c_mfc_clear_ch_id(unsigned int int_type);

void s3c_mfc_chnage_resloution(struct s3c_mfc_inst_ctx *mfc_ctx,
					struct s3c_mfc_dec_exe_arg_t *args);

#endif /* __S3C_MFC_OPR_H */
