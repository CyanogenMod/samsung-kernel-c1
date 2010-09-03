/* linux/drivers/media/video/samsung/tv20/ver_1/sdo.h
 *
 * Copyright (c) 2010 Samsung Electronics
 *		http://www.samsung.com/
 *
 * Header file for TV encoder & DAC of Samsung TVOut driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _SDO_H_
#define _SDO_H_ __FILE__

enum s5p_sd_level {
	SDO_LEVEL_0IRE,
	SDO_LEVEL_75IRE
};

enum s5p_sd_vsync_ratio {
	SDO_VTOS_RATIO_10_4,
	SDO_VTOS_RATIO_7_3
};

enum s5p_sd_order {
	SDO_O_ORDER_COMPONENT_RGB_PRYPB,
	SDO_O_ORDER_COMPONENT_RBG_PRPBY,
	SDO_O_ORDER_COMPONENT_BGR_PBYPR,
	SDO_O_ORDER_COMPONENT_BRG_PBPRY,
	SDO_O_ORDER_COMPONENT_GRB_YPRPB,
	SDO_O_ORDER_COMPONENT_GBR_YPBPR,
	SDO_O_ORDER_COMPOSITE_CVBS_Y_C,
	SDO_O_ORDER_COMPOSITE_CVBS_C_Y,
	SDO_O_ORDER_COMPOSITE_Y_C_CVBS,
	SDO_O_ORDER_COMPOSITE_Y_CVBS_C,
	SDO_O_ORDER_COMPOSITE_C_CVBS_Y,
	SDO_O_ORDER_COMPOSITE_C_Y_CVBS
};

enum s5p_sd_sync_sig_pin {
	SDO_SYNC_SIG_NO,
	SDO_SYNC_SIG_YG,
	SDO_SYNC_SIG_ALL
};

enum s5p_sd_closed_caption_type {
	SDO_NO_INS,
	SDO_INS_1,
	SDO_INS_2,
	SDO_INS_OTHERS
};

enum s5p_sd_525_copy_permit {
	SDO_525_COPY_PERMIT,
	SDO_525_ONECOPY_PERMIT,
	SDO_525_NOCOPY_PERMIT
};

enum s5p_sd_525_mv_psp {
	SDO_525_MV_PSP_OFF,
	SDO_525_MV_PSP_ON_2LINE_BURST,
	SDO_525_MV_PSP_ON_BURST_OFF,
	SDO_525_MV_PSP_ON_4LINE_BURST,
};

enum s5p_sd_525_copy_info {
	SDO_525_COPY_INFO,
	SDO_525_DEFAULT,
};

enum s5p_sd_525_aspect_ratio {
	SDO_525_4_3_NORMAL,
	SDO_525_16_9_ANAMORPIC,
	SDO_525_4_3_LETTERBOX
};

enum s5p_sd_625_subtitles {
	SDO_625_NO_OPEN_SUBTITLES,
	SDO_625_INACT_OPEN_SUBTITLES,
	SDO_625_OUTACT_OPEN_SUBTITLES
};

enum s5p_sd_625_camera_film {
	SDO_625_CAMERA,
	SDO_625_FILM
};

enum s5p_sd_625_color_encoding {
	SDO_625_NORMAL_PAL,
	SDO_625_MOTION_ADAPTIVE_COLORPLUS
};

enum s5p_sd_625_aspect_ratio {
	SDO_625_4_3_FULL_576,
	SDO_625_14_9_LETTERBOX_CENTER_504,
	SDO_625_14_9_LETTERBOX_TOP_504,
	SDO_625_16_9_LETTERBOX_CENTER_430,
	SDO_625_16_9_LETTERBOX_TOP_430,
	SDO_625_16_9_LETTERBOX_CENTER,
	SDO_625_14_9_FULL_CENTER_576,
	SDO_625_16_9_ANAMORPIC_576
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
};

struct s5p_sd_offset_gain {
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

struct s5p_sd_cvbs_compensat {
	bool cvbs_color_compensation;
	u32 y_lower_mid;
	u32 y_bottom;
	u32 y_top;
	u32 y_upper_mid;
	u32 radius;
};

struct s5p_sd_component_porch {
	u32 back_525;
	u32 front_525;
	u32 back_625;
	u32 front_625;
};

struct s5p_sd_ch_xtalk_cancellat_coeff {
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

#endif /* _SDO_H_ */
