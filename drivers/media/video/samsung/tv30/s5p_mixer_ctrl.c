/* linux/drivers/media/video/samsung/tv20/s5p_stda_grp.c
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsung.com/
 *
 * Graphic Layer ftn. file for Samsung TVOut driver
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
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/uaccess.h>

#include "ver_1/mixer.h"
#include "ver_1/tvout_ver_1.h"
#include "s5p_mixer_ctrl.h"

#ifdef COFIG_TVOUT_DBG
#define S5P_GRP_DEBUG 1
#endif

#ifdef S5P_GRP_DEBUG
#define GRPPRINTK(fmt, args...)	\
	printk(KERN_INFO "\t[GRP] %s: " fmt, __func__ , ## args)
#else
#define GRPPRINTK(fmt, args...)
#endif

enum {
	ACLK = 0,
	MUX,
	NO_OF_CLK
};

struct s5p_mixer_grp_layer_info {
	bool pixel_blend;
	bool layer_blend;
	u32 alpha;

	bool chroma_enable;
	u32 chroma_key;

	bool pre_mul_mode;

	u32 src_x;
	u32 src_y;
	u32 dst_x;
	u32 dst_y;
	u32 width;
	u32 height;
	dma_addr_t fb_addr;

	u32 priority;
	u32 bpp;

	enum s5ptvfb_ver_scaling_t ver;
	enum s5ptvfb_hor_scaling_t hor;
};

struct s5p_mixer_ctrl_private_data {
	enum s5p_mixer_burst_mode burst;
	enum s5p_tv_endian endian;
	struct s5p_bg_color bg_color[3];

	struct s5p_mixer_grp_layer_info layer[S5PTV_FB_CNT];

	char			*pow_name;
	struct s5p_tv_clk_info	clk[NO_OF_CLK];

	struct irq_info		irq;
	struct reg_mem_info reg_mem;
};

irqreturn_t s5p_mixer_irq(int irq, void *dev_id);

static struct s5p_mixer_ctrl_private_data s5p_mixer_ctrl_private = {
	.pow_name		= "mixer_pd",

	.clk[ACLK] = {
		.name		= "mixer",
		.ptr 		= NULL
	},
	.clk[MUX] = {
		.name 		= "sclk_mixer",
		.ptr 		= NULL
	},

	.reg_mem = {
		.name = "s5p-mixer",
		.res = NULL,
		.base = NULL
	},

	.irq = {
		.name		= "s5p-mixer",
		.handler	= s5p_mixer_irq,
		.no		= -1
	}
};

#if 0 /* this function will be used */
static void s5p_mixer_ctrl_clock(bool on)
{
	/* power control function is not implemented yet */
	if (on) {
		//s5pv210_pd_enable(s5p_mixer_ctrl_private.pow_name);
		clk_enable(s5p_mixer_ctrl_private.clk[0].ptr);
	} else {
		clk_disable(s5p_mixer_ctrl_private.clk[0].ptr);
		//s5pv210_pd_disable(s5p_mixer_ctrl_private.pow_name);
	}

	mdelay(50);
}
#endif

void s5p_mixer_ctrl_init_private(enum s5p_mixer_layer layer)
{
	s5p_mixer_ctrl_private.layer[layer].pixel_blend = false;
	s5p_mixer_ctrl_private.layer[layer].layer_blend = false;
	s5p_mixer_ctrl_private.layer[layer].alpha = 0xff;

	s5p_mixer_ctrl_private.layer[layer].chroma_enable = false;
	s5p_mixer_ctrl_private.layer[layer].chroma_key = 0x0;

	s5p_mixer_ctrl_private.layer[layer].pre_mul_mode = false;

	s5p_mixer_ctrl_private.layer[layer].src_x = 0;
	s5p_mixer_ctrl_private.layer[layer].src_y = 0;
	s5p_mixer_ctrl_private.layer[layer].dst_x = 0;
	s5p_mixer_ctrl_private.layer[layer].dst_y = 0;
	s5p_mixer_ctrl_private.layer[layer].width = 0;
	s5p_mixer_ctrl_private.layer[layer].height = 0;

	s5p_mixer_ctrl_private.layer[layer].priority = 10;
	s5p_mixer_ctrl_private.layer[layer].bpp = 32;

	s5p_mixer_ctrl_private.layer[layer].ver = VERTICAL_X1;
	s5p_mixer_ctrl_private.layer[layer].hor = HORIZONTAL_X1;

	s5p_mixer_ctrl_private.burst = MIXER_BURST_16;
	s5p_mixer_ctrl_private.endian = TVOUT_LITTLE_ENDIAN;

	s5p_mixer_ctrl_private.bg_color[0].color_y = 0;
	s5p_mixer_ctrl_private.bg_color[0].color_cb = 128;
	s5p_mixer_ctrl_private.bg_color[0].color_cr = 128;
	s5p_mixer_ctrl_private.bg_color[1].color_y = 0;
	s5p_mixer_ctrl_private.bg_color[1].color_cb = 128;
	s5p_mixer_ctrl_private.bg_color[1].color_cr = 128;
	s5p_mixer_ctrl_private.bg_color[2].color_y = 0;
	s5p_mixer_ctrl_private.bg_color[2].color_cb = 128;
	s5p_mixer_ctrl_private.bg_color[2].color_cr = 128;
}

void s5p_mixer_ctrl_init_fb_addr_phy(enum s5p_mixer_layer layer,
				dma_addr_t fb_addr)
{
	s5p_mixer_ctrl_private.layer[layer].fb_addr = fb_addr;
}

void s5p_mixer_ctrl_init_grp_layer(enum s5p_mixer_layer layer)
{
	struct s5ptvfb_user_scaling scaling;

	s5p_mixer_set_priority(layer,
		s5p_mixer_ctrl_private.layer[layer].priority);
	s5p_mixer_set_pre_mul_mode(layer,
		s5p_mixer_ctrl_private.layer[layer].pre_mul_mode);
	s5p_mixer_set_chroma_key(layer,
		s5p_mixer_ctrl_private.layer[layer].chroma_enable,
		s5p_mixer_ctrl_private.layer[layer].chroma_key);
	s5p_mixer_set_layer_blend(layer,
		s5p_mixer_ctrl_private.layer[layer].layer_blend);
	s5p_mixer_set_alpha(layer,
		s5p_mixer_ctrl_private.layer[layer].alpha);
	s5p_mixer_set_grp_layer_dst_pos(layer,
		s5p_mixer_ctrl_private.layer[layer].dst_x,
		s5p_mixer_ctrl_private.layer[layer].dst_y);

	scaling.ver = s5p_mixer_ctrl_private.layer[layer].ver;
	scaling.hor = s5p_mixer_ctrl_private.layer[layer].hor;
	s5p_mixer_ctrl_scaling(layer, scaling);
	s5p_mixer_set_grp_base_address(layer,
		s5p_mixer_ctrl_private.layer[layer].fb_addr);
}

int s5p_mixer_ctrl_set_pixel_format(enum s5p_mixer_layer layer, u32 bpp)
{
	enum s5p_mixer_color_fmt format;

	switch (bpp) {
	case 16:
		format = MIXER_RGB565;
		break;
	case 32:
		format = MIXER_RGB8888;
		break;
	default:
		dev_err(s5ptv_status.dev_fb, "invalid bits per pixel\n");
		return -EINVAL;
	}

	s5p_mixer_ctrl_private.layer[layer].bpp = bpp;
	s5p_mixer_set_color_format(layer, format);
	return 0;
}

int s5p_mixer_ctrl_enable_layer(enum s5p_mixer_layer layer)
{
	switch (layer) {
	case MIXER_VIDEO_LAYER:
	case MIXER_GPR0_LAYER:
	case MIXER_GPR1_LAYER:
		s5p_mixer_set_show(layer, true);
		break;
	default:
		dev_err(s5ptv_status.dev_fb, "invalid layer\n");
		return -EINVAL;
	}

	return 0;
}

int s5p_mixer_ctrl_disable_layer(enum s5p_mixer_layer layer)
{
	switch (layer) {
	case MIXER_VIDEO_LAYER:
	case MIXER_GPR0_LAYER:
	case MIXER_GPR1_LAYER:
		s5p_mixer_set_show(layer, false);
		break;
	default:
		dev_err(s5ptv_status.dev_fb, "invalid layer\n");
		return -EINVAL;
	}

	return 0;
}

int s5p_mixer_ctrl_set_priority(enum s5p_mixer_layer layer, u32 prio)
{
	if ((prio < 0) || (prio > 15)) {
		dev_err(s5ptv_status.dev_fb, "layer priority range : 0 - 15\n");
		return -EINVAL;
	}

	s5p_mixer_ctrl_private.layer[layer].priority = prio;

	switch (layer) {
	case MIXER_VIDEO_LAYER:
	case MIXER_GPR0_LAYER:
	case MIXER_GPR1_LAYER:
		s5p_mixer_set_priority(layer, prio);
		break;
	default:
		dev_err(s5ptv_status.dev_fb, "invalid layer\n");
		return -EINVAL;
	}

	return 0;
}

int s5p_mixer_ctrl_set_dst_win_pos(enum s5p_mixer_layer layer,
				u32 dst_x, u32 dst_y, u32 w, u32 h)
{
	u32 w_t, h_t;
	struct s5p_tv_status *st = &s5ptv_status;

	/*
	 * When tvout resolution was overscanned, there is no
	 * adjust method in H/W. So, framebuffer should be resized.
	 * In this case - TV w/h is greater than FB w/h, grp layer's
	 * dst offset must be changed to fix tv screen.
	 */

	switch (st->tvout_param.disp) {
	case TVOUT_NTSC_M:
	case TVOUT_480P_60_16_9:
	case TVOUT_480P_60_4_3:
	case TVOUT_480P_59:
		w_t = 720;
		h_t = 480;
		break;

	case TVOUT_576P_50_16_9:
	case TVOUT_576P_50_4_3:
		w_t = 720;
		h_t = 576;
		break;

	case TVOUT_720P_60:
	case TVOUT_720P_59:
	case TVOUT_720P_50:
		w_t = 1280;
		h_t = 720;
		break;

	case TVOUT_1080I_60:
	case TVOUT_1080I_59:
	case TVOUT_1080I_50:
	case TVOUT_1080P_60:
	case TVOUT_1080P_59:
	case TVOUT_1080P_50:
	case TVOUT_1080P_30:
		w_t = 1920;
		h_t = 1080;
		break;

	default:
		w_t = 0;
		h_t = 0;
		break;
	}

	if (dst_x < 0)
		dst_x = 0;

	if (dst_y < 0)
		dst_y = 0;

	if (dst_x + w > w_t)
		dst_x = w_t - w;

	if (dst_y + h > h_t)
		dst_y = h_t - h;

	GRPPRINTK("x = %d, y = %d\n", dst_x, dst_y);
	s5p_mixer_ctrl_private.layer[layer].dst_x = dst_x;
	s5p_mixer_ctrl_private.layer[layer].dst_y = dst_y;

	switch (layer) {
	case MIXER_GPR0_LAYER:
	case MIXER_GPR1_LAYER:
		s5p_mixer_set_grp_layer_dst_pos(layer, dst_x, dst_y);
		break;
	default:
		dev_err(s5ptv_status.dev_fb, "invalid layer\n");
		return -EINVAL;
	}

	return 0;
}

int s5p_mixer_ctrl_set_src_win_pos(enum s5p_mixer_layer layer,
				u32 src_x, u32 src_y, u32 w, u32 h)
{
	GRPPRINTK("width = %d, height = %d, x offset = %d, y offset = %d\n",
		w, h, src_x, src_y);
	s5p_mixer_ctrl_private.layer[layer].src_x = src_x;
	s5p_mixer_ctrl_private.layer[layer].src_y = src_y;
	s5p_mixer_ctrl_private.layer[layer].width = w;
	s5p_mixer_ctrl_private.layer[layer].height = h;

	switch (layer) {
	case MIXER_GPR0_LAYER:
	case MIXER_GPR1_LAYER:
		s5p_mixer_set_grp_layer_src_pos(layer, src_x, src_y, w, w, h);
		break;
	default:
		dev_err(s5ptv_status.dev_fb, "invalid layer\n");
		return -EINVAL;
	}

	return 0;
}

int s5p_mixer_ctrl_set_buffer_address(enum s5p_mixer_layer layer,
				dma_addr_t start_addr)
{
	GRPPRINTK("TV frame buffer base address = 0x%x\n", start_addr);
	s5p_mixer_ctrl_private.layer[layer].fb_addr = start_addr;

	switch (layer) {
	case MIXER_GPR0_LAYER:
	case MIXER_GPR1_LAYER:
		s5p_mixer_set_grp_base_address(layer, start_addr);
		break;
	default:
		dev_err(s5ptv_status.dev_fb, "invalid layer\n");
		return -EINVAL;
	}

	return 0;
}

int s5p_mixer_ctrl_set_chroma_key(enum s5p_mixer_layer layer,
				struct s5ptvfb_chroma chroma)
{
	bool enabled = (chroma.enabled) ? true : false;

	s5p_mixer_ctrl_private.layer[layer].chroma_enable = enabled;
	s5p_mixer_ctrl_private.layer[layer].chroma_key = chroma.key;

	switch (layer) {
	case MIXER_GPR0_LAYER:
	case MIXER_GPR1_LAYER:
		s5p_mixer_set_chroma_key(layer, enabled, chroma.key);
		break;
	default:
		dev_err(s5ptv_status.dev_fb, "invalid layer\n");
		return -EINVAL;
	}

	return 0;
}

int s5p_mixer_ctrl_set_alpha_blending(enum s5p_mixer_layer layer,
			enum s5ptvfb_alpha_t blend_mode, unsigned int alpha)
{
	switch (layer) {
	case MIXER_GPR0_LAYER:
	case MIXER_GPR1_LAYER:
		break;
	default:
		dev_err(s5ptv_status.dev_fb, "invalid layer\n");
		return -EINVAL;
	}

	switch (blend_mode) {
	case PIXEL_BLENDING:
		s5p_mixer_ctrl_private.layer[layer].pixel_blend = true;
		s5p_mixer_set_pixel_blend(layer, true);
		break;
	case LAYER_BLENDING:
		s5p_mixer_ctrl_private.layer[layer].layer_blend = true;
		s5p_mixer_ctrl_private.layer[layer].alpha = alpha;
		s5p_mixer_set_layer_blend(layer, true);
		s5p_mixer_set_alpha(layer, alpha);
		break;
	case NONE_BLENDING:
		s5p_mixer_ctrl_private.layer[layer].pixel_blend = false;
		s5p_mixer_ctrl_private.layer[layer].layer_blend = false;
		s5p_mixer_set_pixel_blend(layer, false);
		s5p_mixer_set_layer_blend(layer, false);
		break;
	default:
		dev_err(s5ptv_status.dev_fb, "invalid blending mode\n");
		return -EINVAL;
	}

	return 0;
}

int s5p_mixer_ctrl_scaling(enum s5p_mixer_layer layer,
			struct s5ptvfb_user_scaling scaling)
{
	u32 ver_val, hor_val;

	switch (layer) {
	case MIXER_GPR0_LAYER:
	case MIXER_GPR1_LAYER:
		break;
	default:
		dev_err(s5ptv_status.dev_fb, "invalid layer\n");
		return -EINVAL;
	}

	switch (scaling.ver) {
	case VERTICAL_X1:
		ver_val = 0;
		break;
	case VERTICAL_X2:
		ver_val = 1;
		break;
	default:
		dev_err(s5ptv_status.dev_fb, "invalid vertical size\n");
		return -EINVAL;
	}

	switch (scaling.hor) {
	case HORIZONTAL_X1:
		hor_val = 0;
		break;
	case HORIZONTAL_X2:
		hor_val = 1;
		break;
	default:
		dev_err(s5ptv_status.dev_fb, "invalid horizontal size\n");
		return -EINVAL;
	}

	s5p_mixer_ctrl_private.layer[layer].ver = scaling.ver;
	s5p_mixer_ctrl_private.layer[layer].hor = scaling.hor;
	s5p_mixer_scaling(layer, ver_val, hor_val);

	return 0;
}

int s5p_mixer_ctrl_init_status_reg(void)
{
	int ret;

	ret = s5p_mixer_init_status_reg(s5p_mixer_ctrl_private.burst,
					s5p_mixer_ctrl_private.endian);
	if (ret != 0)
		return -1;

	return ret;
}

int s5p_mixer_ctrl_init_bg_color(void)
{
	int i, ret;

	for (i = MIXER_BG_COLOR_0; i <= MIXER_BG_COLOR_2; i++) {
		ret = s5p_mixer_set_bg_color(i,
			s5p_mixer_ctrl_private.bg_color[i].color_y,
			s5p_mixer_ctrl_private.bg_color[i].color_cb,
			s5p_mixer_ctrl_private.bg_color[i].color_cr);
		if (ret != 0)
			return -1;
	}

	return ret;
}

#if 0 /* These functions will be removed */
bool s5p_mixer_ctrl_grp_start(enum s5p_mixer_layer vm_layer)
{
	int merr;
	struct s5p_tv_status *st = &s5ptv_status;

	if (!(st->grp_layer_enable[0] || st->grp_layer_enable[1])) {

		merr = s5p_mixer_init_status_reg(st->grp_burst,
					st->grp_endian);

		if (merr != 0)
			return false;
	}

	merr = s5p_mixer_init_layer(s5ptv_status.tvout_param.disp,
				vm_layer,
				true,
				s5ptv_overlay[vm_layer].win_blending,
				s5ptv_overlay[vm_layer].win.global_alpha,
				s5ptv_overlay[vm_layer].priority,
				s5ptv_overlay[vm_layer].fb.fmt.pixelformat,
				s5ptv_overlay[vm_layer].blank_change,
				s5ptv_overlay[vm_layer].pixel_blending,
				s5ptv_overlay[vm_layer].pre_mul,
				s5ptv_overlay[vm_layer].blank_color,
				s5ptv_overlay[vm_layer].base_addr,
				s5ptv_overlay[vm_layer].fb.fmt.bytesperline,
				s5ptv_overlay[vm_layer].win.w.width,
				s5ptv_overlay[vm_layer].win.w.height,
				s5ptv_overlay[vm_layer].win.w.left,
				s5ptv_overlay[vm_layer].win.w.top,
				s5ptv_overlay[vm_layer].dst_rect.left,
				s5ptv_overlay[vm_layer].dst_rect.top,
				s5ptv_overlay[vm_layer].dst_rect.width,
				s5ptv_overlay[vm_layer].dst_rect.height);

	if (merr != 0) {
		GRPPRINTK("can't initialize layer(%d)\n\r", merr);

		return false;
	}

	s5p_mixer_start();

	st->grp_layer_enable[vm_layer] = true;

	return true;
}

bool s5p_mixer_ctrl_grp_stop(enum s5p_mixer_layer vm_layer)
{
	int merr;
	struct s5p_tv_status *st = &s5ptv_status;

	merr = s5p_mixer_set_show(vm_layer, false);

	if (merr != 0)
		return false;

	merr = s5p_mixer_set_priority(vm_layer, 0);

	if (merr != 0)
		return false;

	s5p_mixer_start();

	st->grp_layer_enable[vm_layer] = false;

	return true;
}
#endif

extern int s5p_tv_map_resource_mem(
		struct platform_device *pdev,
		char *name,
		void __iomem **base,
		struct resource **res);

int s5p_mixer_ctrl_constructor(struct platform_device *pdev)
{
	int ret = 0, i;

	ret = s5p_tv_map_resource_mem(
		pdev,
		s5p_mixer_ctrl_private.reg_mem.name,
		&(s5p_mixer_ctrl_private.reg_mem.base),
		&(s5p_mixer_ctrl_private.reg_mem.res));

	if (ret)
		goto err_on_res;

	for (i = ACLK; i < NO_OF_CLK; i++) {
		s5p_mixer_ctrl_private.clk[i].ptr =
			clk_get(&pdev->dev, s5p_mixer_ctrl_private.clk[i].name);

		if (IS_ERR(s5p_mixer_ctrl_private.clk[i].ptr)) {
			printk(KERN_ERR "Failed to find clock %s\n",
				s5p_mixer_ctrl_private.clk[i].name);
			ret = -ENOENT;
			goto err_on_clk;
		}
	}

	s5p_mixer_ctrl_private.irq.no =
		platform_get_irq_byname(pdev, s5p_mixer_ctrl_private.irq.name);

	if (s5p_mixer_ctrl_private.irq.no < 0) {
		printk("Failed to call platform_get_irq_byname() for %s\n",
			s5p_mixer_ctrl_private.irq.name);
		ret = s5p_mixer_ctrl_private.irq.no;
		goto err_on_irq;
	}

	ret = request_irq(
			s5p_mixer_ctrl_private.irq.no,
			s5p_mixer_ctrl_private.irq.handler,
			IRQF_DISABLED,
			s5p_mixer_ctrl_private.irq.name,
			NULL);
	if (ret) {
		printk("Failed to call request_irq() for %s\n",
			s5p_mixer_ctrl_private.irq.name);
		goto err_on_irq;
	}

	s5p_mixer_init(s5p_mixer_ctrl_private.reg_mem.base);
	s5p_mixer_ctrl_init_private(MIXER_GPR0_LAYER);
	s5p_mixer_ctrl_init_private(MIXER_GPR1_LAYER);

	return 0;

err_on_irq:
err_on_clk:
	iounmap(s5p_mixer_ctrl_private.reg_mem.base);
	release_resource(s5p_mixer_ctrl_private.reg_mem.res);
	kfree(s5p_mixer_ctrl_private.reg_mem.res);

err_on_res:
	return ret;
}

void s5p_mixer_ctrl_destructor(void)
{
	int i;

	if (s5p_mixer_ctrl_private.reg_mem.base)
		iounmap(s5p_mixer_ctrl_private.reg_mem.base);

	if (s5p_mixer_ctrl_private.reg_mem.res) {
		release_resource(s5p_mixer_ctrl_private.reg_mem.res);
		kfree(s5p_mixer_ctrl_private.reg_mem.res);
	}

	for (i = ACLK; i < NO_OF_CLK; i++) {
		if (s5p_mixer_ctrl_private.clk[i].ptr) {
			clk_disable(s5p_mixer_ctrl_private.clk[i].ptr);
			clk_put(s5p_mixer_ctrl_private.clk[i].ptr);
		}
	}
}

void s5p_mixer_ctrl_suspend(void)
{
}

void s5p_mixer_ctrl_resume(void)
{
}
