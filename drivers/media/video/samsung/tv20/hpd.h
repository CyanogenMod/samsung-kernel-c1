/* linux/drivers/media/video/samsung/tv20/hpd.h
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsung.com/
 *
 * hpd interface header file for Samsung TVOut driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef _LINUX_HPD_H_
#define _LINUX_HPD_H_

#include "ver_1/tvout_ver_1.h"

#define VERSION         "1.2" /* Driver version number */
#define HPD_MINOR       243 /* Major 10, Minor 243, /dev/hpd */

#define HPD_LO          0
#define HPD_HI          1

#define HDMI_ON 	1
#define HDMI_OFF 	0

struct hpd_struct {
	spinlock_t lock;
	wait_queue_head_t waitq;
	atomic_t state;
};

int s5p_hpd_get_state(void);
int s5p_hpd_set_hdmiint(void);
int s5p_hpd_set_eint(void);

#endif /* _LINUX_HPD_H_ */
