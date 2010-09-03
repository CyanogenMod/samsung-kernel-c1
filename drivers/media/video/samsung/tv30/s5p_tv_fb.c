/* linux/drivers/media/video/samsung/tv20/s5p_tv_fb.c
 *
 * Copyright (c) 2009 Samsung Electronics
 * 		http://www.samsung.com/
 *
 * Frame buffer ftn. file for Samsung TVOUT driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/ioctl.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/uaccess.h>

#include "ver_1/mixer.h"
#include "s5p_tvout_ctrl.h"
#include "s5p_tv_base.h"
#include "s5p_tv_v4l2.h"
#include "s5p_mixer_ctrl.h"

#define FB_INDEX(id)	(id - S5PTV_FB_LAYER0_MINOR)

static struct fb_info *fb[S5PTV_FB_CNT];
static struct s5ptvfb_lcd lcd = {
	.width = 1920,
	.height = 1080,
	.bpp = 32,
	.freq = 60,

	.timing = {
		.h_fp = 49,
		.h_bp = 17,
		.h_sw = 33,
		.v_fp = 4,
		.v_fpe = 1,
		.v_bp = 15,
		.v_bpe = 1,
		.v_sw = 6,
	},

	.polarity = {
		.rise_vclk = 0,
		.inv_hsync = 1,
		.inv_vsync = 1,
		.inv_vden = 0,
	},
};

static inline unsigned int s5p_tv_fb_chan_to_field(unsigned int chan,
						struct fb_bitfield bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf.length;

	return chan << bf.offset;
}

static int s5p_tv_fb_set_alpha_info(struct fb_var_screeninfo *var,
				struct s5ptvfb_window *win)
{
	if (var->transp.length > 0)
		win->alpha.mode = PIXEL_BLENDING;
	else
		win->alpha.mode = NONE_BLENDING;

	return 0;
}

static int s5p_tv_fb_release_window(int id)
{
	struct s5ptvfb_window *win = fb[FB_INDEX(id)]->par;

	win->x = 0;
	win->y = 0;

	return 0;
}

int s5p_tv_fb_map_video_memory(int id)
{
	enum s5p_mixer_layer layer;
	struct s5ptvfb_window *win = fb[FB_INDEX(id)]->par;
	struct fb_fix_screeninfo *fix = &fb[FB_INDEX(id)]->fix;

	if (win->path == DATA_PATH_FIFO)
		return 0;

	fb[FB_INDEX(id)]->screen_base = dma_alloc_writecombine(win->dev_fb,
			PAGE_ALIGN(fix->smem_len),
			(unsigned int *) &fix->smem_start, GFP_KERNEL);

	switch (id) {
	case S5PTV_FB_LAYER0_MINOR:
		layer = MIXER_GPR0_LAYER;
		break;
	case S5PTV_FB_LAYER1_MINOR:
		layer = MIXER_GPR1_LAYER;
		break;
	default:
		dev_err(win->dev_fb, "invalid layer\n");
		return -EINVAL;
	}
	s5p_mixer_ctrl_init_fb_addr_phy(layer, fix->smem_start);

	if (!fb[FB_INDEX(id)]->screen_base)
		return -ENOMEM;
	else
		dev_info(win->dev_fb,
			"[fb%d] dma: 0x%08x, cpu: 0x%08x,size: 0x%08x\n",
			win->id, (unsigned int) fix->smem_start,
			(unsigned int) fb[FB_INDEX(id)]->screen_base,
			fix->smem_len);

	memset(fb[FB_INDEX(id)]->screen_base, 0, fix->smem_len);

	return 0;
}

static int s5p_tv_fb_set_bitfield(struct fb_var_screeninfo *var)
{
	switch (var->bits_per_pixel) {
	case 16:
		if (var->transp.length == 1) {
			var->red.offset = 10;
			var->red.length = 5;
			var->green.offset = 5;
			var->green.length = 5;
			var->blue.offset = 0;
			var->blue.length = 5;
			var->transp.offset = 15;
		} else if (var->transp.length == 4) {
			var->red.offset = 8;
			var->red.length = 4;
			var->green.offset = 4;
			var->green.length = 4;
			var->blue.offset = 0;
			var->blue.length = 4;
			var->transp.offset = 12;
		} else {
			var->red.offset = 11;
			var->red.length = 5;
			var->green.offset = 5;
			var->green.length = 6;
			var->blue.offset = 0;
			var->blue.length = 5;
			var->transp.offset = 0;
		}
		break;

	case 24:
		var->red.offset = 16;
		var->red.length = 8;
		var->green.offset = 8;
		var->green.length = 8;
		var->blue.offset = 0;
		var->blue.length = 8;
		var->transp.offset = 0;
		var->transp.length = 0;
		break;

	case 32:
		var->red.offset = 16;
		var->red.length = 8;
		var->green.offset = 8;
		var->green.length = 8;
		var->blue.offset = 0;
		var->blue.length = 8;
		var->transp.offset = 24;
		break;
	}

	return 0;
}

int s5p_tv_fb_draw_logo(struct fb_info *fb)
{
	return 0;
}

static int s5p_tv_fb_cursor(struct fb_info *info, struct fb_cursor *cursor)
{
	/* nothing to do for removing cursor */
	return 0;
}

static int s5p_tv_fb_setcolreg(unsigned int regno, unsigned int red,
				unsigned int green, unsigned int blue,
				unsigned int transp, struct fb_info *fb)
{
	unsigned int *pal = (unsigned int *) fb->pseudo_palette;
	unsigned int val = 0;

	if (regno < 16) {
		/* fake palette of 16 colors */
		val |= s5p_tv_fb_chan_to_field(red, fb->var.red);
		val |= s5p_tv_fb_chan_to_field(green, fb->var.green);
		val |= s5p_tv_fb_chan_to_field(blue, fb->var.blue);
		val |= s5p_tv_fb_chan_to_field(transp, fb->var.transp);

		pal[regno] = val;
	}

	return 0;
}

static int s5p_tv_fb_pan_display(struct fb_var_screeninfo *var,
				struct fb_info *fb)
{
	dma_addr_t start_addr;
	enum s5p_mixer_layer layer;
	struct s5ptvfb_window *win = fb->par;
	struct fb_fix_screeninfo *fix = &fb->fix;

	if (var->yoffset + var->yres > var->yres_virtual) {
		dev_err(win->dev_fb, "invalid yoffset value\n");
		return -EINVAL;
	}

	fb->var.yoffset = var->yoffset;

	dev_dbg(win->dev_fb, "[fb%d] yoffset for pan display: %d\n",
		win->id,
		var->yoffset);

	switch (fb->node) {
	case S5PTV_FB_LAYER0_MINOR:
		layer = MIXER_GPR0_LAYER;
		break;
	case S5PTV_FB_LAYER1_MINOR:
		layer = MIXER_GPR1_LAYER;
		break;
	default:
		dev_err(win->dev_fb, "invalid layer\n");
		return -EINVAL;
	}

	start_addr = fix->smem_start + (var->xres_virtual *
				(var->bits_per_pixel / 8) * var->yoffset);

	s5p_mixer_ctrl_set_buffer_address(layer, start_addr);

	return 0;
}

static int s5p_tv_fb_blank(int blank_mode, struct fb_info *fb)
{
	struct s5ptvfb_window *win = fb->par;
	enum s5p_mixer_layer layer = MIXER_GPR0_LAYER;

	dev_dbg(win->dev_fb, "change blank mode\n");

	switch (fb->node) {
	case S5PTV_FB_LAYER0_MINOR:
		layer = MIXER_GPR0_LAYER;
		break;
	case S5PTV_FB_LAYER1_MINOR:
		layer = MIXER_GPR1_LAYER;
		break;
	}

	switch (blank_mode) {
	case FB_BLANK_UNBLANK:
		if (fb->fix.smem_start) {
			s5p_mixer_ctrl_enable_layer(layer);
		} else
			dev_info(win->dev_fb,
				"[fb%d] no allocated memory for unblank\n",
				win->id);
		break;

	case FB_BLANK_POWERDOWN:
		s5p_mixer_ctrl_disable_layer(layer);
		break;

	default:
		dev_dbg(win->dev_fb, "unsupported blank mode\n");
		/* return -EINVAL; */
	}

	return 0;
}

int s5p_tv_fb_set_par(struct fb_info *fb)
{
	u32 bpp;
	u32 src_x, src_y, w, h;
	struct s5ptvfb_window *win = fb->par;
	enum s5p_mixer_layer layer = MIXER_GPR0_LAYER;

	dev_dbg(win->dev_fb, "[fb%d] set_par\n", win->id);

	if (!fb->fix.smem_start) {
#ifndef CONFIG_USER_ALLOC_TVOUT
		printk(KERN_INFO " The frame buffer is allocated here\n");
		s5p_tv_fb_map_video_memory(win->id);
#else
		printk(KERN_ERR
		"[Warning] The frame buffer should be allocated by ioctl\n");
#endif
	}

	bpp = fb->var.bits_per_pixel;
	w = fb->var.xres;
	h = fb->var.yres;
	src_x = fb->var.xoffset;
	src_y = fb->var.yoffset;

	switch (fb->node) {
	case S5PTV_FB_LAYER0_MINOR:
		layer = MIXER_GPR0_LAYER;
		break;
	case S5PTV_FB_LAYER1_MINOR:
		layer = MIXER_GPR1_LAYER;
		break;
	}

	s5p_mixer_ctrl_init_grp_layer(layer);
	s5p_mixer_ctrl_set_pixel_format(layer, bpp);
	s5p_mixer_ctrl_set_src_win_pos(layer, src_x, src_y, w, h);
	s5p_mixer_ctrl_set_alpha_blending(layer, win->alpha.mode,
				win->alpha.value);
	return 0;
}

int s5p_tv_fb_check_var(struct fb_var_screeninfo *var, struct fb_info *fb)
{
	struct fb_fix_screeninfo *fix = &fb->fix;
	struct s5ptvfb_window *win = fb->par;

	dev_dbg(win->dev_fb, "[fb%d] check_var\n", win->id);

	if (var->bits_per_pixel != 16 && var->bits_per_pixel != 24 &&
		var->bits_per_pixel != 32) {
		dev_err(win->dev_fb, "invalid bits per pixel\n");
		return -EINVAL;
	}

	if (var->xres > lcd.width)
		var->xres = lcd.width;

	if (var->yres > lcd.height)
		var->yres = lcd.height;

	if (var->xres_virtual != var->xres)
		var->xres_virtual = var->xres;

	if (var->yres_virtual > var->yres * (fb->fix.ypanstep + 1))
		var->yres_virtual = var->yres * (fb->fix.ypanstep + 1);

	if (var->xoffset != 0)
		var->xoffset = 0;

	if (var->yoffset + var->yres > var->yres_virtual)
		var->yoffset = var->yres_virtual - var->yres;

	if (win->x + var->xres > lcd.width)
		win->x = lcd.width - var->xres;

	if (win->y + var->yres > lcd.height)
		win->y = lcd.height - var->yres;

	/* modify the fix info */
	fix->line_length = var->xres_virtual * var->bits_per_pixel / 8;
	fix->smem_len = fix->line_length * var->yres_virtual;

	s5p_tv_fb_set_bitfield(var);
	s5p_tv_fb_set_alpha_info(var, win);

	return 0;
}

static int s5p_tv_fb_release(struct fb_info *fb, int user)
{
	int ret;
	struct s5ptvfb_window *win = fb->par;

	s5p_tv_fb_release_window(win->id);

	s5p_tvout_ctrl_stop();

	//s5ptv_status.hdcp_en = false;

	//s5ptv_status.tvout_output_enable = false;

	ret = s5p_tv_clk_gate(false);
	if (ret < 0) {
		printk(KERN_ERR "[Error]Cannot release\n");
		return -1;
	}
	s5p_tv_phy_power(false);

	mutex_lock(&s5ptv_status.fb_lock);
	atomic_dec(&win->in_use);
	mutex_unlock(&s5ptv_status.fb_lock);

	return 0;
}

static int s5p_tv_fb_ioctl(struct fb_info *fb, unsigned int cmd,
			unsigned long arg)
{
	dma_addr_t start_addr;
	enum s5p_mixer_layer layer;
	struct fb_var_screeninfo *var = &fb->var;
	struct s5ptvfb_window *win = fb->par;
	int ret = 0;
	void *argp = (void *) arg;

	union {
		struct s5ptvfb_user_window user_window;
		struct s5ptvfb_user_plane_alpha user_alpha;
		struct s5ptvfb_user_chroma user_chroma;
		struct s5ptvfb_user_scaling user_scaling;
		int vsync;
	} p;

	switch (fb->node) {
	case S5PTV_FB_LAYER0_MINOR:
		layer = MIXER_GPR0_LAYER;
		break;
	case S5PTV_FB_LAYER1_MINOR:
		layer = MIXER_GPR1_LAYER;
		break;
	default:
		printk(KERN_ERR "[Error] invalid layer\n");
		return -EINVAL;
	}

	switch (cmd) {
	case S5PTVFB_WIN_POSITION:
		if (copy_from_user(&p.user_window,
			(struct s5ptvfb_user_window __user *) arg,
			sizeof(p.user_window)))
			ret = -EFAULT;
		else {
			s5p_mixer_ctrl_set_dst_win_pos(layer, p.user_window.x,
				p.user_window.y, var->xres, var->yres);
		}
		break;

	case S5PTVFB_WIN_SET_PLANE_ALPHA:
		if (copy_from_user(&p.user_alpha,
			(struct s5ptvfb_user_plane_alpha __user *) arg,
			sizeof(p.user_alpha)))
			ret = -EFAULT;
		else {
			win->alpha.mode = LAYER_BLENDING;
			win->alpha.value = p.user_alpha.alpha;
			s5p_mixer_ctrl_set_alpha_blending(layer,
				win->alpha.mode, win->alpha.value);
		}
		break;
	case S5PTVFB_WIN_SET_CHROMA:
		if (copy_from_user(&p.user_chroma,
			(struct s5ptvfb_user_chroma __user *) arg,
			sizeof(p.user_chroma)))
			ret = -EFAULT;
		else {
			win->chroma.enabled = p.user_chroma.enabled;
			win->chroma.key = S5PTVFB_CHROMA(p.user_chroma.red,
						p.user_chroma.green,
						p.user_chroma.blue);

			s5p_mixer_ctrl_set_chroma_key(layer, win->chroma);
		}
		break;

	case S5PTVFB_WIN_SET_ADDR:
		fb->fix.smem_start = (unsigned long)argp;
		start_addr = fb->fix.smem_start + (var->xres_virtual *
				(var->bits_per_pixel / 8) * var->yoffset);
		s5p_mixer_ctrl_set_buffer_address(layer, start_addr);
		break;
	case S5PTVFB_SCALING:
		if (copy_from_user(&p.user_scaling,
			(struct s5ptvfb_user_scaling __user *) arg,
			sizeof(p.user_scaling)))
			ret = -EFAULT;
		else
			s5p_mixer_ctrl_scaling(layer, p.user_scaling);
		break;
	}

	return 0;
}

static int s5p_tv_fb_open(struct fb_info *fb, int user)
{
	struct s5ptvfb_window *win = fb->par;
	enum s5p_mixer_layer layer = MIXER_GPR0_LAYER;
	int ret = 0;

	ret = s5p_tv_clk_gate(true);
	if (ret < 0) {
		printk(KERN_ERR "[Error]Cannot open it\n");
		return -1;
	}

	s5p_tv_phy_power(true);

	s5p_tv_v4l2_init_param();

	/*      s5ptv_status.tvout_param.disp = TVOUT_720P_60; */
	s5ptv_status.tvout_param.out  = TVOUT_HDMI_RGB;

	s5p_tvout_ctrl_start(
		s5ptv_status.tvout_param.disp, s5ptv_status.tvout_param.out);

	switch (win->id) {
	case S5PTV_FB_LAYER0_MINOR:
		layer = MIXER_GPR0_LAYER;
		break;
	case S5PTV_FB_LAYER1_MINOR:
		layer = MIXER_GPR1_LAYER;
		break;
	}

#ifndef CONFIG_USER_ALLOC_TVOUT
	s5p_mixer_ctrl_enable_layer(layer);
#endif

	mutex_lock(&s5ptv_status.fb_lock);

	if (atomic_read(&win->in_use)) {
		dev_dbg(s5ptv_status.dev_fb,
		"do not allow multiple open "
		"for window\n");
		ret = -EBUSY;

	} else {
		atomic_inc(&win->in_use);
	}

	mutex_unlock(&s5ptv_status.fb_lock);

	return ret;
}

struct fb_ops s5ptvfb_ops = {
	.owner = THIS_MODULE,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
	.fb_check_var = s5p_tv_fb_check_var,
	.fb_set_par = s5p_tv_fb_set_par,
	.fb_blank = s5p_tv_fb_blank,
	.fb_pan_display = s5p_tv_fb_pan_display,
	.fb_setcolreg = s5p_tv_fb_setcolreg,
	.fb_cursor = s5p_tv_fb_cursor,
	.fb_ioctl = s5p_tv_fb_ioctl,
	.fb_open = s5p_tv_fb_open,
	.fb_release = s5p_tv_fb_release,
};

int s5p_tv_fb_init_fbinfo(int id, struct device *dev_fb)
{
	struct fb_fix_screeninfo *fix = &fb[FB_INDEX(id)]->fix;
	struct fb_var_screeninfo *var = &fb[FB_INDEX(id)]->var;
	struct s5ptvfb_window *win = fb[FB_INDEX(id)]->par;
	struct s5ptvfb_alpha *alpha = &win->alpha;

	memset(win, 0, sizeof(struct s5ptvfb_window));

	platform_set_drvdata(to_platform_device(dev_fb), fb[FB_INDEX(id)]);

	strcpy(fix->id, S5PTVFB_NAME);

	/* fimd specific */
	win->id = id;
	win->path = DATA_PATH_DMA;
	win->dma_burst = 16;
	win->dev_fb = dev_fb;
	alpha->mode = LAYER_BLENDING;
	alpha->value = 0xff;

	/* fbinfo */
	fb[FB_INDEX(id)]->fbops = &s5ptvfb_ops;
	fb[FB_INDEX(id)]->flags = FBINFO_FLAG_DEFAULT;
	fb[FB_INDEX(id)]->pseudo_palette = &win->pseudo_pal;
	fix->xpanstep = 0;
	fix->ypanstep = 0;
	fix->type = FB_TYPE_PACKED_PIXELS;
	fix->accel = FB_ACCEL_NONE;
	fix->visual = FB_VISUAL_TRUECOLOR;
	var->xres = lcd.width;
	var->yres = lcd.height;
	var->xres_virtual = var->xres;
	var->yres_virtual = var->yres + (var->yres * fix->ypanstep);
	var->bits_per_pixel = 32;
	var->xoffset = 0;
	var->yoffset = 0;
	var->width = 0;
	var->height = 0;
	var->transp.length = 0;

	fix->line_length = var->xres_virtual * var->bits_per_pixel / 8;
	fix->smem_len = fix->line_length * var->yres_virtual;

	var->nonstd = 0;
	var->activate = FB_ACTIVATE_NOW;
	var->vmode = FB_VMODE_NONINTERLACED;
	var->hsync_len = lcd.timing.h_sw;
	var->vsync_len = lcd.timing.v_sw;
	var->left_margin = lcd.timing.h_fp;
	var->right_margin = lcd.timing.h_bp;
	var->upper_margin = lcd.timing.v_fp;
	var->lower_margin = lcd.timing.v_bp;

	var->pixclock = lcd.freq * (var->left_margin + var->right_margin +
				var->hsync_len + var->xres) *
				(var->upper_margin + var->lower_margin +
				var->vsync_len + var->yres);

	dev_dbg(dev_fb, "pixclock: %d\n", var->pixclock);

	s5p_tv_fb_set_bitfield(var);
	s5p_tv_fb_set_alpha_info(var, win);

	return 0;
}

int s5p_tv_fb_alloc_framebuffer(struct device *dev_fb)
{
	int ret, i;

	/* alloc for each framebuffer */
	for (i = 0; i < S5PTV_FB_CNT; i++) {
		fb[i] = framebuffer_alloc(sizeof(struct s5ptvfb_window),
			dev_fb);
		if (!fb[i]) {
			dev_err(dev_fb, "not enough memory\n");
			ret = -ENOMEM;
			goto err_alloc_fb;
		}

		ret = s5p_tv_fb_init_fbinfo(i + S5PTV_FB_LAYER0_MINOR, dev_fb);
		if (ret) {
			dev_err(dev_fb, "failed to allocate memory for fb for tv\n");
			ret = -ENOMEM;
			goto err_alloc_fb;
		}

#ifndef CONFIG_USER_ALLOC_TVOUT
		if (s5p_tv_fb_map_video_memory(i + S5PTV_FB_LAYER0_MINOR)) {
			dev_err(dev_fb, "failed to map video memory "
				"for default window \n");
			ret = -ENOMEM;

			goto err_alloc_fb;
		}
#endif
	}

	return 0;

err_alloc_fb:
	for (i = 0; i < S5PTV_FB_CNT; i++) {
		if (fb[i])
			framebuffer_release(fb[i]);
	}

	return ret;
}

int s5p_tv_fb_register_framebuffer(struct device *dev_fb)
{
	int ret, i = 0;

	do {
		ret = register_framebuffer(fb[0]);
		if (ret) {
			dev_err(dev_fb, "failed to register "
				"framebuffer device\n");
			return -EINVAL;
		}
	} while (fb[0]->node < S5PTV_FB_LAYER0_MINOR);

	for (i = 1; i < S5PTV_FB_CNT; i++) {
		ret = register_framebuffer(fb[i]);
		if (ret) {
			dev_err(dev_fb, "failed to register "
				"framebuffer device\n");
			return -EINVAL;
		}
	}

	for (i = 0; i < S5PTV_FB_CNT; i++)
		printk("fb[%d] = %d\n", i, fb[i]->node);

	for (i = 0; i < S5PTV_FB_CNT; i++) {
#ifndef CONFIG_FRAMEBUFFER_CONSOLE
#ifndef CONFIG_USER_ALLOC_TVOUT
		s5p_tv_fb_check_var(&fb[i]->var, fb[i]);
		s5p_tv_fb_set_par(fb[i]);
		s5p_tv_fb_draw_logo(fb[i]);
#endif
#endif
	}

	return 0;
}
