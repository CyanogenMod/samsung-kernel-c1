/*
 * linux/drivers/media/video/samsung/mfc5x/mfc_enc.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Encoder interface for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MFC_ENC_H
#define __MFC_ENC_H __FILE__

enum pixel_cache_encoder {
	PIXEL_CACHE_ENABLE	= 0,
	PIXEL_CACHE_DISABLE	= 3,
};

struct mfc_enc {
	enum pixel_cache_encoder pixelcache;
	unsigned int interlace;
	unsigned int forceframe;
	unsigned int frameskip;
	unsigned int framerate;
	unsigned int bitrate;

	void *encoder_private;
};

struct mfc_enc_h264 {
	unsigned int vui_enable;
	unsigned int hier_p_enable;

	unsigned int i_period;
};

#endif /* __MFC_ENC_H */
