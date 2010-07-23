/* linux/drivers/media/video/samsung/tv20/s5p_stda_tvout_if.h
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsung.com/
 *
 * TVOut interface header file. file for Samsung TVOut driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef _LINUX_S5P_STDA_TVOUT_IF_H_
#define _LINUX_S5P_STDA_TVOUT_IF_H_

#include "ver_1/tvout_ver_1.h"

bool s5p_tv_if_init_param(void);
bool s5p_tv_if_start(void);
bool s5p_tv_if_stop(void);
bool s5p_tv_if_set_disp(void);

#endif /* _LINUX_S5P_STDA_TVOUT_IF_H_ */
