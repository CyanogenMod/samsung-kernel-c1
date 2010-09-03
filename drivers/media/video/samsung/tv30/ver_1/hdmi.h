/* linux/drivers/media/video/samsung/tv20/ver_1/hdmi.h
 *
 * Copyright (c) 2010 Samsung Electronics
 *		http://www.samsung.com/
 *
 * Header file for HDMI of Samsung TVOut driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _HDMI_H_
#define _HDMI_H_ __FILE__

#include "../s5p_tv.h"

#define hdmi_mask_8(x)		((x) & 0xFF)
#define hdmi_mask_16(x)		(((x) >> 8) & 0xFF)
#define hdmi_mask_32(x)		(((x) >> 16) & 0xFF)

#define hdmi_write_16(x, y)				\
	do {						\
		writeb(hdmi_mask_8(x), y);		\
		writeb(hdmi_mask_16(x), y + 4);		\
	} while(0);

#define hdmi_write_32(x, y)				\
	do {						\
		writeb(hdmi_mask_8(x), y);		\
		writeb(hdmi_mask_16(x), y + 4);		\
		writeb(hdmi_mask_32(x), y + 8);		\
	} while(0);

#define hdmi_write_l(buff, base, start, count)		\
	do {						\
		int i = 0;				\
		int a = start;				\
		do {					\
			writeb(buff[i], base + a);	\
			a += 4;				\
			i++; 				\
		} while(i <= (count - 1));		\
	} while(0);

#define hdmi_bit_set(en, reg, val)			\
	do {						\
		if (en)					\
			reg |= val;			\
		else					\
			reg &= ~val;			\
	} while(0);

#define AVI_SAME_WITH_PICTURE_AR	(0x1<<3)
#define AVI_RGB_IF			(0x0<<5)
#define AVI_YCBCR444_IF			(0x2<<5)

#define AVI_ITU601			(0x1<<6)
#define AVI_ITU709			(0x2<<6)

#define AVI_PAR_4_3			(0x1<<4)
#define AVI_PAR_16_9			(0x2<<4)

#define AVI_NO_PXL_REPEAT		(0x0<<0)

enum s5p_hdmi_transmit {
	HDMI_DO_NOT_TANS = 0,
	HDMI_TRANS_ONCE,
	HDMI_TRANS_EVERY_SYNC
};

enum s5p_hdmi_audio_type {
	HDMI_AUDIO_NO,
	HDMI_AUDIO_PCM
};

enum s5p_tv_audio_codec_type {
	PCM = 1,
	AC3,
	MP3,
	WMA
};

enum s5p_hdmi_color_depth {
	HDMI_CD_48,
	HDMI_CD_36,
	HDMI_CD_30,
	HDMI_CD_24
};

enum s5p_hdmi_interrrupt {
	HDMI_IRQ_PIN_POLAR_CTL	= 7,
	HDMI_IRQ_GLOBAL		= 6,
	HDMI_IRQ_I2S		= 5,
	HDMI_IRQ_CEC		= 4,
	HDMI_IRQ_HPD_PLUG	= 3,
	HDMI_IRQ_HPD_UNPLUG	= 2,
	HDMI_IRQ_SPDIF		= 1,
	HDMI_IRQ_HDCP		= 0
};

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

enum s5p_hdmi_pxl_aspect {
	HDMI_PXL_RATIO_4_3,
	HDMI_PXL_RATIO_16_9
};


struct s5p_hdmi_bluescreen {
	bool	enable;
	u8	cb_b;
	u8	y_g;
	u8	cr_r;
};

struct s5p_hdmi_color_range {
	u8	y_min;
	u8	y_max;
	u8	c_min;
	u8	c_max;
};

struct s5p_hdmi_infoframe {
	enum s5p_hdmi_transmit	type;
	u8			*data;
};

struct s5p_hdmi_tg {
	bool correction_en;
	bool bt656_en;
	bool tg_en;
};

typedef int (*hdmi_isr)(int irq);

#endif /* _HDMI_H_ */
