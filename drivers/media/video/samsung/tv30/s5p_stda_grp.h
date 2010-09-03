/* linux/drivers/media/video/samsung/tv20/s5p_stda_grp.h
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsung.com/
 *
 * Graphic Layer header file for Samsung TVOut driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef _LINUX_S5P_STDA_GRP_H_
#define _LINUX_S5P_STDA_GRP_H_

#include "ver_1/mixer.h"

bool s5p_mixer_ctrl_grp_start(enum s5p_mixer_layer vmLayer);
bool s5p_mixer_ctrl_grp_stop(enum s5p_mixer_layer vmLayer);

#endif /* _LINUX_S5P_STDA_GRP_H_ */

