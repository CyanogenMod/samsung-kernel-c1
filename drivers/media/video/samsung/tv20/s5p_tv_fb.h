/* linux/drivers/media/video/samsung/tv20/s5p_tv_fb.c
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsung.com/
 *
 * frame buffer header file. file for Samsung TVOut driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _LINUX_S5P_TV_FB_H_
#define _LINUX_S5P_TV_FB_H_
	
#include "ver_1/tvout_ver_1.h"
#include "s5p_tv.h"

int s5p_tv_fb_display_on(struct s5p_tv_status *ctrl);
int s5p_tv_fb_map_video_memory(struct fb_info *fb);
int s5p_tv_fb_draw_logo(struct fb_info *fb);
int s5p_tv_fb_set_par(struct fb_info *fb);
int s5p_tv_fb_check_var(struct fb_var_screeninfo *var,
		struct fb_info *fb);
int s5p_tv_fb_init_fbinfo(int id);
void s5p_tv_fb_set_lcd_info(struct s5p_tv_status *ctrl);

#endif /* _LINUX_S5P_TV_FB_H_ */

