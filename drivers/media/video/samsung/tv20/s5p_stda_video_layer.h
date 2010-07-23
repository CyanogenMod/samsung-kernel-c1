/* linux/drivers/media/video/samsung/tv20/s5p_stda_video_layer.h
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsung.com/
 *
 * Video Layer header file. file for Samsung TVOut driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef _LINUX_S5P_STDA_VIDEO_LAYER_H_
#define _LINUX_S5P_STDA_VIDEO_LAYER_H_

#include "ver_1/tvout_ver_1.h"

bool s5p_vlayer_start(void);
bool s5p_vlayer_stop(void);
bool s5p_vlayer_set_priority(unsigned long p_buf_in);
bool s5p_vlayer_set_blending(unsigned long p_buf_in);
bool s5p_vlayer_set_alpha(unsigned long p_buf_in);
bool s5p_vlayer_set_top_address(unsigned long p_buf_in);
bool s5p_vlayer_set_img_size(unsigned long p_buf_in);
bool s5p_vlayer_set_src_position(unsigned long p_buf_in);
bool s5p_vlayer_set_dest_position(unsigned long p_buf_in);
bool s5p_vlayer_set_src_size(unsigned long p_buf_in);
bool s5p_vlayer_set_dest_size(unsigned long p_buf_in);
bool s5p_vlayer_init_param(unsigned long p_buf_in);

#endif /* _LINUX_S5P_STDA_VIDEO_LAYER_H_ */
