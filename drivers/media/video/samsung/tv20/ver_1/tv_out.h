/* linux/drivers/media/video/samsung/tv20/ver_1/tv_out.h
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsung.com/
 *
 * tv out header file for Samsung TVOut driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef _LINUX_TV_OUT_H_
#define _LINUX_TV_OUT_H_

#include "tvout_ver_1.h"

/* #define COFIG_TVOUT_RAW_DBG */


#define bit_add_l(val, addr)	writel(readl(addr) | val, addr)
#define bit_add_s(val, addr)	writes(reads(addr) | val, addr)
#define bit_add_b(val, addr)	writeb(readb(addr) | val, addr)
#define bit_del_l(val, addr)	writel(readl(addr) & ~val, addr)
#define bit_del_s(val, addr)	writes(reads(addr) & ~val, addr)
#define bit_del_b(val, addr)	writeb(readb(addr) & ~val, addr)

/* HDCP */
#define INFINITE		0xffffffff

#define AN_SIZE			8
#define AKSV_SIZE		5
#define BKSV_SIZE		5
#define R_VAL_RETRY_CNT		5

#define BCAPS_SIZE		1
#define BSTATUS_SIZE		2
#define SHA_1_HASH_SIZE		20
#define HDCP_MAX_DEVS		128
#define HDCP_KSV_SIZE		5

#define HDCP_Bksv		0x00
#define HDCP_Ri			0x08
#define HDCP_Aksv		0x10
#define HDCP_Ainfo		0x15
#define HDCP_An			0x18
#define HDCP_SHA1		0x20
#define HDCP_Bcaps		0x40
#define HDCP_BStatus		0x41
#define HDCP_KSVFIFO		0x43

#define KSV_FIFO_READY		(0x1 << 5)

#define MAX_CASCADE_EXCEEDED_ERROR 	(-1)
#define MAX_DEVS_EXCEEDED_ERROR    	(-2)
#define REPEATER_ILLEGAL_DEVICE_ERROR	(-3)
#define REPEATER_TIMEOUT_ERROR		(-4)


#define MAX_CASCADE_EXCEEDED       	(0x1 << 3)
#define MAX_DEVS_EXCEEDED          	(0x1 << 7)

enum hdcp_event {
	HDCP_EVENT_STOP,
	HDCP_EVENT_START,
	HDCP_EVENT_READ_BKSV_START,
	HDCP_EVENT_WRITE_AKSV_START,
	HDCP_EVENT_CHECK_RI_START,
	HDCP_EVENT_SECOND_AUTH_START
};

enum hdcp_state {
	NOT_AUTHENTICATED,
	RECEIVER_READ_READY,
	BCAPS_READ_DONE,
	BKSV_READ_DONE,
	AN_WRITE_DONE,
	AKSV_WRITE_DONE,
	FIRST_AUTHENTICATION_DONE,
	SECOND_AUTHENTICATION_RDY,
	RECEIVER_FIFOLSIT_READY,
	SECOND_AUTHENTICATION_DONE,
};

struct s5p_hdcp_info {
	bool	is_repeater;
	bool	hpd_status;
	u32	time_out;
	u32	hdcp_enable;

	spinlock_t 	lock;
	spinlock_t 	reset_lock;

	struct i2c_client 	*client;

	wait_queue_head_t 	waitq;
	enum hdcp_event 	event;
	enum hdcp_state 	auth_status;

	struct work_struct  	work;
};

/* HDMI */
#define I2C_ACK				(1 << 7)
#define I2C_INT				(1 << 5)
#define I2C_PEND			(1 << 4)
#define I2C_INT_CLEAR			(0 << 4)
#define I2C_CLK				(0x41)
#define I2C_CLK_PEND_INT		(I2C_CLK | I2C_INT_CLEAR | I2C_INT)
#define I2C_ENABLE			(1 << 4)
#define I2C_START			(1 << 5)
#define I2C_MODE_MTX			0xC0
#define I2C_MODE_MRX			0x80
#define I2C_IDLE			0

#define STATE_IDLE 			0
#define STATE_TX_EDDC_SEGADDR		1
#define STATE_TX_EDDC_SEGNUM		2
#define STATE_TX_DDC_ADDR		3
#define STATE_TX_DDC_OFFSET		4
#define STATE_RX_DDC_ADDR		5
#define STATE_RX_DDC_DATA		6
#define STATE_RX_ADDR			7
#define STATE_RX_DATA			8
#define STATE_TX_ADDR			9
#define STATE_TX_DATA			10
#define STATE_TX_STOP			11
#define STATE_RX_STOP			12

#define HDMI_IRQ_TOTAL_NUM		6


enum s5p_tv_pwr_err {
	S5P_TV_PWR_ERR_NO_ERROR = 0,
	S5P_TV_PWR_ERR_NOT_INIT_PARAMETERS_UNDER_RUNNING = 0x5000,
	S5P_TV_PWR_ERR_NOT_SET_PARAMETERS_UNDER_STOP,
	S5P_TV_PWR_ERR_INVALID_PARAM
};

enum s5p_yuv_fmt_component {
	TVOUT_YUV_Y,
	TVOUT_YUV_CB,
	TVOUT_YUV_CR
};

enum s5p_tv_clk_hpll_ref {
	S5P_TV_CLK_HPLL_REF_27M,
	S5P_TV_CLK_HPLL_REF_SRCLK
};

enum s5p_tv_clk_mout_hpll {
	S5P_TV_CLK_MOUT_HPLL_27M,
	S5P_TV_CLK_MOUT_HPLL_FOUT_HPLL
};

enum s5p_tv_clk_vmiexr_srcclk {
	TVOUT_CLK_VMIXER_SRCCLK_CLK27M,
	TVOUT_CLK_VMIXER_SRCCLK_VCLK_54,
	TVOUT_CLK_VMIXER_SRCCLK_MOUT_HPLL
};

enum s5p_vp_src_color {
	VPROC_SRC_COLOR_NV12  	= 0,
	VPROC_SRC_COLOR_NV12IW  = 1,
	VPROC_SRC_COLOR_TILE_NV12  	= 2,
	VPROC_SRC_COLOR_TILE_NV12IW  	= 3
};

enum s5p_tv_vp_filter_h_pp {
	/* Don't change the order and the value */
	VPROC_PP_H_NORMAL = 0,
	VPROC_PP_H_8_9,      /* 720 to 640 */
	VPROC_PP_H_1_2,
	VPROC_PP_H_1_3,
	VPROC_PP_H_1_4
};

enum s5p_tv_vp_filter_v_pp {
	/* Don't change the order and the value */
	VPROC_PP_V_NORMAL = 0,
	VPROC_PP_V_5_6,      /* PAL to NTSC */
	VPROC_PP_V_3_4,
	VPROC_PP_V_1_2,
	VPROC_PP_V_1_3,
	VPROC_PP_V_1_4
};

enum s5p_tv_vmx_scan_mode {
	VMIXER_INTERLACED_MODE = 0,
	VMIXER_PROGRESSIVE_MODE = 1
};

enum s5p_tv_coef_y_mode {
	VMIXER_COEF_Y_NARROW = 0,
	VMIXER_COEF_Y_WIDE = 1
};

enum s5p_tv_vmx_rgb {
	RGB601_0_255,
	RGB601_16_235,
	RGB709_0_255,
	RGB709_16_235
};

enum s5p_tv_vmx_out_type {
	MX_YUV444,
	MX_RGB888
};


/*
 * Color Depth for HDMI HW (settings and GCP packet),
 * EDID and PHY HW
 */
enum s5p_hdmi_color_depth {
	HDMI_CD_48,
	HDMI_CD_36,
	HDMI_CD_30,
	HDMI_CD_24
};

enum phy_freq {
	ePHY_FREQ_25_200,
	ePHY_FREQ_25_175,
	ePHY_FREQ_27,
	ePHY_FREQ_27_027,
	ePHY_FREQ_54,
	ePHY_FREQ_54_054,
	ePHY_FREQ_74_250,
	ePHY_FREQ_74_176,
	ePHY_FREQ_148_500,
	ePHY_FREQ_148_352,
	ePHY_FREQ_108_108,
	ePHY_FREQ_72,
	ePHY_FREQ_25,
	ePHY_FREQ_65,
	ePHY_FREQ_108,
	ePHY_FREQ_162
};

/* video format for HDMI HW (timings and AVI) and EDID */
enum s5p_hdmi_v_fmt {
	v640x480p_60Hz = 0,
	v720x480p_60Hz,
	v1280x720p_60Hz,
	v1920x1080i_60Hz,
	v720x480i_60Hz,
	v720x240p_60Hz,
	v2880x480i_60Hz,
	v2880x240p_60Hz,
	v1440x480p_60Hz,
	v1920x1080p_60Hz,
	v720x576p_50Hz,
	v1280x720p_50Hz,
	v1920x1080i_50Hz,
	v720x576i_50Hz,
	v720x288p_50Hz,
	v2880x576i_50Hz,
	v2880x288p_50Hz,
	v1440x576p_50Hz,
	v1920x1080p_50Hz,
	v1920x1080p_24Hz,
	v1920x1080p_25Hz,
	v1920x1080p_30Hz,
	v2880x480p_60Hz,
	v2880x576p_50Hz,
	v1920x1080i_50Hz_1250,
	v1920x1080i_100Hz,
	v1280x720p_100Hz,
	v720x576p_100Hz,
	v720x576i_100Hz,
	v1920x1080i_120Hz,
	v1280x720p_120Hz,
	v720x480p_120Hz,
	v720x480i_120Hz,
	v720x576p_200Hz,
	v720x576i_200Hz,
	v720x480p_240Hz,
	v720x480i_240Hz,
	v720x480p_59Hz,
	v1280x720p_59Hz,
	v1920x1080i_59Hz,
	v1920x1080p_59Hz,

};

enum s5p_tv_hdmi_disp_mode {
	S5P_TV_HDMI_DISP_MODE_480P_60 = 0,
	S5P_TV_HDMI_DISP_MODE_576P_50 = 1,
	S5P_TV_HDMI_DISP_MODE_720P_60 = 2,
	S5P_TV_HDMI_DISP_MODE_720P_50 = 3,
	S5P_TV_HDMI_DISP_MODE_1080I_60 = 4,
	S5P_TV_HDMI_DISP_MODE_1080I_50 = 5,
	S5P_TV_HDMI_DISP_MODE_VGA_60 = 6,
	S5P_TV_HDMI_DISP_MODE_1080P_60 = 7,
	S5P_TV_HDMI_DISP_MODE_1080P_50 = 8,
	S5P_TV_HDMI_DISP_MODE_NUM = 9
};

enum s5p_tv_hdmi_pxl_aspect {
    HDMI_PIXEL_RATIO_4_3,
    HDMI_PIXEL_RATIO_16_9
};


void s5p_hdmi_sw_hpd_enable(bool enable);
void s5p_hdmi_set_hpd_onoff(bool on_off);
int s5p_hdmi_register_isr(hdmi_isr isr, u8 irq_num);
void s5p_hdmi_enable_interrupts(enum s5p_tv_hdmi_interrrupt intr);
void s5p_hdmi_disable_interrupts(enum s5p_tv_hdmi_interrrupt intr);
u8 s5p_hdmi_get_enabled_interrupt(void);
int s5p_hdcp_hdmi_set_dvi(bool en);
void s5p_hdcp_hdmi_mute_en(bool en);
bool s5p_hdcp_stop(void);
void  s5p_hdmi_video_set_bluescreen(bool en, u8 cb, u8 y_g, u8 cr_r);

extern u8 hdcp_protocol_status;
extern void __iomem *hdmi_base;

#endif /* _LINUX_TV_OUT_H_ */
