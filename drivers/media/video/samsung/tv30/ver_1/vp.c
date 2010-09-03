/* linux/drivers/media/video/samsung/tv20/ver_1/vp.c
 *
 * Copyright (c) 2009 Samsung Electronics
 *		http://www.samsung.com/
 *
 * Video processor H/W interfaces for Samsung TVOut driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/slab.h>

#include <plat/clock.h>

#include "tvout_ver_1.h"

#include <mach/regs-vp.h>
#include "vp_coeff.c"

#ifdef COFIG_TVOUT_RAW_DBG
#define S5P_VP_DEBUG 1
#endif

#ifdef S5P_VP_DEBUG
#define VPPRINTK(fmt, args...)\
	printk(KERN_ERR "\t\t[VP] %s: " fmt, __func__ , ## args)
#else
#define VPPRINTK(fmt, args...)
#endif

enum s5p_vp_poly_coeff {
	VP_POLY8_Y0_LL = 0,
	VP_POLY8_Y0_LH,
	VP_POLY8_Y0_HL,
	VP_POLY8_Y0_HH,
	VP_POLY8_Y1_LL,
	VP_POLY8_Y1_LH,
	VP_POLY8_Y1_HL,
	VP_POLY8_Y1_HH,
	VP_POLY8_Y2_LL,
	VP_POLY8_Y2_LH,
	VP_POLY8_Y2_HL,
	VP_POLY8_Y2_HH,
	VP_POLY8_Y3_LL,
	VP_POLY8_Y3_LH,
	VP_POLY8_Y3_HL,
	VP_POLY8_Y3_HH,
	VP_POLY4_Y0_LL = 32,
	VP_POLY4_Y0_LH,
	VP_POLY4_Y0_HL,
	VP_POLY4_Y0_HH,
	VP_POLY4_Y1_LL,
	VP_POLY4_Y1_LH,
	VP_POLY4_Y1_HL,
	VP_POLY4_Y1_HH,
	VP_POLY4_Y2_LL,
	VP_POLY4_Y2_LH,
	VP_POLY4_Y2_HL,
	VP_POLY4_Y2_HH,
	VP_POLY4_Y3_LL,
	VP_POLY4_Y3_LH,
	VP_POLY4_Y3_HL,
	VP_POLY4_Y3_HH,
	VP_POLY4_C0_LL,
	VP_POLY4_C0_LH,
	VP_POLY4_C0_HL,
	VP_POLY4_C0_HH,
	VP_POLY4_C1_LL,
	VP_POLY4_C1_LH,
	VP_POLY4_C1_HL,
	VP_POLY4_C1_HH
};

enum s5p_vp_filter_h_pp {
	VP_PP_H_NORMAL = 0,
	VP_PP_H_8_9,
	VP_PP_H_1_2,
	VP_PP_H_1_3,
	VP_PP_H_1_4
};

enum s5p_vp_filter_v_pp {
	VP_PP_V_NORMAL = 0,
	VP_PP_V_5_6,
	VP_PP_V_3_4,
	VP_PP_V_1_2,
	VP_PP_V_1_3,
	VP_PP_V_1_4
};

enum s5p_vp_csc_coeff {
	VP_CSC_Y2Y_COEF = 0,
	VP_CSC_CB2Y_COEF,
	VP_CSC_CR2Y_COEF,
	VP_CSC_Y2CB_COEF,
	VP_CSC_CB2CB_COEF,
	VP_CSC_CR2CB_COEF,
	VP_CSC_Y2CR_COEF,
	VP_CSC_CB2CR_COEF,
	VP_CSC_CR2CR_COEF
};

static void __iomem *vp_base;

/*
* set
*  - set functions are only called under running video processor
*  - after running set functions, it is need to run s5p_vp_update() function
*    for update shadow registers
*/

static int s5p_vp_set_poly_filter_coef(enum s5p_vp_poly_coeff poly_coeff,
				signed char ch0, signed char ch1,
				signed char ch2, signed char ch3)
{
	VPPRINTK("%d, %d, %d, %d, %d)\n\r", poly_coeff, ch0, ch1, ch2, ch3);

	if (poly_coeff > VP_POLY4_C1_HH || poly_coeff < VP_POLY8_Y0_LL ||
	   (poly_coeff > VP_POLY8_Y3_HH && poly_coeff < VP_POLY4_Y0_LL)) {
		VPPRINTK("invaild poly_coeff parameter \n\r");

		return -1;
	}

	writel((((0xff & ch0) << 24) | ((0xff & ch1) << 16) |
		((0xff & ch2) << 8) | (0xff & ch3)),
			vp_base + S5P_VP_POLY8_Y0_LL + poly_coeff * 4);

	return 0;
}

static void s5p_vp_set_poly_filter_coef_default(u32 h_ratio, u32 v_ratio)
{
	enum s5p_vp_filter_h_pp e_h_filter;
	enum s5p_vp_filter_v_pp e_v_filter;
	u8 *poly_flt_coeff;
	int i, j;

	VPPRINTK("%d, %d\n\r", h_ratio, v_ratio);

	/*
	* For the real interlace mode, the vertical ratio should be
	* used after divided by 2. Because in the interlace mode, all
	* the VP output is used for SDOUT display and it should be the
	* same as one field of the progressive mode. Therefore the same
	* filter coefficients should be used for the same the final
	* output video. When half of the interlace V_RATIO is same as
	* the progressive V_RATIO, the final output video scale is same.
	*/

	if (h_ratio <= (0x1 << 16))		/* 720->720 or zoom in */
		e_h_filter = VP_PP_H_NORMAL;
	else if (h_ratio <= (0x9 << 13))	/* 720->640 */
		e_h_filter = VP_PP_H_8_9;
	else if (h_ratio <= (0x1 << 17))	/* 2->1 */
		e_h_filter = VP_PP_H_1_2;
	else if (h_ratio <= (0x3 << 16))	/* 2->1 */
		e_h_filter = VP_PP_H_1_3;
	else
		e_h_filter = VP_PP_H_1_4;	/* 4->1 */

	/* Vertical Y 4tap */

	if (v_ratio <= (0x1 << 16))          	/* 720->720 or zoom in*/
		e_v_filter = VP_PP_V_NORMAL;
	else if (v_ratio <= (0x5 << 14))	/* 4->3*/
		e_v_filter = VP_PP_V_3_4;
	else if (v_ratio <= (0x3 << 15))	/*6->5*/
		e_v_filter = VP_PP_V_5_6;
	else if (v_ratio <= (0x1 << 17))	/* 2->1*/
		e_v_filter = VP_PP_V_1_2;
	else if (v_ratio <= (0x3 << 16))	/* 3->1*/
		e_v_filter = VP_PP_V_1_3;
	else
		e_v_filter = VP_PP_V_1_4;

	poly_flt_coeff = (u8 *)(g_s_vp8tap_coef_y_h + e_h_filter * 16 * 8);

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			s5p_vp_set_poly_filter_coef(
				VP_POLY8_Y0_LL + (i*4) + j,
				*(poly_flt_coeff + 4*j*8 + (7 - i)),
				*(poly_flt_coeff + (4*j + 1)*8 + (7 - i)),
				*(poly_flt_coeff + (4*j + 2)*8 + (7 - i)),
				*(poly_flt_coeff + (4*j + 3)*8 + (7 - i)));
		}
	}

	poly_flt_coeff = (u8 *)(g_s_vp4tap_coef_c_h + e_h_filter * 16 * 4);

	for (i = 0; i < 2; i++) {
		for (j = 0; j < 4; j++) {
			s5p_vp_set_poly_filter_coef(
				VP_POLY4_C0_LL + (i*4) + j,
				*(poly_flt_coeff + 4*j*4 + (3 - i)),
				*(poly_flt_coeff + (4*j + 1)*4 + (3 - i)),
				*(poly_flt_coeff + (4*j + 2)*4 + (3 - i)),
				*(poly_flt_coeff + (4*j + 3)*4 + (3 - i)));
		}
	}

	poly_flt_coeff = (u8 *)(g_s_vp4tap_coef_y_v + e_v_filter * 16 * 4);

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			s5p_vp_set_poly_filter_coef(
				VP_POLY4_Y0_LL + (i*4) + j,
				*(poly_flt_coeff + 4*j*4 + (3 - i)),
				*(poly_flt_coeff + (4*j + 1)*4 + (3 - i)),
				*(poly_flt_coeff + (4*j + 2)*4 + (3 - i)),
				*(poly_flt_coeff + (4*j + 3)*4 + (3 - i)));
		}
	}

	VPPRINTK("%d, %d\n\r", e_h_filter, e_v_filter);
}


void s5p_vp_set_field_id(enum s5p_vp_field mode)
{
	VPPRINTK("%d\n\r", mode);

	writel((mode == VP_TOP_FIELD) ? VP_TOP_FIELD : VP_BOTTOM_FIELD,
		vp_base + S5P_VP_FIELD_ID);
}

int s5p_vp_set_top_field_address(u32 top_y_addr, u32 top_c_addr)
{
	VPPRINTK("0x%x, 0x%x\n\r", top_y_addr, top_c_addr);

	if (S5P_VP_PTR_ILLEGAL(top_y_addr) || S5P_VP_PTR_ILLEGAL(top_c_addr)) {
		VPPRINTK(" address is not double word align = 0x%x, 0x%x\n\r",
			top_y_addr, top_c_addr);

		return -1;
	}

	writel(top_y_addr, vp_base + S5P_VP_TOP_Y_PTR);
	writel(top_c_addr, vp_base + S5P_VP_TOP_C_PTR);

	return 0;
}

int s5p_vp_set_bottom_field_address(u32 bottom_y_addr, u32 bottom_c_addr)
{
	VPPRINTK("0x%x, 0x%x\n\r", bottom_y_addr, bottom_c_addr);

	if (S5P_VP_PTR_ILLEGAL(bottom_y_addr) ||
			S5P_VP_PTR_ILLEGAL(bottom_c_addr)) {
		VPPRINTK(" address is not double word align = 0x%x, 0x%x\n\r",
			bottom_y_addr, bottom_c_addr);

		return -1;
	}

	writel(bottom_y_addr, vp_base + S5P_VP_BOT_Y_PTR);
	writel(bottom_c_addr, vp_base + S5P_VP_BOT_C_PTR);

	return 0;
}

int s5p_vp_set_img_size(u32 img_width, u32 img_height)
{
	VPPRINTK("%d, %d\n\r", img_width, img_height);

	if (S5P_VP_IMG_SIZE_ILLEGAL(img_width) ||
			S5P_VP_IMG_SIZE_ILLEGAL(img_height)) {
		VPPRINTK(" image full size is not double word align ="
			"%d, %d\n\r", img_width, img_height);

		return -1;
	}

	writel(S5P_VP_IMG_HSIZE(img_width) | S5P_VP_IMG_VSIZE(img_height),
		vp_base + S5P_VP_IMG_SIZE_Y);
	writel(S5P_VP_IMG_HSIZE(img_width) | S5P_VP_IMG_VSIZE(img_height / 2),
		vp_base + S5P_VP_IMG_SIZE_C);

	return 0;
}

void s5p_vp_set_src_position(u32 src_off_x, u32 src_x_fract_step, u32 src_off_y)
{
	VPPRINTK("%d, %d, %d)\n\r", src_off_x, src_x_fract_step, src_off_y);

	writel(S5P_VP_SRC_H_POSITION_VAL(src_off_x) |
		S5P_VP_SRC_X_FRACT_STEP(src_x_fract_step),
			vp_base + S5P_VP_SRC_H_POSITION);
	writel(S5P_VP_SRC_V_POSITION_VAL(src_off_y),
			vp_base + S5P_VP_SRC_V_POSITION);
}

void s5p_vp_set_dest_position(u32 dst_off_x, u32 dst_off_y)
{
	VPPRINTK("%d, %d)\n\r", dst_off_x, dst_off_y);

	writel(S5P_VP_DST_H_POSITION_VAL(dst_off_x),
			vp_base + S5P_VP_DST_H_POSITION);
	writel(S5P_VP_DST_V_POSITION_VAL(dst_off_y),
			vp_base + S5P_VP_DST_V_POSITION);
}

void s5p_vp_set_src_dest_size(u32 src_width, u32 src_height,
				u32 dst_width, u32 dst_height, bool ipc_2d)
{
	u32 h_ratio = (src_width << 16) / dst_width;
	u32 v_ratio = (ipc_2d) ?
			((src_height << 17) / dst_height) :
			((src_height << 16) / dst_height);

	VPPRINTK("(%d, %d, %d, %d)++\n\r", src_width, src_height,
		dst_width, dst_height);

	writel(S5P_VP_SRC_WIDTH_VAL(src_width), vp_base + S5P_VP_SRC_WIDTH);
	writel(S5P_VP_SRC_HEIGHT_VAL(src_height), vp_base + S5P_VP_SRC_HEIGHT);
	writel(S5P_VP_DST_WIDTH_VAL(dst_width), vp_base + S5P_VP_DST_WIDTH);
	writel(S5P_VP_DST_HEIGHT_VAL(dst_height), vp_base + S5P_VP_DST_HEIGHT);
	writel(S5P_VP_H_RATIO_VAL(h_ratio), vp_base + S5P_VP_H_RATIO);
	writel(S5P_VP_V_RATIO_VAL(v_ratio), vp_base + S5P_VP_V_RATIO);

	writel((ipc_2d) ?
		(readl(vp_base + S5P_VP_MODE) | S5P_VP_MODE_2D_IPC_ENABLE) :
		(readl(vp_base + S5P_VP_MODE) & ~S5P_VP_MODE_2D_IPC_ENABLE),
		vp_base + S5P_VP_MODE);
}

int s5p_vp_set_brightness_contrast_control(enum s5p_vp_line_eq eq_num,
						u32 intc, u32 slope)
{
	VPPRINTK("%d, %d, %d\n\r", eq_num, intc, slope);

	if (eq_num > VP_LINE_EQ_7 || eq_num < VP_LINE_EQ_0) {
		VPPRINTK("invaild eq_num parameter \n\r");

		return -1;
	}

	writel(S5P_VP_LINE_INTC(intc) | S5P_VP_LINE_SLOPE(slope),
	       vp_base + S5P_PP_LINE_EQ0 + eq_num*4);

	return 0;
}

int s5p_vp_update(void)
{
	writel(readl(vp_base + S5P_VP_SHADOW_UPDATE) |
		S5P_VP_SHADOW_UPDATE_ENABLE,
			vp_base + S5P_VP_SHADOW_UPDATE);

	return 0;
}

bool s5p_vp_get_update_status(void)
{
	return readl(vp_base + S5P_VP_SHADOW_UPDATE) &
		S5P_VP_SHADOW_UPDATE_ENABLE;
}

void s5p_vp_init_op_mode(bool line_skip,
			enum s5p_vp_mem_mode mem_mode,
			enum s5p_vp_chroma_expansion chroma_exp,
			enum s5p_vp_filed_id_toggle toggle_id)
{
	u32 temp_reg;
	VPPRINTK("%d, %d, %d, %d\n\r", line_skip, mem_mode,
					chroma_exp, toggle_id);

	temp_reg = (line_skip) ?
			S5P_VP_MODE_LINE_SKIP_ON : S5P_VP_MODE_LINE_SKIP_OFF;
	temp_reg |= (mem_mode == VP_2D_TILE_MODE) ?
			S5P_VP_MODE_MEM_MODE_2D_TILE :
			S5P_VP_MODE_MEM_MODE_LINEAR;
	temp_reg |= (chroma_exp == VP_C_TOP_BOTTOM) ?
			S5P_VP_MODE_CROMA_EXP_C_TOPBOTTOM_PTR :
			S5P_VP_MODE_CROMA_EXP_C_TOP_PTR;
	temp_reg |= (toggle_id == VP_TOGGLE_VSYNC) ?
			S5P_VP_MODE_FIELD_ID_AUTO_TOGGLING :
			S5P_VP_MODE_FIELD_ID_MAN_TOGGLING;

	writel(temp_reg, vp_base + S5P_VP_MODE);
}

void s5p_vp_init_pixel_rate_control(enum s5p_vp_pxl_rate rate)
{
	VPPRINTK("%d\n\r", rate);

	writel(S5P_VP_PEL_RATE_CTRL(rate), vp_base + S5P_VP_PER_RATE_CTRL);
}

int s5p_vp_init_layer(u32 top_y_addr,
			u32 top_c_addr,
			u32 bottom_y_addr,
			u32 bottom_c_addr,
			enum s5p_tv_endian src_img_endian,
			u32 img_width,
			u32 img_height,
			u32 src_off_x,
			u32 src_x_fract_step,
			u32 src_off_y,
			u32 src_width,
			u32 src_height,
			u32 dst_off_x,
			u32 dst_off_y,
			u32 dst_width,
			u32 dst_height,
			bool ipc_2d)
{
	int error = 0;

	VPPRINTK("%d\n\r", src_img_endian);

	writel(1, vp_base + S5P_VP_ENDIAN_MODE);

	error = s5p_vp_set_top_field_address(top_y_addr, top_c_addr);

	if (error != 0)
		return error;

	error = s5p_vp_set_bottom_field_address(bottom_y_addr,
		bottom_c_addr);

	if (error != 0)
		return error;

	error = s5p_vp_set_img_size(img_width, img_height);

	if (error != 0)
		return error;

	s5p_vp_set_src_position(src_off_x, src_x_fract_step, src_off_y);

	s5p_vp_set_dest_position(dst_off_x, dst_off_y);

	s5p_vp_set_src_dest_size(src_width, src_height, dst_width,
		dst_height, ipc_2d);

	return error;
}

int s5p_vp_init_layer_def_poly_filter_coef(u32 top_y_addr,
					u32 top_c_addr,
					u32 bottom_y_addr,
					u32 bottom_c_addr,
					enum s5p_tv_endian src_img_endian,
					u32 img_width,
					u32 img_height,
					u32 src_off_x,
					u32 src_x_fract_step,
					u32 src_off_y,
					u32 src_width,
					u32 src_height,
					u32 dst_off_x,
					u32 dst_off_y,
					u32 dst_width,
					u32 dst_height,
					bool ipc_2d)
{
	int error = 0;

	u32 h_ratio = (src_width << 16) / dst_width;
	u32 v_ratio = (ipc_2d) ? ((src_height << 17) / dst_height) :
				((src_height << 16) / dst_height);

	s5p_vp_set_poly_filter_coef_default(h_ratio, v_ratio);
	error = s5p_vp_init_layer(top_y_addr, top_c_addr,
				bottom_y_addr, bottom_c_addr,
				src_img_endian,
				img_width, img_height,
				src_off_x, src_x_fract_step, src_off_y,
				src_width, src_height,
				dst_off_x, dst_off_y,
				dst_width, dst_height,
				ipc_2d);

	return error;
}

void s5p_vp_init_bypass_post_process(bool bypass)
{
	VPPRINTK("%d\n\r", bypass);

	writel((bypass) ? S5P_VP_BY_PASS_ENABLE : S5P_VP_BY_PASS_DISABLE,
			vp_base + S5P_PP_BYPASS);
}

int s5p_vp_init_csc_coef(enum s5p_vp_csc_coeff csc_coeff, u32 coeff)
{
	VPPRINTK("%d, %d\n\r", csc_coeff, coeff);

	if (csc_coeff > VP_CSC_CR2CR_COEF ||
			csc_coeff < VP_CSC_Y2Y_COEF) {
		VPPRINTK("invaild csc_coeff parameter \n\r");

		return -1;
	}

	writel(S5P_PP_CSC_COEF(coeff),
			vp_base + S5P_PP_CSC_Y2Y_COEF + csc_coeff*4);

	return 0;
}

void s5p_vp_init_saturation(u32 sat)
{
	VPPRINTK("%d\n\r", sat);

	writel(S5P_VP_SATURATION(sat), vp_base + S5P_PP_SATURATION);
}

void s5p_vp_init_sharpness(u32 th_h_noise,
				enum s5p_vp_sharpness_control sharpness)
{
	VPPRINTK("%d, %d\n\r", th_h_noise, sharpness);

	writel(S5P_VP_TH_HNOISE(th_h_noise) | S5P_VP_SHARPNESS(sharpness),
			vp_base + S5P_PP_SHARPNESS);
}

void s5p_vp_init_brightness_contrast(u16 b, u8 c)
{
	int i;

	for (i = 0; i < 8; i++)
		writel(S5P_VP_LINE_INTC(b) | S5P_VP_LINE_SLOPE(c),
			vp_base + S5P_PP_LINE_EQ0 + i*4);
}

void s5p_vp_init_brightness_offset(u32 offset)
{
	VPPRINTK("%d\n\r", offset);

	writel(S5P_VP_BRIGHT_OFFSET(offset), vp_base + S5P_PP_BRIGHT_OFFSET);
}

void s5p_vp_init_csc_control(bool sub_y_offset_en, bool csc_en)
{
	u32 temp_reg;
	VPPRINTK("%d, %d\n\r", sub_y_offset_en, csc_en);

	temp_reg = (sub_y_offset_en) ? S5P_VP_SUB_Y_OFFSET_ENABLE :
					S5P_VP_SUB_Y_OFFSET_DISABLE;
	temp_reg |= (csc_en) ? S5P_VP_CSC_ENABLE : S5P_VP_CSC_DISABLE;
	writel(temp_reg, vp_base + S5P_PP_CSC_EN);
}

int s5p_vp_init_csc_coef_default(enum s5p_vp_csc_type csc_type)
{
	VPPRINTK("%d\n\r", csc_type);

	switch (csc_type) {
	case VP_CSC_SD_HD:
		writel(S5P_PP_Y2Y_COEF_601_TO_709,
				vp_base + S5P_PP_CSC_Y2Y_COEF);
		writel(S5P_PP_CB2Y_COEF_601_TO_709,
				vp_base + S5P_PP_CSC_CB2Y_COEF);
		writel(S5P_PP_CR2Y_COEF_601_TO_709,
				vp_base + S5P_PP_CSC_CR2Y_COEF);
		writel(S5P_PP_Y2CB_COEF_601_TO_709,
				vp_base + S5P_PP_CSC_Y2CB_COEF);
		writel(S5P_PP_CB2CB_COEF_601_TO_709,
				vp_base + S5P_PP_CSC_CB2CB_COEF);
		writel(S5P_PP_CR2CB_COEF_601_TO_709,
				vp_base + S5P_PP_CSC_CR2CB_COEF);
		writel(S5P_PP_Y2CR_COEF_601_TO_709,
				vp_base + S5P_PP_CSC_Y2CR_COEF);
		writel(S5P_PP_CB2CR_COEF_601_TO_709,
				vp_base + S5P_PP_CSC_CB2CR_COEF);
		writel(S5P_PP_CR2CR_COEF_601_TO_709,
				vp_base + S5P_PP_CSC_CR2CR_COEF);
		break;

	case VP_CSC_HD_SD:
		writel(S5P_PP_Y2Y_COEF_709_TO_601,
				vp_base + S5P_PP_CSC_Y2Y_COEF);
		writel(S5P_PP_CB2Y_COEF_709_TO_601,
				vp_base + S5P_PP_CSC_CB2Y_COEF);
		writel(S5P_PP_CR2Y_COEF_709_TO_601,
				vp_base + S5P_PP_CSC_CR2Y_COEF);
		writel(S5P_PP_Y2CB_COEF_709_TO_601,
				vp_base + S5P_PP_CSC_Y2CB_COEF);
		writel(S5P_PP_CB2CB_COEF_709_TO_601,
				vp_base + S5P_PP_CSC_CB2CB_COEF);
		writel(S5P_PP_CR2CB_COEF_709_TO_601,
				vp_base + S5P_PP_CSC_CR2CB_COEF);
		writel(S5P_PP_Y2CR_COEF_709_TO_601,
				vp_base + S5P_PP_CSC_Y2CR_COEF);
		writel(S5P_PP_CB2CR_COEF_709_TO_601,
				vp_base + S5P_PP_CSC_CB2CR_COEF);
		writel(S5P_PP_CR2CR_COEF_709_TO_601,
				vp_base + S5P_PP_CSC_CR2CR_COEF);
		break;

	default:
		VPPRINTK("invalid csc_type parameter = %d\n\r", csc_type);
		return -1;
	}

	return 0;
}

int s5p_vp_start(void)
{
	int error = 0;

	writel(S5P_VP_ENABLE_ON, vp_base + S5P_VP_ENABLE);

	error = s5p_vp_update();

	return error;
}

int s5p_vp_stop(void)
{
	int error = 0;

	writel((readl(vp_base + S5P_VP_ENABLE) & ~S5P_VP_ENABLE_ON),
		vp_base + S5P_VP_ENABLE);

	error = s5p_vp_update();

	while (!(readl(vp_base + S5P_VP_ENABLE) & S5P_VP_ENABLE_OPERATING))
		msleep(1);

	return error;
}

void s5p_vp_sw_reset(void)
{
	writel((readl(vp_base + S5P_VP_SRESET) | S5P_VP_SRESET_PROCESSING),
		vp_base + S5P_VP_SRESET);

	while (readl(vp_base + S5P_VP_SRESET) & S5P_VP_SRESET_PROCESSING)
		msleep(10);

}

void s5p_vp_init(void __iomem *addr)
{
	vp_base = addr;
}
