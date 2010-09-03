/* linux/drivers/media/video/samsung/tv20/s5p_vp_ctrl.c
 *
 * Copyright (c) 2009 Samsung Electronics
 *		http://www.samsung.com/
 *
 * Video processor ctrl class for Samsung TVOUT driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/delay.h>
#include <linux/ioctl.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/uaccess.h>

//#include <mach/pd.h>

#include "ver_1/mixer.h"
#include "ver_1/tvout_ver_1.h"

#ifdef COFIG_TVOUT_DBG
#define S5P_VLAYER_DEBUG  1
#endif

#ifdef S5P_VLAYER_DEBUG
#define VLAYERPRINTK(fmt, args...) \
	printk(KERN_INFO "\t[VLAYER] %s: " fmt, __func__ , ## args)
#else
#define VLAYERPRINTK(fmt, args...)
#endif

#define INTERLACED 			0
#define PROGRESSIVE 			1

struct s5p_img_size {
	u32 w;
	u32 h;
};

struct s5p_img_offset {
	u32 x;
	u32 y;
};

struct s5p_video_img_address {
	u32 y_address;
	u32 c_address;
};

struct s5p_vl_mode {
	bool line_skip;
	enum s5p_vp_mem_mode mem_mode;
	enum s5p_vp_chroma_expansion chroma_exp;
	enum s5p_vp_filed_id_toggle toggle_id;
};

struct s5p_vl_sharpness {
	u32 th_noise;
	enum s5p_vp_sharpness_control sharpness;
};

struct s5p_vl_csc_ctrl {
	bool sub_y_offset_en;
	bool csc_en;
};

struct s5p_vl_bright_contrast_ctrl {
	enum s5p_vp_line_eq eq_num;
	u32 intc;
	u32 slope;
};

struct s5p_vl_param {
	bool win_blending;
	u32 alpha;
	u32 priority;
	u32 top_y_address;
	u32 top_c_address;
	enum s5p_tv_endian src_img_endian;
	u32 img_width;
	u32 img_height;
	u32 src_offset_x;
	u32 src_offset_y;
	u32 src_width;
	u32 src_height;
	u32 dest_offset_x;
	u32 dest_offset_y;
	u32 dest_width;
	u32 dest_height;
};

struct s5p_vp_ctrl_private_data {
	enum s5p_vp_src_color src_color;
	enum s5p_vp_field field_id;
	enum s5p_vp_pxl_rate vl_rate;
	enum s5p_vp_csc_type vl_csc_type;

	u32	vl_top_y_address;
	u32	vl_top_c_address;
	u32	vl_bottom_y_address;
	u32	vl_bottom_c_address;
	u32	vl_src_offset_x;
	u32	vl_src_offset_y;
	u32	vl_src_width;
	u32	vl_src_height;
	u32	vl_src_x_fact_step;

	u32	vl_dest_offset_x;
	u32	vl_dest_offset_y;
	u32	vl_dest_width;
	u32	vl_dest_height;
	bool	vl2d_ipc;

	bool	vl_poly_filter_default;
	bool	vl_bypass_post_process;
	u32	vl_saturation;
	bool	vl_brightness;
	u8	vl_contrast;
	u32	vl_bright_offset;
	bool	vl_csc_coef_default;

	struct s5p_vl_param vl_basic_param;
	struct s5p_vl_mode vl_op_mode;
	struct s5p_vl_sharpness vl_sharpness;
	struct s5p_vl_csc_ctrl vl_csc_control;
	struct s5p_vl_bright_contrast_ctrl vl_bc_control[8];

	bool	running;

	struct reg_mem_info reg_mem;

	struct s5p_tv_clk_info		clk;
	char 				*pow_name;
};

extern void s5p_vp_init_brightness_contrast(u16 b, u8 c);
extern void s5p_vp_init(void __iomem *addr);

extern unsigned long tv_pgd;

static struct s5p_vp_ctrl_private_data s5p_vp_ctrl_private = {
	.reg_mem = {
		.name = "s5p-vp",
		.res = NULL,
		.base = NULL
	},
	.clk = {
		.name = "vp",
		.ptr	= NULL
	},
		.pow_name = "vp_pd",
};

static u8 s5p_vp_ctrl_get_src_scan_mode(enum s5p_vp_src_color color)
{
	u8 ret = PROGRESSIVE;

	if (color == VP_SRC_COLOR_NV12IW || color == VP_SRC_COLOR_TILE_NV12IW)
		ret = INTERLACED;

	return ret;
}

static u8 s5p_vp_ctrl_get_dest_scan_mode(
		enum s5p_tv_disp_mode display,
		enum s5p_tv_o_mode out)
{
	u8 ret = PROGRESSIVE;

	switch (out) {
	case TVOUT_COMPOSITE:
		ret = INTERLACED;
		break;

	case TVOUT_HDMI_RGB:
	case TVOUT_HDMI:
	case TVOUT_DVI:
		if (display == TVOUT_1080I_60 ||
		   display == TVOUT_1080I_59 ||
		   display == TVOUT_1080I_50)
			ret = INTERLACED;
		break;

	default:
		break;
	}

	return ret;
}

static void s5p_vp_ctrl_calc_inner_values(void)
{
	struct s5p_tv_status *st = &s5ptv_status;
	struct s5p_vl_param *video = &(s5p_vp_ctrl_private.vl_basic_param);
	u8 o_mode, i_mode;

	i_mode = s5p_vp_ctrl_get_src_scan_mode(s5p_vp_ctrl_private.src_color);
	o_mode = s5p_vp_ctrl_get_dest_scan_mode(st->tvout_param.disp,
				st->tvout_param.out);

	s5p_vp_ctrl_private.vl_top_y_address = video->top_y_address;
	s5p_vp_ctrl_private.vl_top_c_address = video->top_c_address;

	if (s5p_vp_ctrl_private.src_color == VP_SRC_COLOR_NV12IW) {
		s5p_vp_ctrl_private.vl_bottom_y_address =
			s5p_vp_ctrl_private.vl_top_y_address + video->img_width;
		s5p_vp_ctrl_private.vl_bottom_c_address =
			s5p_vp_ctrl_private.vl_top_c_address + video->img_width;
	} else if (s5p_vp_ctrl_private.src_color == VP_SRC_COLOR_TILE_NV12IW) {
		s5p_vp_ctrl_private.vl_bottom_y_address =
			s5p_vp_ctrl_private.vl_top_y_address + 0x40;
		s5p_vp_ctrl_private.vl_bottom_c_address =
			s5p_vp_ctrl_private.vl_top_c_address + 0x40;
	}

	s5p_vp_ctrl_private.vl_src_offset_x	= video->src_offset_x;
	s5p_vp_ctrl_private.vl_src_offset_y	= video->src_offset_y;
	s5p_vp_ctrl_private.vl_src_width	= video->src_width;
	s5p_vp_ctrl_private.vl_src_height	= video->src_height;

	s5p_vp_ctrl_private.vl_dest_offset_x	= video->dest_offset_x;
	s5p_vp_ctrl_private.vl_dest_offset_y	= video->dest_offset_y;
	s5p_vp_ctrl_private.vl_dest_width	= video->dest_width;
	s5p_vp_ctrl_private.vl_dest_height	= video->dest_height;

	if (o_mode == INTERLACED) {
		s5p_vp_ctrl_private.vl_src_offset_y	/= 2;
		s5p_vp_ctrl_private.vl_src_height	/= 2;
		s5p_vp_ctrl_private.vl_dest_offset_y	/= 2;
		s5p_vp_ctrl_private.vl_dest_height	/= 2;
	} else if (i_mode == INTERLACED) {
		s5p_vp_ctrl_private.vl_src_offset_y 	/= 2;
		s5p_vp_ctrl_private.vl_src_height 	/= 2;
	}
}

static bool s5p_vp_ctrl_set_top_addr(unsigned long buf_in)
{
	u32 t_y_addr = 0;
	u32 t_c_addr = 0;
	u32 b_y_addr = 0;
	u32 b_c_addr = 0;

	struct s5p_video_img_address *addr =
		(struct s5p_video_img_address *)buf_in;
	int verr;

	s5p_vp_ctrl_private.vl_basic_param.top_y_address = addr->y_address;
	s5p_vp_ctrl_private.vl_basic_param.top_c_address = addr->c_address;

	s5p_vp_ctrl_calc_inner_values();

	t_y_addr = s5p_vp_ctrl_private.vl_top_y_address;
	t_c_addr = s5p_vp_ctrl_private.vl_top_c_address;
	b_y_addr = s5p_vp_ctrl_private.vl_bottom_y_address;
	b_c_addr = s5p_vp_ctrl_private.vl_bottom_c_address;

//. d: sichoi
//	if (s5p_vp_get_update_status())
//		return false;

	verr = s5p_vp_set_top_field_address(t_y_addr, t_c_addr);

	if (verr != 0)
		return false;

	if (s5p_vp_ctrl_get_src_scan_mode(s5p_vp_ctrl_private.src_color)
		== INTERLACED) {
		s5p_vp_set_field_id(s5p_vp_ctrl_private.field_id);
		verr = s5p_vp_set_bottom_field_address(b_y_addr, b_c_addr);

		if (verr != 0)
			return false;
	}

	s5p_vp_update();

	return true;
}

static void s5p_vp_ctrl_set_src_pos(unsigned long buf_in)
{
	struct s5p_img_offset *offset = (struct s5p_img_offset *) buf_in;

	s5p_vp_ctrl_private.vl_basic_param.src_offset_x = offset->x;
	s5p_vp_ctrl_private.vl_basic_param.src_offset_y = offset->y;
	s5p_vp_ctrl_calc_inner_values();

//. d: sichoi
//	if (s5p_vp_get_update_status())
//		return false;

	s5p_vp_set_src_position(
		s5p_vp_ctrl_private.vl_src_offset_x,
		s5p_vp_ctrl_private.vl_src_x_fact_step,
		s5p_vp_ctrl_private.vl_src_offset_y);

	s5p_vp_update();
}

static void s5p_vp_ctrl_set_src_size(unsigned long buf_in)
{
	struct s5p_img_size *size = (struct s5p_img_size *) buf_in;

	s5p_vp_ctrl_private.vl_basic_param.src_width = size->w;
	s5p_vp_ctrl_private.vl_basic_param.src_height = size->h;
	s5p_vp_ctrl_calc_inner_values();

//. d: sichoi
//	if (s5p_vp_get_update_status())
//		return false;

	s5p_vp_set_src_dest_size(
		s5p_vp_ctrl_private.vl_src_width,
		s5p_vp_ctrl_private.vl_src_height,
		s5p_vp_ctrl_private.vl_dest_width,
		s5p_vp_ctrl_private.vl_dest_height,
		s5p_vp_ctrl_private.vl2d_ipc);

	s5p_vp_update();
}

static void s5p_vp_ctrl_init_private(void)
{
	int i;

	s5p_vp_ctrl_private.vl_src_x_fact_step		= 0;
	s5p_vp_ctrl_private.field_id			= VP_TOP_FIELD;
	s5p_vp_ctrl_private.vl_rate			= VP_PXL_PER_RATE_1_1;
	s5p_vp_ctrl_private.vl_poly_filter_default	= true;
	s5p_vp_ctrl_private.vl_bypass_post_process	= false;

	s5p_vp_ctrl_private.vl_saturation		= 0x80;
	s5p_vp_ctrl_private.vl_brightness		= 0x00;
	s5p_vp_ctrl_private.vl_bright_offset		= 0x00;
	s5p_vp_ctrl_private.vl_contrast			= 0x80;

	s5p_vp_ctrl_private.vl_sharpness.th_noise	= 0;
	s5p_vp_ctrl_private.vl_sharpness.sharpness	= VP_SHARPNESS_NO;

	s5p_vp_ctrl_private.vl_op_mode.chroma_exp	= 0;

	s5p_vp_ctrl_private.vl_csc_control.csc_en	= false;
	s5p_vp_ctrl_private.vl_csc_coef_default		= true;
	s5p_vp_ctrl_private.vl_csc_control.sub_y_offset_en = false;
	for (i = 0; i < 8; i++) 
		s5p_vp_ctrl_private.vl_bc_control[i].eq_num = VP_LINE_EQ_7 + 1;

	s5p_vp_ctrl_private.running	= false;
}

static bool s5p_vp_ctrl_set_reg(void)
{
	int i;
	int verr;
	int merr;
	struct s5p_video_img_address temp_addr;
	struct s5p_img_size img_size;

	struct s5p_vl_param param = s5p_vp_ctrl_private.vl_basic_param;

	u8 contrast = s5p_vp_ctrl_private.vl_contrast;

	u32 ty_addr = s5p_vp_ctrl_private.vl_top_y_address;
	u32 tc_addr = s5p_vp_ctrl_private.vl_top_c_address;
	u32 by_addr = s5p_vp_ctrl_private.vl_bottom_y_address;
	u32 bc_addr = s5p_vp_ctrl_private.vl_bottom_c_address;
	u32 endian = param.src_img_endian;
	u32 i_w  = param.src_width;
	u32 i_h  = param.src_height;
	u32 s_ox = s5p_vp_ctrl_private.vl_src_offset_x;
	u32 s_xf = s5p_vp_ctrl_private.vl_src_x_fact_step;
	u32 s_oy = s5p_vp_ctrl_private.vl_src_offset_y;
	u32 s_w  = s5p_vp_ctrl_private.vl_src_width;
	u32 s_h  = s5p_vp_ctrl_private.vl_src_height;
	u32 d_ox = s5p_vp_ctrl_private.vl_dest_offset_x;
	u32 d_oy = s5p_vp_ctrl_private.vl_dest_offset_y;
	u32 d_w  = s5p_vp_ctrl_private.vl_dest_width;
	u32 d_h  = s5p_vp_ctrl_private.vl_dest_height;
	u32 noise = s5p_vp_ctrl_private.vl_sharpness.th_noise;
	u32 saturation = s5p_vp_ctrl_private.vl_saturation;
	u32 alpha = param.alpha;
	u32 priority = param.priority;
	u32 br_offset = s5p_vp_ctrl_private.vl_bright_offset;

	bool ipc = s5p_vp_ctrl_private.vl2d_ipc;
	bool l_skip = s5p_vp_ctrl_private.vl_op_mode.line_skip;
	bool bypass = s5p_vp_ctrl_private.vl_bypass_post_process;
	bool po_def = s5p_vp_ctrl_private.vl_poly_filter_default;
	bool bright = s5p_vp_ctrl_private.vl_brightness;
	bool w_blend = param.win_blending;
	bool csc_en = s5p_vp_ctrl_private.vl_csc_control.csc_en;
	bool s_off_en = s5p_vp_ctrl_private.vl_csc_control.sub_y_offset_en;
	bool csc_coef_def = s5p_vp_ctrl_private.vl_csc_coef_default;

	enum s5p_vp_mem_mode m_mode = s5p_vp_ctrl_private.vl_op_mode.mem_mode;
	enum s5p_vp_chroma_expansion cro_ex =
		s5p_vp_ctrl_private.vl_op_mode.chroma_exp;
	enum s5p_vp_filed_id_toggle f_id_tog =
		s5p_vp_ctrl_private.vl_op_mode.toggle_id;
	enum s5p_vp_sharpness_control sharp =
		s5p_vp_ctrl_private.vl_sharpness.sharpness;

	s5p_vp_sw_reset();
	s5p_vp_set_field_id(s5p_vp_ctrl_private.field_id);
	s5p_vp_init_op_mode(l_skip, m_mode, cro_ex, f_id_tog);
	s5p_vp_init_pixel_rate_control(s5p_vp_ctrl_private.vl_rate);

	temp_addr.y_address = param.top_y_address;
	temp_addr.c_address = param.top_c_address;
	img_size.w = param.src_width;
	img_size.h = param.src_height;

	s5p_vp_ctrl_set_top_addr((unsigned long)&temp_addr);
	s5p_vp_set_img_size(param.img_width, param.img_height);
	s5p_vp_ctrl_set_src_size((unsigned long)&img_size);

	if (po_def)
		verr = s5p_vp_init_layer_def_poly_filter_coef(ty_addr,
			tc_addr, by_addr, bc_addr, endian, i_w, i_h, s_ox,
			s_xf, s_oy, s_w, s_h, d_ox, d_oy, d_w, d_h, ipc);
	else
		verr = s5p_vp_init_layer(ty_addr, tc_addr, by_addr, bc_addr,
			endian, i_w, i_h, s_ox, s_xf, s_oy, s_w, s_h, d_ox,
			d_oy, d_w, d_h, ipc);

	if (verr != 0)
		return false;

	s5p_vp_init_bypass_post_process(bypass);
	s5p_vp_init_sharpness(noise, sharp);
	s5p_vp_init_saturation(saturation);
	s5p_vp_init_brightness_contrast(bright, contrast);

	for (i = VP_LINE_EQ_0; i <= VP_LINE_EQ_7; i++) {
		if (s5p_vp_ctrl_private.vl_bc_control[i].eq_num == i)
			verr = s5p_vp_set_brightness_contrast_control(
				s5p_vp_ctrl_private.vl_bc_control[i].eq_num,
				s5p_vp_ctrl_private.vl_bc_control[i].intc,
				s5p_vp_ctrl_private.vl_bc_control[i].slope);

		if (verr != 0)
			return false;
	}

	s5p_vp_init_brightness_offset(br_offset);

	s5p_vp_init_csc_control(s_off_en, csc_en);

	if (csc_en && csc_coef_def) {
		verr = s5p_vp_init_csc_coef_default(
				s5p_vp_ctrl_private.vl_csc_type);

		if (verr != 0)
			return false;
	}

	verr = s5p_vp_start();

	if (verr != 0)
		return false;

	merr = s5p_mixer_init_layer(s5ptv_status.tvout_param.disp,
				MIXER_VIDEO_LAYER, true, w_blend, alpha, priority,
				MIXER_RGB565, false, false, false,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	if (merr != 0)
		return false;

	s5p_mixer_start();

	return true;
}

static void s5p_vp_ctrl_internal_stop(void)
{
	// sichoi: change sequence of calling follows
	s5p_vp_stop();

	s5p_mixer_set_show(MIXER_VIDEO_LAYER, false);
}

static void s5p_vp_ctrl_clock(bool on)
{
	if (on) {
//		s5pv210_pd_enable(s5p_vp_ctrl_private.pow_name);
		clk_enable(s5p_vp_ctrl_private.clk.ptr);

	} else {
		clk_disable(s5p_vp_ctrl_private.clk.ptr);
//		s5pv210_pd_disable(s5p_vp_ctrl_private.pow_name);
	}

	mdelay(50);
}



void s5p_vp_ctrl_set_src_plane(
		u32			base_y,
		u32			base_c,
		u32			width,
		u32			height,
		enum s5p_vp_src_color	color,
		enum s5p_vp_field	field )
{
	s5p_vp_ctrl_private.src_color	= color;
	s5p_vp_ctrl_private.field_id	= field;

	s5p_vp_ctrl_private.vl_basic_param.top_y_address	= base_y;
	s5p_vp_ctrl_private.vl_basic_param.top_c_address	= base_c;
	s5p_vp_ctrl_private.vl_basic_param.img_width		= width;
	s5p_vp_ctrl_private.vl_basic_param.img_height		= height;

	if (s5p_vp_ctrl_private.running) {
		struct s5p_video_img_address	temp_addr;

		temp_addr.y_address = base_y;
		temp_addr.c_address = base_c;

		/*
		 * don't change following function call sequence.
		 *  - s5p_vp_set_img_size() doest not update VP H/W.
		 *  - VP H/W update is done by s5p_vp_ctrl_set_top_addr().
		 */
		s5p_vp_set_img_size(width, height);
		s5p_vp_ctrl_set_top_addr((unsigned long) &temp_addr);
	}
}

void s5p_vp_ctrl_set_src_win(u32 left, u32 top, u32 width, u32 height)
{
	s5p_vp_ctrl_private.vl_basic_param.src_offset_x	= left;
	s5p_vp_ctrl_private.vl_basic_param.src_offset_y	= top;
	s5p_vp_ctrl_private.vl_basic_param.src_width	= width;
	s5p_vp_ctrl_private.vl_basic_param.src_height	= height;

	if (s5p_vp_ctrl_private.running) {
		struct s5p_img_offset	img_offset;
		struct s5p_img_size	img_size;

		img_offset.x	= left;
		img_offset.y	= top;
		img_size.w	= width;
		img_size.h	= height;

		s5p_vp_ctrl_set_src_size((unsigned long) &img_size);
		s5p_vp_ctrl_set_src_pos((unsigned long) &img_offset);
	}
}

void s5p_vp_ctrl_set_dest_win(u32 left, u32 top, u32 width, u32 height)
{
	s5p_vp_ctrl_private.vl_basic_param.dest_offset_x	= left;
	s5p_vp_ctrl_private.vl_basic_param.dest_offset_y	= top;
	s5p_vp_ctrl_private.vl_basic_param.dest_width		= width;
	s5p_vp_ctrl_private.vl_basic_param.dest_height		= height;

	if (s5p_vp_ctrl_private.running) {
		s5p_vp_ctrl_calc_inner_values();

		s5p_vp_set_dest_position(
			s5p_vp_ctrl_private.vl_dest_offset_x,
			s5p_vp_ctrl_private.vl_dest_offset_y);

		s5p_vp_set_src_dest_size(
			s5p_vp_ctrl_private.vl_src_width,
			s5p_vp_ctrl_private.vl_src_height,
			s5p_vp_ctrl_private.vl_dest_width,
			s5p_vp_ctrl_private.vl_dest_height,
			s5p_vp_ctrl_private.vl2d_ipc);

		s5p_vp_update();
	}
}

void s5p_vp_ctrl_set_dest_win_alpha_val(u32 alpha)
{
	s5p_vp_ctrl_private.vl_basic_param.alpha = alpha;

	if (s5p_vp_ctrl_private.running)
		s5p_mixer_set_alpha(MIXER_VIDEO_LAYER, alpha);
}

void s5p_vp_ctrl_set_dest_win_blend(bool enable)
{
	s5p_vp_ctrl_private.vl_basic_param.win_blending = enable;

	if (s5p_vp_ctrl_private.running)
		s5p_mixer_set_layer_blend(MIXER_VIDEO_LAYER, enable);
}

void s5p_vp_ctrl_set_dest_win_priority(u32 prio)
{
	s5p_vp_ctrl_private.vl_basic_param.priority = prio;

	if (s5p_vp_ctrl_private.running)
		s5p_mixer_set_priority(MIXER_VIDEO_LAYER, prio);
}

void s5p_vp_ctrl_stop(void)
{
	if (s5p_vp_ctrl_private.running) {
		s5p_vp_ctrl_internal_stop();
		s5p_vp_ctrl_clock(0);

		s5p_vp_ctrl_private.running = false;
	}
}

bool s5p_vp_ctrl_start(void)
{
	enum s5p_tv_disp_mode disp = s5ptv_status.tvout_param.disp;
	enum s5p_tv_o_mode out = s5ptv_status.tvout_param.out;

	bool i_mode, o_mode; /* 0 for interlaced, 1 for progressive */

	switch (disp) {
	case TVOUT_480P_60_16_9:
	case TVOUT_480P_60_4_3:
	case TVOUT_576P_50_16_9:
	case TVOUT_576P_50_4_3:
	case TVOUT_480P_59:
		s5p_vp_ctrl_private.vl_csc_type = VP_CSC_SD_HD;
		break;

	case TVOUT_1080I_50:
	case TVOUT_1080I_60:
	case TVOUT_1080P_50:
	case TVOUT_1080P_30:
	case TVOUT_1080P_60:
	case TVOUT_720P_59:
	case TVOUT_1080I_59:
	case TVOUT_1080P_59:
	case TVOUT_720P_50:
	case TVOUT_720P_60:
		s5p_vp_ctrl_private.vl_csc_type = VP_CSC_HD_SD;
		break;

	default:
		break;
	}

	i_mode = s5p_vp_ctrl_get_src_scan_mode(s5p_vp_ctrl_private.src_color);
	o_mode = s5p_vp_ctrl_get_dest_scan_mode(disp,out);

	/* check o_mode */
	if (i_mode == INTERLACED) {
		/* i to i : line skip 1, ipc 0, auto toggle 0 */
		if (o_mode == INTERLACED) {
			s5p_vp_ctrl_private.vl_op_mode.line_skip = true;
			s5p_vp_ctrl_private.vl2d_ipc 		 = false;
			s5p_vp_ctrl_private.vl_op_mode.toggle_id = false;
		} else {
		/* i to p : line skip 1, ipc 1, auto toggle 0 */
			s5p_vp_ctrl_private.vl_op_mode.line_skip = true;
			s5p_vp_ctrl_private.vl2d_ipc 		 = true;
			s5p_vp_ctrl_private.vl_op_mode.toggle_id = false;
		}
	} else {
		/* p to i : line skip 1, ipc 0, auto toggle 0 */
		if (o_mode == INTERLACED) {
			s5p_vp_ctrl_private.vl_op_mode.line_skip = true;
			s5p_vp_ctrl_private.vl2d_ipc 		 = false;
			s5p_vp_ctrl_private.vl_op_mode.toggle_id = false;
		} else {
		/* p to p : line skip 0, ipc 0, auto toggle 0 */
			s5p_vp_ctrl_private.vl_op_mode.line_skip = false;
			s5p_vp_ctrl_private.vl2d_ipc 		 = false;
			s5p_vp_ctrl_private.vl_op_mode.toggle_id = false;
		}
	}

	s5p_vp_ctrl_private.vl_op_mode.mem_mode
		= ((s5p_vp_ctrl_private.src_color == VP_SRC_COLOR_NV12) ||
			s5p_vp_ctrl_private.src_color == VP_SRC_COLOR_NV12IW) ?
			VP_LINEAR_MODE : VP_2D_TILE_MODE;

	s5p_vp_ctrl_calc_inner_values();

	if (s5p_vp_ctrl_private.running)
		s5p_vp_ctrl_internal_stop();
	else {
		s5p_vp_ctrl_clock(1);

		s5p_vp_ctrl_private.running = true;
	}
	
	s5p_vp_ctrl_set_reg();

	mdelay(50);

//. d: sichoi
/*
	if (s5ptv_status.hdmi.hdcp_en)
		s5p_hdcp_start();
*/
	return true;
}

extern int s5p_tv_map_resource_mem(
		struct platform_device *pdev,
		char *name,
		void __iomem **base,
		struct resource **res);

int s5p_vp_ctrl_constructor(struct platform_device *pdev)
{
	int ret = 0;

	ret = s5p_tv_map_resource_mem(
		pdev,
		s5p_vp_ctrl_private.reg_mem.name,
		&(s5p_vp_ctrl_private.reg_mem.base),
		&(s5p_vp_ctrl_private.reg_mem.res));

	if (ret)
		goto err_on_res;

	s5p_vp_ctrl_private.clk.ptr =
		clk_get(&pdev->dev, s5p_vp_ctrl_private.clk.name);

	if (IS_ERR(s5p_vp_ctrl_private.clk.ptr)) {
		printk(KERN_ERR	"Failed to find clock %s\n",
			s5p_vp_ctrl_private.clk.name);
		ret = -ENOENT;
		goto err_on_clk;
	}

	s5p_vp_init(s5p_vp_ctrl_private.reg_mem.base);
	s5p_vp_ctrl_init_private();

	return 0;

err_on_clk:
	iounmap(s5p_vp_ctrl_private.reg_mem.base);
	release_resource(s5p_vp_ctrl_private.reg_mem.res);
	kfree(s5p_vp_ctrl_private.reg_mem.res);

err_on_res:
	return ret;
}

void s5p_vp_ctrl_destructor(void)
{
	if (s5p_vp_ctrl_private.reg_mem.base)
		iounmap(s5p_vp_ctrl_private.reg_mem.base);

	if (s5p_vp_ctrl_private.reg_mem.res) {
		release_resource(s5p_vp_ctrl_private.reg_mem.res);
		kfree(s5p_vp_ctrl_private.reg_mem.res);
	}

	if (s5p_vp_ctrl_private.clk.ptr) {
		if (s5p_vp_ctrl_private.running)
			clk_disable(s5p_vp_ctrl_private.clk.ptr);
		clk_put(s5p_vp_ctrl_private.clk.ptr);
	}
}

void s5p_vp_ctrl_suspend(void)
{
}

void s5p_vp_ctrl_resume(void)
{
}
