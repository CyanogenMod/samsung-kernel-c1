/* linux/drivers/media/video/samsung/tv20/s5p_tv_v4l2.h
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsung.com/
 *
 * Video4Linux API header file. file for Samsung TVOut driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _LINUX_S5P_TV_V4L2_H_
#define _LINUX_S5P_TV_V4L2_H_
	
#include "ver_1/tvout_ver_1.h"

void s5p_tv_v4l2_init_param(void);
long s5p_tv_ioctl(struct file *file, u32 cmd, unsigned long arg);
long s5p_tv_vid_ioctl(struct file *file, u32 cmd, unsigned long arg);
long s5p_tv_v_ioctl(struct file *file, u32 cmd, unsigned long arg);
long s5p_tv_vo_ioctl(struct file *file, u32 cmd, unsigned long arg);

extern const struct v4l2_ioctl_ops s5p_tv_v4l2_v_ops;
extern const struct v4l2_ioctl_ops s5p_tv_v4l2_vo_ops;
extern const struct v4l2_ioctl_ops s5p_tv_v4l2_ops;
extern const struct v4l2_ioctl_ops s5p_tv_v4l2_vid_ops;

#endif /* _LINUX_S5P_TV_V4L2_H_ */
