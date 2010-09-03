/* linux/drivers/media/video/samsung/tv20/s5p_tv_v4l2.c
 *
 * Copyright (c) 2009 Samsung Electronics
 *		http://www.samsung.com/
 *
 * V4L2 API file for Samsung TVOOUT driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/string.h>
#include <linux/version.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

#include <media/v4l2-common.h>
#include <media/v4l2-ioctl.h>

#include "s5p_tv.h"
#include "s5p_tv_v4l2.h"

#include "s5p_stda_grp.h"
#include "s5p_tvout_ctrl.h"

#ifdef COFIG_TVOUT_DBG
#define S5P_V4L2_DEBUG 1
#endif

#ifdef S5P_V4L2_DEBUG
#define V4L2PRINTK(fmt, args...)\
	printk(KERN_INFO "[V4L2_IF] %s: " fmt, __func__ , ## args)
#else
#define V4L2PRINTK(fmt, args...)
#endif

/* start: external functions for HDMI */
bool s5p_tvout_ctrl_set_std_if(
		enum s5p_tv_disp_mode std,
		enum s5p_tv_o_mode inf);
/* end: external functions for HDMI */

/* start: external functions for HDMI */
#include "ver_1/hdmi.h"

extern int s5p_hdmi_audio_enable(bool en);
extern void s5p_hdmi_set_bluescreen(bool en);
/* end: external functions for HDMI */

/* start: external functions for VP */
extern void s5p_vp_ctrl_set_src_plane(
		u32 base_y, u32 base_c, u32 width, u32 height,
		enum s5p_vp_src_color color, enum s5p_vp_field field);
extern void s5p_vp_ctrl_set_src_win(u32 left, u32 top, u32 width, u32 height);
extern void s5p_vp_ctrl_set_dest_win(u32 left, u32 top, u32 width, u32 height);
extern void s5p_vp_ctrl_set_dest_win_alpha_val(u32 alpha);
extern void s5p_vp_ctrl_set_dest_win_blend(bool enable);
extern void s5p_vp_ctrl_set_dest_win_priority(u32 prio);
extern bool s5p_vp_ctrl_start(void);
extern void s5p_vp_ctrl_stop(void);
/* end: external functions for VP */

/* temporary used for testing system mmu */
extern void *s5p_getaddress(unsigned int cookie);

#define CVBS_S_VIDEO (V4L2_STD_NTSC_M | V4L2_STD_NTSC_M_JP| \
	V4L2_STD_PAL | V4L2_STD_PAL_M | V4L2_STD_PAL_N | V4L2_STD_PAL_Nc | \
	V4L2_STD_PAL_60 | V4L2_STD_NTSC_443)

static struct v4l2_output s5p_tv_outputs[] = {
	{
		.index		= 0,
		.name		= "Analog  COMPOSITE",
		.type		= V4L2_OUTPUT_TYPE_COMPOSITE,
		.audioset	= 0,
		.modulator	= 0,
		.std		= CVBS_S_VIDEO,
	}, {
		.index		= 1,
		.name		= "Digital HDMI(YCbCr)",
		.type		= V4L2_OUTPUT_TYPE_HDMI,
		.audioset	= 2,
		.modulator	= 0,
		.std		= V4L2_STD_480P_60_16_9 |
				V4L2_STD_480P_60_16_9 | V4L2_STD_720P_60 |
				V4L2_STD_720P_50
				| V4L2_STD_1080P_60 | V4L2_STD_1080P_50 |
				V4L2_STD_1080I_60 | V4L2_STD_1080I_50 |
				V4L2_STD_480P_59 | V4L2_STD_720P_59 |
				V4L2_STD_1080I_59 | V4L2_STD_1080P_59 |
				V4L2_STD_1080P_30,
	}, {
		.index		= 2,
		.name		= "Digital HDMI(RGB)",
		.type		= V4L2_OUTPUT_TYPE_HDMI_RGB,
		.audioset	= 2,
		.modulator	= 0,
		.std		= V4L2_STD_480P_60_16_9 |
				V4L2_STD_480P_60_16_9 |
				V4L2_STD_720P_60 | V4L2_STD_720P_50
				| V4L2_STD_1080P_60 | V4L2_STD_1080P_50 |
				V4L2_STD_1080I_60 | V4L2_STD_1080I_50 |
				V4L2_STD_480P_59 | V4L2_STD_720P_59 |
				V4L2_STD_1080I_59 | V4L2_STD_1080P_59 |
				V4L2_STD_1080P_30,
	}, {
		.index		= 3,
		.name		= "Digital DVI",
		.type		= V4L2_OUTPUT_TYPE_DVI,
		.audioset	= 2,
		.modulator	= 0,
		.std		= V4L2_STD_480P_60_16_9 |
				V4L2_STD_480P_60_16_9 |
				V4L2_STD_720P_60 | V4L2_STD_720P_50
				| V4L2_STD_1080P_60 | V4L2_STD_1080P_50 |
				V4L2_STD_1080I_60 | V4L2_STD_1080I_50 |
				V4L2_STD_480P_59 | V4L2_STD_720P_59 |
				V4L2_STD_1080I_59 | V4L2_STD_1080P_59 |
				V4L2_STD_1080P_30,
	}

};

const struct v4l2_fmtdesc s5p_tv_o_fmt_desc[] = {
	{
		.index    	= 0,
		.type     	= V4L2_BUF_TYPE_VIDEO_OUTPUT,
		.description 	= "YUV420, NV12 (Video Processor)",
		.pixelformat   	= V4L2_PIX_FMT_NV12,
		.flags    	= FORMAT_FLAGS_CrCb,
	}
};

const struct v4l2_fmtdesc s5p_tv_o_overlay_fmt_desc[] = {
	{
		.index    	= 0,
		.type     	= V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY,
		.description 	= "16bpp RGB, le - RGB[565]",
		.pixelformat   	= V4L2_PIX_FMT_RGB565,
		.flags    	= FORMAT_FLAGS_PACKED,
	}, {
		.index    	= 1,
		.type     	= V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY,
		.description 	= "16bpp RGB, le - ARGB[1555]",
		.pixelformat   	= V4L2_PIX_FMT_RGB555,
		.flags    	= FORMAT_FLAGS_PACKED,
	}, {
		.index    	= 2,
		.type     	= V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY,
		.description 	= "16bpp RGB, le - ARGB[4444]",
		.pixelformat   	= V4L2_PIX_FMT_RGB444,
		.flags    	= FORMAT_FLAGS_PACKED,
	}, {
		.index    	= 3,
		.type     	= V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY,
		.description 	= "32bpp RGB, le - ARGB[8888]",
		.pixelformat   	= V4L2_PIX_FMT_RGB32,
		.flags    	= FORMAT_FLAGS_PACKED,
	}
};

const struct v4l2_standard s5p_tv_standards[] = {
	{
		.index  = 0,
		.id     = V4L2_STD_NTSC_M,
		.name	= "NTSC_M",
	}, {

		.index	= 1,
		.id	= V4L2_STD_PAL_BDGHI,
		.name	= "PAL_BDGHI",
	}, {
		.index  = 2,
		.id     = V4L2_STD_PAL_M,
		.name 	= "PAL_M",
	}, {
		.index  = 3,
		.id     = V4L2_STD_PAL_N,
		.name 	= "PAL_N",
	}, {
		.index  = 4,
		.id     = V4L2_STD_PAL_Nc,
		.name 	= "PAL_Nc",
	}, {
		.index  = 5,
		.id     = V4L2_STD_PAL_60,
		.name 	= "PAL_60",
	}, {
		.index  = 6,
		.id     = V4L2_STD_NTSC_443,
		.name 	= "NTSC_443",
	}, {
		.index  = 7,
		.id     = V4L2_STD_480P_60_16_9,
		.name 	= "480P_60_16_9",
	}, {
		.index  = 8,
		.id     = V4L2_STD_480P_60_4_3,
		.name 	= "480P_60_4_3",
	}, {
		.index  = 9,
		.id     = V4L2_STD_576P_50_16_9,
		.name 	= "576P_50_16_9",
	}, {
		.index  = 10,
		.id     = V4L2_STD_576P_50_4_3,
		.name 	= "576P_50_4_3",
	}, {
		.index  = 11,
		.id     = V4L2_STD_720P_60,
		.name 	= "720P_60",
	}, {
		.index  = 12,
		.id     = V4L2_STD_720P_50,
		.name 	= "720P_50",
	},
	{
		.index  = 13,
		.id     = V4L2_STD_1080P_60,
		.name 	= "1080P_60",
	}, {
		.index  = 14,
		.id     = V4L2_STD_1080P_50,
		.name 	= "1080P_50",
	}, {
		.index  = 15,
		.id     = V4L2_STD_1080I_60,
		.name 	= "1080I_60",
	}, {
		.index  = 16,
		.id     = V4L2_STD_1080I_50,
		.name 	= "1080I_50",
	}, {
		.index  = 17,
		.id     = V4L2_STD_480P_59,
		.name 	= "480P_59",
	}, {
		.index  = 18,
		.id     = V4L2_STD_720P_59,
		.name 	= "720P_59",
	}, {
		.index  = 19,
		.id     = V4L2_STD_1080I_59,
		.name 	= "1080I_59",
	}, {
		.index  = 20,
		.id     = V4L2_STD_1080P_59,
		.name 	= "1080I_50",
	}, {
		.index  = 21,
		.id     = V4L2_STD_1080P_30,
		.name 	= "1080I_30",
	}
};

const struct v4l2_format s5p_tv_format[] = {
	{
		.type = V4L2_BUF_TYPE_VIDEO_OUTPUT,
	}, {
		.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY,
	}, {
		.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY,
	},
};

#define S5P_TVOUT_MAX_STANDARDS \
	ARRAY_SIZE(s5p_tv_standards)
#define S5P_TVOUT_MAX_O_TYPES \
	ARRAY_SIZE(s5p_tv_outputs)
#define S5P_TVOUT_MAX_O_FMT \
	ARRAY_SIZE(s5p_tv_format)
#define S5P_TVOUT_MAX_O_FMT_DESC \
	ARRAY_SIZE(s5p_tv_o_fmt_desc)
#define S5P_TVOUT_MAX_O_OVERLAY_FMT_DESC \
	ARRAY_SIZE(s5p_tv_o_overlay_fmt_desc)


void s5p_tv_v4l2_init_param(void)
{
	s5ptv_status.v4l2.output = (struct v4l2_output *)&s5p_tv_outputs[0];
	s5ptv_status.v4l2.std = (struct v4l2_standard *)&s5p_tv_standards[0];
	s5ptv_status.v4l2.fmt_v = (struct v4l2_format *)&s5p_tv_o_fmt_desc[0];
	s5ptv_status.v4l2.fmt_vo_0 = (struct v4l2_format *)&s5p_tv_format[1];
	s5ptv_status.v4l2.fmt_vo_1 = (struct v4l2_format *)&s5p_tv_format[2];
}

static int s5p_tv_v4l2_querycap(struct file *file, void *fh,
				struct v4l2_capability *cap)
{
	struct s5p_tv_vo *layer = (struct s5p_tv_vo *)fh;
	u32 index;

	if (layer == NULL) {
		index = 0;
		strcpy(cap->driver, "S3C TV Vid drv");
		cap->capabilities = V4L2_CAP_VIDEO_OUTPUT;
	} else {
		index = layer->index + 1;
		strcpy(cap->driver, "S3C TV Grp drv");
		cap->capabilities = V4L2_CAP_VIDEO_OUTPUT_OVERLAY;
	}

	strlcpy(cap->card, s5ptv_status.video_dev[index]->name,
		sizeof(cap->card));

	sprintf(cap->bus_info, "ARM AHB BUS");
	cap->version = KERNEL_VERSION(2, 6, 29);

	return 0;
}

#if 0 /* These functions will be used */
static int s5p_tv_v4l2_enum_fmt_vid_out(struct file *file, void *fh,
					struct v4l2_fmtdesc *f)
{
	int index = f->index;

	if (index >= S5P_TVOUT_MAX_O_FMT_DESC) {
		V4L2PRINTK("exceeded S5P_TVOUT_MAX_O_FMT_DESC\n");

		return -EINVAL;
	}

	memcpy(f, &s5p_tv_o_fmt_desc[index], sizeof(struct v4l2_fmtdesc));

	return 0;

}

static int s5p_tv_v4l2_enum_fmt_vid_out_overlay(struct file *file,
					void *fh, struct v4l2_fmtdesc *f)
{
	int index = f->index;

	if (index >= S5P_TVOUT_MAX_O_OVERLAY_FMT_DESC) {
		V4L2PRINTK("exceeded S5P_TVOUT_MAX_O_OVERLAY_FMT_DESC\n");

		return -EINVAL;
	}

	memcpy(f, &s5p_tv_o_overlay_fmt_desc[index],
			sizeof(struct v4l2_fmtdesc));

	return 0;
}
#endif

static int s5p_tv_v4l2_g_std(struct file *file, void *fh, v4l2_std_id *norm)
{
	*norm = s5ptv_status.v4l2.std->id;

	return 0;
}

static int s5p_tv_v4l2_s_std(struct file *file, void *fh, v4l2_std_id *norm)
{
	unsigned int i = 0;
	v4l2_std_id std_id = *norm;

	s5ptv_status.v4l2.std = NULL;

	do {
		if (s5p_tv_standards[i].id == std_id) {
			s5ptv_status.v4l2.std =
				(struct v4l2_standard *)&s5p_tv_standards[i];
			break;
		}

		i++;
	} while (i < S5P_TVOUT_MAX_STANDARDS);

	if (i >= S5P_TVOUT_MAX_STANDARDS || s5ptv_status.v4l2.std == NULL) {
		V4L2PRINTK("(ERR) There is no tv-out standards :"
			"index = 0x%08Lx\n", std_id);

		return -EINVAL;
	}

	switch (std_id) {
	case V4L2_STD_NTSC_M:
		s5ptv_status.tvout_param.disp = TVOUT_NTSC_M;
		break;

	case V4L2_STD_PAL_BDGHI:
		s5ptv_status.tvout_param.disp = TVOUT_PAL_BDGHI;
		break;

	case V4L2_STD_PAL_M:
		s5ptv_status.tvout_param.disp = TVOUT_PAL_M;
		break;

	case V4L2_STD_PAL_N:
		s5ptv_status.tvout_param.disp = TVOUT_PAL_N;
		break;

	case V4L2_STD_PAL_Nc:
		s5ptv_status.tvout_param.disp = TVOUT_PAL_NC;
		break;

	case V4L2_STD_PAL_60:
		s5ptv_status.tvout_param.disp = TVOUT_PAL_60;
		break;

	case V4L2_STD_NTSC_443:
		s5ptv_status.tvout_param.disp = TVOUT_NTSC_443;
		break;

	case V4L2_STD_480P_60_16_9:
		s5ptv_status.tvout_param.disp = TVOUT_480P_60_16_9;
		break;

	case V4L2_STD_480P_60_4_3:
		s5ptv_status.tvout_param.disp = TVOUT_480P_60_4_3;
		break;

	case V4L2_STD_480P_59:
		s5ptv_status.tvout_param.disp = TVOUT_480P_59;
		break;
	case V4L2_STD_576P_50_16_9:
		s5ptv_status.tvout_param.disp = TVOUT_576P_50_16_9;
		break;

	case V4L2_STD_576P_50_4_3:
		s5ptv_status.tvout_param.disp = TVOUT_576P_50_4_3;
		break;

	case V4L2_STD_720P_60:
		s5ptv_status.tvout_param.disp = TVOUT_720P_60;
		break;

	case V4L2_STD_720P_59:
		s5ptv_status.tvout_param.disp = TVOUT_720P_59;
		break;

	case V4L2_STD_720P_50:
		s5ptv_status.tvout_param.disp = TVOUT_720P_50;
		break;

	case V4L2_STD_1080I_60:
		s5ptv_status.tvout_param.disp = TVOUT_1080I_60;
		break;

	case V4L2_STD_1080I_59:
		s5ptv_status.tvout_param.disp = TVOUT_1080I_59;
		break;

	case V4L2_STD_1080I_50:
		s5ptv_status.tvout_param.disp = TVOUT_1080I_50;
		break;

	case V4L2_STD_1080P_30:
		s5ptv_status.tvout_param.disp = TVOUT_1080P_30;
		break;

	case V4L2_STD_1080P_60:
		s5ptv_status.tvout_param.disp = TVOUT_1080P_60;
		break;

	case V4L2_STD_1080P_59:
		s5ptv_status.tvout_param.disp = TVOUT_1080P_59;
		break;

	case V4L2_STD_1080P_50:
		s5ptv_status.tvout_param.disp = TVOUT_1080P_50;
		break;

	default:
		V4L2PRINTK("(ERR) not supported standard id :"
			"index = 0x%08Lx\n", std_id);

		return -EINVAL;
	}

	return 0;
}

static int s5p_tv_v4l2_enum_output(struct file *file, void *fh,
	struct v4l2_output *a)
{
	unsigned int index = a->index;

	if (index >= S5P_TVOUT_MAX_O_TYPES) {
		V4L2PRINTK("exceeded supported output!!\n");

		return -EINVAL;
	}

	memcpy(a, &s5p_tv_outputs[index], sizeof(struct v4l2_output));

	return 0;
}

static int s5p_tv_v4l2_g_output(struct file *file, void *fh, unsigned int *i)
{
	*i = s5ptv_status.v4l2.output->index;

	return 0;
}

static int s5p_tv_v4l2_s_output(struct file *file, void *fh, unsigned int i)
{

	if (i >= S5P_TVOUT_MAX_O_TYPES)
		return -EINVAL;

	s5ptv_status.v4l2.output = &s5p_tv_outputs[i];

	switch (s5ptv_status.v4l2.output->type) {
	case V4L2_OUTPUT_TYPE_COMPOSITE:
		s5ptv_status.tvout_param.out =
			TVOUT_COMPOSITE;
		break;

	case V4L2_OUTPUT_TYPE_HDMI:
		s5ptv_status.tvout_param.out =
			TVOUT_HDMI;
		break;

	case V4L2_OUTPUT_TYPE_HDMI_RGB:
		s5ptv_status.tvout_param.out =
			TVOUT_HDMI_RGB;
		break;

	case V4L2_OUTPUT_TYPE_DVI:
		s5ptv_status.tvout_param.out =
			TVOUT_DVI;
		break;

	default:
		break;
	}

//	s5p_tvout_ctrl_set_std_if(
//		s5ptv_status.tvout_param.disp,
//		s5ptv_status.tvout_param.out);

	s5p_tvout_ctrl_start(
		s5ptv_status.tvout_param.disp,
		s5ptv_status.tvout_param.out);

	return 0;
};

#if 0 /* These functions will be used */
static int s5p_tv_v4l2_enum_fmt_type_private(
		struct file		*file,
		void			*fh,
		struct v4l2_format	*a)
{
	return 0;
}

static int s5p_tv_v4l2_g_fmt_type_private(
		struct file		*file,
		void			*fh,
		struct v4l2_format	*a)
{
	return 0;
}
#endif

static int s5p_tv_v4l2_s_fmt_type_private(
		struct file		*file,
		void			*fh,
		struct v4l2_format	*a)
{
	struct v4l2_vid_overlay_src	vparam;
	struct v4l2_pix_format		*pix_fmt;
	enum s5p_vp_src_color		color;
	enum s5p_vp_field		field;
	unsigned long base_y, base_c;
	
	memcpy(&vparam, a->fmt.raw_data, sizeof(struct v4l2_vid_overlay_src));

	pix_fmt = &vparam.pix_fmt;

	/* check progressive or not */
	if (pix_fmt->field == V4L2_FIELD_NONE) {
		/* progressive */
		switch (pix_fmt->pixelformat) {
		case V4L2_PIX_FMT_NV12:
			/* linear */
			color = VP_SRC_COLOR_NV12;
			break;

		case V4L2_PIX_FMT_NV12T:
			/* tiled */
			color = VP_SRC_COLOR_TILE_NV12;
			break;

		default:
			V4L2PRINTK("src img format not supported\n");
			goto error_on_s_fmt_type_private;
		}

		field = VP_TOP_FIELD;
	} else if ((pix_fmt->field == V4L2_FIELD_TOP) || 
			(pix_fmt->field == V4L2_FIELD_BOTTOM)) {
		/* interlaced */
		switch (pix_fmt->pixelformat) {
		case V4L2_PIX_FMT_NV12:
			/* linear */
			color = VP_SRC_COLOR_NV12IW;
			break;

		case V4L2_PIX_FMT_NV12T:
			/* tiled */
			color = VP_SRC_COLOR_TILE_NV12IW;
			break;

		default:
			V4L2PRINTK("src img format not supported\n");
			goto error_on_s_fmt_type_private;
		}

		field = (pix_fmt->field == V4L2_FIELD_BOTTOM) ?
				VP_BOTTOM_FIELD : VP_TOP_FIELD;

	} else {
		V4L2PRINTK("this field id not supported\n");

		goto error_on_s_fmt_type_private;
	}

#if defined(CONFIG_S5P_SYSMMU_TV) && defined(CONFIG_S5P_VMEM)
	/*
	 * For TV system mmu test
	 * vparam.base_y : cookie
	 * vparam.base_c : offset of base_c from base_y
	 */
	base_y = (unsigned long) s5p_getaddress((unsigned int)vparam.base_y);
	base_c = base_y + (unsigned long)vparam.base_c;
	s5p_vp_ctrl_set_src_plane(base_y, base_c, pix_fmt->width,
		pix_fmt->height, color, field );
#else
	s5p_vp_ctrl_set_src_plane(vparam.base_y, vparam.base_c,
		pix_fmt->width, pix_fmt->height, color, field );
#endif
	return 0;

error_on_s_fmt_type_private:
	return -1;
}

#if 0 /* This function will be used */
static int s5p_tv_v4l2_g_fmt_vid_overlay(
		struct file		*file,
		void			*fh,
		struct v4l2_format	*a)
{
	return 0;
}
#endif

static int s5p_tv_v4l2_s_fmt_vid_overlay(
		struct file		*file,
		void			*fh,
		struct v4l2_format	*a)
{
	struct v4l2_rect *rect = &a->fmt.win.w;

	s5p_vp_ctrl_set_dest_win_alpha_val(a->fmt.win.global_alpha);
	s5p_vp_ctrl_set_dest_win(
		rect->left, rect->top,
		rect->width, rect->height);

	return 0;
}

#if 0 /* These functions will be used */
static int s5p_tv_v4l2_cropcap(struct file *file, void *fh,
				struct v4l2_cropcap *a)
{
#ifndef OLD
#else
	struct v4l2_cropcap *cropcap = a;

	switch (cropcap->type) {
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		break;

	case V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY:
		break;

	default:
		return -1;
	}

	switch (s5ptv_status.tvout_param.disp) {
	case TVOUT_NTSC_M:
	case TVOUT_NTSC_443:
	case TVOUT_480P_60_16_9:
	case TVOUT_480P_60_4_3:
	case TVOUT_480P_59:
		cropcap->bounds.top = 0;
		cropcap->bounds.left = 0;
		cropcap->bounds.width = 720;
		cropcap->bounds.height = 480;

		cropcap->defrect.top = 0;
		cropcap->defrect.left = 0;
		cropcap->defrect.width = 720;
		cropcap->defrect.height = 480;
		break;

	case TVOUT_PAL_M:
	case TVOUT_PAL_BDGHI:
	case TVOUT_PAL_N:
	case TVOUT_PAL_NC:
	case TVOUT_PAL_60:
	case TVOUT_576P_50_16_9:
	case TVOUT_576P_50_4_3:
		cropcap->bounds.top = 0;
		cropcap->bounds.left = 0;
		cropcap->bounds.width = 720;
		cropcap->bounds.height = 576;

		cropcap->defrect.top = 0;
		cropcap->defrect.left = 0;
		cropcap->defrect.width = 720;
		cropcap->defrect.height = 576;
		break;

	case TVOUT_720P_60:
	case TVOUT_720P_59:
	case TVOUT_720P_50:
		cropcap->bounds.top = 0;
		cropcap->bounds.left = 0;
		cropcap->bounds.width = 1280;
		cropcap->bounds.height = 720;

		cropcap->defrect.top = 0;
		cropcap->defrect.left = 0;
		cropcap->defrect.width = 1280;
		cropcap->defrect.height = 720;
		break;

	case TVOUT_1080I_60:
	case TVOUT_1080I_59:
	case TVOUT_1080I_50:
	case TVOUT_1080P_60:
	case TVOUT_1080P_59:
	case TVOUT_1080P_50:
	case TVOUT_1080P_30:
		cropcap->bounds.top = 0;
		cropcap->bounds.left = 0;
		cropcap->bounds.width = 1920;
		cropcap->bounds.height = 1080;

		cropcap->defrect.top = 0;
		cropcap->defrect.left = 0;
		cropcap->defrect.width = 1920;
		cropcap->defrect.height = 1080;
		break;

	default:
		return -1;

	}

	V4L2PRINTK("[input type 0x%08x] : left: [%d], top [%d],"
		"width[%d], height[%d]\n",
			s5ptv_status.tvout_param.disp,
			cropcap->bounds.top ,
			cropcap->bounds.left ,
			cropcap->bounds.width,
			cropcap->bounds.height);
#endif
	return 0;
}

static int s5p_tv_v4l2_g_crop(struct file *file, void *fh, struct v4l2_crop *a)
{
#ifndef OLD
#else
	struct v4l2_crop *crop = a;
	struct s5p_tv_vo *layer = (struct s5p_tv_vo *)fh;

	switch (crop->type) {
		/* Vlayer */

	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		crop->c.left	= s5ptv_status.vl_basic_param.dest_offset_x;
		crop->c.top	= s5ptv_status.vl_basic_param.dest_offset_y;
		crop->c.width 	= s5ptv_status.vl_basic_param.dest_width;
		crop->c.height	= s5ptv_status.vl_basic_param.dest_height;
		break;

	default:
		break;
	}
#endif
	return 0;
}
#endif

static int s5p_tv_v4l2_s_crop(
		struct file		*file,
		void			*fh,
		struct v4l2_crop	*a)
{
	switch (a->type) {
	case V4L2_BUF_TYPE_PRIVATE: {
		struct v4l2_rect *rect = &a->c;

		s5p_vp_ctrl_set_src_win(rect->left, rect->top,
			rect->width, rect->height);
		break;
	}
	default:
		break;
	}

	return 0;
}

#if 0 /* This function will be used */
static int s5p_tv_v4l2_g_fbuf(
		struct file		*file,
		void			*fh,
		struct v4l2_framebuffer	*a)
{
#ifndef OLD
#else

	struct v4l2_framebuffer *fbuf = a;
	struct s5p_tv_vo *layer = (struct s5p_tv_vo *)fh;

	fbuf->base = (void *)s5ptv_overlay[layer->index].base_addr;
	fbuf->fmt.pixelformat = s5ptv_overlay[layer->index].fb.fmt.pixelformat;
#endif
	return 0;
}
#endif

static int s5p_tv_v4l2_s_fbuf(
		struct file		*file,
		void			*fh,
		struct v4l2_framebuffer	*a)
{
	s5p_vp_ctrl_set_dest_win_blend(
		(a->flags & V4L2_FBUF_FLAG_GLOBAL_ALPHA) ? 1 : 0);

	s5p_vp_ctrl_set_dest_win_priority(a->fmt.priv);

	return 0;
}

static int s5p_tv_v4l2_overlay(
		struct file	*file,
		void		*fh,
		unsigned int	i)
{
	if (i)
		s5p_vp_ctrl_start();
	else 
		s5p_vp_ctrl_stop();

	return 0;
}

#define VIDIOC_HDCP_ENABLE _IOWR('V', 100, unsigned int)
#define VIDIOC_HDCP_STATUS _IOR('V', 101, unsigned int)
#define VIDIOC_HDCP_PROT_STATUS _IOR('V', 102, unsigned int)
#define VIDIOC_INIT_AUDIO _IOR('V', 103, unsigned int)
#define VIDIOC_AV_MUTE _IOR('V', 104, unsigned int)
#define VIDIOC_G_AVMUTE _IOR('V', 105, unsigned int)

long s5p_tv_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case VIDIOC_HDCP_ENABLE:
//		s5ptv_status.hdmi.hdcp_en = (unsigned int) arg; //. d: sichoi
		V4L2PRINTK("HDCP status is %s\n",
		       s5ptv_status.hdcp_en ? "enabled" : "disabled");

		return 0;

	case VIDIOC_HDCP_STATUS: {

		unsigned int *status = (unsigned int *)&arg;

		*status = 1;

		V4L2PRINTK("HPD status is %s\n",
		       s5ptv_status.hpd_status ? "plugged" : "unplugged");

		return 0;
	}

	case VIDIOC_HDCP_PROT_STATUS: {

		unsigned int *prot = (unsigned int *)&arg;

		*prot = 1;

//		V4L2PRINTK("hdcp prot status is %d\n", hdcp_protocol_status);

		return 0;
	}

	case VIDIOC_ENUMSTD: {

		struct v4l2_standard *p = (struct v4l2_standard *)arg;

		if (p->index >= S5P_TVOUT_MAX_STANDARDS) {
			V4L2PRINTK("exceeded S5P_TVOUT_MAX_STANDARDS\n");

			return -EINVAL;
		}

		memcpy(p, &s5p_tv_standards[p->index],
			sizeof(struct v4l2_standard));

		return 0;
	}

	default:
		break;
	}

	return video_ioctl2(file, cmd, arg);
}

long s5p_tv_v_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case VIDIOC_INIT_AUDIO:
//		s5ptv_status.hdmi.audio = (unsigned int) arg; //. d: sichoi

		if (arg) {
			s5p_hdmi_set_audio(true);

			if (s5ptv_status.tvout_output_enable)
				s5p_hdmi_audio_enable(true);
		} else {
			s5p_hdmi_set_audio(false);

			if (s5ptv_status.tvout_output_enable)
				s5p_hdmi_audio_enable(false);

		}

		return 0;

	case VIDIOC_AV_MUTE:
		if (arg) {
//			s5ptv_status.hdmi.audio = HDMI_AUDIO_NO; //. d: sichoi

			if (s5ptv_status.tvout_output_enable) {
				s5p_hdmi_audio_enable(false);
				s5p_hdmi_set_bluescreen(true);
			}

			s5p_hdmi_set_mute(true);
		} else {
//			s5ptv_status.hdmi.audio = HDMI_AUDIO_PCM; //. d: sichoi

			if (s5ptv_status.tvout_output_enable) {
				s5p_hdmi_audio_enable(true);
				s5p_hdmi_set_bluescreen(false);
			}

			s5p_hdmi_set_mute(false);
		}

		return 0;
	case VIDIOC_G_AVMUTE:
		return s5p_hdmi_get_mute();

	case VIDIOC_HDCP_ENABLE:
//		s5ptv_status.hdmi.hdcp_en = (unsigned int) arg; //. d: sichoi
		V4L2PRINTK("HDCP status is %s\n",
		       s5ptv_status.hdcp_en ? "enabled" : "disabled");

		return 0;

	case VIDIOC_HDCP_STATUS: {
		unsigned int *status = (unsigned int *)&arg;

		*status = 1;

		V4L2PRINTK("HPD status is %s\n",
		       s5ptv_status.hpd_status ? "plugged" : "unplugged");

		return 0;
	}

	case VIDIOC_HDCP_PROT_STATUS: {
		unsigned int *prot = (unsigned int *)&arg;

		*prot = 1;

//		V4L2PRINTK("hdcp prot status is %d\n", hdcp_protocol_status);

		return 0;
	}

	case VIDIOC_ENUMSTD: {
		struct v4l2_standard *p = (struct v4l2_standard *)arg;

		if (p->index >= S5P_TVOUT_MAX_STANDARDS) {
			V4L2PRINTK("exceeded S5P_TVOUT_MAX_STANDARDS\n");

			return -EINVAL;
		}

		memcpy(p, &s5p_tv_standards[p->index],
			sizeof(struct v4l2_standard));

		return 0;
	}

	default:
		break;
	}

	return video_ioctl2(file, cmd, arg);
}

const struct v4l2_ioctl_ops s5p_tv_v4l2_ops = {
	.vidioc_querycap		= s5p_tv_v4l2_querycap,
	.vidioc_g_std			= s5p_tv_v4l2_g_std,
	.vidioc_s_std			= s5p_tv_v4l2_s_std,
	.vidioc_enum_output		= s5p_tv_v4l2_enum_output,
	.vidioc_g_output		= s5p_tv_v4l2_g_output,
	.vidioc_s_output		= s5p_tv_v4l2_s_output,
};

const struct v4l2_ioctl_ops s5p_tv_v4l2_vid_ops = {
//	.vidioc_cropcap			= s5p_tv_v4l2_cropcap,	// not yet

//	.vidioc_enum_fmt_type_private	= s5p_tv_v4l2_enum_fmt_type_private, // not yet
//	.vidioc_g_fmt_type_private	= s5p_tv_v4l2_g_fmt_type_private,    // not yet
	.vidioc_s_fmt_type_private	= s5p_tv_v4l2_s_fmt_type_private,

//	.vidioc_g_fmt_vid_overlay	= s5p_tv_v4l2_g_fmt_vid_overlay,
	.vidioc_s_fmt_vid_overlay	= s5p_tv_v4l2_s_fmt_vid_overlay,

//	.vidioc_cropcap			= s5p_tv_v4l2_cropcap,	// not yet
//	.vidioc_g_crop			= s5p_tv_v4l2_g_crop,	// not yet
	.vidioc_s_crop			= s5p_tv_v4l2_s_crop,

//	.vidioc_g_fbuf			= s5p_tv_v4l2_g_fbuf,	// not yet
	.vidioc_s_fbuf			= s5p_tv_v4l2_s_fbuf,

	.vidioc_overlay			= s5p_tv_v4l2_overlay,
};
