/* linux/drivers/media/video/samsung/tv20/ver_1/tvout_ver_1.h
 *
 * Copyright (c) 2009 Samsung Electronics
 *		http://www.samsung.com/
 *
 * header for Samsung TVOut driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _LINUX_EXTERN_FUNC_TVOUT_H
#define _LINUX_EXTERN_FUNC_TVOUT_H

#include <linux/platform_device.h>
#include <linux/irqreturn.h>
#include <linux/i2c.h>

#include "../s5p_tv.h"

/* #define COFIG_TVOUT_RAW_DBG */

#define CEC_IOC_MAGIC        'c'
#define CEC_IOC_SETLADDR     _IOW(CEC_IOC_MAGIC, 0, unsigned int)

enum cec_state {
	STATE_RX,
	STATE_TX,
	STATE_DONE,
	STATE_ERROR
};

struct cec_rx_struct {
	spinlock_t lock;
	wait_queue_head_t waitq;
	atomic_t state;
	u8 *buffer;
	unsigned int size;
};

struct cec_tx_struct {
	wait_queue_head_t waitq;
	atomic_t state;
};

enum s5p_vp_src_color {
	VP_SRC_COLOR_NV12  	= 0,
	VP_SRC_COLOR_NV12IW  = 1,
	VP_SRC_COLOR_TILE_NV12  	= 2,
	VP_SRC_COLOR_TILE_NV12IW  	= 3
};

enum s5p_vp_field {
	VP_TOP_FIELD,
	VP_BOTTOM_FIELD
};

enum s5p_vp_line_eq {
	VP_LINE_EQ_0  = 0,
	VP_LINE_EQ_1  = 1,
	VP_LINE_EQ_2  = 2,
	VP_LINE_EQ_3  = 3,
	VP_LINE_EQ_4  = 4,
	VP_LINE_EQ_5  = 5,
	VP_LINE_EQ_6  = 6,
	VP_LINE_EQ_7  = 7
};

enum s5p_vp_mem_mode {
	VP_LINEAR_MODE,
	VP_2D_TILE_MODE
};

enum s5p_vp_chroma_expansion {
	VP_C_TOP,
	VP_C_TOP_BOTTOM
};

enum s5p_vp_filed_id_toggle {
	VP_TOGGLE_USER,
	VP_TOGGLE_VSYNC
};

enum s5p_vp_pxl_rate {
	VP_PXL_PER_RATE_1_1  = 0,
	VP_PXL_PER_RATE_1_2  = 1,
	VP_PXL_PER_RATE_1_3  = 2,
	VP_PXL_PER_RATE_1_4  = 3
};

enum s5p_vp_sharpness_control {
	VP_SHARPNESS_NO     = 0,
	VP_SHARPNESS_MIN    = 1,
	VP_SHARPNESS_MOD    = 2,
	VP_SHARPNESS_MAX    = 3
};

enum s5p_vp_csc_type {
	VP_CSC_SD_HD,
	VP_CSC_HD_SD
};

enum phy_freq {
	ePHY_FREQ_25_200,
	ePHY_FREQ_25_175,
	ePHY_FREQ_27,
	ePHY_FREQ_27_027,
	ePHY_FREQ_54,
	ePHY_FREQ_54_054,
	ePHY_FREQ_74_250,
	ePHY_FREQ_74_176,
	ePHY_FREQ_148_500,
	ePHY_FREQ_148_352,
	ePHY_FREQ_108_108,
	ePHY_FREQ_72,
	ePHY_FREQ_25,
	ePHY_FREQ_65,
	ePHY_FREQ_108,
	ePHY_FREQ_162
};

/* cec.c */
extern struct cec_rx_struct cec_rx_struct;
extern struct cec_tx_struct cec_tx_struct;

void s5p_cec_set_divider(void);
void s5p_cec_enable_rx(void);
void s5p_cec_mask_rx_interrupts(void);
void s5p_cec_unmask_rx_interrupts(void);
void s5p_cec_mask_tx_interrupts(void);
void s5p_cec_unmask_tx_interrupts(void);
void s5p_cec_reset(void);
void s5p_cec_tx_reset(void);
void s5p_cec_rx_reset(void);
void s5p_cec_threshold(void);
void s5p_cec_set_tx_state(enum cec_state state);
void s5p_cec_set_rx_state(enum cec_state state);
void s5p_cec_copy_packet(char *data, size_t count);
void s5p_cec_set_addr(u32 addr);
u32 s5p_cec_get_status(void);
void s5p_clr_pending_tx(void);
void s5p_clr_pending_rx(void);
void s5p_cec_get_rx_buf(u32 size, u8 *buffer);
void __init s5p_cec_mem_probe(struct platform_device *pdev);

/* hdcp.c */
void s5p_hdmi_set_audio(bool en);
int s5p_hdmi_set_mute(bool en);
int s5p_hdmi_get_mute(void);
int s5p_hdmi_audio_enable(bool en);
bool s5p_hdcp_start(void);
int __init s5p_hdcp_init(void);
int s5p_hdcp_encrypt_stop(bool on);

/* vp.c */
void s5p_vp_set_field_id(enum s5p_vp_field mode);
int s5p_vp_set_top_field_address(u32 top_y_addr, u32 top_c_addr);
int s5p_vp_set_bottom_field_address(u32 bottom_y_addr, u32 bottom_c_addr);
int s5p_vp_set_img_size(u32 img_width, u32 img_height);
void s5p_vp_set_src_position(u32 src_off_x, u32 src_x_fract_step,
			u32 src_off_y);
void s5p_vp_set_dest_position(u32 dst_off_x, u32 dst_off_y);
void s5p_vp_set_src_dest_size(u32 src_width, u32 src_height, u32 dst_width,
			u32 dst_height, bool ipc_2d);
int s5p_vp_set_brightness_contrast_control(enum s5p_vp_line_eq eq_num,
			u32 intc, u32 slope);
void s5p_vp_set_brightness(bool brightness);
void s5p_vp_set_contrast(u8 contrast);
int s5p_vp_update(void);
bool s5p_vp_get_update_status(void);
void s5p_vp_init_op_mode(bool line_skip, enum s5p_vp_mem_mode mem_mode,
			enum s5p_vp_chroma_expansion chroma_exp,
			enum s5p_vp_filed_id_toggle toggle_id);
void s5p_vp_init_pixel_rate_control(enum s5p_vp_pxl_rate rate);
int s5p_vp_init_layer(u32 top_y_addr, u32 top_c_addr, u32 bottom_y_addr,
			u32 bottom_c_addr, enum s5p_tv_endian src_img_endian,
			u32 img_width, u32 img_height, u32 src_off_x,
			u32 src_x_fract_step, u32 src_off_y, u32 src_width,
			u32 src_height, u32 dst_off_x, u32 dst_off_y,
			u32 dst_width, u32 dst_height, bool ipc_2d);
int s5p_vp_init_layer_def_poly_filter_coef(u32 top_y_addr, u32 top_c_addr,
			u32 bottom_y_addr, u32 bottom_c_addr,
			enum s5p_tv_endian src_img_endian, u32 img_width,
			u32 img_height, u32 src_off_x, u32 src_x_fract_step,
			u32 src_off_y, u32 src_width, u32 src_height,
			u32 dst_off_x, u32 dst_off_y, u32 dst_width,
			u32 dst_height, bool ipc_2d);
void s5p_vp_init_bypass_post_process(bool bypass);
void s5p_vp_init_saturation(u32 sat);
void s5p_vp_init_sharpness(u32 th_h_noise,
			enum s5p_vp_sharpness_control sharpness);
void s5p_vp_init_brightness_offset(u32 offset);
void s5p_vp_init_csc_control(bool suy_offset_en, bool csc_en);
int s5p_vp_init_csc_coef_default(enum s5p_vp_csc_type csc_type);
int s5p_vp_start(void);
int s5p_vp_stop(void);
void s5p_vp_sw_reset(void);
#endif /* _LINUX_EXTERN_FUNC_TVOUT_H */
