/* linux/drivers/media/video/samsung/tv20/s5p_tv.h
 *
 * Copyright (c) 2009 Samsung Electronics
 *		http://www.samsung.com/
 *
 * TV out driver header file for Samsung TVOut driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _LINUX_S5P_TV_H_
#define _LINUX_S5P_TV_H_

#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/videodev2.h>
#include <linux/videodev2_samsung.h>
#include <linux/platform_device.h>
#include <linux/fb.h>

/* #define COFIG_TVOUT_DBG */

#define DRV_NAME	"S5P_TV"

#define tv_err(fmt, ...)					\
		printk(KERN_ERR "%s:%s(): " fmt,		\
			DRV_NAME, __func__, ##__VA_ARGS__)

#ifdef CONFIG_TV_DEBUG
#define tv_dbg(fmt, ...)					\
		printk(KERN_DEBUG "%s:%s(): " fmt,		\
			DRV_NAME, __func__, ##__VA_ARGS__)
#else
#define tv_dbg(fmt, ...)
#endif


#define V4L2_STD_ALL_HD			((v4l2_std_id)0xffffffff)

#define TVOUT_MINOR_TVOUT		14
#define TVOUT_MINOR_VID			21

#define USE_MIXER_INTERRUPT		1
#define S5PTV_FB_CNT	2

#define S5PTV_FB_LAYER0_MINOR	10
#define S5PTV_FB_LAYER1_MINOR	11

#define HDMI_START_NUM 0x1000

enum s5p_tv_disp_mode {
	TVOUT_NTSC_M = 0,
	TVOUT_PAL_BDGHI,
	TVOUT_PAL_M,
	TVOUT_PAL_N,
	TVOUT_PAL_NC,
	TVOUT_PAL_60,
	TVOUT_NTSC_443,

	TVOUT_480P_60_16_9 = HDMI_START_NUM,
	TVOUT_480P_60_4_3,
	TVOUT_480P_59,

	TVOUT_576P_50_16_9,
	TVOUT_576P_50_4_3,

	TVOUT_720P_60,
	TVOUT_720P_50,
	TVOUT_720P_59,

	TVOUT_1080P_60,
	TVOUT_1080P_50,
	TVOUT_1080P_59,
	TVOUT_1080P_30,

	TVOUT_1080I_60,
	TVOUT_1080I_50,
	TVOUT_1080I_59,
};

enum s5p_tv_o_mode {
	TVOUT_COMPOSITE,
	TVOUT_HDMI,
	TVOUT_HDMI_RGB,
	TVOUT_DVI
};

struct tvout_output_if {
	enum s5p_tv_disp_mode	disp;
	enum s5p_tv_o_mode	out;
};

struct s5p_tv_vo {
	u32 index;
	struct v4l2_framebuffer fb;
	struct v4l2_window win;
	struct v4l2_rect dst_rect;
	bool win_blending;
	bool blank_change;
	bool pixel_blending;
	bool pre_mul;
	u32 blank_color;
	u32 priority;
	u32 base_addr;
};

struct s5p_bg_color {
	u32 color_y;
	u32 color_cb;
	u32 color_cr;
};

struct s5p_tv_v4l2 {
	struct v4l2_output      *output;
	struct v4l2_standard	*std;
	struct v4l2_format	*fmt_v;
	struct v4l2_format	*fmt_vo_0;
	struct v4l2_format	*fmt_vo_1;
};

#define S5PTVFB_AVALUE(r, g, b)	\
	(((r & 0xf) << 8) | ((g & 0xf) << 4) | ((b & 0xf) << 0))
#define S5PTVFB_CHROMA(r, g, b)	\
	(((r & 0xff) << 16) | ((g & 0xff) << 8) | ((b & 0xff) << 0))

#define S5PTVFB_WIN_POSITION \
	_IOW('F', 213, struct s5ptvfb_user_window)
#define S5PTVFB_WIN_SET_PLANE_ALPHA \
	_IOW('F', 214, struct s5ptvfb_user_plane_alpha)
#define S5PTVFB_WIN_SET_CHROMA \
	_IOW('F', 215, struct s5ptvfb_user_chroma)
#define S5PTVFB_SCALING	\
	_IOW('F', 222, struct s5ptvfb_user_scaling)

enum s5p_tv_endian {
	TVOUT_LITTLE_ENDIAN = 0,
	TVOUT_BIG_ENDIAN = 1
};

enum s5p_mixer_burst_mode {
	MIXER_BURST_8 = 0,
	MIXER_BURST_16 = 1
};

enum s5ptvfb_data_path_t {
	DATA_PATH_FIFO = 0,
	DATA_PATH_DMA = 1,
};

enum s5ptvfb_alpha_t {
	LAYER_BLENDING,
	PIXEL_BLENDING,
	NONE_BLENDING,
};

enum s5ptvfb_chroma_dir_t {
	CHROMA_FG,
	CHROMA_BG,
};

enum s5ptvfb_ver_scaling_t {
	VERTICAL_X1,
	VERTICAL_X2,
};

enum s5ptvfb_hor_scaling_t {
	HORIZONTAL_X1,
	HORIZONTAL_X2,
};

struct s5ptvfb_alpha {
	enum 		s5ptvfb_alpha_t mode;
	int		channel;
	unsigned int	value;
};

struct s5ptvfb_chroma {
	int 		enabled;
	unsigned int	key;
};

struct s5ptvfb_user_window {
	int x;
	int y;
};

struct s5ptvfb_user_plane_alpha {
	int 		channel;
	unsigned char	alpha;
};

struct s5ptvfb_user_chroma {
	int 		enabled;
	unsigned char	red;
	unsigned char	green;
	unsigned char	blue;
};

struct s5ptvfb_user_scaling {
	enum s5ptvfb_ver_scaling_t ver;
	enum s5ptvfb_hor_scaling_t hor;
};

struct s5ptvfb_window {
	int		id;
	struct device	*dev_fb;
	int		enabled;
	atomic_t	in_use;
	int		x;
	int		y;
	enum 		s5ptvfb_data_path_t path;
	int		local_channel;
	int		dma_burst;
	unsigned int	pseudo_pal[16];
	struct		s5ptvfb_alpha alpha;
	struct		s5ptvfb_chroma chroma;
	int		(*suspend_fifo)(void);
	int		(*resume_fifo)(void);
};

struct s5ptvfb_lcd_timing {
	int	h_fp;
	int	h_bp;
	int	h_sw;
	int	v_fp;
	int	v_fpe;
	int	v_bp;
	int	v_bpe;
	int	v_sw;
};

struct s5ptvfb_lcd_polarity {
	int	rise_vclk;
	int	inv_hsync;
	int	inv_vsync;
	int	inv_vden;
};

struct s5ptvfb_lcd {
	int	width;
	int	height;
	int	bpp;
	int	freq;
	struct 	s5ptvfb_lcd_timing timing;
	struct 	s5ptvfb_lcd_polarity polarity;

	void 	(*init_ldi)(void);
};

struct s5p_tv_status {
	/* TVOUT_SET_INTERFACE_PARAM */
	struct tvout_output_if tvout_param;

	/* TVOUT_SET_OUTPUT_ENABLE/DISABLE */
	bool tvout_output_enable;

	/* TVOUT_SET_LAYER_ENABLE/DISABLE */
	bool grp_layer_enable[2];

	/* i2c for hdcp port */

	struct i2c_client 	*hdcp_i2c_client;

	struct video_device      *video_dev[3];

	struct clk *mixer_clk;
	struct clk *i2c_phy_clk;
	struct clk *sclk_hdmiphy;
	struct clk *sclk_pixel;
	struct clk *sclk_dac;
	struct clk *sclk_hdmi;
	struct clk *sclk_mixer;

	struct s5p_tv_v4l2 	v4l2;

	struct device *dev_fb;

	struct mutex fb_lock;
};

struct reg_mem_info {
	char		*name;
	struct resource *res;
	void __iomem	*base;
};

struct irq_info {
	char		*name;
	irq_handler_t	handler;
	int		no;
};

struct s5p_tv_clk_info {
	char		*name;
	struct clk	*ptr;
};

struct s5p_tv_power_clk_info {
	char		*pow_name;
	char		*clk_name;
	struct clk	*clk_ptr;
};

#define S5PTVFB_NAME		"s5ptvfb"

#define V4L2_INPUT_TYPE_MSDMA			3
#define V4L2_INPUT_TYPE_FIFO			4

#define V4L2_OUTPUT_TYPE_MSDMA			4
#define V4L2_OUTPUT_TYPE_COMPOSITE		5
#define V4L2_OUTPUT_TYPE_SVIDEO			6
#define V4L2_OUTPUT_TYPE_YPBPR_INERLACED	7
#define V4L2_OUTPUT_TYPE_YPBPR_PROGRESSIVE	8
#define V4L2_OUTPUT_TYPE_RGB_PROGRESSIVE	9
#define V4L2_OUTPUT_TYPE_DIGITAL		10
#define V4L2_OUTPUT_TYPE_HDMI			V4L2_OUTPUT_TYPE_DIGITAL
#define V4L2_OUTPUT_TYPE_HDMI_RGB		11
#define V4L2_OUTPUT_TYPE_DVI			12

#define V4L2_STD_PAL_BDGHI 	(V4L2_STD_PAL_B|\
				V4L2_STD_PAL_D|	\
				V4L2_STD_PAL_G|	\
				V4L2_STD_PAL_H|	\
				V4L2_STD_PAL_I)

#define V4L2_STD_480P_60_16_9	((v4l2_std_id)0x04000000)
#define V4L2_STD_480P_60_4_3	((v4l2_std_id)0x05000000)
#define V4L2_STD_576P_50_16_9	((v4l2_std_id)0x06000000)
#define V4L2_STD_576P_50_4_3	((v4l2_std_id)0x07000000)
#define V4L2_STD_720P_60	((v4l2_std_id)0x08000000)
#define V4L2_STD_720P_50	((v4l2_std_id)0x09000000)
#define V4L2_STD_1080P_60	((v4l2_std_id)0x0a000000)
#define V4L2_STD_1080P_50	((v4l2_std_id)0x0b000000)
#define V4L2_STD_1080I_60	((v4l2_std_id)0x0c000000)
#define V4L2_STD_1080I_50	((v4l2_std_id)0x0d000000)
#define V4L2_STD_480P_59	((v4l2_std_id)0x0e000000)
#define V4L2_STD_720P_59	((v4l2_std_id)0x0f000000)
#define V4L2_STD_1080I_59	((v4l2_std_id)0x10000000)
#define V4L2_STD_1080P_59	((v4l2_std_id)0x11000000)
#define V4L2_STD_1080P_30	((v4l2_std_id)0x12000000)

#define FORMAT_FLAGS_DITHER       	0x01
#define FORMAT_FLAGS_PACKED       	0x02
#define FORMAT_FLAGS_PLANAR       	0x04
#define FORMAT_FLAGS_RAW         	0x08
#define FORMAT_FLAGS_CrCb         	0x10

#define V4L2_FBUF_FLAG_PRE_MULTIPLY	0x0040
#define V4L2_FBUF_CAP_PRE_MULTIPLY	0x0080

struct v4l2_vid_overlay_src {
	void			*base_y;
	void			*base_c;
	struct v4l2_pix_format	pix_fmt;
};

#define S5PTVFB_POWER_OFF	_IOW('F', 217, u32)
#define S5PTVFB_POWER_ON	_IOW('F', 218, u32)
#define S5PTVFB_WIN_SET_ADDR	_IOW('F', 219, u32)
#define S5PTVFB_SET_WIN_ON	_IOW('F', 220, u32)
#define S5PTVFB_SET_WIN_OFF	_IOW('F', 221, u32)

extern struct s5p_tv_status s5ptv_status;
extern struct s5p_tv_vo s5ptv_overlay[2];

#endif /* _LINUX_S5P_TV_H_ */

