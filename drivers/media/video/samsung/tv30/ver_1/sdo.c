/* linux/drivers/media/video/samsung/tv20/ver_1/sdo.c
 *
 * Copyright (c) 2009 Samsung Electronics
 *		http://www.samsung.com/
 *
 * Functions for TV encoder & DAC of Samsung TVOut driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/memory.h>
#include <linux/slab.h>

#include <plat/clock.h>
#include <mach/regs-clock.h>
#include <mach/regs-sdo.h>
//#include <mach/pd.h>

#include "mixer.h"
#include "tvout_ver_1.h"
#include "sdo.h"

#ifdef COFIG_TVOUT_RAW_DBG
#define S5P_SDAOUT_DEBUG 1
#endif

#ifdef S5P_SDAOUT_DEBUG
#define SDOPRINTK(fmt, args...)	\
	printk(KERN_INFO "\t\t[SDO] %s: " fmt, __func__ , ## args)
#else
#define SDOPRINTK(fmt, args...)
#endif

enum {
	PCLK = 0,
	MUX,
	NO_OF_CLK
};

struct s5p_sdo_ctrl_private_data {
	struct s5p_sd_vscale_cfg sdo_video_scale_cfg;
	struct s5p_sd_vbi sdo_vbi;
	struct s5p_sd_offset_gain sdo_offset_gain;
	struct s5p_sd_delay sdo_delay;
	struct s5p_sd_bright_hue_saturat sdo_bri_hue_set;
	struct s5p_sd_cvbs_compensat sdo_cvbs_compen;
	struct s5p_sd_component_porch sdo_comp_porch;
	struct s5p_sd_ch_xtalk_cancellat_coeff sdo_xtalk_cc;
	struct s5p_sd_closed_caption sdo_closed_capt;
	struct s5p_sd_525_data sdo_wss_525;
	struct s5p_sd_625_data sdo_wss_625;
	struct s5p_sd_525_data sdo_cgms_525;
	struct s5p_sd_625_data sdo_cgms_625;

	enum s5p_sd_sync_sig_pin sdo_sync_pin;

	bool sdo_color_sub_carrier_phase_adj;

	bool running;

	struct s5p_tv_clk_info	clk[NO_OF_CLK];
	char			*pow_name;
	struct reg_mem_info	reg_mem;
};

/* private data area */
void __iomem		*sdo_base;

static struct s5p_sdo_ctrl_private_data s5p_sdo_ctrl_private = {
	.clk[PCLK] = {
		.name = "tvenc",
		.ptr = NULL
	},
	.clk[MUX] = {
		.name ="sclk_dac",
		.ptr = NULL
	},
		.pow_name = "tv_enc_pd",
	.reg_mem = {
		.name	= "s5p-sdo",
		.res	= NULL,
		.base	= NULL
	}
};

static int s5p_sdo_init_video_scale_cfg(enum s5p_sd_level component_level,
				enum s5p_sd_vsync_ratio component_ratio,
				enum s5p_sd_level composite_level,
				enum s5p_sd_vsync_ratio composite_ratio)
{
	u32 temp_reg = 0;

	SDOPRINTK("%d, %d, %d, %d\n\r", component_level, component_ratio,
		composite_level, composite_ratio);

	switch (component_level) {
	case SDO_LEVEL_0IRE:
		temp_reg = S5P_SDO_COMPONENT_LEVEL_SEL_0IRE;
		break;

	case SDO_LEVEL_75IRE:
		temp_reg = S5P_SDO_COMPONENT_LEVEL_SEL_75IRE;
		break;

	default:
		SDOPRINTK("invalid component_level parameter(%d)\n\r",
			component_level);
		return -1;
	}

	switch (composite_level) {
	case SDO_VTOS_RATIO_10_4:
		temp_reg |= S5P_SDO_COMPONENT_VTOS_RATIO_10_4;
		break;

	case SDO_VTOS_RATIO_7_3:
		temp_reg |= S5P_SDO_COMPONENT_VTOS_RATIO_7_3;
		break;

	default:
		SDOPRINTK("invalid composite_level parameter(%d)\n\r",
			composite_level);
		return -1;
	}

	switch (composite_level) {
	case SDO_LEVEL_0IRE:
		temp_reg |= S5P_SDO_COMPOSITE_LEVEL_SEL_0IRE;
		break;

	case SDO_LEVEL_75IRE:
		temp_reg |= S5P_SDO_COMPOSITE_LEVEL_SEL_75IRE;
		break;

	default:
		SDOPRINTK("invalid composite_ratio parameter(%d)\n\r",
			composite_ratio);
		return -1;
	}

	switch (composite_ratio) {
	case SDO_VTOS_RATIO_10_4:
		temp_reg |= S5P_SDO_COMPOSITE_VTOS_RATIO_10_4;
		break;

	case SDO_VTOS_RATIO_7_3:
		temp_reg |= S5P_SDO_COMPOSITE_VTOS_RATIO_7_3;
		break;

	default:
		SDOPRINTK("invalid component_ratio parameter(%d)\n\r",
			component_ratio);
		return -1;
	}

	writel(temp_reg, sdo_base + S5P_SDO_SCALE);

	return 0;
}

static int s5p_sdo_init_sync_signal_pin(enum s5p_sd_sync_sig_pin pin)
{
	SDOPRINTK("%d\n\r", pin);

	switch (pin) {
	case SDO_SYNC_SIG_NO:
		writel(S5P_SDO_COMPONENT_SYNC_ABSENT,
			sdo_base + S5P_SDO_SYNC);
		break;

	case SDO_SYNC_SIG_YG:
		writel(S5P_SDO_COMPONENT_SYNC_YG,
			sdo_base + S5P_SDO_SYNC);
		break;

	case SDO_SYNC_SIG_ALL:
		writel(S5P_SDO_COMPONENT_SYNC_ALL,
			sdo_base + S5P_SDO_SYNC);
		break;

	default:
		SDOPRINTK("invalid pin parameter(%d)\n\r", pin);
		return -1;
	}

	return 0;
}

int s5p_sdo_init_vbi(bool wss_cvbs,
			enum s5p_sd_closed_caption_type caption_cvbs)
{
	u32 temp_reg = 0;

	SDOPRINTK("%d, %d\n\r", wss_cvbs, caption_cvbs);

	if (wss_cvbs)
		temp_reg = S5P_SDO_CVBS_WSS_INS;
	else
		temp_reg = S5P_SDO_CVBS_NO_WSS;

	switch (caption_cvbs) {
	case SDO_NO_INS:
		temp_reg |= S5P_SDO_CVBS_NO_CLOSED_CAPTION;
		break;

	case SDO_INS_1:
		temp_reg |= S5P_SDO_CVBS_21H_CLOSED_CAPTION;
		break;

	case SDO_INS_2:
		temp_reg |= S5P_SDO_CVBS_21H_284H_CLOSED_CAPTION;
		break;

	case SDO_INS_OTHERS:
		temp_reg |= S5P_SDO_CVBS_USE_OTHERS;
		break;

	default:
		SDOPRINTK("invalid caption_cvbs parameter(%d)\n\r",
			caption_cvbs);
		return -1;
	}


	writel(temp_reg, sdo_base + S5P_SDO_VBI);

	return 0;
}

static void s5p_sdo_init_offset_gain(u32 offset, u32 gain)
{
	SDOPRINTK("%d, %d\n\r", offset, gain);

	writel(S5P_SDO_SCALE_CONV_OFFSET(offset) |
		S5P_SDO_SCALE_CONV_GAIN(gain),
		sdo_base + S5P_SDO_SCALE_CH0);
}

static void s5p_sdo_init_delay(u32 delay_y,
			u32 offset_video_start,
			u32 offset_video_end)
{
	SDOPRINTK("%d, %d, %d\n\r", delay_y, offset_video_start,
		offset_video_end);

	writel(S5P_SDO_DELAY_YTOC(delay_y) |
		S5P_SDO_ACTIVE_START_OFFSET(offset_video_start) |
		S5P_SDO_ACTIVE_END_OFFSET(offset_video_end),
		sdo_base + S5P_SDO_YCDELAY);
}

static void s5p_sdo_init_schlock(bool color_sucarrier_pha_adj)
{
	SDOPRINTK("%d\n\r", color_sucarrier_pha_adj);

	if (color_sucarrier_pha_adj)
		writel(S5P_SDO_COLOR_SC_PHASE_ADJ,
			sdo_base + S5P_SDO_SCHLOCK);
	else
		writel(S5P_SDO_COLOR_SC_PHASE_NOADJ,
			sdo_base + S5P_SDO_SCHLOCK);
}

static void s5p_sdo_init_color_compensaton_onoff(bool bright_hue_saturation_adj,
					bool y_cvbs_color_compensation)
{
	u32 temp_reg = 0;

	SDOPRINTK("%d, %d, %d, %d, %d\n\r", bright_hue_saturation_adj,
		y_cvbs_color_compensation);

	if (bright_hue_saturation_adj)
		temp_reg &= ~S5P_SDO_COMPONENT_BHS_ADJ_OFF;
	else
		temp_reg |= S5P_SDO_COMPONENT_BHS_ADJ_OFF;

	if (y_cvbs_color_compensation)
		temp_reg &= ~S5P_SDO_COMPONENT_CVBS_COMP_OFF;
	else
		temp_reg |= S5P_SDO_COMPONENT_CVBS_COMP_OFF;


	writel(temp_reg, sdo_base + S5P_SDO_CCCON);
}

static void s5p_sdo_init_brightness_hue_saturation(u32 gain_brightness,
					u32 offset_brightness,
					u32 gain0_cb_hue_saturation,
					u32 gain1_cb_hue_saturation,
					u32 gain0_cr_hue_saturation,
					u32 gain1_cr_hue_saturation,
					u32 offset_cb_hue_saturation,
					u32 offset_cr_hue_saturation)
{
	SDOPRINTK("%d, %d, %d, %d, %d, %d, %d, %d\n\r", gain_brightness,
		offset_brightness, gain0_cb_hue_saturation,
		gain1_cb_hue_saturation, gain0_cr_hue_saturation,
		gain1_cr_hue_saturation, offset_cb_hue_saturation,
		offset_cr_hue_saturation);

	writel(S5P_SDO_BRIGHTNESS_GAIN(gain_brightness) |
		S5P_SDO_BRIGHTNESS_OFFSET(offset_brightness),
			sdo_base + S5P_SDO_YSCALE);

	writel(S5P_SDO_HS_CB_GAIN0(gain0_cb_hue_saturation) |
		S5P_SDO_HS_CB_GAIN1(gain1_cb_hue_saturation),
			sdo_base + S5P_SDO_CBSCALE);

	writel(S5P_SDO_HS_CR_GAIN0(gain0_cr_hue_saturation) |
		S5P_SDO_HS_CR_GAIN1(gain1_cr_hue_saturation),
			sdo_base + S5P_SDO_CRSCALE);

	writel(S5P_SDO_HS_CR_OFFSET(offset_cr_hue_saturation) |
		S5P_SDO_HS_CB_OFFSET(offset_cb_hue_saturation),
			sdo_base + S5P_SDO_CB_CR_OFFSET);
}

static void s5p_sdo_init_cvbs_color_compensation(u32 y_lower_mid,
					u32 y_bottom,
					u32 y_top,
					u32 y_upper_mid,
					u32 radius)
{
	SDOPRINTK("%d, %d, %d, %d, %d\n\r", y_lower_mid, y_bottom, y_top,
		y_upper_mid, radius);

	writel(S5P_SDO_Y_LOWER_MID_CVBS_CORN(y_lower_mid) |
		S5P_SDO_Y_BOTTOM_CVBS_CORN(y_bottom),
			sdo_base + S5P_SDO_CVBS_CC_Y1);
	writel(S5P_SDO_Y_TOP_CVBS_CORN(y_top) |
		S5P_SDO_Y_UPPER_MID_CVBS_CORN(y_upper_mid),
			sdo_base + S5P_SDO_CVBS_CC_Y2);
	writel(S5P_SDO_RADIUS_CVBS_CORN(radius),
			sdo_base + S5P_SDO_CVBS_CC_C);
}

static void s5p_sdo_init_component_porch(u32 back_525, u32 front_525,
					u32 back_625, u32 front_625)
{
	SDOPRINTK("%d, %d, %d, %d\n\r",
			back_525, front_525, back_625, front_625);

	writel(S5P_SDO_COMPONENT_525_BP(back_525) |
		S5P_SDO_COMPONENT_525_FP(front_525),
			sdo_base + S5P_SDO_CSC_525_PORCH);
	writel(S5P_SDO_COMPONENT_625_BP(back_625) |
		S5P_SDO_COMPONENT_625_FP(front_625),
			sdo_base + S5P_SDO_CSC_625_PORCH);
}

static void s5p_sdo_init_ch_xtalk_cancel_coef(u32 coeff2, u32 coeff1)
{
	SDOPRINTK("%d, %d\n\r", coeff2, coeff1);

	writel(S5P_SDO_XTALK_COEF02(coeff2) |
		S5P_SDO_XTALK_COEF01(coeff1),
			sdo_base + S5P_SDO_XTALK0);
}

static void s5p_sdo_init_closed_caption(u32 display_cc, u32 non_display_cc)
{
	SDOPRINTK("%d, %d\n\r", display_cc, non_display_cc);

	writel(S5P_SDO_DISPLAY_CC_CAPTION(display_cc) |
		S5P_SDO_NON_DISPLAY_CC_CAPTION(non_display_cc),
		sdo_base + S5P_SDO_ARMCC);
}

static u32 s5p_sdo_init_wss_cgms_crc(u32 value)
{
	u8 i;
	u8 CGMS[14], CRC[6], OLD_CRC;
	u32 temp_in;

	temp_in = value;

	for (i = 0; i < 14; i++)
		CGMS[i] = (u8)(temp_in >> i) & 0x1 ;

	/* initialize state */
	for (i = 0; i < 6; i++)
		CRC[i] = 0x1;

	/* round 20 */
	for (i = 0; i < 14; i++) {
		OLD_CRC = CRC[0];
		CRC[0] = CRC[1];
		CRC[1] = CRC[2];
		CRC[2] = CRC[3];
		CRC[3] = CRC[4];
		CRC[4] = OLD_CRC ^ CGMS[i] ^ CRC[5];
		CRC[5] = OLD_CRC ^ CGMS[i];
	}

	/* recompose to return crc */
	temp_in &= 0x3fff;

	for (i = 0; i < 6; i++)
		temp_in |= ((u32)(CRC[i] & 0x1) << i);

	return temp_in;
}

static int s5p_sdo_init_wss525_data(enum s5p_sd_525_copy_permit copy_permit,
				enum s5p_sd_525_mv_psp mv_psp,
				enum s5p_sd_525_copy_info copy_info,
				bool analog_on,
				enum s5p_sd_525_aspect_ratio display_ratio)
{
	u32 temp_reg = 0;

	SDOPRINTK("%d, %d, %d, %d\n\r", copy_permit, mv_psp, copy_info,
		display_ratio);

	switch (copy_permit) {
	case SDO_525_COPY_PERMIT:
		temp_reg = S5P_SDO_WORD2_WSS525_COPY_PERMIT;
		break;

	case SDO_525_ONECOPY_PERMIT:
		temp_reg = S5P_SDO_WORD2_WSS525_ONECOPY_PERMIT;
		break;

	case SDO_525_NOCOPY_PERMIT:
		temp_reg = S5P_SDO_WORD2_WSS525_NOCOPY_PERMIT;
		break;

	default:
		SDOPRINTK("invalid copy_permit parameter(%d)\n\r", copy_permit);
		return -1;
	}

	switch (mv_psp) {
	case SDO_525_MV_PSP_OFF:
		temp_reg |= S5P_SDO_WORD2_WSS525_MV_PSP_OFF;
		break;

	case SDO_525_MV_PSP_ON_2LINE_BURST:
		temp_reg |= S5P_SDO_WORD2_WSS525_MV_PSP_ON_2LINE_BURST;
		break;

	case SDO_525_MV_PSP_ON_BURST_OFF:
		temp_reg |= S5P_SDO_WORD2_WSS525_MV_PSP_ON_BURST_OFF;
		break;

	case SDO_525_MV_PSP_ON_4LINE_BURST:
		temp_reg |= S5P_SDO_WORD2_WSS525_MV_PSP_ON_4LINE_BURST;
		break;

	default:
		SDOPRINTK("invalid mv_psp parameter(%d)\n\r", mv_psp);
		return -1;
	}

	switch (copy_info) {
	case SDO_525_COPY_INFO:
		temp_reg |= S5P_SDO_WORD1_WSS525_COPY_INFO;
		break;

	case SDO_525_DEFAULT:
		temp_reg |= S5P_SDO_WORD1_WSS525_DEFAULT;
		break;

	default:
		SDOPRINTK("invalid copy_info parameter(%d)\n\r", copy_info);
		return -1;
	}

	if (analog_on)
		temp_reg |= S5P_SDO_WORD2_WSS525_ANALOG_ON;
	else
		temp_reg |= S5P_SDO_WORD2_WSS525_ANALOG_OFF;

	switch (display_ratio) {
	case SDO_525_COPY_PERMIT:
		temp_reg |= S5P_SDO_WORD0_WSS525_4_3_NORMAL;
		break;

	case SDO_525_ONECOPY_PERMIT:
		temp_reg |= S5P_SDO_WORD0_WSS525_16_9_ANAMORPIC;
		break;

	case SDO_525_NOCOPY_PERMIT:
		temp_reg |= S5P_SDO_WORD0_WSS525_4_3_LETTERBOX;
		break;

	default:
		SDOPRINTK("invalid display_ratio parameter(%d)\n\r",
			display_ratio);
		return -1;
	}

	writel(temp_reg |
		S5P_SDO_CRC_WSS525(s5p_sdo_init_wss_cgms_crc(temp_reg)),
		sdo_base + S5P_SDO_WSS525);

	return 0;
}

static int s5p_sdo_init_wss625_data(bool surround_sound,
				 bool copyright,
				 bool copy_protection,
				 bool text_subtitles,
				 enum s5p_sd_625_subtitles open_subtitles,
				 enum s5p_sd_625_camera_film camera_film,
				 enum s5p_sd_625_color_encoding color_encoding,
				 bool helper_signal,
				 enum s5p_sd_625_aspect_ratio display_ratio)
{
	u32 temp_reg = 0;

	SDOPRINTK("%d, %d, %d, %d, %d, %d, %d, %d, %d\n\r",
		 surround_sound, copyright, copy_protection,
		 text_subtitles, open_subtitles, camera_film,
		 color_encoding, helper_signal, display_ratio);

	if (surround_sound)
		temp_reg = S5P_SDO_WSS625_SURROUND_SOUND_ENABLE;
	else
		temp_reg = S5P_SDO_WSS625_SURROUND_SOUND_DISABLE;

	if (copyright)
		temp_reg |= S5P_SDO_WSS625_COPYRIGHT;
	else
		temp_reg |= S5P_SDO_WSS625_NO_COPYRIGHT;

	if (copy_protection)
		temp_reg |= S5P_SDO_WSS625_COPY_RESTRICTED;
	else
		temp_reg |= S5P_SDO_WSS625_COPY_NOT_RESTRICTED;

	if (text_subtitles)
		temp_reg |= S5P_SDO_WSS625_TELETEXT_SUBTITLES;
	else
		temp_reg |= S5P_SDO_WSS625_TELETEXT_NO_SUBTITLES;

	switch (open_subtitles) {
	case SDO_625_NO_OPEN_SUBTITLES:
		temp_reg |= S5P_SDO_WSS625_NO_OPEN_SUBTITLES;
		break;

	case SDO_625_INACT_OPEN_SUBTITLES:
		temp_reg |= S5P_SDO_WSS625_INACT_OPEN_SUBTITLES;
		break;

	case SDO_625_OUTACT_OPEN_SUBTITLES:
		temp_reg |= S5P_SDO_WSS625_OUTACT_OPEN_SUBTITLES;
		break;

	default:
		SDOPRINTK("invalid open_subtitles parameter(%d)\n\r",
			open_subtitles);
		return -1;
	}

	switch (camera_film) {
	case SDO_625_CAMERA:
		temp_reg |= S5P_SDO_WSS625_CAMERA;
		break;

	case SDO_625_FILM:
		temp_reg |= S5P_SDO_WSS625_FILM;
		break;

	default:
		SDOPRINTK("invalid camera_film parameter(%d)\n\r",
			camera_film);
		return -1;
	}

	switch (color_encoding) {
	case SDO_625_NORMAL_PAL:
		temp_reg |= S5P_SDO_WSS625_NORMAL_PAL;
		break;

	case SDO_625_MOTION_ADAPTIVE_COLORPLUS:
		temp_reg |= S5P_SDO_WSS625_MOTION_ADAPTIVE_COLORPLUS;
		break;

	default:
		SDOPRINTK("invalid color_encoding parameter(%d)\n\r",
			color_encoding);
		return -1;
	}

	if (helper_signal)
		temp_reg |= S5P_SDO_WSS625_HELPER_SIG;
	else
		temp_reg |= S5P_SDO_WSS625_HELPER_NO_SIG;

	switch (display_ratio) {
	case SDO_625_4_3_FULL_576:
		temp_reg |= S5P_SDO_WSS625_4_3_FULL_576;
		break;

	case SDO_625_14_9_LETTERBOX_CENTER_504:
		temp_reg |= S5P_SDO_WSS625_14_9_LETTERBOX_CENTER_504;
		break;

	case SDO_625_14_9_LETTERBOX_TOP_504:
		temp_reg |= S5P_SDO_WSS625_14_9_LETTERBOX_TOP_504;
		break;

	case SDO_625_16_9_LETTERBOX_CENTER_430:
		temp_reg |= S5P_SDO_WSS625_16_9_LETTERBOX_CENTER_430;
		break;

	case SDO_625_16_9_LETTERBOX_TOP_430:
		temp_reg |= S5P_SDO_WSS625_16_9_LETTERBOX_TOP_430;
		break;

	case SDO_625_16_9_LETTERBOX_CENTER:
		temp_reg |= S5P_SDO_WSS625_16_9_LETTERBOX_CENTER;
		break;

	case SDO_625_14_9_FULL_CENTER_576:
		temp_reg |= S5P_SDO_WSS625_14_9_FULL_CENTER_576;
		break;

	case SDO_625_16_9_ANAMORPIC_576:
		temp_reg |= S5P_SDO_WSS625_16_9_ANAMORPIC_576;
		break;

	default:
		SDOPRINTK("invalid display_ratio parameter(%d)\n\r",
			display_ratio);
		return -1;
	}

	writel(temp_reg, sdo_base + S5P_SDO_WSS625);

	return 0;
}

static int s5p_sdo_init_cgmsa525_data(enum s5p_sd_525_copy_permit copy_permit,
				enum s5p_sd_525_mv_psp mv_psp,
				enum s5p_sd_525_copy_info copy_info,
				bool analog_on,
				enum s5p_sd_525_aspect_ratio display_ratio)
{
	u32 temp_reg = 0;

	SDOPRINTK("%d, %d, %d, %d\n\r", copy_permit, mv_psp, copy_info,
		display_ratio);

	switch (copy_permit) {
	case SDO_525_COPY_PERMIT:
		temp_reg = S5P_SDO_WORD2_CGMS525_COPY_PERMIT;
		break;

	case SDO_525_ONECOPY_PERMIT:
		temp_reg = S5P_SDO_WORD2_CGMS525_ONECOPY_PERMIT;
		break;

	case SDO_525_NOCOPY_PERMIT:
		temp_reg = S5P_SDO_WORD2_CGMS525_NOCOPY_PERMIT;
		break;

	default:
		SDOPRINTK("invalid copy_permit parameter(%d)\n\r", copy_permit);
		return -1;
	}

	switch (mv_psp) {
	case SDO_525_MV_PSP_OFF:
		temp_reg |= S5P_SDO_WORD2_CGMS525_MV_PSP_OFF;
		break;

	case SDO_525_MV_PSP_ON_2LINE_BURST:
		temp_reg |= S5P_SDO_WORD2_CGMS525_MV_PSP_ON_2LINE_BURST;
		break;

	case SDO_525_MV_PSP_ON_BURST_OFF:
		temp_reg |= S5P_SDO_WORD2_CGMS525_MV_PSP_ON_BURST_OFF;
		break;

	case SDO_525_MV_PSP_ON_4LINE_BURST:
		temp_reg |= S5P_SDO_WORD2_CGMS525_MV_PSP_ON_4LINE_BURST;
		break;

	default:
		SDOPRINTK("invalid mv_psp parameter(%d)\n\r", mv_psp);
		return -1;
	}

	switch (copy_info) {
	case SDO_525_COPY_INFO:
		temp_reg |= S5P_SDO_WORD1_CGMS525_COPY_INFO;
		break;

	case SDO_525_DEFAULT:
		temp_reg |= S5P_SDO_WORD1_CGMS525_DEFAULT;
		break;

	default:
		SDOPRINTK("invalid copy_info parameter(%d)\n\r", copy_info);
		return -1;
	}

	if (analog_on)
		temp_reg |= S5P_SDO_WORD2_CGMS525_ANALOG_ON;
	else
		temp_reg |= S5P_SDO_WORD2_CGMS525_ANALOG_OFF;

	switch (display_ratio) {
	case SDO_525_COPY_PERMIT:
		temp_reg |= S5P_SDO_WORD0_CGMS525_4_3_NORMAL;
		break;

	case SDO_525_ONECOPY_PERMIT:
		temp_reg |= S5P_SDO_WORD0_CGMS525_16_9_ANAMORPIC;
		break;

	case SDO_525_NOCOPY_PERMIT:
		temp_reg |= S5P_SDO_WORD0_CGMS525_4_3_LETTERBOX;
		break;

	default:
		SDOPRINTK("invalid display_ratio parameter(%d)\n\r",
			display_ratio);
		return -1;
	}

	writel(temp_reg | S5P_SDO_CRC_CGMS525(
		s5p_sdo_init_wss_cgms_crc(temp_reg)),
		sdo_base + S5P_SDO_CGMS525);

	return 0;
}

static int s5p_sdo_init_cgmsa625_data(bool surround_sound,
				bool copyright,
				bool copy_protection,
				bool text_subtitles,
				enum s5p_sd_625_subtitles open_subtitles,
				enum s5p_sd_625_camera_film camera_film,
				enum s5p_sd_625_color_encoding color_encoding,
				bool helper_signal,
				enum s5p_sd_625_aspect_ratio display_ratio)
{
	u32 temp_reg = 0;

	SDOPRINTK("%d, %d, %d, %d, %d, %d, %d, %d, %d\n\r", surround_sound,
		copyright, copy_protection,
		text_subtitles, open_subtitles,
		camera_film, color_encoding, helper_signal,
		display_ratio);

	if (surround_sound)
		temp_reg = S5P_SDO_CGMS625_SURROUND_SOUND_ENABLE;
	else
		temp_reg = S5P_SDO_CGMS625_SURROUND_SOUND_DISABLE;

	if (copyright)
		temp_reg |= S5P_SDO_CGMS625_COPYRIGHT;
	else
		temp_reg |= S5P_SDO_CGMS625_NO_COPYRIGHT;

	if (copy_protection)
		temp_reg |= S5P_SDO_CGMS625_COPY_RESTRICTED;
	else
		temp_reg |= S5P_SDO_CGMS625_COPY_NOT_RESTRICTED;

	if (text_subtitles)
		temp_reg |= S5P_SDO_CGMS625_TELETEXT_SUBTITLES;
	else
		temp_reg |= S5P_SDO_CGMS625_TELETEXT_NO_SUBTITLES;

	switch (open_subtitles) {
	case SDO_625_NO_OPEN_SUBTITLES:
		temp_reg |= S5P_SDO_CGMS625_NO_OPEN_SUBTITLES;
		break;

	case SDO_625_INACT_OPEN_SUBTITLES:
		temp_reg |= S5P_SDO_CGMS625_INACT_OPEN_SUBTITLES;
		break;

	case SDO_625_OUTACT_OPEN_SUBTITLES:
		temp_reg |= S5P_SDO_CGMS625_OUTACT_OPEN_SUBTITLES;
		break;

	default:
		SDOPRINTK("invalid open_subtitles parameter(%d)\n\r",
			open_subtitles);
		return -1;
	}

	switch (camera_film) {
	case SDO_625_CAMERA:
		temp_reg |= S5P_SDO_CGMS625_CAMERA;
		break;

	case SDO_625_FILM:
		temp_reg |= S5P_SDO_CGMS625_FILM;
		break;

	default:
		SDOPRINTK("invalid camera_film parameter(%d)\n\r",
			camera_film);
		return -1;
	}

	switch (color_encoding) {
	case SDO_625_NORMAL_PAL:
		temp_reg |= S5P_SDO_CGMS625_NORMAL_PAL;
		break;

	case SDO_625_MOTION_ADAPTIVE_COLORPLUS:
		temp_reg |= S5P_SDO_CGMS625_MOTION_ADAPTIVE_COLORPLUS;
		break;

	default:
		SDOPRINTK("invalid color_encoding parameter(%d)\n\r",
			color_encoding);
		return -1;
	}

	if (helper_signal)
		temp_reg |= S5P_SDO_CGMS625_HELPER_SIG;
	else
		temp_reg |= S5P_SDO_CGMS625_HELPER_NO_SIG;

	switch (display_ratio) {
	case SDO_625_4_3_FULL_576:
		temp_reg |= S5P_SDO_CGMS625_4_3_FULL_576;
		break;

	case SDO_625_14_9_LETTERBOX_CENTER_504:
		temp_reg |= S5P_SDO_CGMS625_14_9_LETTERBOX_CENTER_504;
		break;

	case SDO_625_14_9_LETTERBOX_TOP_504:
		temp_reg |= S5P_SDO_CGMS625_14_9_LETTERBOX_TOP_504;
		break;

	case SDO_625_16_9_LETTERBOX_CENTER_430:
		temp_reg |= S5P_SDO_CGMS625_16_9_LETTERBOX_CENTER_430;
		break;

	case SDO_625_16_9_LETTERBOX_TOP_430:
		temp_reg |= S5P_SDO_CGMS625_16_9_LETTERBOX_TOP_430;
		break;

	case SDO_625_16_9_LETTERBOX_CENTER:
		temp_reg |= S5P_SDO_CGMS625_16_9_LETTERBOX_CENTER;
		break;

	case SDO_625_14_9_FULL_CENTER_576:
		temp_reg |= S5P_SDO_CGMS625_14_9_FULL_CENTER_576;
		break;

	case SDO_625_16_9_ANAMORPIC_576:
		temp_reg |= S5P_SDO_CGMS625_16_9_ANAMORPIC_576;
		break;

	default:
		SDOPRINTK("invalid display_ratio parameter(%d)\n\r",
			display_ratio);
		return -1;
	}

	writel(temp_reg, sdo_base + S5P_SDO_CGMS625);

	return 0;
}

static int s5p_sdo_init_antialias_filter_coeff_default(
					enum s5p_sd_level composite_level,
					enum s5p_sd_vsync_ratio composite_ratio)
{
	SDOPRINTK("%d, %d\n\r", composite_level, composite_ratio);

	switch (composite_level) {
	case SDO_LEVEL_0IRE:
		switch (composite_ratio) {
		case SDO_VTOS_RATIO_10_4:
			writel(0x00000000, sdo_base + S5P_SDO_Y3);
			writel(0x00000000, sdo_base + S5P_SDO_Y4);
			writel(0x00000000, sdo_base + S5P_SDO_Y5);
			writel(0x00000000, sdo_base + S5P_SDO_Y6);
			writel(0x00000000, sdo_base + S5P_SDO_Y7);
			writel(0x00000000, sdo_base + S5P_SDO_Y8);
			writel(0x00000000, sdo_base + S5P_SDO_Y9);
			writel(0x00000000, sdo_base + S5P_SDO_Y10);
			writel(0x0000029a, sdo_base + S5P_SDO_Y11);
			writel(0x00000000, sdo_base + S5P_SDO_CB0);
			writel(0x00000000, sdo_base + S5P_SDO_CB1);
			writel(0x00000000, sdo_base + S5P_SDO_CB2);
			writel(0x00000000, sdo_base + S5P_SDO_CB3);
			writel(0x00000000, sdo_base + S5P_SDO_CB4);
			writel(0x00000001, sdo_base + S5P_SDO_CB5);
			writel(0x00000007, sdo_base + S5P_SDO_CB6);
			writel(0x00000015, sdo_base + S5P_SDO_CB7);
			writel(0x0000002b, sdo_base + S5P_SDO_CB8);
			writel(0x00000045, sdo_base + S5P_SDO_CB9);
			writel(0x00000059, sdo_base + S5P_SDO_CB10);
			writel(0x00000061, sdo_base + S5P_SDO_CB11);
			writel(0x00000000, sdo_base + S5P_SDO_CR1);
			writel(0x00000000, sdo_base + S5P_SDO_CR2);
			writel(0x00000000, sdo_base + S5P_SDO_CR3);
			writel(0x00000000, sdo_base + S5P_SDO_CR4);
			writel(0x00000002, sdo_base + S5P_SDO_CR5);
			writel(0x0000000a, sdo_base + S5P_SDO_CR6);
			writel(0x0000001e, sdo_base + S5P_SDO_CR7);
			writel(0x0000003d, sdo_base + S5P_SDO_CR8);
			writel(0x00000061, sdo_base + S5P_SDO_CR9);
			writel(0x0000007a, sdo_base + S5P_SDO_CR10);
			writel(0x0000008f, sdo_base + S5P_SDO_CR11);
			break;

		case SDO_VTOS_RATIO_7_3:
			writel(0x00000000, sdo_base + S5P_SDO_Y0);
			writel(0x00000000, sdo_base + S5P_SDO_Y1);
			writel(0x00000000, sdo_base + S5P_SDO_Y2);
			writel(0x00000000, sdo_base + S5P_SDO_Y3);
			writel(0x00000000, sdo_base + S5P_SDO_Y4);
			writel(0x00000000, sdo_base + S5P_SDO_Y5);
			writel(0x00000000, sdo_base + S5P_SDO_Y6);
			writel(0x00000000, sdo_base + S5P_SDO_Y7);
			writel(0x00000000, sdo_base + S5P_SDO_Y8);
			writel(0x00000000, sdo_base + S5P_SDO_Y9);
			writel(0x00000000, sdo_base + S5P_SDO_Y10);
			writel(0x00000281, sdo_base + S5P_SDO_Y11);
			writel(0x00000000, sdo_base + S5P_SDO_CB0);
			writel(0x00000000, sdo_base + S5P_SDO_CB1);
			writel(0x00000000, sdo_base + S5P_SDO_CB2);
			writel(0x00000000, sdo_base + S5P_SDO_CB3);
			writel(0x00000000, sdo_base + S5P_SDO_CB4);
			writel(0x00000001, sdo_base + S5P_SDO_CB5);
			writel(0x00000007, sdo_base + S5P_SDO_CB6);
			writel(0x00000015, sdo_base + S5P_SDO_CB7);
			writel(0x0000002a, sdo_base + S5P_SDO_CB8);
			writel(0x00000044, sdo_base + S5P_SDO_CB9);
			writel(0x00000057, sdo_base + S5P_SDO_CB10);
			writel(0x0000005f, sdo_base + S5P_SDO_CB11);
			writel(0x00000000, sdo_base + S5P_SDO_CR1);
			writel(0x00000000, sdo_base + S5P_SDO_CR2);
			writel(0x00000000, sdo_base + S5P_SDO_CR3);
			writel(0x00000000, sdo_base + S5P_SDO_CR4);
			writel(0x00000002, sdo_base + S5P_SDO_CR5);
			writel(0x0000000a, sdo_base + S5P_SDO_CR6);
			writel(0x0000001d, sdo_base + S5P_SDO_CR7);
			writel(0x0000003c, sdo_base + S5P_SDO_CR8);
			writel(0x0000005f, sdo_base + S5P_SDO_CR9);
			writel(0x0000007b, sdo_base + S5P_SDO_CR10);
			writel(0x00000086, sdo_base + S5P_SDO_CR11);
			break;

		default:
			SDOPRINTK("invalid composite_ratio parameter(%d)\n\r",
				composite_ratio);
			return -1;
		}

		break;

	case SDO_LEVEL_75IRE:
		switch (composite_ratio) {
		case SDO_VTOS_RATIO_10_4:
			writel(0x00000000, sdo_base + S5P_SDO_Y0);
			writel(0x00000000, sdo_base + S5P_SDO_Y1);
			writel(0x00000000, sdo_base + S5P_SDO_Y2);
			writel(0x00000000, sdo_base + S5P_SDO_Y3);
			writel(0x00000000, sdo_base + S5P_SDO_Y4);
			writel(0x00000000, sdo_base + S5P_SDO_Y5);
			writel(0x00000000, sdo_base + S5P_SDO_Y6);
			writel(0x00000000, sdo_base + S5P_SDO_Y7);
			writel(0x00000000, sdo_base + S5P_SDO_Y8);
			writel(0x00000000, sdo_base + S5P_SDO_Y9);
			writel(0x00000000, sdo_base + S5P_SDO_Y10);
			writel(0x0000025d, sdo_base + S5P_SDO_Y11);
			writel(0x00000000, sdo_base + S5P_SDO_CB0);
			writel(0x00000000, sdo_base + S5P_SDO_CB1);
			writel(0x00000000, sdo_base + S5P_SDO_CB2);
			writel(0x00000000, sdo_base + S5P_SDO_CB3);
			writel(0x00000000, sdo_base + S5P_SDO_CB4);
			writel(0x00000001, sdo_base + S5P_SDO_CB5);
			writel(0x00000007, sdo_base + S5P_SDO_CB6);
			writel(0x00000014, sdo_base + S5P_SDO_CB7);
			writel(0x00000028, sdo_base + S5P_SDO_CB8);
			writel(0x0000003f, sdo_base + S5P_SDO_CB9);
			writel(0x00000052, sdo_base + S5P_SDO_CB10);
			writel(0x0000005a, sdo_base + S5P_SDO_CB11);
			writel(0x00000000, sdo_base + S5P_SDO_CR1);
			writel(0x00000000, sdo_base + S5P_SDO_CR2);
			writel(0x00000000, sdo_base + S5P_SDO_CR3);
			writel(0x00000000, sdo_base + S5P_SDO_CR4);
			writel(0x00000001, sdo_base + S5P_SDO_CR5);
			writel(0x00000009, sdo_base + S5P_SDO_CR6);
			writel(0x0000001c, sdo_base + S5P_SDO_CR7);
			writel(0x00000039, sdo_base + S5P_SDO_CR8);
			writel(0x0000005a, sdo_base + S5P_SDO_CR9);
			writel(0x00000074, sdo_base + S5P_SDO_CR10);
			writel(0x0000007e, sdo_base + S5P_SDO_CR11);
			break;

		case SDO_VTOS_RATIO_7_3:
			writel(0x00000000, sdo_base + S5P_SDO_Y0);
			writel(0x00000000, sdo_base + S5P_SDO_Y1);
			writel(0x00000000, sdo_base + S5P_SDO_Y2);
			writel(0x00000000, sdo_base + S5P_SDO_Y3);
			writel(0x00000000, sdo_base + S5P_SDO_Y4);
			writel(0x00000000, sdo_base + S5P_SDO_Y5);
			writel(0x00000000, sdo_base + S5P_SDO_Y6);
			writel(0x00000000, sdo_base + S5P_SDO_Y7);
			writel(0x00000000, sdo_base + S5P_SDO_Y8);
			writel(0x00000000, sdo_base + S5P_SDO_Y9);
			writel(0x00000000, sdo_base + S5P_SDO_Y10);
			writel(0x00000251, sdo_base + S5P_SDO_Y11);
			writel(0x00000000, sdo_base + S5P_SDO_CB0);
			writel(0x00000000, sdo_base + S5P_SDO_CB1);
			writel(0x00000000, sdo_base + S5P_SDO_CB2);
			writel(0x00000000, sdo_base + S5P_SDO_CB3);
			writel(0x00000000, sdo_base + S5P_SDO_CB4);
			writel(0x00000001, sdo_base + S5P_SDO_CB5);
			writel(0x00000006, sdo_base + S5P_SDO_CB6);
			writel(0x00000013, sdo_base + S5P_SDO_CB7);
			writel(0x00000028, sdo_base + S5P_SDO_CB8);
			writel(0x0000003f, sdo_base + S5P_SDO_CB9);
			writel(0x00000051, sdo_base + S5P_SDO_CB10);
			writel(0x00000056, sdo_base + S5P_SDO_CB11);
			writel(0x00000000, sdo_base + S5P_SDO_CR1);
			writel(0x00000000, sdo_base + S5P_SDO_CR2);
			writel(0x00000000, sdo_base + S5P_SDO_CR3);
			writel(0x00000000, sdo_base + S5P_SDO_CR4);
			writel(0x00000002, sdo_base + S5P_SDO_CR5);
			writel(0x00000005, sdo_base + S5P_SDO_CR6);
			writel(0x00000018, sdo_base + S5P_SDO_CR7);
			writel(0x00000037, sdo_base + S5P_SDO_CR8);
			writel(0x0000005A, sdo_base + S5P_SDO_CR9);
			writel(0x00000076, sdo_base + S5P_SDO_CR10);
			writel(0x0000007e, sdo_base + S5P_SDO_CR11);
			break;

		default:
			SDOPRINTK("invalid composite_ratio parameter(%d)\n\r",
				composite_ratio);
			return -1;
		}

		break;

	default:
		SDOPRINTK("invalid composite_level parameter(%d)\n\r",
			composite_level);
		return -1;
	}

	return 0;
}

static int s5p_sdo_init_display_mode(enum s5p_tv_disp_mode disp_mode,
				enum s5p_sd_order order)
{
	u32 temp_reg = 0;

	SDOPRINTK("%d, %d\n\r", disp_mode, order);

	switch (disp_mode) {
	case TVOUT_NTSC_M:
		temp_reg |= S5P_SDO_NTSC_M;
		s5p_sdo_init_video_scale_cfg(SDO_LEVEL_0IRE,
				SDO_VTOS_RATIO_7_3, SDO_LEVEL_75IRE,
				SDO_VTOS_RATIO_10_4);

		s5p_sdo_init_antialias_filter_coeff_default(
			SDO_LEVEL_75IRE,
			SDO_VTOS_RATIO_10_4);
		break;

	case TVOUT_PAL_BDGHI:
		temp_reg |= S5P_SDO_PAL_BGHID;
		s5p_sdo_init_video_scale_cfg(SDO_LEVEL_0IRE,
			SDO_VTOS_RATIO_7_3, SDO_LEVEL_0IRE,
			SDO_VTOS_RATIO_7_3);

		s5p_sdo_init_antialias_filter_coeff_default(
			SDO_LEVEL_0IRE,
			SDO_VTOS_RATIO_7_3);
		break;

	case TVOUT_PAL_M:
		temp_reg |= S5P_SDO_PAL_M;
		s5p_sdo_init_video_scale_cfg(SDO_LEVEL_0IRE,
				SDO_VTOS_RATIO_7_3, SDO_LEVEL_0IRE,
				SDO_VTOS_RATIO_7_3);

		s5p_sdo_init_antialias_filter_coeff_default(
			SDO_LEVEL_0IRE,
			SDO_VTOS_RATIO_7_3);
		break;

	case TVOUT_PAL_N:
		temp_reg |= S5P_SDO_PAL_N;
		s5p_sdo_init_video_scale_cfg(SDO_LEVEL_0IRE,
				SDO_VTOS_RATIO_7_3, SDO_LEVEL_0IRE,
				SDO_VTOS_RATIO_7_3);

		s5p_sdo_init_antialias_filter_coeff_default(
			SDO_LEVEL_75IRE,
			SDO_VTOS_RATIO_10_4);
		break;

	case TVOUT_PAL_NC:
		temp_reg |= S5P_SDO_PAL_NC;
		s5p_sdo_init_video_scale_cfg(SDO_LEVEL_0IRE,
				SDO_VTOS_RATIO_7_3, SDO_LEVEL_0IRE,
				SDO_VTOS_RATIO_7_3);

		s5p_sdo_init_antialias_filter_coeff_default(
			SDO_LEVEL_0IRE,
			SDO_VTOS_RATIO_7_3);
		break;

	case TVOUT_PAL_60:
		temp_reg |= S5P_SDO_PAL_60;
		s5p_sdo_init_video_scale_cfg(SDO_LEVEL_0IRE,
				SDO_VTOS_RATIO_7_3, SDO_LEVEL_0IRE,
				SDO_VTOS_RATIO_7_3);
		s5p_sdo_init_antialias_filter_coeff_default(
			SDO_LEVEL_0IRE,
			SDO_VTOS_RATIO_7_3);
		break;

	case TVOUT_NTSC_443:
		temp_reg |= S5P_SDO_NTSC_443;
		s5p_sdo_init_video_scale_cfg(SDO_LEVEL_0IRE,
				SDO_VTOS_RATIO_7_3, SDO_LEVEL_75IRE,
				SDO_VTOS_RATIO_10_4);
		s5p_sdo_init_antialias_filter_coeff_default(
			SDO_LEVEL_75IRE,
			SDO_VTOS_RATIO_10_4);
		break;

	default:
		SDOPRINTK("invalid disp_mode parameter(%d)\n\r", disp_mode);
		return -1;
	}

	temp_reg |= S5P_SDO_COMPOSITE | S5P_SDO_INTERLACED;

	switch (order) {

	case SDO_O_ORDER_COMPOSITE_CVBS_Y_C:
		temp_reg |= S5P_SDO_DAC2_CVBS | S5P_SDO_DAC1_Y |
				S5P_SDO_DAC0_C;
		break;

	case SDO_O_ORDER_COMPOSITE_CVBS_C_Y:
		temp_reg |= S5P_SDO_DAC2_CVBS | S5P_SDO_DAC1_C |
				S5P_SDO_DAC0_Y;
		break;

	case SDO_O_ORDER_COMPOSITE_Y_C_CVBS:
		temp_reg |= S5P_SDO_DAC2_Y | S5P_SDO_DAC1_C |
				S5P_SDO_DAC0_CVBS;
		break;

	case SDO_O_ORDER_COMPOSITE_Y_CVBS_C:
		temp_reg |= S5P_SDO_DAC2_Y | S5P_SDO_DAC1_CVBS |
				S5P_SDO_DAC0_C;
		break;

	case SDO_O_ORDER_COMPOSITE_C_CVBS_Y:
		temp_reg |= S5P_SDO_DAC2_C | S5P_SDO_DAC1_CVBS |
				S5P_SDO_DAC0_Y;
		break;

	case SDO_O_ORDER_COMPOSITE_C_Y_CVBS:
		temp_reg |= S5P_SDO_DAC2_C | S5P_SDO_DAC1_Y |
				S5P_SDO_DAC0_CVBS;
		break;

	default:
		SDOPRINTK("invalid order parameter(%d)\n\r", order);
		return -1;
	}

	writel(temp_reg, sdo_base + S5P_SDO_CONFIG);

	return 0;
}

static void s5p_sdo_clock_on(bool on)
{
	SDOPRINTK("%d\n\r", on);

	if (on)
		writel(S5P_SDO_TVOUT_CLOCK_ON, sdo_base + S5P_SDO_CLKCON);
	else {
		mdelay(100);

		writel(S5P_SDO_TVOUT_CLOCK_OFF, sdo_base + S5P_SDO_CLKCON);
	}
}

static void s5p_sdo_dac_on(bool on)
{
	SDOPRINTK("%d\n\r", on);

	if (on) {
		writel(S5P_SDO_POWER_ON_DAC, sdo_base + S5P_SDO_DAC);

		writel(S5P_DAC_ENABLE, S5P_DAC_CONTROL);
	}
	else {
		writel(S5P_DAC_DISABLE, S5P_DAC_CONTROL);

		writel(S5P_SDO_POWER_DOWN_DAC, sdo_base + S5P_SDO_DAC);
	}
}

static void s5p_sdo_sw_reset(bool active)
{
	SDOPRINTK("%d\n\r", active);

	if (active)
		writel(readl(sdo_base + S5P_SDO_CLKCON) |
			S5P_SDO_TVOUT_SW_RESET,
				sdo_base + S5P_SDO_CLKCON);
	else
		writel(readl(sdo_base + S5P_SDO_CLKCON) &
			~S5P_SDO_TVOUT_SW_RESET,
				sdo_base + S5P_SDO_CLKCON);
}

static void s5p_sdo_set_interrupt_enable(bool vsync_intr_en)
{
	SDOPRINTK("%d\n\r", vsync_intr_en);

	if (vsync_intr_en)
		writel(readl(sdo_base + S5P_SDO_IRQMASK) &
			~S5P_SDO_VSYNC_IRQ_DISABLE,
				sdo_base + S5P_SDO_IRQMASK);
	else
		writel(readl(sdo_base + S5P_SDO_IRQMASK) |
			S5P_SDO_VSYNC_IRQ_DISABLE,
				sdo_base + S5P_SDO_IRQMASK);
}

static void s5p_sdo_clear_interrupt_pending(void)
{
	writel(readl(sdo_base + S5P_SDO_IRQ) | S5P_SDO_VSYNC_IRQ_PEND,
			sdo_base + S5P_SDO_IRQ);
}

void s5p_sdo_init(void __iomem *addr)
{
	sdo_base = addr;
}


static void s5p_sdo_ctrl_init_private(void)
{
	struct s5p_sdo_ctrl_private_data *st = &s5p_sdo_ctrl_private;

	st->sdo_sync_pin = SDO_SYNC_SIG_YG;

	st->sdo_vbi.wss_cvbs = true;
	st->sdo_vbi.caption_cvbs = SDO_INS_OTHERS;

	st->sdo_offset_gain.offset = 0;
	st->sdo_offset_gain.gain = 0x800;

	st->sdo_delay.delay_y = 0x00;
	st->sdo_delay.offset_video_start = 0xfa;
	st->sdo_delay.offset_video_end = 0x00;

	st->sdo_color_sub_carrier_phase_adj = false;

	st->sdo_bri_hue_set.bright_hue_sat_adj = false;
	st->sdo_bri_hue_set.gain_brightness = 0x80;
	st->sdo_bri_hue_set.offset_brightness = 0x00;
	st->sdo_bri_hue_set.gain0_cb_hue_saturation = 0x00;
	st->sdo_bri_hue_set.gain1_cb_hue_saturation = 0x00;
	st->sdo_bri_hue_set.gain0_cr_hue_saturation = 0x00;
	st->sdo_bri_hue_set.gain1_cr_hue_saturation = 0x00;
	st->sdo_bri_hue_set.offset_cb_hue_saturation = 0x00;
	st->sdo_bri_hue_set.offset_cr_hue_saturation = 0x00;

	st->sdo_cvbs_compen.cvbs_color_compensation = false;
	st->sdo_cvbs_compen.y_lower_mid = 0x200;
	st->sdo_cvbs_compen.y_bottom = 0x000;
	st->sdo_cvbs_compen.y_top = 0x3ff;
	st->sdo_cvbs_compen.y_upper_mid = 0x200;
	st->sdo_cvbs_compen.radius = 0x1ff;
	st->sdo_comp_porch.back_525 = 0x8a;
	st->sdo_comp_porch.front_525 = 0x359;
	st->sdo_comp_porch.back_625 = 0x96;
	st->sdo_comp_porch.front_625 = 0x35c;
	st->sdo_xtalk_cc.coeff2 = 0;
	st->sdo_xtalk_cc.coeff1 = 0;

	st->sdo_closed_capt.display_cc = 0;
	st->sdo_closed_capt.nondisplay_cc = 0;
	st->sdo_wss_525.copy_permit = SDO_525_COPY_PERMIT;
	st->sdo_wss_525.mv_psp = SDO_525_MV_PSP_OFF;
	st->sdo_wss_525.copy_info = SDO_525_COPY_INFO;
	st->sdo_wss_525.analog_on = false;
	st->sdo_wss_525.display_ratio = SDO_525_4_3_NORMAL;
	st->sdo_wss_625.surroun_f_sound = false;
	st->sdo_wss_625.copyright = false;
	st->sdo_wss_625.copy_protection = false;
	st->sdo_wss_625.text_subtitles = false;
	st->sdo_wss_625.open_subtitles = SDO_625_NO_OPEN_SUBTITLES;
	st->sdo_wss_625.camera_film = SDO_625_CAMERA;
	st->sdo_wss_625.color_encoding = SDO_625_NORMAL_PAL;
	st->sdo_wss_625.helper_signal = false;
	st->sdo_wss_625.display_ratio = SDO_625_4_3_FULL_576;
	st->sdo_cgms_525.copy_permit = SDO_525_COPY_PERMIT;
	st->sdo_cgms_525.mv_psp = SDO_525_MV_PSP_OFF;
	st->sdo_cgms_525.copy_info = SDO_525_COPY_INFO;
	st->sdo_cgms_525.analog_on = false;
	st->sdo_cgms_525.display_ratio = SDO_525_4_3_NORMAL;
	st->sdo_cgms_625.surroun_f_sound = false;
	st->sdo_cgms_625.copyright = false;
	st->sdo_cgms_625.copy_protection = false;
	st->sdo_cgms_625.text_subtitles = false;
	st->sdo_cgms_625.open_subtitles = SDO_625_NO_OPEN_SUBTITLES;
	st->sdo_cgms_625.camera_film = SDO_625_CAMERA;
	st->sdo_cgms_625.color_encoding = SDO_625_NORMAL_PAL;
	st->sdo_cgms_625.helper_signal = false;
	st->sdo_cgms_625.display_ratio = SDO_625_4_3_FULL_576;

	st->running = false;
}

static bool s5p_sdo_ctrl_set_reg(enum s5p_tv_disp_mode disp_mode)
{
	struct s5p_sdo_ctrl_private_data *st = &s5p_sdo_ctrl_private;
	int sderr = 0;

	u32 delay = st->sdo_delay.delay_y;
	u32 off_v_start = st->sdo_delay.offset_video_start;
	u32 off_v_end = st->sdo_delay.offset_video_end;

	u32 g_bright = st->sdo_bri_hue_set.gain_brightness;
	u32 off_bright = st->sdo_bri_hue_set.offset_brightness;
	u32 g0_cb_h_sat = st->sdo_bri_hue_set.gain0_cb_hue_saturation;
	u32 g1_cb_h_sat = st->sdo_bri_hue_set.gain1_cb_hue_saturation;
	u32 g0_cr_h_sat = st->sdo_bri_hue_set.gain0_cr_hue_saturation;
	u32 g1_cr_h_sat = st->sdo_bri_hue_set.gain1_cr_hue_saturation;
	u32 off_cb_h_sat = st->sdo_bri_hue_set.offset_cb_hue_saturation;
	u32 off_cr_h_sat = st->sdo_bri_hue_set.offset_cr_hue_saturation;

	u32 y_l_m_c = st->sdo_cvbs_compen.y_lower_mid;
	u32 y_b_c = st->sdo_cvbs_compen.y_bottom;
	u32 y_t_c = st->sdo_cvbs_compen.y_top;
	u32 y_u_m_c = st->sdo_cvbs_compen.y_upper_mid;
	u32 rad_c = st->sdo_cvbs_compen.radius;

	u32 back_525 = st->sdo_comp_porch.back_525;
	u32 front_525 = st->sdo_comp_porch.front_525;
	u32 back_625 = st->sdo_comp_porch.back_625;
	u32 front_625 = st->sdo_comp_porch.front_625;

	u32 display_cc = st->sdo_closed_capt.display_cc;
	u32 nondisplay_cc = st->sdo_closed_capt.nondisplay_cc;

	bool br_hue_sat_adj = st->sdo_bri_hue_set.bright_hue_sat_adj;
	bool wss_cvbs = st->sdo_vbi.wss_cvbs;
	bool phase_adj = st->sdo_color_sub_carrier_phase_adj;
	bool cvbs_compen = st->sdo_cvbs_compen.cvbs_color_compensation;

	bool w5_analog_on = st->sdo_wss_525.analog_on;
	bool w6_surroun_f_sound = st->sdo_wss_625.surroun_f_sound;
	bool w6_copyright = st->sdo_wss_625.copyright;
	bool w6_copy_protection = st->sdo_wss_625.copy_protection;
	bool w6_text_subtitles = st->sdo_wss_625.text_subtitles;
	bool w6_helper_signal = st->sdo_wss_625.helper_signal;

	bool c5_analog_on = st->sdo_cgms_525.analog_on;
	bool c6_surroun_f_sound = st->sdo_cgms_625.surroun_f_sound;
	bool c6_copyright = st->sdo_cgms_625.copyright;
	bool c6_copy_protection = st->sdo_cgms_625.copy_protection;
	bool c6_text_subtitles = st->sdo_cgms_625.text_subtitles;
	bool c6_helper_signal = st->sdo_cgms_625.helper_signal;

	enum s5p_sd_level cpn_lev = st->sdo_video_scale_cfg.component_level;
	enum s5p_sd_level cps_lev = st->sdo_video_scale_cfg.composite_level;
	enum s5p_sd_vsync_ratio cpn_rat =
		st->sdo_video_scale_cfg.component_ratio;
	enum s5p_sd_vsync_ratio cps_rat =
		st->sdo_video_scale_cfg.composite_ratio;
	enum s5p_sd_closed_caption_type cap_cvbs = st->sdo_vbi.caption_cvbs;
	enum s5p_sd_sync_sig_pin sync_pin = st->sdo_sync_pin;

	enum s5p_sd_525_copy_permit w5_copy_permit =
		st->sdo_wss_525.copy_permit;
	enum s5p_sd_525_mv_psp w5_mv_psp = st->sdo_wss_525.mv_psp;
	enum s5p_sd_525_copy_info w5_copy_info = st->sdo_wss_525.copy_info;
	enum s5p_sd_525_aspect_ratio w5_display_ratio =
		st->sdo_wss_525.display_ratio;
	enum s5p_sd_625_subtitles w6_open_subtitles =
		st->sdo_wss_625.open_subtitles;
	enum s5p_sd_625_camera_film w6_camera_film =
		st->sdo_wss_625.camera_film;
	enum s5p_sd_625_color_encoding w6_color_encoding =
		st->sdo_wss_625.color_encoding;
	enum s5p_sd_625_aspect_ratio w6_display_ratio =
		st->sdo_wss_625.display_ratio;

	enum s5p_sd_525_copy_permit c5_copy_permit =
		st->sdo_cgms_525.copy_permit;
	enum s5p_sd_525_mv_psp c5_mv_psp = st->sdo_cgms_525.mv_psp;
	enum s5p_sd_525_copy_info c5_copy_info =
		st->sdo_cgms_525.copy_info;
	enum s5p_sd_525_aspect_ratio c5_display_ratio =
		st->sdo_cgms_525.display_ratio;
	enum s5p_sd_625_subtitles c6_open_subtitles =
		st->sdo_cgms_625.open_subtitles;
	enum s5p_sd_625_camera_film c6_camera_film =
		st->sdo_cgms_625.camera_film;
	enum s5p_sd_625_color_encoding c6_color_encoding =
		st->sdo_cgms_625.color_encoding;
	enum s5p_sd_625_aspect_ratio c6_display_ratio =
		st->sdo_cgms_625.display_ratio;

	s5p_sdo_sw_reset(true);

	sderr = s5p_sdo_init_display_mode(
			disp_mode, SDO_O_ORDER_COMPOSITE_Y_C_CVBS);

	if (sderr != 0)
		return false;

	sderr = s5p_sdo_init_video_scale_cfg(cpn_lev, cpn_rat,
						cps_lev, cps_rat);

	if (sderr != 0)
		return false;

	sderr = s5p_sdo_init_sync_signal_pin(sync_pin);

	if (sderr != 0)
		return false;

	sderr = s5p_sdo_init_vbi(wss_cvbs, cap_cvbs);

	if (sderr != 0)
		return false;

	s5p_sdo_init_offset_gain(
		st->sdo_offset_gain.offset, st->sdo_offset_gain.gain);

	s5p_sdo_init_delay(delay, off_v_start, off_v_end);

	s5p_sdo_init_schlock(phase_adj);

	s5p_sdo_init_color_compensaton_onoff(br_hue_sat_adj, cvbs_compen);

	s5p_sdo_init_brightness_hue_saturation(g_bright, off_bright,
					g0_cb_h_sat, g1_cb_h_sat, g0_cr_h_sat,
					g1_cr_h_sat, off_cb_h_sat,
					off_cr_h_sat);

	s5p_sdo_init_cvbs_color_compensation(y_l_m_c, y_b_c, y_t_c,
						y_u_m_c, rad_c);

	s5p_sdo_init_component_porch(back_525, front_525, back_625,
		front_625);

	s5p_sdo_init_ch_xtalk_cancel_coef(
		st->sdo_xtalk_cc.coeff2, st->sdo_xtalk_cc.coeff1);

	s5p_sdo_init_closed_caption(display_cc, nondisplay_cc);

	sderr = s5p_sdo_init_wss525_data(w5_copy_permit, w5_mv_psp,
				w5_copy_info, w5_analog_on, w5_display_ratio);
	if (sderr != 0)
		return false;


	sderr = s5p_sdo_init_wss625_data(w6_surroun_f_sound, w6_copyright,
					w6_copy_protection, w6_text_subtitles,
					w6_open_subtitles, w6_camera_film,
					w6_color_encoding, w6_helper_signal,
					w6_display_ratio);
	if (sderr != 0)
		return false;

	sderr = s5p_sdo_init_cgmsa525_data(c5_copy_permit, c5_mv_psp,
					c5_copy_info, c5_analog_on,
					c5_display_ratio);
	if (sderr != 0)
		return false;

	sderr = s5p_sdo_init_cgmsa625_data(c6_surroun_f_sound, c6_copyright,
					c6_copy_protection, c6_text_subtitles,
					c6_open_subtitles, c6_camera_film,
					c6_color_encoding, c6_helper_signal,
					c6_display_ratio);
	if (sderr != 0)
		return false;

	s5p_sdo_set_interrupt_enable(false);

	s5p_sdo_clear_interrupt_pending();

	s5p_sdo_clock_on(true);
	s5p_sdo_dac_on(true);

	return true;
}

static void s5p_sdo_ctrl_internal_stop(void)
{
	s5p_sdo_clock_on(false);
	s5p_sdo_dac_on(false);
}

static void s5p_sdo_ctrl_clock(bool on)
{
	if (on) {
		/* power control function is not implemented yet */
		//s5pv210_pd_enable(s5p_sdo_ctrl_private.pow_name);
		clk_enable(s5p_sdo_ctrl_private.clk[MUX].ptr);
		clk_enable(s5p_sdo_ctrl_private.clk[PCLK].ptr);
	} else {
		clk_disable(s5p_sdo_ctrl_private.clk[PCLK].ptr);
		clk_disable(s5p_sdo_ctrl_private.clk[MUX].ptr);
		//s5pv210_pd_disable(s5p_sdo_ctrl_private.pow_name);
	}
	
	mdelay(50);
}

void s5p_sdo_ctrl_stop(void)
{
	if (s5p_sdo_ctrl_private.running) {
		s5p_sdo_ctrl_internal_stop();
		s5p_sdo_ctrl_clock(0);

		s5p_sdo_ctrl_private.running = false;
	}
}

int s5p_sdo_ctrl_start(enum s5p_tv_disp_mode	disp_mode)
{
	struct s5p_sdo_ctrl_private_data *sdo_private = &s5p_sdo_ctrl_private;

	switch (disp_mode) {
	case TVOUT_NTSC_M:
	case TVOUT_NTSC_443:
		sdo_private->sdo_video_scale_cfg.component_level =
			SDO_LEVEL_0IRE;
		sdo_private->sdo_video_scale_cfg.component_ratio =
			SDO_VTOS_RATIO_7_3;
		sdo_private->sdo_video_scale_cfg.composite_level =
			SDO_LEVEL_75IRE;
		sdo_private->sdo_video_scale_cfg.composite_ratio =
			SDO_VTOS_RATIO_10_4;
		break;

	case TVOUT_PAL_BDGHI:
	case TVOUT_PAL_M:
	case TVOUT_PAL_N:
	case TVOUT_PAL_NC:
	case TVOUT_PAL_60:
		sdo_private->sdo_video_scale_cfg.component_level =
			SDO_LEVEL_0IRE;
		sdo_private->sdo_video_scale_cfg.component_ratio =
			SDO_VTOS_RATIO_7_3;
		sdo_private->sdo_video_scale_cfg.composite_level =
			SDO_LEVEL_0IRE;
		sdo_private->sdo_video_scale_cfg.composite_ratio =
			SDO_VTOS_RATIO_7_3;
		break;

	default:
		SDOPRINTK("invalid disp_mode(%d) for SDO\n",
			disp_mode);
		goto err_on_s5p_sdo_start;
	}

	if (sdo_private->running)
		s5p_sdo_ctrl_internal_stop();
	else {
		s5p_sdo_ctrl_clock(1);

		sdo_private->running = true;
	}

	if (!s5p_sdo_ctrl_set_reg(disp_mode))
		goto err_on_s5p_sdo_start;

	return 0;

err_on_s5p_sdo_start:
	return -1;
}

extern int s5p_tv_map_resource_mem(
		struct platform_device *pdev,
		char *name,
		void __iomem **base,
		struct resource **res);
extern void s5p_tv_unmap_resource_mem(void __iomem *base, struct resource *res);

int s5p_sdo_ctrl_constructor(struct platform_device *pdev)
{
	int ret;
	int i, j;

	ret = s5p_tv_map_resource_mem(
		pdev,
		s5p_sdo_ctrl_private.reg_mem.name,
		&(s5p_sdo_ctrl_private.reg_mem.base),
		&(s5p_sdo_ctrl_private.reg_mem.res));

	if (ret)
		goto err_on_res;

	for (i = PCLK; i < NO_OF_CLK; i++) {
		s5p_sdo_ctrl_private.clk[i].ptr =
			clk_get(&pdev->dev, s5p_sdo_ctrl_private.clk[i].name);

		if (IS_ERR(s5p_sdo_ctrl_private.clk[i].ptr)) {
		printk(KERN_ERR	"Failed to find clock %s\n",
				s5p_sdo_ctrl_private.clk[i].name);
		ret = -ENOENT;
		goto err_on_clk;
		}
	}
	
	s5p_sdo_ctrl_init_private();
	s5p_sdo_init(s5p_sdo_ctrl_private.reg_mem.base);

	return 0;

err_on_clk:
	for (j = 0; j < i; j++)
		clk_put(s5p_sdo_ctrl_private.clk[j].ptr);

	s5p_tv_unmap_resource_mem(
		s5p_sdo_ctrl_private.reg_mem.base,
		s5p_sdo_ctrl_private.reg_mem.res);

err_on_res:
	return ret;	
}

void s5p_sdo_ctrl_destructor(void)
{
	int i;

	s5p_tv_unmap_resource_mem(
		s5p_sdo_ctrl_private.reg_mem.base,
		s5p_sdo_ctrl_private.reg_mem.res);

	for (i = PCLK; i < NO_OF_CLK; i++)
		if (s5p_sdo_ctrl_private.clk[i].ptr) {
			if (s5p_sdo_ctrl_private.running)
				clk_disable(s5p_sdo_ctrl_private.clk[i].ptr);
			clk_put(s5p_sdo_ctrl_private.clk[i].ptr);
	}
}
