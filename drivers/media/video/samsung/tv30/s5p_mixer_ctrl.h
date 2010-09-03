/* linux/drivers/media/video/samsung/tv20/s5p_mixer_ctrl.h
 *
 * Copyright (c) 2010 Samsung Electronics
 *		http://www.samsung.com/
 *
 * Header file for MIXER of Samsung TVOut driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _S5P_MIXER_CTRL_H_
#define _S5P_MIXER_CTRL_H_

#include "ver_1/tvout_ver_1.h"

void s5p_mixer_ctrl_init_private(enum s5p_mixer_layer layer);
void s5p_mixer_ctrl_init_fb_addr_phy(enum s5p_mixer_layer layer,
	dma_addr_t fb_addr);
void s5p_mixer_ctrl_init_grp_layer(enum s5p_mixer_layer layer);
int s5p_mixer_ctrl_set_pixel_format(enum s5p_mixer_layer layer, u32 bpp);
int s5p_mixer_ctrl_enable_layer(enum s5p_mixer_layer layer);
int s5p_mixer_ctrl_disable_layer(enum s5p_mixer_layer layer);
int s5p_mixer_ctrl_set_priority(enum s5p_mixer_layer layer, u32 prio);
int s5p_mixer_ctrl_set_dst_win_pos(enum s5p_mixer_layer layer,
	u32 dst_x, u32 dst_y, u32 w, u32 h);
int s5p_mixer_ctrl_set_src_win_pos(enum s5p_mixer_layer layer,
	u32 src_x, u32 src_y, u32 w, u32 h);
int s5p_mixer_ctrl_set_buffer_address(enum s5p_mixer_layer layer,
	dma_addr_t start_addr);
int s5p_mixer_ctrl_set_chroma_key(enum s5p_mixer_layer layer,
	struct s5ptvfb_chroma chroma);
int s5p_mixer_ctrl_set_alpha_blending(enum s5p_mixer_layer layer,
	enum s5ptvfb_alpha_t blend_mode, unsigned int alpha);
int s5p_mixer_ctrl_scaling(enum s5p_mixer_layer,
	struct s5ptvfb_user_scaling scaling);
int s5p_mixer_ctrl_init_status_reg(void);
int s5p_mixer_ctrl_init_bg_color(void);
int s5p_mixer_ctrl_constructor(struct platform_device *pdev);
void s5p_mixer_ctrl_destructor(void);
void s5p_mixer_ctrl_suspend(void);
void s5p_mixer_ctrl_resume(void);
#endif /* _S5P_MIXER_CTRL_H_ */
