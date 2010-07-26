/* linux/drivers/media/video/samsung/tv20/ver_1/extern_func_tvout.h
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsung.com/
 *
 * header for Samsung TVOut driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef _LINUX_EXTERN_FUNC_TVOUT_H
#define _LINUX_EXTERN_FUNC_TVOUT_H

#include <linux/platform_device.h>
#include <linux/irqreturn.h>
#include <linux/i2c.h>

#define CEC_IOC_MAGIC        'c'
#define CEC_IOC_SETLADDR     _IOW(CEC_IOC_MAGIC, 0, unsigned int)

#define HDMI_START_NUM 0x1000


typedef int (*hdmi_isr)(int irq);


enum cec_state {
	STATE_RX,
	STATE_TX,
	STATE_DONE,
	STATE_ERROR
};

struct cec_rx_struct {
	spinlock_t lock;
	wait_queue_head_t waitq;
	atomic_t state;
	u8 *buffer;
	unsigned int size;
};

struct cec_tx_struct {
	wait_queue_head_t waitq;
	atomic_t state;
};

enum s5p_tv_audio_codec_type {
	PCM = 1, AC3, MP3, WMA
};

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
	TVOUT_OUTPUT_COMPOSITE,
	TVOUT_OUTPUT_SVIDEO,
	TVOUT_OUTPUT_COMPONENT_YPBPR_INERLACED,
	TVOUT_OUTPUT_COMPONENT_YPBPR_PROGRESSIVE,
	TVOUT_OUTPUT_COMPONENT_RGB_PROGRESSIVE,
	TVOUT_OUTPUT_HDMI,
	TVOUT_OUTPUT_HDMI_RGB,
	TVOUT_OUTPUT_DVI
};

enum s5p_hdmi_transmit {
	HDMI_DO_NOT_TANS = 0,
	HDMI_TRANS_ONCE,
	HDMI_TRANS_EVERY_SYNC
};

enum s5p_hdmi_audio_type {
	HDMI_AUDIO_NO,
	HDMI_AUDIO_PCM
};

enum s5p_tv_hdmi_interrrupt {
	HDMI_IRQ_PIN_POLAR_CTL	= 7,
	HDMI_IRQ_GLOBAL		= 6,
	HDMI_IRQ_I2S		= 5,
	HDMI_IRQ_CEC		= 4,
	HDMI_IRQ_HPD_PLUG	= 3,
	HDMI_IRQ_HPD_UNPLUG	= 2,
	HDMI_IRQ_SPDIF		= 1,
	HDMI_IRQ_HDCP		= 0
};

enum s5p_tv_vmx_layer {
	VM_VIDEO_LAYER = 2,
	VM_GPR0_LAYER = 0,
	VM_GPR1_LAYER = 1
};

enum s5p_tv_vmx_bg_color_num {
	VMIXER_BG_COLOR_0 = 0,
	VMIXER_BG_COLOR_1 = 1,
	VMIXER_BG_COLOR_2 = 2
};

enum s5p_vmx_burst_mode {
	VM_BURST_8 = 0,
	VM_BURST_16 = 1
};

enum s5p_tv_vmx_color_fmt {
	VM_DIRECT_RGB565  = 4,
	VM_DIRECT_RGB1555 = 5,
	VM_DIRECT_RGB4444 = 6,
	VM_DIRECT_RGB8888 = 7
};

enum s5p_endian_type {
	TVOUT_LITTLE_ENDIAN_MODE = 0,
	TVOUT_BIG_ENDIAN_MODE = 1
};

enum s5p_tv_vmx_csc_type {
	VMIXER_CSC_RGB_TO_YUV601_LR,
	VMIXER_CSC_RGB_TO_YUV601_FR,
	VMIXER_CSC_RGB_TO_YUV709_LR,
	VMIXER_CSC_RGB_TO_YUV709_FR
};

enum s5p_sd_level {
	S5P_TV_SD_LEVEL_0IRE,
	S5P_TV_SD_LEVEL_75IRE
};

enum s5p_sd_vsync_ratio {
	SDOUT_VTOS_RATIO_10_4,
	SDOUT_VTOS_RATIO_7_3
};

enum s5p_sd_sync_sig_pin {
	SDOUT_SYNC_SIG_NO,
	SDOUT_SYNC_SIG_YG,
	SDOUT_SYNC_SIG_ALL
};

enum s5p_sd_closed_caption_type {
	SDOUT_NO_INS,
	SDOUT_INS_1,
	SDOUT_INS_2,
	SDOUT_INS_OTHERS
};

enum s5p_sd_channel_sel {
	SDOUT_CHANNEL_0 = 0,
	SDOUT_CHANNEL_1 = 1,
	SDOUT_CHANNEL_2 = 2
};

enum s5p_sd_vesa_rgb_sync_type {
	SDOUT_VESA_RGB_SYNC_COMPOSITE,
	SDOUT_VESA_RGB_SYNC_SEPARATE
};

enum s5p_tv_active_polarity {
	TVOUT_POL_ACTIVE_LOW,
	TVOUT_POL_ACTIVE_HIGH
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

enum s5p_sd_order {
	S5P_TV_SD_O_ORDER_COMPONENT_RGB_PRYPB,
	S5P_TV_SD_O_ORDER_COMPONENT_RBG_PRPBY,
	S5P_TV_SD_O_ORDER_COMPONENT_BGR_PBYPR,
	S5P_TV_SD_O_ORDER_COMPONENT_BRG_PBPRY,
	S5P_TV_SD_O_ORDER_COMPONENT_GRB_YPRPB,
	S5P_TV_SD_O_ORDER_COMPONENT_GBR_YPBPR,
	S5P_TV_SD_O_ORDER_COMPOSITE_CVBS_Y_C,
	S5P_TV_SD_O_ORDER_COMPOSITE_CVBS_C_Y,
	S5P_TV_SD_O_ORDER_COMPOSITE_Y_C_CVBS,
	S5P_TV_SD_O_ORDER_COMPOSITE_Y_CVBS_C,
	S5P_TV_SD_O_ORDER_COMPOSITE_C_CVBS_Y,
	S5P_TV_SD_O_ORDER_COMPOSITE_C_Y_CVBS
};

enum s5p_vp_field {
	VPROC_TOP_FIELD,
	VPROC_BOTTOM_FIELD
};

enum s5p_vp_poly_coeff {
	VPROC_POLY8_Y0_LL = 0,
	VPROC_POLY8_Y0_LH,
	VPROC_POLY8_Y0_HL,
	VPROC_POLY8_Y0_HH,
	VPROC_POLY8_Y1_LL,
	VPROC_POLY8_Y1_LH,
	VPROC_POLY8_Y1_HL,
	VPROC_POLY8_Y1_HH,
	VPROC_POLY8_Y2_LL,
	VPROC_POLY8_Y2_LH,
	VPROC_POLY8_Y2_HL,
	VPROC_POLY8_Y2_HH,
	VPROC_POLY8_Y3_LL,
	VPROC_POLY8_Y3_LH,
	VPROC_POLY8_Y3_HL,
	VPROC_POLY8_Y3_HH,
	VPROC_POLY4_Y0_LL = 32,
	VPROC_POLY4_Y0_LH,
	VPROC_POLY4_Y0_HL,
	VPROC_POLY4_Y0_HH,
	VPROC_POLY4_Y1_LL,
	VPROC_POLY4_Y1_LH,
	VPROC_POLY4_Y1_HL,
	VPROC_POLY4_Y1_HH,
	VPROC_POLY4_Y2_LL,
	VPROC_POLY4_Y2_LH,
	VPROC_POLY4_Y2_HL,
	VPROC_POLY4_Y2_HH,
	VPROC_POLY4_Y3_LL,
	VPROC_POLY4_Y3_LH,
	VPROC_POLY4_Y3_HL,
	VPROC_POLY4_Y3_HH,
	VPROC_POLY4_C0_LL,
	VPROC_POLY4_C0_LH,
	VPROC_POLY4_C0_HL,
	VPROC_POLY4_C0_HH,
	VPROC_POLY4_C1_LL,
	VPROC_POLY4_C1_LH,
	VPROC_POLY4_C1_HL,
	VPROC_POLY4_C1_HH
};

enum s5p_vp_line_eq {
	VProc_LINE_EQ_0  = 0,
	VProc_LINE_EQ_1  = 1,
	VProc_LINE_EQ_2  = 2,
	VProc_LINE_EQ_3  = 3,
	VProc_LINE_EQ_4  = 4,
	VProc_LINE_EQ_5  = 5,
	VProc_LINE_EQ_6  = 6,
	VProc_LINE_EQ_7  = 7
};

enum s5p_vp_mem_mode {
	VPROC_LINEAR_MODE,
	VPROC_2D_TILE_MODE
};

enum s5p_vp_chroma_expansion {
	VPROC_USING_C_TOP,
	VPROC_USING_C_TOP_BOTTOM
};

enum s5p_vp_filed_id_toggle {
	S5P_TV_VP_FILED_ID_TOGGLE_USER,
	S5P_TV_VP_FILED_ID_TOGGLE_VSYNC
};

enum s5p_vp_pxl_rate {
	VPROC_PIXEL_PER_RATE_1_1  = 0,
	VPROC_PIXEL_PER_RATE_1_2  = 1,
	VPROC_PIXEL_PER_RATE_1_3  = 2,
	VPROC_PIXEL_PER_RATE_1_4  = 3
};

enum s5p_vp_csc_coeff {
	VPROC_CSC_Y2Y_COEF = 0,
	VPROC_CSC_CB2Y_COEF,
	VPROC_CSC_CR2Y_COEF,
	VPROC_CSC_Y2CB_COEF,
	VPROC_CSC_CB2CB_COEF,
	VPROC_CSC_CR2CB_COEF,
	VPROC_CSC_Y2CR_COEF,
	VPROC_CSC_CB2CR_COEF,
	VPROC_CSC_CR2CR_COEF
};

enum s5p_vp_sharpness_control {
	VPROC_SHARPNESS_NO     = 0,
	VPROC_SHARPNESS_MIN    = 1,
	VPROC_SHARPNESS_MOD    = 2,
	VPROC_SHARPNESS_MAX    = 3
};

enum s5p_vp_csc_type {
	VPROC_CSC_SD_HD,
	VPROC_CSC_HD_SD
};

/* cec.c */
extern struct cec_rx_struct cec_rx_struct;
extern struct cec_tx_struct cec_tx_struct;

void s5p_cec_set_divider(void);
void s5p_cec_enable_rx(void);
void s5p_cec_mask_rx_interrupts(void);
void s5p_cec_unmask_rx_interrupts(void);
void s5p_cec_mask_tx_interrupts(void);
void s5p_cec_unmask_tx_interrupts(void);
void s5p_cec_reset(void);
void s5p_cec_tx_reset(void);
void s5p_cec_rx_reset(void);
void s5p_cec_threshold(void);
void s5p_cec_set_tx_state(enum cec_state state);
void s5p_cec_set_rx_state(enum cec_state state);
void s5p_cec_copy_packet(char *data, size_t count);
void s5p_cec_set_addr(u32 addr);
u32 s5p_cec_get_status(void);
void s5p_clr_pending_tx(void);
void s5p_clr_pending_rx(void);
void s5p_cec_get_rx_buf(u32 size, u8 *buffer);
void __init s5p_cec_mem_probe(struct platform_device *pdev);

/* hdcp.c */
void s5p_hdcp_hdmi_set_audio(bool en);
int s5p_hdcp_hdmi_set_mute(bool en);
int s5p_hdcp_hdmi_get_mute(void);
int s5p_hdcp_hdmi_audio_enable(bool en);
bool s5p_hdcp_start(void);
int __init s5p_hdcp_init(void);
int s5p_hdcp_encrypt_stop(bool on);

/* hdmi.c */
int s5p_hdmi_phy_power(bool on);
void s5p_hdmi_video_set_bluescreen(bool en, u8 cb, u8 y_g, u8 cr_r);
int s5p_hdmi_init_spd(enum s5p_hdmi_transmit trans_type,
				u8 *spd_header, u8 *spd_data);
int s5p_hdmi_audio_init(enum s5p_tv_audio_codec_type audio_codec,
			u32 sample_rate, u32 bits, u32 frame_size_code);
int s5p_hdmi_video_init_display_mode(enum s5p_tv_disp_mode disp_mode,
			enum s5p_tv_o_mode out_mode, u8 *avidata);
void s5p_hdmi_video_init_bluescreen(bool en, u8 cb, u8 y_g, u8 cr_r);
void s5p_hdmi_video_init_color_range(u8 y_min, u8 y_max, u8 c_min, u8 c_max);
int s5p_hdmi_video_init_avi(enum s5p_hdmi_transmit trans_type,
			u8 check_sum, u8 *pavi_data);
int s5p_hdmi_video_init_mpg(enum s5p_hdmi_transmit trans_type,
			u8 check_sum, u8 *pmpg_data);
void s5p_hdmi_video_init_tg_cmd(bool t_correction_en, bool BT656_sync_en,
			bool tg_en);
bool s5p_hdmi_start(enum s5p_hdmi_audio_type hdmi_audio_type, bool HDCP_en,
			struct i2c_client *ddc_port);
void s5p_hdmi_stop(void);
int __init s5p_hdmi_probe(struct platform_device *pdev, u32 res_num,
			u32 res_num2);
int __init s5p_hdmi_release(struct platform_device *pdev);
int s5p_hdmi_register_isr(hdmi_isr isr, u8 irq_num);
irqreturn_t s5p_hdmi_irq(int irq, void *dev_id);
void s5p_hdmi_enable_interrupts(enum s5p_tv_hdmi_interrrupt intr);
void s5p_hdmi_disable_interrupts(enum s5p_tv_hdmi_interrrupt intr);
void s5p_hdmi_clear_pending(enum s5p_tv_hdmi_interrrupt intr);
u8 s5p_hdmi_get_interrupts(void);
u8 s5p_hdmi_get_hpd_status(void);
void s5p_hdmi_hpd_gen(void);

/* mixer.c */
int s5p_vmx_set_layer_show(enum s5p_tv_vmx_layer layer, bool show);
int s5p_vmx_set_layer_priority(enum s5p_tv_vmx_layer layer, u32 priority);
int s5p_vmx_set_win_blend(enum s5p_tv_vmx_layer layer, bool enable);
int s5p_vmx_set_layer_alpha(enum s5p_tv_vmx_layer layer, u32 alpha);
int s5p_vmx_set_grp_base_address(enum s5p_tv_vmx_layer layer, u32 baseaddr);
int s5p_vmx_set_grp_layer_position(enum s5p_tv_vmx_layer layer,
			u32 dst_offs_x, u32 dst_offs_y);
int s5p_vmx_set_grp_layer_size(enum s5p_tv_vmx_layer layer, u32 span,
			u32 width, u32 height, u32 src_offs_x, u32 src_offs_y);
int s5p_vmx_set_bg_color(enum s5p_tv_vmx_bg_color_num colornum,
			u32 color_y, u32 color_cb, u32 color_cr);
int s5p_vmx_init_status_reg(enum s5p_vmx_burst_mode burst,
			enum s5p_endian_type endian);
int s5p_vmx_init_display_mode(enum s5p_tv_disp_mode mode,
			enum s5p_tv_o_mode output_mode);
void s5p_vmx_set_ctrl(enum s5p_tv_vmx_layer layer, bool premul,
			bool pixel_blending, bool blank_change,
			bool win_blending, enum s5p_tv_vmx_color_fmt color,
			u32 alpha, u32 blank_color);
int s5p_vmx_init_layer(enum s5p_tv_disp_mode mode, enum s5p_tv_vmx_layer layer,
			bool show, bool winblending, u32 alpha, u32 priority,
			enum s5p_tv_vmx_color_fmt color, bool blankchange,
			bool pixelblending, bool premul, u32 blankcolor,
			u32 baseaddr, u32 span, u32 width, u32 height,
			u32 src_offs_x, u32 src_offs_y, u32 dst_offs_x,
			u32 dst_offs_y, u32 dst_x, u32 dst_y);
void s5p_vmx_init_bg_dither_enable(bool cr_dither_enable, bool cdither_enable,
			bool y_dither_enable);
void s5p_vmx_init_csc_coef_default(enum s5p_tv_vmx_csc_type csc_type);
void s5p_vmx_start(void);
void s5p_vmx_stop(void);
int s5p_vmx_set_underflow_interrupt_enable(enum s5p_tv_vmx_layer layer,
			bool en);
void s5p_vmx_clear_pend_all(void);
irqreturn_t s5p_vmx_irq(int irq, void *dev_id);
int __init s5p_vmx_probe(struct platform_device *pdev, u32 res_num);
int __init s5p_vmx_release(struct platform_device *pdev);

/* sdout.c */
int s5p_sdout_init_video_scale_cfg(enum s5p_sd_level component_level,
			enum s5p_sd_vsync_ratio component_ratio,
			enum s5p_sd_level composite_level,
			enum s5p_sd_vsync_ratio composite_ratio);
int s5p_sdout_init_sync_signal_pin(enum s5p_sd_sync_sig_pin pin);
int s5p_sdout_init_vbi(bool wss_cvbs,
			enum s5p_sd_closed_caption_type caption_cvbs,
			bool wss_y_sideo,
			enum s5p_sd_closed_caption_type caption_y_sideo,
			bool cgmsa_rgb,
			bool wss_rgb,
			enum s5p_sd_closed_caption_type caption_rgb,
			bool cgmsa_y_ppr, bool wss_y_ppr,
			enum s5p_sd_closed_caption_type caption_y_ppr);
int s5p_sdout_init_offset_gain(enum s5p_sd_channel_sel channel,
			u32 offset, u32 gain);
void s5p_sdout_init_delay(u32 delay_y, u32 offset_video_start,
			u32 offset_video_end);
void s5p_sdout_init_schlock(bool color_sucarrier_pha_adj);
int s5p_sdout_init_dac_power_onoff(enum s5p_sd_channel_sel channel,
			bool dac_on);
void s5p_sdout_init_color_compensaton_onoff(bool bright_hue_saturation_adj,
			bool y_ppr_color_compensation,
			bool rgb_color_compensation,
			bool y_c_color_compensation,
			bool y_cvbs_color_compensation);
void s5p_sdout_init_brightness_hue_saturation(u32 gain_brightness,
			u32 offset_brightness,
			u32 gain0_cb_hue_saturation,
			u32 gain1_cb_hue_saturation,
			u32 gain0_cr_hue_saturation,
			u32 gain1_cr_hue_saturation,
			u32 offset_cb_hue_saturation,
			u32 offset_cr_hue_saturation);
void s5p_sdout_init_rgb_color_compensation(u32 max_rgb_cube, u32 min_rgb_cube);
void s5p_sdout_init_cvbs_color_compensation(u32 y_lower_mid, u32 y_bottom,
			u32 y_top, u32 y_upper_mid, u32 radius);
void s5p_sdout_init_svideo_color_compensation(u32 y_top, u32 y_bottom,
			u32 y_c_cylinder);
void s5p_sdout_init_component_porch(u32 back_525, u32 front_525,
			u32 back_625, u32 front_625);
int s5p_sdout_init_vesa_rgb_sync(enum s5p_sd_vesa_rgb_sync_type sync_type,
			enum s5p_tv_active_polarity v_sync_active,
			enum s5p_tv_active_polarity h_sync_active);
int s5p_sdout_init_ch_xtalk_cancel_coef(enum s5p_sd_channel_sel channel,
			u32 coeff2, u32 coeff1);
void s5p_sdout_init_closed_caption(u32 display_cc, u32 non_display_cc);
int s5p_sdout_init_wss525_data(enum s5p_sd_525_copy_permit copy_permit,
			enum s5p_sd_525_mv_psp mv_psp,
			enum s5p_sd_525_copy_info copy_info, bool analog_on,
			enum s5p_sd_525_aspect_ratio display_ratio);
int s5p_sdout_init_wss625_data(bool surround_sound, bool copyright,
			bool copy_protection, bool text_subtitles,
			enum s5p_sd_625_subtitles open_subtitles,
			enum s5p_sd_625_camera_film camera_film,
			enum s5p_sd_625_color_encoding color_encoding,
			bool helper_signal,
			enum s5p_sd_625_aspect_ratio display_ratio);
int s5p_sdout_init_cgmsa525_data(enum s5p_sd_525_copy_permit copy_permit,
			enum s5p_sd_525_mv_psp mv_psp,
			enum s5p_sd_525_copy_info copy_info, bool analog_on,
			enum s5p_sd_525_aspect_ratio display_ratio);
int s5p_sdout_init_cgmsa625_data(bool surround_sound, bool copyright,
			bool copy_protection, bool text_subtitles,
			enum s5p_sd_625_subtitles open_subtitles,
			enum s5p_sd_625_camera_film camera_film,
			enum s5p_sd_625_color_encoding color_encoding,
			bool helper_signal,
			enum s5p_sd_625_aspect_ratio display_ratio);
int s5p_sdout_init_display_mode(enum s5p_tv_disp_mode disp_mode,
			enum s5p_tv_o_mode out_mode, enum s5p_sd_order order);
void s5p_sdout_start(void);
void s5p_sdout_stop(void);
void s5p_sdout_sw_reset(bool active);
void s5p_sdout_set_interrupt_enable(bool vsync_intr_en);
void s5p_sdout_clear_interrupt_pending(void);
int __init s5p_sdout_probe(struct platform_device *pdev, u32 res_num);
int __init s5p_sdout_release(struct platform_device *pdev);

/* tv_power.c */
void s5p_tv_power_set_dac_onoff(bool on);
void s5p_tv_power_on(void);

/* vp.c */
void s5p_vp_set_field_id(enum s5p_vp_field mode);
int s5p_vp_set_top_field_address(u32 top_y_addr, u32 top_c_addr);
int s5p_vp_set_bottom_field_address(u32 bottom_y_addr, u32 bottom_c_addr);
int s5p_vp_set_img_size(u32 img_width, u32 img_height);
void s5p_vp_set_src_position(u32 src_off_x, u32 src_x_fract_step,
			u32 src_off_y);
void s5p_vp_set_dest_position(u32 dst_off_x, u32 dst_off_y);
void s5p_vp_set_src_dest_size(u32 src_width, u32 src_height, u32 dst_width,
			u32 dst_height, bool ipc_2d);
int s5p_vp_set_poly_filter_coef(enum s5p_vp_poly_coeff poly_coeff,
			signed char ch0, signed char ch1,
			signed char ch2, signed char ch3);
int s5p_vp_set_brightness_contrast_control(enum s5p_vp_line_eq eq_num,
			u32 intc, u32 slope);
void s5p_vp_set_brightness(bool brightness);
void s5p_vp_set_contrast(u8 contrast);
int s5p_vp_update(void);
bool s5p_vp_get_update_status(void);
void s5p_vp_init_op_mode(bool line_skip, enum s5p_vp_mem_mode mem_mode,
			enum s5p_vp_chroma_expansion chroma_exp,
			enum s5p_vp_filed_id_toggle toggle_id);
void s5p_vp_init_pixel_rate_control(enum s5p_vp_pxl_rate rate);
int s5p_vp_init_layer(u32 top_y_addr, u32 top_c_addr, u32 bottom_y_addr,
			u32 bottom_c_addr, enum s5p_endian_type src_img_endian,
			u32 img_width, u32 img_height, u32 src_off_x,
			u32 src_x_fract_step, u32 src_off_y, u32 src_width,
			u32 src_height, u32 dst_off_x, u32 dst_off_y,
			u32 dst_width, u32 dst_height, bool ipc_2d);
int s5p_vp_init_layer_def_poly_filter_coef(u32 top_y_addr, u32 top_c_addr,
			u32 bottom_y_addr, u32 bottom_c_addr,
			enum s5p_endian_type src_img_endian, u32 img_width,
			u32 img_height, u32 src_off_x, u32 src_x_fract_step,
			u32 src_off_y, u32 src_width, u32 src_height,
			u32 dst_off_x, u32 dst_off_y, u32 dst_width,
			u32 dst_height, bool ipc_2d);
void s5p_vp_init_bypass_post_process(bool bypass);
int s5p_vp_init_csc_coef(enum s5p_vp_csc_coeff csc_coeff, u32 coeff);
void s5p_vp_init_saturation(u32 sat);
void s5p_vp_init_sharpness(u32 th_h_noise,
			enum s5p_vp_sharpness_control sharpness);
void s5p_vp_init_brightness_offset(u32 offset);
void s5p_vp_init_csc_control(bool suy_offset_en, bool csc_en);
int s5p_vp_init_csc_coef_default(enum s5p_vp_csc_type csc_type);
int s5p_vp_start(void);
int s5p_vp_stop(void);
void s5p_vp_sw_reset(void);
int __init s5p_vp_probe(struct platform_device *pdev, u32 res_num);
int __init s5p_vp_release(struct platform_device *pdev);
#endif /* _LINUX_EXTERN_FUNC_TVOUT_H */
