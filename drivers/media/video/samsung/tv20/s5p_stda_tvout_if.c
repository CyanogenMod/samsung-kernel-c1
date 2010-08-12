/* linux/drivers/media/video/samsung/tv20/s5p_stda_tvout_if.c
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsung.com/
 *
 * TVOut interface ftn. file for Samsung TVOut driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

#include <plat/clock.h>

#include "s5p_tv.h"
#include "s5p_stda_tvout_if.h"

#ifdef COFIG_TVOUT_DBG
#define S5P_STDA_TVOUTIF_DEBUG 1
#endif

#ifdef S5P_STDA_TVOUTIF_DEBUG
#define TVOUTIFPRINTK(fmt, args...) \
	printk(KERN_INFO "\t[TVOUT_IF] %s: " fmt, __func__ , ## args)
#else
#define TVOUTIFPRINTK(fmt, args...)
#endif

bool s5p_tv_if_init_param(void)
{
	struct s5p_tv_status *st = &s5ptv_status;

	/* Initialize GRPx Layer Parameters to Default Values */
	st->grp_burst = VM_BURST_16;
	st->grp_endian = TVOUT_LITTLE_ENDIAN_MODE;

	/* Initialize BG Layer Parameters to Default Values */
	st->bg_dither.cr_dither_en = false;
	st->bg_dither.cb_dither_en = false;
	st->bg_dither.y_dither_en = false;

	st->bg_color[0].color_y = 0;
	st->bg_color[0].color_cb = 128;
	st->bg_color[0].color_cr = 128;
	st->bg_color[1].color_y = 0;
	st->bg_color[1].color_cb = 128;
	st->bg_color[1].color_cr = 128;
	st->bg_color[2].color_y = 0;
	st->bg_color[2].color_cb = 128;
	st->bg_color[2].color_cr = 128;

	/* Initialize Video Mixer Parameters to Default Values */
	st->vm_csc_coeff_default = true;

	/* Initialize SDout Parameters to Default Values */
	st->sdout_sync_pin = SDOUT_SYNC_SIG_YG;
	st->sdout_vbi.wss_cvbs = true;
	st->sdout_vbi.caption_cvbs = SDOUT_INS_OTHERS;
	st->sdout_vbi.wss_y_svideo = true;
	st->sdout_vbi.caption_y_svideo = SDOUT_INS_OTHERS;
	st->sdout_vbi.cgmsa_rgb = true;
	st->sdout_vbi.wss_rgb = true;
	st->sdout_vbi.caption_rgb = SDOUT_INS_OTHERS;
	st->sdout_vbi.cgmsa_y_pb_pr = true;
	st->sdout_vbi.wss_y_pb_pr = true;
	st->sdout_vbi.caption_y_pb_pr = SDOUT_INS_OTHERS;
	st->sdout_offset_gain[0].channel = SDOUT_CHANNEL_0;
	st->sdout_offset_gain[0].offset = 0;
	st->sdout_offset_gain[0].gain = 0x800;
	st->sdout_offset_gain[1].channel = SDOUT_CHANNEL_1;
	st->sdout_offset_gain[1].offset = 0;
	st->sdout_offset_gain[1].gain = 0x800;
	st->sdout_offset_gain[2].channel = SDOUT_CHANNEL_2;
	st->sdout_offset_gain[2].offset = 0;
	st->sdout_offset_gain[2].gain = 0x800;
	st->sdout_delay.delay_y = 0x00;
	st->sdout_delay.offset_video_start = 0xfa;
	st->sdout_delay.offset_video_end = 0x00;
	st->sdout_color_sub_carrier_phase_adj = false;
	st->sdout_bri_hue_set.bright_hue_sat_adj = false;
	st->sdout_bri_hue_set.gain_brightness = 0x80;
	st->sdout_bri_hue_set.offset_brightness = 0x00;
	st->sdout_bri_hue_set.gain0_cb_hue_saturation = 0x00;
	st->sdout_bri_hue_set.gain1_cb_hue_saturation = 0x00;
	st->sdout_bri_hue_set.gain0_cr_hue_saturation = 0x00;
	st->sdout_bri_hue_set.gain1_cr_hue_saturation = 0x00;
	st->sdout_bri_hue_set.offset_cb_hue_saturation = 0x00;
	st->sdout_bri_hue_set.offset_cr_hue_saturation = 0x00;
	st->sdout_y_pb_pr_comp = false;
	st->sdout_rgb_compen.rgb_color_compensation = false;
	st->sdout_rgb_compen.max_rgb_cube = 0xeb;
	st->sdout_rgb_compen.min_rgb_cube = 0x10;
	st->sdout_cvbs_compen.cvbs_color_compensation = false;
	st->sdout_cvbs_compen.y_lower_mid = 0x200;
	st->sdout_cvbs_compen.y_bottom = 0x000;
	st->sdout_cvbs_compen.y_top = 0x3ff;
	st->sdout_cvbs_compen.y_upper_mid = 0x200;
	st->sdout_cvbs_compen.radius = 0x1ff;
	st->sdout_svideo_compen.y_color_compensation = false;
	st->sdout_svideo_compen.y_top = 0x3ff;
	st->sdout_svideo_compen.y_bottom = 0x000;
	st->sdout_svideo_compen.yc_cylinder = 0x1ff;
	st->sdout_comp_porch.back_525 = 0x8a;
	st->sdout_comp_porch.front_525 = 0x359;
	st->sdout_comp_porch.back_625 = 0x96;
	st->sdout_comp_porch.front_625 = 0x35c;
	st->sdout_rgb_sync.sync_type = SDOUT_VESA_RGB_SYNC_COMPOSITE;
	st->sdout_rgb_sync.vsync_active = TVOUT_POL_ACTIVE_HIGH;
	st->sdout_rgb_sync.hsync_active = TVOUT_POL_ACTIVE_HIGH;
	st->sdout_xtalk_cc[0].channel = SDOUT_CHANNEL_0;
	st->sdout_xtalk_cc[0].coeff2 = 0;
	st->sdout_xtalk_cc[0].coeff1 = 0;
	st->sdout_xtalk_cc[1].channel = SDOUT_CHANNEL_1;
	st->sdout_xtalk_cc[1].coeff2 = 0;
	st->sdout_xtalk_cc[1].coeff1 = 0;
	st->sdout_xtalk_cc[2].channel = SDOUT_CHANNEL_2;
	st->sdout_xtalk_cc[2].coeff2 = 0;
	st->sdout_xtalk_cc[2].coeff1 = 0;
	st->sdout_closed_capt.display_cc = 0;
	st->sdout_closed_capt.nondisplay_cc = 0;
	st->sdout_wss_525.copy_permit = SDO_525_COPY_PERMIT;
	st->sdout_wss_525.mv_psp = SDO_525_MV_PSP_OFF;
	st->sdout_wss_525.copy_info = SDO_525_COPY_INFO;
	st->sdout_wss_525.analog_on = false;
	st->sdout_wss_525.display_ratio = SDO_525_4_3_NORMAL;
	st->sdout_wss_625.surroun_f_sound = false;
	st->sdout_wss_625.copyright = false;
	st->sdout_wss_625.copy_protection = false;
	st->sdout_wss_625.text_subtitles = false;
	st->sdout_wss_625.open_subtitles = SDO_625_NO_OPEN_SUBTITLES;
	st->sdout_wss_625.camera_film = SDO_625_CAMERA;
	st->sdout_wss_625.color_encoding = SDO_625_NORMAL_PAL;
	st->sdout_wss_625.helper_signal = false;
	st->sdout_wss_625.display_ratio = SDO_625_4_3_FULL_576;
	st->sdout_cgms_525.copy_permit = SDO_525_COPY_PERMIT;
	st->sdout_cgms_525.mv_psp = SDO_525_MV_PSP_OFF;
	st->sdout_cgms_525.copy_info = SDO_525_COPY_INFO;
	st->sdout_cgms_525.analog_on = false;
	st->sdout_cgms_525.display_ratio = SDO_525_4_3_NORMAL;
	st->sdout_cgms_625.surroun_f_sound = false;
	st->sdout_cgms_625.copyright = false;
	st->sdout_cgms_625.copy_protection = false;
	st->sdout_cgms_625.text_subtitles = false;
	st->sdout_cgms_625.open_subtitles = SDO_625_NO_OPEN_SUBTITLES;
	st->sdout_cgms_625.camera_film = SDO_625_CAMERA;
	st->sdout_cgms_625.color_encoding = SDO_625_NORMAL_PAL;
	st->sdout_cgms_625.helper_signal = false;
	st->sdout_cgms_625.display_ratio = SDO_625_4_3_FULL_576;

	/* Initialize HDMI video Parameters to Default Values */
	st->hdmi_video_blue_screen.enable = false;
	st->hdmi_color_range.y_min = 1;
	st->hdmi_color_range.y_max = 254;
	st->hdmi_color_range.c_min = 1;
	st->hdmi_color_range.c_max = 254;
	st->hdmi_av_info_frame.trans_type = HDMI_DO_NOT_TANS;
	st->hdmi_av_info_frame.check_sum = 0;
	st->hdmi_av_info_frame.data = st->avi_byte;
	st->hdmi_mpg_info_frame.trans_type = HDMI_DO_NOT_TANS;
	st->hdmi_mpg_info_frame.check_sum = 0;
	st->hdmi_mpg_info_frame.data = st->mpg_byte;
	memset((void *)(st->avi_byte), 0, 13);
	memset((void *)(st->mpg_byte), 0, 5);
	st->hdmi_tg_cmd.timing_correction_en = false;
	st->hdmi_tg_cmd.bt656_sync_en = false;
	st->hdmi_tg_cmd.tg_en = false;

	/* Initialize HDMI Parameters to Default Values */
	st->hdmi_spd_info_frame.trans_type = HDMI_TRANS_EVERY_SYNC;
	st->hdmi_spd_info_frame.spd_header = NULL;
	st->hdmi_spd_info_frame.spd_data = NULL;

	st->hdcp_en = false;
	st->hdmi_audio_type = HDMI_AUDIO_PCM;

	st->tvout_param_available = true;

	return true;
}

static bool s5p_tv_if_init_vm_reg(void)
{
	u8 i = 0;
	int merr = 0;
	struct s5p_tv_status *st = &s5ptv_status;
	enum s5p_tv_o_mode out_mode = st->tvout_param.out_mode;
	enum s5p_tv_disp_mode disp_mode = st->tvout_param.disp_mode;

	bool cr_en = st->bg_dither.cr_dither_en;
	bool cb_en = st->bg_dither.cr_dither_en;
	bool y_en = st->bg_dither.cr_dither_en;

	enum s5p_vmx_burst_mode burst = st->grp_burst;
	enum s5p_endian_type endian = st->grp_endian;

	merr = s5p_vmx_init_status_reg(burst, endian);

	if (merr != 0)
		return false;

	merr = s5p_vmx_init_display_mode(disp_mode, out_mode);

	if (merr != 0)
		return false;

	s5p_vmx_init_bg_dither_enable(cr_en, cb_en, y_en);

	for (i = VMIXER_BG_COLOR_0; i <= VMIXER_BG_COLOR_2; i++) {
		merr = s5p_vmx_set_bg_color(i,
				     st->bg_color[i].color_y,
				     st->bg_color[i].color_cb,
				     st->bg_color[i].color_cr);

		if (merr != 0)
			return false;
	}

	switch (out_mode) {
	case TVOUT_OUTPUT_COMPOSITE:
	case TVOUT_OUTPUT_SVIDEO:
	case TVOUT_OUTPUT_COMPONENT_YPBPR_INERLACED:
	case TVOUT_OUTPUT_COMPONENT_YPBPR_PROGRESSIVE:
	case TVOUT_OUTPUT_COMPONENT_RGB_PROGRESSIVE:
		s5p_vmx_init_csc_coef_default(VMIXER_CSC_RGB_TO_YUV601_FR);
		break;

	case TVOUT_OUTPUT_HDMI_RGB:
	case TVOUT_OUTPUT_HDMI:
	case TVOUT_OUTPUT_DVI:
		switch (disp_mode) {
		case TVOUT_NTSC_M:
		case TVOUT_PAL_BDGHI:
		case TVOUT_PAL_M:
		case TVOUT_PAL_N:
		case TVOUT_PAL_NC:
		case TVOUT_PAL_60:
		case TVOUT_NTSC_443:
			break;

		case TVOUT_480P_60_16_9:
		case TVOUT_480P_60_4_3:
		case TVOUT_480P_59:
		case TVOUT_576P_50_16_9:
		case TVOUT_576P_50_4_3:
			s5p_vmx_init_csc_coef_default(
				VMIXER_CSC_RGB_TO_YUV601_FR);
			break;

		case TVOUT_720P_60:
		case TVOUT_720P_50:
		case TVOUT_720P_59:
		case TVOUT_1080I_60:
		case TVOUT_1080I_59:
		case TVOUT_1080I_50:
		case TVOUT_1080P_60:
		case TVOUT_1080P_30:
		case TVOUT_1080P_59:
		case TVOUT_1080P_50:
			s5p_vmx_init_csc_coef_default(
				VMIXER_CSC_RGB_TO_YUV709_FR);
			break;
		default:
			break;
		}

		break;

	default:
		TVOUTIFPRINTK("invalid tvout_param.out_mode parameter(%d)\n\r",
			      out_mode);
		return false;
	}

	s5p_vmx_start();

	return true;
}

static bool s5p_tv_if_init_sd_reg(void)
{
	u8 i = 0;
	int sderr = 0;
	struct s5p_tv_status *st = &s5ptv_status;
	enum s5p_tv_o_mode out_mode = st->tvout_param.out_mode;
	enum s5p_tv_disp_mode disp_mode = st->tvout_param.disp_mode;
	enum s5p_sd_order order = st->sdout_order;

	u32 delay = st->sdout_delay.delay_y;
	u32 off_v_start = st->sdout_delay.offset_video_start;
	u32 off_v_end = st->sdout_delay.offset_video_end;

	u32 g_bright = st->sdout_bri_hue_set.gain_brightness;
	u32 off_bright = st->sdout_bri_hue_set.offset_brightness;
	u32 g0_cb_h_sat = st->sdout_bri_hue_set.gain0_cb_hue_saturation;
	u32 g1_cb_h_sat = st->sdout_bri_hue_set.gain1_cb_hue_saturation;
	u32 g0_cr_h_sat = st->sdout_bri_hue_set.gain0_cr_hue_saturation;
	u32 g1_cr_h_sat = st->sdout_bri_hue_set.gain1_cr_hue_saturation;
	u32 off_cb_h_sat = st->sdout_bri_hue_set.offset_cb_hue_saturation;
	u32 off_cr_h_sat = st->sdout_bri_hue_set.offset_cr_hue_saturation;

	u32 max_rgb_cube = st->sdout_rgb_compen.max_rgb_cube;
	u32 min_rgb_cube = st->sdout_rgb_compen.min_rgb_cube;
	u32 y_l_m_c = st->sdout_cvbs_compen.y_lower_mid;
	u32 y_b_c = st->sdout_cvbs_compen.y_bottom;
	u32 y_t_c = st->sdout_cvbs_compen.y_top;
	u32 y_u_m_c = st->sdout_cvbs_compen.y_upper_mid;
	u32 rad_c = st->sdout_cvbs_compen.radius;
	u32 y_t_s = st->sdout_svideo_compen.y_top;
	u32 y_b_s = st->sdout_svideo_compen.y_bottom;
	u32 y_cylinder_s = st->sdout_svideo_compen.yc_cylinder;

	u32 back_525 = st->sdout_comp_porch.back_525;
	u32 front_525 = st->sdout_comp_porch.front_525;
	u32 back_625 = st->sdout_comp_porch.back_625;
	u32 front_625 = st->sdout_comp_porch.front_625;

	u32 display_cc = st->sdout_closed_capt.display_cc;
	u32 nondisplay_cc = st->sdout_closed_capt.nondisplay_cc;

	bool br_hue_sat_adj = st->sdout_bri_hue_set.bright_hue_sat_adj;
	bool wss_cvbs = st->sdout_vbi.wss_cvbs;
	bool wss_y_svideo = st->sdout_vbi.wss_y_svideo;
	bool cgmsa_rgb 	 = st->sdout_vbi.cgmsa_rgb;
	bool wss_rgb = st->sdout_vbi.wss_rgb;
	bool cgmsa_y = st->sdout_vbi.cgmsa_y_pb_pr;
	bool wss_y = st->sdout_vbi.wss_y_pb_pr;
	bool phase_adj = st->sdout_color_sub_carrier_phase_adj;
	bool ypbpr_comp = st->sdout_y_pb_pr_comp;
	bool rgb_compen = st->sdout_rgb_compen.rgb_color_compensation;
	bool y_compen = st->sdout_svideo_compen.y_color_compensation;
	bool cvbs_compen = st->sdout_cvbs_compen.cvbs_color_compensation;

	bool w5_analog_on = st->sdout_wss_525.analog_on;
	bool w6_surroun_f_sound = st->sdout_wss_625.surroun_f_sound;
	bool w6_copyright = st->sdout_wss_625.copyright;
	bool w6_copy_protection = st->sdout_wss_625.copy_protection;
	bool w6_text_subtitles = st->sdout_wss_625.text_subtitles;
	bool w6_helper_signal = st->sdout_wss_625.helper_signal;

	bool c5_analog_on = st->sdout_cgms_525.analog_on;
	bool c6_surroun_f_sound = st->sdout_cgms_625.surroun_f_sound;
	bool c6_copyright = st->sdout_cgms_625.copyright;
	bool c6_copy_protection = st->sdout_cgms_625.copy_protection;
	bool c6_text_subtitles = st->sdout_cgms_625.text_subtitles;
	bool c6_helper_signal = st->sdout_cgms_625.helper_signal;

	enum s5p_sd_level cpn_lev = st->sdout_video_scale_cfg.component_level;
	enum s5p_sd_level cps_lev = st->sdout_video_scale_cfg.composite_level;
	enum s5p_sd_vsync_ratio cpn_rat =
		st->sdout_video_scale_cfg.component_ratio;
	enum s5p_sd_vsync_ratio cps_rat =
		st->sdout_video_scale_cfg.composite_ratio;
	enum s5p_sd_closed_caption_type cap_cvbs = st->sdout_vbi.caption_cvbs;
	enum s5p_sd_closed_caption_type cap_y_svideo =
		st->sdout_vbi.caption_y_svideo;
	enum s5p_sd_closed_caption_type cap_rgb = st->sdout_vbi.caption_rgb;
	enum s5p_sd_closed_caption_type cap_y_pb_pr =
		st->sdout_vbi.caption_y_pb_pr;
	enum s5p_sd_sync_sig_pin sync_pin = st->sdout_sync_pin;
	enum s5p_sd_vesa_rgb_sync_type sync_type =
		st->sdout_rgb_sync.sync_type;
	enum s5p_tv_active_polarity vsync_active =
		st->sdout_rgb_sync.vsync_active;
	enum s5p_tv_active_polarity hsync_active =
		st->sdout_rgb_sync.hsync_active;

	enum s5p_sd_525_copy_permit w5_copy_permit =
		st->sdout_wss_525.copy_permit;
	enum s5p_sd_525_mv_psp w5_mv_psp = st->sdout_wss_525.mv_psp;
	enum s5p_sd_525_copy_info w5_copy_info = st->sdout_wss_525.copy_info;
	enum s5p_sd_525_aspect_ratio w5_display_ratio =
		st->sdout_wss_525.display_ratio;
	enum s5p_sd_625_subtitles w6_open_subtitles =
		st->sdout_wss_625.open_subtitles;
	enum s5p_sd_625_camera_film w6_camera_film =
		st->sdout_wss_625.camera_film;
	enum s5p_sd_625_color_encoding w6_color_encoding =
		st->sdout_wss_625.color_encoding;
	enum s5p_sd_625_aspect_ratio w6_display_ratio =
		st->sdout_wss_625.display_ratio;

	enum s5p_sd_525_copy_permit c5_copy_permit =
		st->sdout_cgms_525.copy_permit;
	enum s5p_sd_525_mv_psp c5_mv_psp = st->sdout_cgms_525.mv_psp;
	enum s5p_sd_525_copy_info c5_copy_info =
		st->sdout_cgms_525.copy_info;
	enum s5p_sd_525_aspect_ratio c5_display_ratio =
		st->sdout_cgms_525.display_ratio;
	enum s5p_sd_625_subtitles c6_open_subtitles =
		st->sdout_cgms_625.open_subtitles;
	enum s5p_sd_625_camera_film c6_camera_film =
		st->sdout_cgms_625.camera_film;
	enum s5p_sd_625_color_encoding c6_color_encoding =
		st->sdout_cgms_625.color_encoding;
	enum s5p_sd_625_aspect_ratio c6_display_ratio =
		st->sdout_cgms_625.display_ratio;

	s5p_sdout_sw_reset(true);

	sderr = s5p_sdout_init_display_mode(disp_mode, out_mode, order);

	if (sderr != 0)
		return false;

	sderr = s5p_sdout_init_video_scale_cfg(cpn_lev, cpn_rat,
						cps_lev, cps_rat);

	if (sderr != 0)
		return false;

	sderr = s5p_sdout_init_sync_signal_pin(sync_pin);

	if (sderr != 0)
		return false;

	sderr = s5p_sdout_init_vbi(wss_cvbs, cap_cvbs, wss_y_svideo,
			cap_y_svideo, cgmsa_rgb, wss_rgb, cap_rgb, cgmsa_y,
			wss_y, cap_y_pb_pr);

	if (sderr != 0)
		return false;

	for (i = SDOUT_CHANNEL_0; i <= SDOUT_CHANNEL_2; i++) {
		u32 offset = st->sdout_offset_gain[i].offset;
		u32 gain = st->sdout_offset_gain[i].gain;

		sderr = s5p_sdout_init_offset_gain(i, offset, gain);

		if (sderr != 0)
			return false;
	}

	s5p_sdout_init_delay(delay, off_v_start, off_v_end);

	s5p_sdout_init_schlock(phase_adj);

	s5p_sdout_init_color_compensaton_onoff(br_hue_sat_adj, ypbpr_comp,
					rgb_compen, y_compen, cvbs_compen);

	s5p_sdout_init_brightness_hue_saturation(g_bright, off_bright,
					g0_cb_h_sat, g1_cb_h_sat, g0_cr_h_sat,
					g1_cr_h_sat, off_cb_h_sat,
					off_cr_h_sat);

	s5p_sdout_init_rgb_color_compensation(max_rgb_cube, min_rgb_cube);

	s5p_sdout_init_cvbs_color_compensation(y_l_m_c, y_b_c, y_t_c,
						y_u_m_c, rad_c);

	s5p_sdout_init_svideo_color_compensation(y_t_s, y_b_s, y_cylinder_s);

	s5p_sdout_init_component_porch(back_525, front_525, back_625,
		front_625);

	sderr = s5p_sdout_init_vesa_rgb_sync(sync_type, vsync_active,
		hsync_active);

	if (sderr != 0)
		return false;

	for (i = SDOUT_CHANNEL_0; i <= SDOUT_CHANNEL_2; i++) {
		enum s5p_sd_channel_sel channel = st->sdout_xtalk_cc[i].channel;
		u32 coeff1 = st->sdout_xtalk_cc[i].coeff1;
		u32 coeff2 = st->sdout_xtalk_cc[i].coeff2;

		sderr = s5p_sdout_init_ch_xtalk_cancel_coef(channel,
			coeff2, coeff1);

		if (sderr != 0)
			return false;
	}

	s5p_sdout_init_closed_caption(display_cc, nondisplay_cc);

	sderr = s5p_sdout_init_wss525_data(w5_copy_permit, w5_mv_psp,
				w5_copy_info, w5_analog_on, w5_display_ratio);

	if (sderr != 0)
		return false;


	sderr = s5p_sdout_init_wss625_data(w6_surroun_f_sound, w6_copyright,
					w6_copy_protection, w6_text_subtitles,
					w6_open_subtitles, w6_camera_film,
					w6_color_encoding, w6_helper_signal,
					w6_display_ratio);

	if (sderr != 0)
		return false;

	sderr = s5p_sdout_init_cgmsa525_data(c5_copy_permit, c5_mv_psp,
					c5_copy_info, c5_analog_on,
					c5_display_ratio);

	if (sderr != 0)
		return false;

	sderr = s5p_sdout_init_cgmsa625_data(c6_surroun_f_sound, c6_copyright,
					c6_copy_protection, c6_text_subtitles,
					c6_open_subtitles, c6_camera_film,
					c6_color_encoding, c6_helper_signal,
					c6_display_ratio);

	if (sderr != 0)
		return false;

	/* Disable All Interrupt */
	s5p_sdout_set_interrupt_enable(false);

	/* Clear All Interrupt Pending */
	s5p_sdout_clear_interrupt_pending();

	s5p_sdout_start();

	s5p_tv_power_set_dac_onoff(true);

	for (i = SDOUT_CHANNEL_0; i <= SDOUT_CHANNEL_2; i++) {
		bool dac = st->sdout_dac_on[i];

		sderr = s5p_sdout_init_dac_power_onoff(i, dac);

		if (sderr != 0)
			return false;
	}

	return true;
}

static unsigned char s5p_tv_if_video_avi_checksum(void)
{
	u8 i;
	u32 sum = 0;
	struct s5p_tv_status *st = &s5ptv_status;

	for (i = 0; i < 13; i++)
		sum += (u32)(st->avi_byte[i]);

	return (u8)(0x100 - ((0x91 + sum) & 0xff));
}

static bool s5p_tv_if_init_avi_frame(struct tvout_output_if *tvout_if)
{
	struct s5p_tv_status *st = &s5ptv_status;

	TVOUTIFPRINTK("(%d, %d)\n\r", tvout_if->disp_mode,
			tvout_if->out_mode);

	st->hdmi_av_info_frame.trans_type = HDMI_TRANS_EVERY_SYNC;
	st->avi_byte[1] = AVI_ITU709;
	st->avi_byte[4] = AVI_NO_PIXEL_REPEAT;

	switch (tvout_if->disp_mode) {
	case TVOUT_480P_59:
	case TVOUT_480P_60_16_9:
		st->avi_byte[1] = AVI_PAR_16_9 | AVI_ITU601;
		st->avi_byte[3] = AVI_VIC_3;
		break;

	case TVOUT_480P_60_4_3:
		st->avi_byte[1] = AVI_PAR_4_3 | AVI_ITU601;
		st->avi_byte[3] = AVI_VIC_2;
		break;

	case TVOUT_576P_50_16_9:
		st->avi_byte[1] = AVI_PAR_16_9 | AVI_ITU601;
		st->avi_byte[3] = AVI_VIC_18;
		break;

	case TVOUT_576P_50_4_3:
		st->avi_byte[1] = AVI_PAR_4_3 | AVI_ITU601;
		st->avi_byte[3] = AVI_VIC_17;
		break;

	case TVOUT_720P_50:
		st->avi_byte[1] = AVI_PAR_16_9 | AVI_ITU709;
		st->avi_byte[3] = AVI_VIC_19;
		break;

	case TVOUT_720P_60:
	case TVOUT_720P_59:
		st->avi_byte[1] = AVI_PAR_16_9 | AVI_ITU709;
		st->avi_byte[3] = AVI_VIC_4;
		break;

	case TVOUT_1080I_50:
		st->avi_byte[1] = AVI_PAR_16_9 | AVI_ITU709;
		st->avi_byte[3] = AVI_VIC_20;
		break;

	case TVOUT_1080I_59:
	case TVOUT_1080I_60:
		st->avi_byte[1] = AVI_PAR_16_9 | AVI_ITU709;
		st->avi_byte[3] = AVI_VIC_5;
		break;

	case TVOUT_1080P_50:
		st->avi_byte[1] = AVI_PAR_16_9 | AVI_ITU709;
		st->avi_byte[3] = AVI_VIC_31;
		break;

	case TVOUT_1080P_30:
		st->avi_byte[1] = AVI_PAR_16_9 | AVI_ITU709;
		st->avi_byte[3] = AVI_VIC_34;

	case TVOUT_1080P_59:
	case TVOUT_1080P_60:
		st->avi_byte[1] = AVI_PAR_16_9 | AVI_ITU709;
		st->avi_byte[3] = AVI_VIC_16;
		break;

	default:
		TVOUTIFPRINTK("invalid disp_mode parameter(%d)\n\r",
			tvout_if->out_mode);
		return false;
	}

	switch (tvout_if->out_mode) {
	case TVOUT_OUTPUT_DVI:
		st->hdmi_av_info_frame.trans_type = HDMI_DO_NOT_TANS;
		st->avi_byte[0] = AVI_RGB_IF;
		break;

	case TVOUT_OUTPUT_HDMI_RGB:
		st->hdmi_av_info_frame.trans_type = HDMI_TRANS_EVERY_SYNC;
		st->avi_byte[0] = AVI_RGB_IF;
		break;

	case TVOUT_OUTPUT_HDMI:
		st->hdmi_av_info_frame.trans_type = HDMI_TRANS_EVERY_SYNC;
		st->avi_byte[0] = AVI_YCBCR444_IF;
		break;

	default:
		TVOUTIFPRINTK("invalid out_mode parameter(%d)\n\r",
			tvout_if->out_mode);
		return false;
	}

	st->avi_byte[1] |= AVI_SAME_WITH_PICTURE_AR;

	TVOUTIFPRINTK("()\n\r");
	return true;
}

static bool s5p_tv_if_init_hd_video_reg(void)
{
	int herr = 0;
	struct s5p_tv_status *st = &s5ptv_status;

	u8 cb_b = st->hdmi_video_blue_screen.cb_b;
	u8 y_g = st->hdmi_video_blue_screen.y_g;
	u8 cr_r = st->hdmi_video_blue_screen.cr_r;

	u8 y_min = st->hdmi_color_range.y_min;
	u8 y_max = st->hdmi_color_range.y_max;
	u8 c_min = st->hdmi_color_range.c_min;
	u8 c_max = st->hdmi_color_range.c_max;

	enum s5p_tv_disp_mode disp_mode = st->tvout_param.disp_mode;
	enum s5p_tv_o_mode out_mode = st->tvout_param.out_mode;

	enum s5p_hdmi_transmit *a_trans_type =
		&st->hdmi_av_info_frame.trans_type;
	u8 *a_check_sum = &st->hdmi_av_info_frame.check_sum;
	u8 *a_data = st->hdmi_av_info_frame.data;

	enum s5p_hdmi_transmit m_trans_type =
		st->hdmi_mpg_info_frame.trans_type;
	u8 m_check_sum = st->hdmi_mpg_info_frame.check_sum;
	u8 *m_data = st->hdmi_mpg_info_frame.data;

	enum s5p_hdmi_transmit s_trans_type =
		st->hdmi_spd_info_frame.trans_type;
	u8 *spd_header = st->hdmi_spd_info_frame.spd_header;
	u8 *spd_data = st->hdmi_spd_info_frame.spd_data;

	if (!s5p_tv_if_init_avi_frame(&st->tvout_param)) {
		st->tvout_param_available = false;

		return false;
	}

	herr = s5p_hdmi_video_init_display_mode(disp_mode, out_mode, a_data);

	if (herr != 0)
		return false;

	st->hdmi_av_info_frame.check_sum = s5p_tv_if_video_avi_checksum();

	if (!st->hdcp_en)
		s5p_hdmi_video_set_bluescreen(
				st->hdmi_video_blue_screen.enable,
				cb_b, y_g, cr_r);

	s5p_hdmi_video_init_color_range(y_min, y_max, c_min, c_max);

	if (out_mode == TVOUT_OUTPUT_DVI)
		s_trans_type = HDMI_DO_NOT_TANS;

	herr =  s5p_hdmi_video_init_avi(*a_trans_type,
					*a_check_sum, a_data);

	if (herr != 0)
		return false;


	herr = s5p_hdmi_video_init_mpg(m_trans_type,
					m_check_sum, m_data);

	if (herr != 0)
		return false;

	herr = s5p_hdmi_init_spd(s_trans_type,
					spd_header, spd_data);

	if (herr != 0)
		return false;

	return true;
}

static bool s5p_tv_if_init_hd_reg(void)
{
	struct s5p_tv_status *st = &s5ptv_status;
	bool timing_correction_en = st->hdmi_tg_cmd.timing_correction_en;
	bool bt656_sync_en = st->hdmi_tg_cmd.bt656_sync_en;
	bool tg_en;

	TVOUTIFPRINTK("audio type : %d, hdcp : %s)\n\r",
		st->hdmi_audio_type, st->hdcp_en ? "enabled" : "disabled");

	if (!s5p_tv_if_init_hd_video_reg())
		return false;

	switch (st->hdmi_audio_type) {
	case HDMI_AUDIO_PCM:
		/*
		* PCM, Samplingrate 44100, 16bit,
		* ignore framesize cuz stream is PCM.
		*/
		s5p_hdmi_audio_init(PCM, 44100, 16, 0);
		break;

	case HDMI_AUDIO_NO:
		break;

	default:
		TVOUTIFPRINTK("invalid hdmi_audio_type(%d)\n\r",
			st->hdmi_audio_type);
		return false;
	}

	if (!s5p_hdmi_start(st->hdmi_audio_type,
			      st->hdcp_en,
			      st->hdcp_i2c_client)) {
		return false;
	}

	st->hdmi_tg_cmd.tg_en = true;
	tg_en = st->hdmi_tg_cmd.tg_en;

	s5p_hdmi_video_init_tg_cmd(timing_correction_en, bt656_sync_en, tg_en);

	return true;
}

bool s5p_tv_if_start(void)
{
	struct s5p_tv_status *st = &s5ptv_status;
	enum s5p_tv_o_mode out_mode = st->tvout_param.out_mode;

	s5p_vmx_set_underflow_interrupt_enable(VM_VIDEO_LAYER, false);
	s5p_vmx_set_underflow_interrupt_enable(VM_GPR0_LAYER, false);
	s5p_vmx_set_underflow_interrupt_enable(VM_GPR1_LAYER, false);

	s5p_tv_if_stop();

	/* Clear All Interrupt Pending */
	s5p_vmx_clear_pend_all();

	switch (out_mode) {
	case TVOUT_OUTPUT_COMPOSITE:
	case TVOUT_OUTPUT_SVIDEO:
	case TVOUT_OUTPUT_COMPONENT_YPBPR_INERLACED:
	case TVOUT_OUTPUT_COMPONENT_YPBPR_PROGRESSIVE:
	case TVOUT_OUTPUT_COMPONENT_RGB_PROGRESSIVE:
		clk_set_parent(st->sclk_mixer, st->sclk_dac);
		break;

	case TVOUT_OUTPUT_HDMI:
	case TVOUT_OUTPUT_HDMI_RGB:
	case TVOUT_OUTPUT_DVI:
		clk_set_parent(st->sclk_mixer, st->sclk_hdmi);
		clk_set_parent(st->sclk_hdmi, st->sclk_hdmiphy);
		break;

	default:
		s5p_tv_if_stop();
		TVOUTIFPRINTK("invalid out_mode parameter(%d)\n\r",
			st->tvout_param.out_mode);
		st->tvout_param_available = false;
		return false;
	}

	if (!s5p_tv_if_init_vm_reg())
		return false;


	switch (out_mode) {
	case TVOUT_OUTPUT_COMPOSITE:
	case TVOUT_OUTPUT_SVIDEO:
	case TVOUT_OUTPUT_COMPONENT_YPBPR_INERLACED:
	case TVOUT_OUTPUT_COMPONENT_YPBPR_PROGRESSIVE:
	case TVOUT_OUTPUT_COMPONENT_RGB_PROGRESSIVE:
		if (!s5p_tv_if_init_sd_reg())
			return false;

		break;

	case TVOUT_OUTPUT_DVI:
		st->hdmi_audio_type = HDMI_AUDIO_NO;

	case TVOUT_OUTPUT_HDMI:
	case TVOUT_OUTPUT_HDMI_RGB:
		if (!s5p_tv_if_init_hd_reg())
			return false;

		break;

	default:
		s5p_tv_if_stop();
		TVOUTIFPRINTK("invalid out_mode parameter(%d)\n\r",
				out_mode);
		return false;
	}

	st->tvout_output_enable = true;

	s5p_vmx_set_underflow_interrupt_enable(VM_VIDEO_LAYER, true);
	s5p_vmx_set_underflow_interrupt_enable(VM_GPR0_LAYER, true);
	s5p_vmx_set_underflow_interrupt_enable(VM_GPR1_LAYER, true);

	/* Clear All Interrupt Pending */
	s5p_vmx_clear_pend_all();

	return true;
}

/*
 * TV cut off sequence
 * VP stop -> Mixer stop -> HDMI stop -> HDMI TG stop
 * Above sequence should be satisfied.
 */
bool s5p_tv_if_stop(void)
{
	struct s5p_tv_status *st = &s5ptv_status;

	bool t_corr_en 	 = st->hdmi_tg_cmd.timing_correction_en;
	bool sync_en 	 = st->hdmi_tg_cmd.bt656_sync_en;
	enum s5p_tv_o_mode out_mode = st->tvout_param.out_mode;

	TVOUTIFPRINTK("tvout sub sys. stopped!!\n");

	s5p_vmx_stop();

	switch (out_mode) {
	case TVOUT_OUTPUT_COMPOSITE:
	case TVOUT_OUTPUT_SVIDEO:
	case TVOUT_OUTPUT_COMPONENT_YPBPR_INERLACED:
	case TVOUT_OUTPUT_COMPONENT_YPBPR_PROGRESSIVE:
	case TVOUT_OUTPUT_COMPONENT_RGB_PROGRESSIVE:
		if (st->tvout_output_enable)
			s5p_sdout_stop();

		break;

	case TVOUT_OUTPUT_HDMI:
	case TVOUT_OUTPUT_HDMI_RGB:
	case TVOUT_OUTPUT_DVI:
		if (st->tvout_output_enable) {
			s5p_hdmi_stop();
			s5p_hdmi_video_init_tg_cmd(t_corr_en, sync_en, false);
		}

		break;

	default:
		TVOUTIFPRINTK("invalid out_mode parameter(%d)\n\r", out_mode);
		return false;
	}


	st->tvout_output_enable = false;
	st->tvout_param_available = false;

	return true;
}

/*
* before call this ftn. set the status data!!
*/
bool s5p_tv_if_set_disp(void)
{
	struct s5p_tv_status *st = &s5ptv_status;

	enum s5p_tv_disp_mode disp_mode = st->tvout_param.disp_mode;
	enum s5p_tv_o_mode out_mode = st->tvout_param.out_mode;

	TVOUTIFPRINTK("(%d, %d)\n\r", disp_mode, out_mode);

	switch (disp_mode) {
	case TVOUT_NTSC_M:
	case TVOUT_NTSC_443:
		st->sdout_video_scale_cfg.component_level =
			S5P_TV_SD_LEVEL_0IRE;
		st->sdout_video_scale_cfg.component_ratio =
			SDOUT_VTOS_RATIO_7_3;
		st->sdout_video_scale_cfg.composite_level =
			S5P_TV_SD_LEVEL_75IRE;
		st->sdout_video_scale_cfg.composite_ratio =
			SDOUT_VTOS_RATIO_10_4;
		break;

	case TVOUT_PAL_BDGHI:
	case TVOUT_PAL_M:
	case TVOUT_PAL_N:
	case TVOUT_PAL_NC:
	case TVOUT_PAL_60:
		st->sdout_video_scale_cfg.component_level =
			S5P_TV_SD_LEVEL_0IRE;
		st->sdout_video_scale_cfg.component_ratio =
			SDOUT_VTOS_RATIO_7_3;
		st->sdout_video_scale_cfg.composite_level =
			S5P_TV_SD_LEVEL_0IRE;
		st->sdout_video_scale_cfg.composite_ratio =
			SDOUT_VTOS_RATIO_7_3;
		break;

	case TVOUT_480P_60_16_9:
	case TVOUT_480P_60_4_3:
	case TVOUT_576P_50_16_9:
	case TVOUT_576P_50_4_3:
	case TVOUT_720P_50:
	case TVOUT_720P_60:
	case TVOUT_1080I_50:
	case TVOUT_1080I_60:
	case TVOUT_1080P_50:
	case TVOUT_1080P_60:
	case TVOUT_1080P_30:
	case TVOUT_480P_59:
	case TVOUT_720P_59:
	case TVOUT_1080I_59:
	case TVOUT_1080P_59:
		break;

	default:
		TVOUTIFPRINTK("invalid disp_mode parameter(%d)\n\r",
			disp_mode);
		st->tvout_param_available = false;
		return false;
	}

	switch (out_mode) {
	case TVOUT_OUTPUT_COMPOSITE:
		st->sdout_order = S5P_TV_SD_O_ORDER_COMPOSITE_Y_C_CVBS;
		st->sdout_dac_on[2] = false;
		st->sdout_dac_on[1] = false;
		st->sdout_dac_on[0] = true;
		break;

	case TVOUT_OUTPUT_SVIDEO:
		st->sdout_order = S5P_TV_SD_O_ORDER_COMPOSITE_C_Y_CVBS;
		st->sdout_dac_on[2] = true;
		st->sdout_dac_on[1] = true;
		st->sdout_dac_on[0] = false;
		break;

	case TVOUT_OUTPUT_COMPONENT_YPBPR_INERLACED:
	case TVOUT_OUTPUT_COMPONENT_YPBPR_PROGRESSIVE:
	case TVOUT_OUTPUT_COMPONENT_RGB_PROGRESSIVE:
		st->sdout_order = S5P_TV_SD_O_ORDER_COMPONENT_RBG_PRPBY;
		st->sdout_dac_on[2] = true;
		st->sdout_dac_on[1] = true;
		st->sdout_dac_on[0] = true;
		break;

	case TVOUT_OUTPUT_HDMI_RGB:
	case TVOUT_OUTPUT_HDMI:
	case TVOUT_OUTPUT_DVI:
		st->hdmi_video_blue_screen.cb_b = 0;/* 128 */;
		st->hdmi_video_blue_screen.y_g  = 0;
		st->hdmi_video_blue_screen.cr_r = 0;/* 128 */;
		break;

	default:
		TVOUTIFPRINTK("invalid out_mode parameter(%d)\n\r",
			out_mode);
		st->tvout_param_available = false;
		return false;
	}

	s5p_tv_if_start();

	return true;
}

