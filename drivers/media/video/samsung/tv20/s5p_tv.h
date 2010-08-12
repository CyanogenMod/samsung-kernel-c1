/* linux/drivers/media/video/samsung/tv20/s5p_tv.h
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsung.com/
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


#include "ver_1/tv_out.h"


/* #define COFIG_TVOUT_DBG */

#define V4L2_STD_ALL_HD			((v4l2_std_id)0xffffffff)


#ifdef CONFIG_TV_FB
#define TVOUT_MINOR_TVOUT		14
#define TVOUT_MINOR_VID			21
#else
#define TVOUT_MINOR_VIDEO		14
#define TVOUT_MINOR_GRP0		21
#define TVOUT_MINOR_GRP1		22
#endif

#define USE_VMIXER_INTERRUPT		1

#define AVI_SAME_WITH_PICTURE_AR	(0x1<<3)

#define AVI_RGB_IF			(0x0<<5)
#define AVI_YCBCR444_IF			(0x2<<5)

#define AVI_ITU601			(0x1<<6)
#define AVI_ITU709			(0x2<<6)

#define AVI_PAR_4_3			(0x1<<4)
#define AVI_PAR_16_9			(0x2<<4)

#define AVI_NO_PIXEL_REPEAT		(0x0<<0)

#define AVI_VIC_2			(2<<0)
#define AVI_VIC_3			(3<<0)
#define AVI_VIC_4			(4<<0)
#define AVI_VIC_5			(5<<0)
#define AVI_VIC_16			(16<<0)
#define AVI_VIC_17			(17<<0)
#define AVI_VIC_18			(18<<0)
#define AVI_VIC_19			(19<<0)
#define AVI_VIC_20			(20<<0)
#define AVI_VIC_31			(31<<0)
#define AVI_VIC_34			(34<<0)

#define VP_UPDATE_RETRY_MAXIMUM 	30
#define VP_WAIT_UPDATE_SLEEP 		3

#define INTERLACED 			0
#define PROGRESSIVE 			1

struct tvout_output_if {
	enum s5p_tv_disp_mode	disp_mode;
	enum s5p_tv_o_mode	out_mode;
};

struct s5p_img_size {
	u32 img_width;
	u32 img_height;
};

struct s5p_img_offset {
	u32 offset_x;
	u32 offset_y;
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

struct s5p_video_poly_filter_coef {
	enum s5p_vp_poly_coeff poly_coeff;
	signed char ch0;
	signed char ch1;
	signed char ch2;
	signed char ch3;
};

struct s5p_vl_bright_contrast_ctrl {
	enum s5p_vp_line_eq eq_num;
	u32 intc;
	u32 slope;
};

struct s5p_video_csc_coef {
	enum s5p_vp_csc_coeff csc_coeff;
	u32 coeff;
};

struct s5p_vl_param {
	bool win_blending;
	u32 alpha;
	u32 priority;
	u32 top_y_address;
	u32 top_c_address;
	enum s5p_endian_type src_img_endian;
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

struct s5p_bg_dither {
	bool cr_dither_en;
	bool cb_dither_en;
	bool y_dither_en;
};


struct s5p_bg_color {
	u32 color_y;
	u32 color_cb;
	u32 color_cr;
};

struct s5p_vm_csc_coef {
	enum s5p_yuv_fmt_component component;
	enum s5p_tv_coef_y_mode mode;
	u32 coeff_0;
	u32 coeff_1;
	u32 coeff_2;
};

struct s5p_sdout_order {
	enum s5p_sd_order order;
	bool dac[3];
};

struct s5p_sd_vscale_cfg {
	enum s5p_sd_level component_level;
	enum s5p_sd_vsync_ratio component_ratio;
	enum s5p_sd_level composite_level;
	enum s5p_sd_vsync_ratio composite_ratio;
};

struct s5p_sd_vbi {
	bool wss_cvbs;
	enum s5p_sd_closed_caption_type caption_cvbs;
	bool wss_y_svideo;
	enum s5p_sd_closed_caption_type caption_y_svideo;
	bool cgmsa_rgb;
	bool wss_rgb;
	enum s5p_sd_closed_caption_type caption_rgb;
	bool cgmsa_y_pb_pr;
	bool wss_y_pb_pr;
	enum s5p_sd_closed_caption_type caption_y_pb_pr;
};

struct s5p_sd_offset_gain {
	enum s5p_sd_channel_sel channel;
	u32 offset;
	u32 gain;
};

struct s5p_sd_delay {
	u32 delay_y;
	u32 offset_video_start;
	u32 offset_video_end;
};

struct s5p_sd_bright_hue_saturat {
	bool bright_hue_sat_adj;
	u32 gain_brightness;
	u32 offset_brightness;
	u32 gain0_cb_hue_saturation;
	u32 gain1_cb_hue_saturation;
	u32 gain0_cr_hue_saturation;
	u32 gain1_cr_hue_saturation;
	u32 offset_cb_hue_saturation;
	u32 offset_cr_hue_saturation;
};

struct s5p_sd_rgb_compensat {
	bool rgb_color_compensation;
	u32 max_rgb_cube;
	u32 min_rgb_cube;
};

struct s5p_sd_cvbs_compensat {
	bool cvbs_color_compensation;
	u32 y_lower_mid;
	u32 y_bottom;
	u32 y_top;
	u32 y_upper_mid;
	u32 radius;
};

struct s5p_sd_svideo_compensat {
	bool y_color_compensation;
	u32 y_top;
	u32 y_bottom;
	u32 yc_cylinder;
};

struct s5p_sd_component_porch {
	u32 back_525;
	u32 front_525;
	u32 back_625;
	u32 front_625;
};

struct s5p_sd_vesa_rgb_sync {
	enum s5p_sd_vesa_rgb_sync_type sync_type;
	enum s5p_tv_active_polarity vsync_active;
	enum s5p_tv_active_polarity hsync_active;
};

struct s5p_sd_ch_xtalk_cancellat_coeff {
	enum s5p_sd_channel_sel channel;
	u32 coeff1;
	u32 coeff2;
};

struct s5p_sd_closed_caption {
	u32 display_cc;
	u32 nondisplay_cc;
};

struct s5p_sd_525_data {
	bool analog_on;
	enum s5p_sd_525_copy_permit copy_permit;
	enum s5p_sd_525_mv_psp mv_psp;
	enum s5p_sd_525_copy_info copy_info;
	enum s5p_sd_525_aspect_ratio display_ratio;
};

struct s5p_sd_625_data {
	bool surroun_f_sound;
	bool copyright;
	bool copy_protection;
	bool text_subtitles;
	enum s5p_sd_625_subtitles open_subtitles;
	enum s5p_sd_625_camera_film camera_film;
	enum s5p_sd_625_color_encoding color_encoding;
	bool helper_signal;
	enum s5p_sd_625_aspect_ratio display_ratio;
};

struct s5p_hdmi_bluescreen {
	bool enable;
	u8 cb_b;
	u8 y_g;
	u8 cr_r;
};

struct s5p_hdmi_color_range {
	u8 y_min;
	u8 y_max;
	u8 c_min;
	u8 c_max;
};

struct s5p_hdmi_infoframe {
	enum s5p_hdmi_transmit trans_type;
	u8 check_sum;
	u8 *data;
};

struct s5p_hdmi_tg_cmd {
	bool timing_correction_en;
	bool bt656_sync_en;
	bool tg_en;
};

struct s5p_hdmi_spd {
	enum s5p_hdmi_transmit trans_type;
	u8 *spd_header;
	u8 *spd_data;
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

enum s5ptvfb_data_path_t {
	DATA_PATH_FIFO = 0,
	DATA_PATH_DMA = 1,
};

enum s5ptvfb_alpha_t {
	PLANE_BLENDING,
	PIXEL_BLENDING,
};

enum s5ptvfb_chroma_dir_t {
	CHROMA_FG,
	CHROMA_BG,
};

struct s5ptvfb_alpha {
	enum 		s5ptvfb_alpha_t mode;
	int		channel;
	unsigned int	value;
};

struct s5ptvfb_chroma {
	int 		enabled;
	int 		blended;
	unsigned int	key;
	unsigned int	comp_key;
	unsigned int	alpha;
	enum 		s5ptvfb_chroma_dir_t dir;
};

struct s5ptvfb_user_window {
	int x;
	int y;
};

struct s5ptvfb_user_plane_alpha {
	int 		channel;
	unsigned char	red;
	unsigned char	green;
	unsigned char	blue;
};

struct s5ptvfb_user_chroma {
	int 		enabled;
	unsigned char	red;
	unsigned char	green;
	unsigned char	blue;
};


struct s5ptvfb_window {
	int		id;
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
	bool tvout_param_available;
	struct tvout_output_if tvout_param;

	/* TVOUT_SET_OUTPUT_ENABLE/DISABLE */
	bool tvout_output_enable;

	/* TVOUT_SET_LAYER_MODE/POSITION */
	bool vl_mode;
	bool grp_mode[2];

	/* Video Layer Parameters */
	struct s5p_vl_param vl_basic_param;
	struct s5p_vl_mode vl_op_mode;
	struct s5p_vl_sharpness vl_sharpness;
	struct s5p_vl_csc_ctrl vl_csc_control;
	struct s5p_vl_bright_contrast_ctrl vl_bc_control[8];

	enum s5p_vp_src_color src_color;
	enum s5p_vp_field field_id;
	enum s5p_vp_pxl_rate vl_rate;
	enum s5p_vp_csc_type vl_csc_type;

	u32 vl_top_y_address;
	u32 vl_top_c_address;
	u32 vl_bottom_y_address;
	u32 vl_bottom_c_address;
	u32 vl_src_offset_x;
	u32 vl_src_x_fact_step;
	u32 vl_src_offset_y;
	u32 vl_src_width;
	u32 vl_src_height;
	u32 vl_dest_offset_x;
	u32 vl_dest_offset_y;
	u32 vl_dest_width;
	u32 vl_dest_height;
	bool vl2d_ipc;

	bool vl_poly_filter_default;
	bool vl_bypass_post_process;
	u32 vl_saturation;
	bool us_vl_brightness;
	u8 vl_contrast;
	u32 vl_bright_offset;
	bool vl_csc_coef_default;

	/* GRP Layer Common Parameters */
	enum s5p_vmx_burst_mode grp_burst;
	enum s5p_endian_type grp_endian;

	/* BackGroung Layer Parameters */
	struct s5p_bg_dither bg_dither;
	struct s5p_bg_color bg_color[3];

	/* Video Mixer Parameters */
	bool vm_csc_coeff_default;

	/* SDout Parameters */
	struct s5p_sd_vscale_cfg sdout_video_scale_cfg;
	struct s5p_sd_vbi sdout_vbi;
	struct s5p_sd_offset_gain sdout_offset_gain[3];
	struct s5p_sd_delay sdout_delay;
	struct s5p_sd_bright_hue_saturat sdout_bri_hue_set;
	struct s5p_sd_rgb_compensat sdout_rgb_compen;
	struct s5p_sd_cvbs_compensat sdout_cvbs_compen;
	struct s5p_sd_svideo_compensat sdout_svideo_compen;
	struct s5p_sd_component_porch sdout_comp_porch;
	struct s5p_sd_vesa_rgb_sync sdout_rgb_sync;
	struct s5p_sd_ch_xtalk_cancellat_coeff sdout_xtalk_cc[3];
	struct s5p_sd_closed_caption sdout_closed_capt;
	struct s5p_sd_525_data sdout_wss_525;
	struct s5p_sd_625_data sdout_wss_625;
	struct s5p_sd_525_data sdout_cgms_525;
	struct s5p_sd_625_data sdout_cgms_625;

	enum s5p_sd_order sdout_order;
	enum s5p_sd_sync_sig_pin sdout_sync_pin;

	bool sdout_color_sub_carrier_phase_adj;
	bool sdout_dac_on[3];
	bool sdout_y_pb_pr_comp;

	/* HDMI video parameters */
	struct s5p_hdmi_bluescreen hdmi_video_blue_screen;
	struct s5p_hdmi_color_range hdmi_color_range;
	struct s5p_hdmi_infoframe hdmi_av_info_frame;
	struct s5p_hdmi_infoframe hdmi_mpg_info_frame;
	struct s5p_hdmi_tg_cmd hdmi_tg_cmd;
	u8 avi_byte[13];
	u8 mpg_byte[5];

	/* HDMI parameters */
	struct s5p_hdmi_spd hdmi_spd_info_frame;
	u8 spd_header[3];
	u8 spd_data[28];
	bool hdcp_en;
	enum s5p_hdmi_audio_type hdmi_audio_type;
	bool hpd_status;

	/* TVOUT_SET_LAYER_ENABLE/DISABLE */
	bool vp_layer_enable;
	bool grp_layer_enable[2];

	/* i2c for hdcp port */

	struct i2c_client 	*hdcp_i2c_client;

	struct s5p_tv_vo	overlay[2];

	struct video_device      *video_dev[3];

	struct clk *tvenc_clk;
	struct clk *vp_clk;
	struct clk *mixer_clk;
	struct clk *hdmi_clk;
	struct clk *i2c_phy_clk;
	struct clk *sclk_hdmiphy;
	struct clk *sclk_pixel;
	struct clk *sclk_dac;
	struct clk *sclk_hdmi;
	struct clk *sclk_mixer;

	struct s5p_tv_v4l2 	v4l2;

	struct s5ptvfb_window win;
	struct fb_info	*fb;
	struct device *dev_fb;

	struct s5ptvfb_lcd *lcd;
	struct mutex fb_lock;
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

struct v4l2_window_s5p_tvout {
	u32	capability;
	u32	flags;
	u32	priority;

	struct v4l2_window	win;
};

struct v4l2_pix_format_s5p_tvout {
	void *base_y;
	void *base_c;
	bool src_img_endian;

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

