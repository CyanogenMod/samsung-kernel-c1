/* linux/drivers/media/video/samsung/tv20/ver_1/hdmi.h
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsung.com/
 *
 * hdmi parameter header file for Samsung TVOut driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _LINUX_HDMI_H
#define _LINUX_HDMI_H

#define PHY_I2C_ADDRESS       	0x70
#define PHY_REG_MODE_SET_DONE 	0x1F

struct hdmi_v_params {
	u16 h_blank;
	u32 v_blank;
	u32 hvline;
	u32 h_sync_gen;
	u32 v_sync_gen;
	u8  avi_vic;
	u8  avi_vic_16_9;
	u8  interlaced;
	u8  repetition;
	u8  polarity;
	u32 v_blank_f;
	u32 v_sync_gen2;
	u32 v_sync_gen3;
	enum phy_freq pixel_clock;
};

struct _hdmi_tg_param {
	u16 h_fsz;
	u16 hact_st;
	u16 hact_sz;
	u16 v_fsz;
	u16 vsync;
	u16 vsync2;
	u16 vact_st;
	u16 vact_sz;
	u16 field_chg;
	u16 vact_st2;
	u16 vsync_top_hdmi;
	u16 vsync_bot_hdmi;
	u16 field_top_hdmi;
	u16 field_bot_hdmi;
	u8 mhl_hsync_width;
	u8 mhl_vsync_width;
};
#endif /* _LINUX_HDMI_H */
