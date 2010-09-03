/* linux/drivers/media/video/samsung/tv20/s5p_tvout_ctrl.h
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
#ifndef _S5P_TVOUT_CTRL_H_
#define _S5P_TVOUT_CTRL_H_ __FILE__

#include "ver_1/tvout_ver_1.h"

extern bool s5p_tvout_ctrl_start(
		enum s5p_tv_disp_mode std, enum s5p_tv_o_mode inf);
extern void s5p_tvout_ctrl_stop(void);

#endif /* _S5P_TVOUT_CTRL_H_ */
