/* linux/drivers/media/video/samsung/tv20/s5p_tv_base.h
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsung.com/
 *
 * Entry header file for Samsung TVOut driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _LINUX_S5P_TV_BASE_H_
#define _LINUX_S5P_TV_BASE_H_

#include "ver_1/tvout_ver_1.h"

int s5p_tv_phy_power(bool on);
int s5p_tv_clk_gate(bool on);
void s5p_tv_kobject_uevent(void);

#endif /* _LINUX_S5P_TV_BASE_H_ */

