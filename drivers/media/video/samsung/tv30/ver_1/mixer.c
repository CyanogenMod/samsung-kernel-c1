/* linux/drivers/media/video/samsung/tv20/ver_1/mixer.c
 *
 * Copyright (c) 2009 Samsung Electronics
 *		http://www.samsung.com/
 *
 * Mixer raw ftn  file for Samsung TVOut driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/slab.h>

#include <plat/clock.h>

#include <mach/regs-vmx.h>

#include "mixer.h"

#ifdef COFIG_TVOUT_RAW_DBG
#define S5P_MXR_DEBUG 1
#endif

#ifdef S5P_MXR_DEBUG
#define VMPRINTK(fmt, args...)	\
	printk(KERN_INFO "\t\t[VM] %s: " fmt, __func__ , ## args)
#else
#define VMPRINTK(fmt, args...)
#endif

void __iomem		*mixer_base;
spinlock_t	lock_mixer;

static u32 s5p_mixer_grp_scaling_factor(u32 src, u32 dst, u32 h_v)
{
	u32 factor; /* for scaling factor */

	/* check scale or not */
	if (src == dst)
		factor = 0;

	if (dst % src) {
		factor = 0;

		VMPRINTK(" can't %s scaling src(%d) into dst(%d)\n"
			, h_v ? "horizontal" : "vertical"
			, src_w, dst_w);
		VMPRINTK(" scaling vector must be 2/4/8x\n");
	}

	factor = dst / src;

	switch (factor) {
	case 2:
		factor = 1;
		break;

	case 4:
		factor = 2;
		break;

	case 8:
		factor = 3;
		break;

	default:
		VMPRINTK(" scaling vector must be 2/4/8x\n");
		factor = 0;
		break;
	}

	return factor;
}

int s5p_mixer_set_show(enum s5p_mixer_layer layer, bool show)
{
	u32 mxr_config;

	VMPRINTK("%d, %d\n\r", layer, show);

	switch (layer) {
	case MIXER_VIDEO_LAYER:
		mxr_config = (show) ?
				(readl(mixer_base + S5P_MXR_CFG) |
					S5P_MXR_CFG_VIDEO_ENABLE) :
				(readl(mixer_base + S5P_MXR_CFG) &
					~S5P_MXR_CFG_VIDEO_ENABLE);
		break;

	case MIXER_GPR0_LAYER:
		mxr_config = (show) ?
				(readl(mixer_base + S5P_MXR_CFG) |
					S5P_MXR_CFG_GRAPHIC0_ENABLE) :
				(readl(mixer_base + S5P_MXR_CFG) &
					~S5P_MXR_CFG_GRAPHIC0_ENABLE);
		break;

	case MIXER_GPR1_LAYER:
		mxr_config = (show) ?
				(readl(mixer_base + S5P_MXR_CFG) |
					S5P_MXR_CFG_GRAPHIC1_ENABLE) :
				(readl(mixer_base + S5P_MXR_CFG) &
					~S5P_MXR_CFG_GRAPHIC1_ENABLE);
		break;

	default:
		VMPRINTK(" invalid layer parameter = %d\n\r", layer);
		return -1;
	}

	writel(mxr_config, mixer_base + S5P_MXR_CFG);

	return 0;
}

int s5p_mixer_set_priority(enum s5p_mixer_layer layer, u32 priority)
{
	u32 layer_cfg;

	VMPRINTK("%d, %d\n\r", layer, priority);

	switch (layer) {
	case MIXER_VIDEO_LAYER:
		layer_cfg = S5P_MXR_LAYER_CFG_VID_PRIORITY_CLR(
				readl(mixer_base + S5P_MXR_LAYER_CFG)) |
				S5P_MXR_LAYER_CFG_VID_PRIORITY(priority);
		break;

	case MIXER_GPR0_LAYER:
		layer_cfg = S5P_MXR_LAYER_CFG_GRP0_PRIORITY_CLR(
				readl(mixer_base + S5P_MXR_LAYER_CFG)) |
				S5P_MXR_LAYER_CFG_GRP0_PRIORITY(priority);
		break;

	case MIXER_GPR1_LAYER:
		layer_cfg = S5P_MXR_LAYER_CFG_GRP1_PRIORITY_CLR(
				readl(mixer_base + S5P_MXR_LAYER_CFG)) |
				S5P_MXR_LAYER_CFG_GRP1_PRIORITY(priority);
		break;

	default:
		VMPRINTK(" invalid layer parameter = %d\n\r", layer);
		return -1;
	}

	writel(layer_cfg, mixer_base + S5P_MXR_LAYER_CFG);

	return 0;
}

void s5p_mixer_set_pre_mul_mode(enum s5p_mixer_layer layer, bool enable)
{
	u32 reg;

	switch (layer) {
	case MIXER_GPR0_LAYER:
		reg = readl(mixer_base + S5P_MXR_GRAPHIC0_CFG);

		if (enable)
			reg |= S5P_MXR_PRE_MUL_MODE;
		else
			reg &= ~S5P_MXR_PRE_MUL_MODE;

		writel(reg, mixer_base + S5P_MXR_GRAPHIC0_CFG);
		break;
	case MIXER_GPR1_LAYER:
		reg = readl(mixer_base + S5P_MXR_GRAPHIC1_CFG);

		if (enable)
			reg |= S5P_MXR_PRE_MUL_MODE;
		else
			reg &= ~S5P_MXR_PRE_MUL_MODE;

		writel(reg, mixer_base + S5P_MXR_GRAPHIC1_CFG);
		break;
	case MIXER_VIDEO_LAYER:
		break;
	}
}

int s5p_mixer_set_pixel_blend(enum s5p_mixer_layer layer, bool enable)
{
	u32 temp_reg;

	VMPRINTK("%d, %d\n\r", layer, enable);

	switch (layer) {
	case MIXER_GPR0_LAYER:
		temp_reg = readl(mixer_base + S5P_MXR_GRAPHIC0_CFG)
			& (~S5P_MXR_PIXEL_BLEND_ENABLE) ;

		if (enable)
			temp_reg |= S5P_MXR_PIXEL_BLEND_ENABLE;
		else
			temp_reg |= S5P_MXR_PIXEL_BLEND_DISABLE;

		writel(temp_reg, mixer_base + S5P_MXR_GRAPHIC0_CFG);
		break;

	case MIXER_GPR1_LAYER:
		temp_reg = readl(mixer_base + S5P_MXR_GRAPHIC1_CFG)
			& (~S5P_MXR_PIXEL_BLEND_ENABLE) ;

		if (enable)
			temp_reg |= S5P_MXR_PIXEL_BLEND_ENABLE;
		else
			temp_reg |= S5P_MXR_PIXEL_BLEND_DISABLE;

		writel(temp_reg, mixer_base + S5P_MXR_GRAPHIC1_CFG);
		break;

	default:
		VMPRINTK(" invalid layer parameter = %d\n\r", layer);

		return -1;
	}

	return 0;
}

int s5p_mixer_set_layer_blend(enum s5p_mixer_layer layer, bool enable)
{
	u32 temp_reg;

	VMPRINTK("%d, %d\n\r", layer, enable);

	switch (layer) {
	case MIXER_VIDEO_LAYER:
		temp_reg = readl(mixer_base + S5P_MXR_VIDEO_CFG)
			   & (~S5P_MXR_VIDEO_CFG_BLEND_EN) ;

		if (enable)
			temp_reg |= S5P_MXR_VIDEO_CFG_BLEND_EN;
		else
			temp_reg |= S5P_MXR_VIDEO_CFG_BLEND_DIS;

		writel(temp_reg, mixer_base + S5P_MXR_VIDEO_CFG);
		break;

	case MIXER_GPR0_LAYER:
		temp_reg = readl(mixer_base + S5P_MXR_GRAPHIC0_CFG)
			   & (~S5P_MXR_WIN_BLEND_ENABLE) ;

		if (enable)
			temp_reg |= S5P_MXR_WIN_BLEND_ENABLE;
		else
			temp_reg |= S5P_MXR_WIN_BLEND_DISABLE;

		writel(temp_reg, mixer_base + S5P_MXR_GRAPHIC0_CFG);
		break;

	case MIXER_GPR1_LAYER:
		temp_reg = readl(mixer_base + S5P_MXR_GRAPHIC1_CFG)
			   & (~S5P_MXR_WIN_BLEND_ENABLE) ;

		if (enable)
			temp_reg |= S5P_MXR_WIN_BLEND_ENABLE;
		else
			temp_reg |= S5P_MXR_WIN_BLEND_DISABLE;

		writel(temp_reg, mixer_base + S5P_MXR_GRAPHIC1_CFG);
		break;

	default:
		VMPRINTK(" invalid layer parameter = %d\n\r", layer);

		return -1;
	}

	return 0;
}

int s5p_mixer_set_alpha(enum s5p_mixer_layer layer, u32 alpha)
{
	u32 temp_reg;

	VMPRINTK("%d, %d\n\r", layer, alpha);

	switch (layer) {
	case MIXER_VIDEO_LAYER:
		temp_reg = readl(mixer_base + S5P_MXR_VIDEO_CFG)
			   & (~S5P_MXR_VIDEO_CFG_ALPHA_MASK) ;
		temp_reg |= S5P_MXR_VIDEO_CFG_ALPHA_VALUE(alpha);
		writel(temp_reg, mixer_base + S5P_MXR_VIDEO_CFG);
		break;

	case MIXER_GPR0_LAYER:
		temp_reg = readl(mixer_base + S5P_MXR_GRAPHIC0_CFG)
			   & (~S5P_MXR_VIDEO_CFG_ALPHA_MASK) ;
		temp_reg |= S5P_MXR_GRP_ALPHA_VALUE(alpha);
		writel(temp_reg, mixer_base + S5P_MXR_GRAPHIC0_CFG);
		break;

	case MIXER_GPR1_LAYER:
		temp_reg = readl(mixer_base + S5P_MXR_GRAPHIC1_CFG)
			   & (~S5P_MXR_VIDEO_CFG_ALPHA_MASK) ;
		temp_reg |= S5P_MXR_GRP_ALPHA_VALUE(alpha);
		writel(temp_reg, mixer_base + S5P_MXR_GRAPHIC1_CFG);
		break;

	default:
		VMPRINTK(" invalid layer parameter = %d\n\r", layer);
		return -1;
	}

	return 0;
}

int s5p_mixer_set_grp_base_address(enum s5p_mixer_layer layer, u32 base_addr)
{
	VMPRINTK("%d, 0x%x\n\r", layer, base_addr);

	if (S5P_MXR_GRP_ADDR_ILLEGAL(base_addr)) {
		VMPRINTK(" address is not word align = %d\n\r", base_addr);

		return -1;
	}

	switch (layer) {
	case MIXER_GPR0_LAYER:
		writel(S5P_MXR_GPR_BASE(base_addr),
			mixer_base + S5P_MXR_GRAPHIC0_BASE);
		break;

	case MIXER_GPR1_LAYER:
		writel(S5P_MXR_GPR_BASE(base_addr),
			mixer_base + S5P_MXR_GRAPHIC1_BASE);
		break;

	default:
		VMPRINTK(" invalid layer parameter = %d\n\r", layer);
		return -1;
	}

	return 0;
}

int s5p_mixer_set_grp_layer_dst_pos(enum s5p_mixer_layer layer,
					u32 dst_offs_x, u32 dst_offs_y)
{
	VMPRINTK("%d, %d, %d)\n\r", layer, dst_offs_x, dst_offs_y);

	switch (layer) {
	case MIXER_GPR0_LAYER:
		writel(S5P_MXR_GRP_DESTX(dst_offs_x) |
			S5P_MXR_GRP_DESTY(dst_offs_y),
			mixer_base + S5P_MXR_GRAPHIC0_DXY);
		break;

	case MIXER_GPR1_LAYER:
		writel(S5P_MXR_GRP_DESTX(dst_offs_x) |
			S5P_MXR_GRP_DESTY(dst_offs_y),
			mixer_base + S5P_MXR_GRAPHIC1_DXY);
		break;

	default:
		VMPRINTK("invalid layer parameter = %d\n\r", layer);
		return -1;
	}

	return 0;
}

int s5p_mixer_set_grp_layer_src_pos(enum s5p_mixer_layer layer, u32 src_offs_x,
			u32 src_offs_y, u32 span, u32 width, u32 height)
{
	VMPRINTK("%d, %d, %d, %d, %d, %d)\n\r", layer, span, width, height,
		 src_offs_x, src_offs_y);

	switch (layer) {
	case MIXER_GPR0_LAYER:
		writel(S5P_MXR_GRP_SPAN(span),
			mixer_base + S5P_MXR_GRAPHIC0_SPAN);
		writel(S5P_MXR_GRP_WIDTH(width) | S5P_MXR_GRP_HEIGHT(height),
		       mixer_base + S5P_MXR_GRAPHIC0_WH);
		writel(S5P_MXR_GRP_STARTX(src_offs_x) |
			S5P_MXR_GRP_STARTY(src_offs_y),
		       mixer_base + S5P_MXR_GRAPHIC0_SXY);
		break;

	case MIXER_GPR1_LAYER:
		writel(S5P_MXR_GRP_SPAN(span),
			mixer_base + S5P_MXR_GRAPHIC1_SPAN);
		writel(S5P_MXR_GRP_WIDTH(width) | S5P_MXR_GRP_HEIGHT(height),
		       mixer_base + S5P_MXR_GRAPHIC1_WH);
		writel(S5P_MXR_GRP_STARTX(src_offs_x) |
			S5P_MXR_GRP_STARTY(src_offs_y),
		       mixer_base + S5P_MXR_GRAPHIC1_SXY);
		break;

	default:
		VMPRINTK(" invalid layer parameter = %d\n\r", layer);
		return -1;
	}

	return 0;
}

int s5p_mixer_set_bg_color(enum s5p_mixer_bg_color_num colornum,
				u32 color_y, u32 color_cb, u32 color_cr)
{
	u32 reg_value;
	VMPRINTK("%d, %d, %d, %d)\n\r", colornum, color_y, color_cb, color_cr);

	reg_value = S5P_MXR_BG_COLOR_Y(color_y) |
			S5P_MXR_BG_COLOR_CB(color_cb) |
			S5P_MXR_BG_COLOR_CR(color_cr);

	switch (colornum) {
	case MIXER_BG_COLOR_0:
		writel(reg_value, mixer_base + S5P_MXR_BG_COLOR0);
		break;

	case MIXER_BG_COLOR_1:
		writel(reg_value, mixer_base + S5P_MXR_BG_COLOR1);
		break;

	case MIXER_BG_COLOR_2:
		writel(reg_value, mixer_base + S5P_MXR_BG_COLOR2);
		break;

	default:
		VMPRINTK(" invalid uiColorNum parameter = %d\n\r", colornum);
		return -1;
	}

	return 0;
}

int s5p_mixer_init_status_reg(enum s5p_mixer_burst_mode burst,
				enum s5p_tv_endian endian)
{
	u32 temp_reg = 0;

	VMPRINTK("++(%d, %d)\n\r", burst, endian);

	temp_reg = S5P_MXR_STATUS_SYNC_ENABLE | S5P_MXR_STATUS_OPERATING;

	switch (burst) {
	case MIXER_BURST_8:
		temp_reg |= S5P_MXR_STATUS_8_BURST;
		break;

	case MIXER_BURST_16:
		temp_reg |= S5P_MXR_STATUS_16_BURST;
		break;

	default:
		VMPRINTK("[ERR] : invalid burst parameter = %d\n\r", burst);
		return -1;
	}

	switch (endian) {
	case TVOUT_BIG_ENDIAN:
		temp_reg |= S5P_MXR_STATUS_BIG_ENDIAN;
		break;

	case TVOUT_LITTLE_ENDIAN:
		temp_reg |= S5P_MXR_STATUS_LITTLE_ENDIAN;
		break;

	default:
		VMPRINTK("[ERR] : invalid endian parameter = %d\n\r", endian);
		return -1;
	}

	writel(temp_reg, mixer_base + S5P_MXR_STATUS);

	return 0;
}

int s5p_mixer_init_display_mode(enum s5p_tv_disp_mode mode,
				enum s5p_tv_o_mode output_mode)
{
	u32 temp_reg = readl(mixer_base + S5P_MXR_CFG);

	VMPRINTK("%d, %d)\n\r", mode, output_mode);

	switch (mode) {
	case TVOUT_NTSC_M:
	case TVOUT_NTSC_443:
		temp_reg &= ~S5P_MXR_CFG_HD;
		temp_reg &= ~S5P_MXR_CFG_PAL;
		temp_reg &= S5P_MXR_CFG_INTERLACE;
		break;

	case TVOUT_PAL_BDGHI:
	case TVOUT_PAL_M:
	case TVOUT_PAL_N:
	case TVOUT_PAL_NC:
	case TVOUT_PAL_60:
		temp_reg &= ~S5P_MXR_CFG_HD;
		temp_reg |= S5P_MXR_CFG_PAL;
		temp_reg &= S5P_MXR_CFG_INTERLACE;
		break;

	case TVOUT_480P_60_16_9:
	case TVOUT_480P_60_4_3:
	case TVOUT_480P_59:
		temp_reg &= ~S5P_MXR_CFG_HD;
		temp_reg &= ~S5P_MXR_CFG_PAL;
		temp_reg |= S5P_MXR_CFG_PROGRASSIVE;
		temp_reg |= MIXER_RGB601_16_235<<9;
		break;

	case TVOUT_576P_50_16_9:
	case TVOUT_576P_50_4_3:
		temp_reg &= ~S5P_MXR_CFG_HD;
		temp_reg |= S5P_MXR_CFG_PAL;
		temp_reg |= S5P_MXR_CFG_PROGRASSIVE;
		temp_reg |= MIXER_RGB601_16_235<<9;
		break;

	case TVOUT_720P_50:
	case TVOUT_720P_59:
	case TVOUT_720P_60:
		temp_reg |= S5P_MXR_CFG_HD;
		temp_reg &= ~S5P_MXR_CFG_HD_1080I;
		temp_reg |= S5P_MXR_CFG_PROGRASSIVE;
		temp_reg |= MIXER_RGB709_16_235<<9;
		break;

	case TVOUT_1080I_50:
	case TVOUT_1080I_59:
	case TVOUT_1080I_60:
		temp_reg |= S5P_MXR_CFG_HD;
		temp_reg |= S5P_MXR_CFG_HD_1080I;
		temp_reg &= S5P_MXR_CFG_INTERLACE;
		temp_reg |= MIXER_RGB709_16_235<<9;
		break;

	case TVOUT_1080P_50:
	case TVOUT_1080P_59:
	case TVOUT_1080P_60:
	case TVOUT_1080P_30:
		temp_reg |= S5P_MXR_CFG_HD;
		temp_reg |= S5P_MXR_CFG_HD_1080P;
		temp_reg |= S5P_MXR_CFG_PROGRASSIVE;
		temp_reg |= MIXER_RGB709_16_235<<9;
		break;

	default:
		VMPRINTK(" invalid mode parameter = %d\n\r", mode);
		return -1;
	}

	switch (output_mode) {
	case TVOUT_COMPOSITE:
		temp_reg &= S5P_MXR_CFG_TV_OUT;
		temp_reg &= ~(0x1<<8);
		temp_reg |= MIXER_YUV444<<8;
		break;

	case TVOUT_HDMI_RGB:
	case TVOUT_DVI:
		temp_reg |= S5P_MXR_CFG_HDMI_OUT;
		temp_reg &= ~(0x1<<8);
		temp_reg |= MIXER_RGB888<<8;
		break;

	case TVOUT_HDMI:
		temp_reg |= S5P_MXR_CFG_HDMI_OUT;
		temp_reg &= ~(0x1<<8);
		temp_reg |= MIXER_YUV444<<8;
		break;

	default:
		VMPRINTK(" invalid mode parameter = %d\n\r", mode);
		return -1;
	}

	writel(temp_reg, mixer_base + S5P_MXR_CFG);

	return 0;
}

void s5p_mixer_scaling(enum s5p_mixer_layer layer, u32 ver_val, u32 hor_val)
{
	u32 reg;

	switch (layer) {
	case MIXER_GPR0_LAYER:
		reg = readl(mixer_base + S5P_MXR_GRAPHIC0_WH);
		reg |= S5P_MXR_GRP_V_SCALE(ver_val);
		reg |= S5P_MXR_GRP_H_SCALE(hor_val);
		writel(reg, mixer_base + S5P_MXR_GRAPHIC0_WH);
		break;
	case MIXER_GPR1_LAYER:
		reg = readl(mixer_base + S5P_MXR_GRAPHIC1_WH);
		reg |= S5P_MXR_GRP_V_SCALE(ver_val);
		reg |= S5P_MXR_GRP_H_SCALE(hor_val);
		writel(reg, mixer_base + S5P_MXR_GRAPHIC1_WH);
		break;
	case MIXER_VIDEO_LAYER:
		break;
	}
}

void s5p_mixer_set_color_format(enum s5p_mixer_layer layer,
	enum s5p_mixer_color_fmt format)
{
	u32 reg;

	switch (layer) {
	case MIXER_GPR0_LAYER:
		reg = readl(mixer_base + S5P_MXR_GRAPHIC0_CFG);
		reg &= ~(S5P_MXR_EG_COLOR_FORMAT(0xf));
		reg |= S5P_MXR_EG_COLOR_FORMAT(format);
		writel(reg, mixer_base + S5P_MXR_GRAPHIC0_CFG);
		break;
	case MIXER_GPR1_LAYER:
		reg = readl(mixer_base + S5P_MXR_GRAPHIC1_CFG);
		reg &= ~(S5P_MXR_EG_COLOR_FORMAT(0xf));
		reg |= S5P_MXR_EG_COLOR_FORMAT(format);
		writel(reg, mixer_base + S5P_MXR_GRAPHIC1_CFG);
		break;
	case MIXER_VIDEO_LAYER:
		break;
	}
}

void s5p_mixer_set_chroma_key(enum s5p_mixer_layer layer, bool enabled, u32 key)
{
	u32 reg;

	switch (layer) {
	case MIXER_GPR0_LAYER:
		reg = readl(mixer_base + S5P_MXR_GRAPHIC0_CFG);

		if (enabled)
			reg &= ~S5P_MXR_BLANK_CHANGE_NEW_PIXEL;
		else
			reg |= S5P_MXR_BLANK_CHANGE_NEW_PIXEL;

		writel(reg, mixer_base + S5P_MXR_GRAPHIC0_CFG);
		writel(S5P_MXR_GPR_BLANK_COLOR(key),
			mixer_base + S5P_MXR_GRAPHIC0_BLANK);
		break;
	case MIXER_GPR1_LAYER:
		reg = readl(mixer_base + S5P_MXR_GRAPHIC1_CFG);

		if (enabled)
			reg &= ~S5P_MXR_BLANK_CHANGE_NEW_PIXEL;
		else
			reg |= S5P_MXR_BLANK_CHANGE_NEW_PIXEL;

		writel(reg, mixer_base + S5P_MXR_GRAPHIC1_CFG);
		writel(S5P_MXR_GPR_BLANK_COLOR(key),
		             mixer_base + S5P_MXR_GRAPHIC1_BLANK);
		break;
	case MIXER_VIDEO_LAYER:
		break;
	}
}

int s5p_mixer_init_layer(enum s5p_tv_disp_mode mode,
				enum s5p_mixer_layer layer,
				bool show,
				bool win_blending,
				u32 alpha,
				u32 priority,
				enum s5p_mixer_color_fmt color,
				bool blank_change,
				bool pixel_blending,
				bool premul,
				u32 blank_color,
				u32 base_addr,
				u32 span,
				u32 width,
				u32 height,
				u32 src_offs_x,
				u32 src_offs_y,
				u32 dst_offs_x,
				u32 dst_offs_y,
				u32 dst_width,
				u32 dst_height)
{
	u32 temp_reg = 0;
	u32 h_factor = 0, v_factor = 0;

	VMPRINTK("%d, %d, %d, %d, %d, %d, %d, %d, %d, 0x%x,"
		"0x%x, %d, %d, %d, %d, %d, %d, %d)\n\r",
		 layer, show, win_blending, alpha, priority,
		 color, blank_change, pixel_blending, premul,
		 blank_color, base_addr, span, width, height,
		 src_offs_x, src_offs_y, dst_offs_x, dst_offs_y);

	switch (layer) {
	case MIXER_VIDEO_LAYER:
		temp_reg = (win_blending) ? S5P_MXR_VIDEO_CFG_BLEND_EN :
			S5P_MXR_VIDEO_CFG_BLEND_DIS;
		temp_reg |= S5P_MXR_VIDEO_CFG_ALPHA_VALUE(alpha);
		/* temp yuv pxl limiter setting*/
		temp_reg &= ~(1<<17);
		writel(temp_reg, mixer_base + S5P_MXR_VIDEO_CFG);
		break;

	case MIXER_GPR0_LAYER:
		temp_reg = (blank_change) ?
				S5P_MXR_BLANK_NOT_CHANGE_NEW_PIXEL :
				S5P_MXR_BLANK_CHANGE_NEW_PIXEL;
		temp_reg |= (premul) ? S5P_MXR_PRE_MUL_MODE :
			    S5P_MXR_NORMAL_MODE;
		temp_reg |= (win_blending) ? S5P_MXR_WIN_BLEND_ENABLE :
			    S5P_MXR_WIN_BLEND_DISABLE;
		temp_reg |= (pixel_blending) ? S5P_MXR_PIXEL_BLEND_ENABLE :
			    S5P_MXR_PIXEL_BLEND_DISABLE;
		temp_reg |= S5P_MXR_EG_COLOR_FORMAT(color);
		temp_reg |= S5P_MXR_GRP_ALPHA_VALUE(alpha);
		writel(temp_reg, mixer_base + S5P_MXR_GRAPHIC0_CFG);
		writel(S5P_MXR_GPR_BLANK_COLOR(blank_color),
			mixer_base + S5P_MXR_GRAPHIC0_BLANK);

		s5p_mixer_set_grp_layer_src_pos(layer, src_offs_x, src_offs_y,
			span, width, height);

		s5p_mixer_set_grp_base_address(layer, base_addr);
		s5p_mixer_set_grp_layer_dst_pos(layer, dst_offs_x,
			dst_offs_y);

		temp_reg = readl(mixer_base + S5P_MXR_GRAPHIC0_WH);
		h_factor = s5p_mixer_grp_scaling_factor(width, dst_width, 1);
		v_factor = s5p_mixer_grp_scaling_factor(height, dst_height, 0);

		temp_reg &= ~((0x3<<28)|(0x3<<12));

		if (v_factor) {
			u32 reg = readl(mixer_base + S5P_MXR_CFG);

			/* In interlaced mode, vertical scaling must be
			 * replaced by PROGRESSIVE_MODE - pixel duplication
			 */
			if (mode == TVOUT_1080I_50 ||
			     mode == TVOUT_1080I_59 ||
			     mode == TVOUT_1080I_60) {
				/* scaled up by progressive setting */
				reg |= S5P_MXR_CFG_PROGRASSIVE;
				writel(reg, mixer_base + S5P_MXR_CFG);
			} else {
				/* scaled up by scale factor */
				temp_reg |= v_factor << 12;
			}
		} else {
			u32 reg = readl(mixer_base + S5P_MXR_CFG);

			/*
			 * if v_factor is 0, recover the original mode
			 */
			if (mode == TVOUT_1080I_50 ||
			     mode == TVOUT_1080I_59 ||
			     mode == TVOUT_1080I_60) {
				reg &= S5P_MXR_CFG_INTERLACE;
				writel(reg, mixer_base + S5P_MXR_CFG);
			}
		}

		temp_reg |= h_factor << 28;

		writel(temp_reg , mixer_base + S5P_MXR_GRAPHIC0_WH);
		break;

	case MIXER_GPR1_LAYER:
		temp_reg = (blank_change) ?
			S5P_MXR_BLANK_NOT_CHANGE_NEW_PIXEL :
			S5P_MXR_BLANK_CHANGE_NEW_PIXEL;
		temp_reg |= (premul) ? S5P_MXR_PRE_MUL_MODE :
			S5P_MXR_NORMAL_MODE;
		temp_reg |= (win_blending) ? S5P_MXR_WIN_BLEND_ENABLE :
			S5P_MXR_WIN_BLEND_DISABLE;
		temp_reg |= (pixel_blending) ? S5P_MXR_PIXEL_BLEND_ENABLE :
			S5P_MXR_PIXEL_BLEND_DISABLE;
		temp_reg |= S5P_MXR_EG_COLOR_FORMAT(color);
		temp_reg |= S5P_MXR_GRP_ALPHA_VALUE(alpha);

		writel(temp_reg, mixer_base + S5P_MXR_GRAPHIC1_CFG);
		writel(S5P_MXR_GPR_BLANK_COLOR(blank_color),
			mixer_base + S5P_MXR_GRAPHIC1_BLANK);

		s5p_mixer_set_grp_layer_src_pos(layer, src_offs_x, src_offs_y,
			span, width, height);

		s5p_mixer_set_grp_base_address(layer, base_addr);
		s5p_mixer_set_grp_layer_dst_pos(layer, dst_offs_x, dst_offs_y);

		temp_reg = readl(mixer_base + S5P_MXR_GRAPHIC1_WH);
		h_factor = s5p_mixer_grp_scaling_factor(width, dst_width, 1);
		v_factor = s5p_mixer_grp_scaling_factor(height, dst_height, 0);

		temp_reg &= ~((0x3<<28)|(0x3<<12));

		if (v_factor) {
			u32 reg = readl(mixer_base + S5P_MXR_CFG);

			/* In interlaced mode, vertical scaling must be
			 * replaced by PROGRESSIVE_MODE - pixel duplication
			 */
			if (mode == TVOUT_1080I_50 ||
			     mode == TVOUT_1080I_59 ||
			     mode == TVOUT_1080I_60) {
				/* scaled up by progressive setting */
				reg |= S5P_MXR_CFG_PROGRASSIVE;
				writel(reg, mixer_base + S5P_MXR_CFG);
			} else
				/* scaled up by scale factor */
				temp_reg |= v_factor << 12;
		} else {
			u32 reg = readl(mixer_base + S5P_MXR_CFG);

			/*
			 * if v_factor is 0, recover the original mode
			 */
			if (mode == TVOUT_1080I_50 ||
			     mode == TVOUT_1080I_59 ||
			     mode == TVOUT_1080I_60) {
				reg &= S5P_MXR_CFG_INTERLACE;
				writel(reg, mixer_base + S5P_MXR_CFG);
			}
		}

		temp_reg |= h_factor << 28;

		writel(temp_reg , mixer_base + S5P_MXR_GRAPHIC1_WH);
		break;

	default:
		VMPRINTK("invalid layer parameter = %d\n\r", layer);
		return -1;
	}

	s5p_mixer_set_priority(layer, priority);

	s5p_mixer_set_show(layer, show);

	return 0;
}

void s5p_mixer_init_bg_dither_enable(bool cr_dither_enable,
					bool cb_dither_enable,
					bool y_dither_enable)
{
	u32 temp_reg = 0;

	VMPRINTK("%d, %d, %d\n\r", cr_dither_enable, cb_dither_enable,
		y_dither_enable);

	temp_reg = (cr_dither_enable) ?
		   (temp_reg | S5P_MXR_BG_CR_DIHER_EN) :
		   (temp_reg & ~S5P_MXR_BG_CR_DIHER_EN);
	temp_reg = (cb_dither_enable) ?
		   (temp_reg | S5P_MXR_BG_CB_DIHER_EN) :
		   (temp_reg & ~S5P_MXR_BG_CB_DIHER_EN);
	temp_reg = (y_dither_enable) ?
		   (temp_reg | S5P_MXR_BG_Y_DIHER_EN) :
		   (temp_reg & ~S5P_MXR_BG_Y_DIHER_EN);

	writel(temp_reg, mixer_base + S5P_MXR_BG_CFG);

}

void s5p_mixer_init_csc_coef_default(enum s5p_mixer_csc_type csc_type)
{
	VMPRINTK("%d\n\r", csc_type);

	switch (csc_type) {
	case MIXER_CSC_601_LR:
		writel((0 << 30) | (153 << 20) | (300 << 10) | (58 << 0),
			mixer_base + S5P_MXR_CM_COEFF_Y);
		writel((936 << 20) | (851 << 10) | (262 << 0),
			mixer_base + S5P_MXR_CM_COEFF_CB);
		writel((262 << 20) | (805 << 10) | (982 << 0),
			mixer_base + S5P_MXR_CM_COEFF_CR);
		break;

	case MIXER_CSC_601_FR:
		writel((1 << 30) | (132 << 20) | (258 << 10) | (50 << 0),
			mixer_base + S5P_MXR_CM_COEFF_Y);
		writel((948 << 20) | (875 << 10) | (225 << 0),
			mixer_base + S5P_MXR_CM_COEFF_CB);
		writel((225 << 20) | (836 << 10) | (988 << 0),
			mixer_base + S5P_MXR_CM_COEFF_CR);
		break;

	case MIXER_CSC_709_LR:
		writel((0 << 30) | (109 << 20) | (366 << 10) | (36 << 0),
			mixer_base + S5P_MXR_CM_COEFF_Y);
		writel((964 << 20) | (822 << 10) | (216 << 0),
			mixer_base + S5P_MXR_CM_COEFF_CB);
		writel((262 << 20) | (787 << 10) | (1000 << 0),
			mixer_base + S5P_MXR_CM_COEFF_CR);
		break;

	case MIXER_CSC_709_FR:
		writel((1 << 30) | (94 << 20) | (314 << 10) | (32 << 0),
			mixer_base + S5P_MXR_CM_COEFF_Y);
		writel((972 << 20) | (851 << 10) | (225 << 0),
			mixer_base + S5P_MXR_CM_COEFF_CB);
		writel((225 << 20) | (820 << 10) | (1004 << 0),
			mixer_base + S5P_MXR_CM_COEFF_CR);
		break;

	default:
		VMPRINTK(" invalid csc_type parameter = %d\n\r", csc_type);
		break;
	}
}

void s5p_mixer_start(void)
{
	writel((readl(mixer_base + S5P_MXR_STATUS) | S5P_MXR_STATUS_RUN),
		mixer_base + S5P_MXR_STATUS);
}

void s5p_mixer_stop(void)
{
	u32 reg = readl(mixer_base + S5P_MXR_STATUS);

	reg &= ~S5P_MXR_STATUS_RUN;

	writel(reg, mixer_base + S5P_MXR_STATUS);

	do {
		reg = readl(mixer_base + S5P_MXR_STATUS);
	} while (reg & S5P_MXR_STATUS_RUN);
}

int s5p_mixer_set_underflow_interrupt_enable(enum s5p_mixer_layer layer, bool en)
{
	u32 enablemaks;

	VMPRINTK("%d, %d\n\r", layer, en);

	switch (layer) {
	case MIXER_VIDEO_LAYER:
		enablemaks = S5P_MXR_INT_EN_VP_ENABLE;
		break;

	case MIXER_GPR0_LAYER:
		enablemaks = S5P_MXR_INT_EN_GRP0_ENABLE;
		break;

	case MIXER_GPR1_LAYER:
		enablemaks = S5P_MXR_INT_EN_GRP1_ENABLE;
		break;

	default:
		VMPRINTK("invalid layer parameter = %d\n\r", layer);
		return -1;
	}

	if (en) {
		writel((readl(mixer_base + S5P_MXR_INT_EN) | enablemaks),
			mixer_base + S5P_MXR_INT_EN);
	} else {
		writel((readl(mixer_base + S5P_MXR_INT_EN) & ~enablemaks),
			mixer_base + S5P_MXR_INT_EN);
	}

	return 0;
}

void s5p_mixer_clear_pend_all(void)
{
	writel(S5P_MXR_INT_STATUS_INT_FIRED | S5P_MXR_INT_STATUS_VP_FIRED |
		S5P_MXR_INT_STATUS_GRP0_FIRED | S5P_MXR_INT_STATUS_GRP1_FIRED,
			mixer_base + S5P_MXR_INT_EN);
}

irqreturn_t s5p_mixer_irq(int irq, void *dev_id)
{
	bool v_i_f;
	bool g0_i_f;
	bool g1_i_f;
	bool mxr_i_f;
	u32 temp_reg = 0;

	spin_lock_irq(&lock_mixer);

	v_i_f = (readl(mixer_base + S5P_MXR_INT_STATUS)
			& S5P_MXR_INT_STATUS_VP_FIRED) ? true : false;
	g0_i_f = (readl(mixer_base + S5P_MXR_INT_STATUS)
			& S5P_MXR_INT_STATUS_GRP0_FIRED) ? true : false;
	g1_i_f = (readl(mixer_base + S5P_MXR_INT_STATUS)
			& S5P_MXR_INT_STATUS_GRP1_FIRED) ? true : false;
	mxr_i_f = (readl(mixer_base + S5P_MXR_INT_STATUS)
			& S5P_MXR_INT_STATUS_INT_FIRED) ? true : false;

	if (mxr_i_f) {
		temp_reg |= S5P_MXR_INT_STATUS_INT_FIRED;

		if (v_i_f) {
			temp_reg |= S5P_MXR_INT_STATUS_VP_FIRED;
			printk("VP fifo under run!!\n\r");
		}

		if (g0_i_f) {
			temp_reg |= S5P_MXR_INT_STATUS_GRP0_FIRED;
			printk("GRP0 fifo under run!!\n\r");
		}

		if (g1_i_f) {
			temp_reg |= S5P_MXR_INT_STATUS_GRP1_FIRED;
			printk("GRP1 fifo under run!!\n\r");
		}

		writel(temp_reg, mixer_base + S5P_MXR_INT_STATUS);
	}

	spin_unlock_irq(&lock_mixer);

	return IRQ_HANDLED;
}

void s5p_mixer_init(void __iomem *addr)
{
	mixer_base = addr;

	spin_lock_init(&lock_mixer);
}
