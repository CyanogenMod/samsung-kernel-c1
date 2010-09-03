/* linux/drivers/media/video/samsung/tv20/ver_1/mixer.h
 *
 * Copyright (c) 2010 Samsung Electronics
 *		http://www.samsung.com/
 *
 * header for Samsung TVOUT driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _MIXER_H_
#define _MIXER_H_

#include <linux/irqreturn.h>
#include "../s5p_tv.h"

enum s5p_mixer_layer {
	MIXER_VIDEO_LAYER = 2,
	MIXER_GPR0_LAYER = 0,
	MIXER_GPR1_LAYER = 1
};

enum s5p_mixer_bg_color_num {
	MIXER_BG_COLOR_0 = 0,
	MIXER_BG_COLOR_1 = 1,
	MIXER_BG_COLOR_2 = 2
};

enum s5p_mixer_color_fmt {
	MIXER_RGB565  = 4,
	MIXER_RGB1555 = 5,
	MIXER_RGB4444 = 6,
	MIXER_RGB8888 = 7
};

enum s5p_mixer_csc_type {
	MIXER_CSC_601_LR,
	MIXER_CSC_601_FR,
	MIXER_CSC_709_LR,
	MIXER_CSC_709_FR
};

enum s5p_mixer_rgb {
	MIXER_RGB601_0_255,
	MIXER_RGB601_16_235,
	MIXER_RGB709_0_255,
	MIXER_RGB709_16_235
};

enum s5p_mixer_out_type {
	MIXER_YUV444,
	MIXER_RGB888
};

int s5p_mixer_set_show(enum s5p_mixer_layer layer, bool show);
int s5p_mixer_set_priority(enum s5p_mixer_layer layer, u32 priority);
void s5p_mixer_set_pre_mul_mode(enum s5p_mixer_layer layer, bool enable);
int s5p_mixer_set_pixel_blend(enum s5p_mixer_layer layer, bool enable);
int s5p_mixer_set_layer_blend(enum s5p_mixer_layer layer, bool enable);
int s5p_mixer_set_alpha(enum s5p_mixer_layer layer, u32 alpha);
int s5p_mixer_set_grp_base_address(enum s5p_mixer_layer layer, u32 baseaddr);
int s5p_mixer_set_grp_layer_dst_pos(enum s5p_mixer_layer layer,
			u32 dst_offs_x, u32 dst_offs_y);
int s5p_mixer_set_grp_layer_src_pos(enum s5p_mixer_layer layer, u32 span,
			u32 width, u32 height, u32 src_offs_x, u32 src_offs_y);
int s5p_mixer_set_bg_color(enum s5p_mixer_bg_color_num colornum,
			u32 color_y, u32 color_cb, u32 color_cr);
int s5p_mixer_init_status_reg(enum s5p_mixer_burst_mode burst,
			enum s5p_tv_endian endian);
int s5p_mixer_init_display_mode(enum s5p_tv_disp_mode mode,
			enum s5p_tv_o_mode output_mode);
void s5p_mixer_scaling(enum s5p_mixer_layer layer, u32 ver_val, u32 hor_val);
void s5p_mixer_set_color_format(enum s5p_mixer_layer layer,
			enum s5p_mixer_color_fmt format);
void s5p_mixer_set_chroma_key(enum s5p_mixer_layer layer, bool enabled,
			u32 key);
int s5p_mixer_init_layer(enum s5p_tv_disp_mode mode, enum s5p_mixer_layer layer,
			bool show, bool winblending, u32 alpha, u32 priority,
			enum s5p_mixer_color_fmt color, bool blankchange,
			bool pixelblending, bool premul, u32 blankcolor,
			u32 baseaddr, u32 span, u32 width, u32 height,
			u32 src_offs_x, u32 src_offs_y, u32 dst_offs_x,
			u32 dst_offs_y, u32 dst_x, u32 dst_y);
void s5p_mixer_init_bg_dither_enable(bool cr_dither_enable, bool cdither_enable,
			bool y_dither_enable);
void s5p_mixer_init_csc_coef_default(enum s5p_mixer_csc_type csc_type);
void s5p_mixer_start(void);
void s5p_mixer_stop(void);
int s5p_mixer_set_underflow_interrupt_enable(enum s5p_mixer_layer layer,
			bool en);
void s5p_mixer_clear_pend_all(void);
irqreturn_t s5p_mixer_irq(int irq, void *dev_id);
void s5p_mixer_init(void __iomem *addr);

#endif /* _MIXER_H_ */
