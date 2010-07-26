/* linux/drivers/media/video/samsung/tv20/cec.h
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsung.com/
 *
 * cec interface header for Samsung TVOut driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _LINUX_CEC_IF_H_
#define _LINUX_CEC_IF_H_

#include <linux/platform_device.h>

#include "ver_1/tvout_ver_1.h"

/*#define CECDEBUG	1*/

#define VERSION   "1.0" /* Driver version number */
#define CEC_MINOR 242   /* Major 10, Minor 242, /dev/cec */


#define CEC_STATUS_TX_RUNNING       (1<<0)
#define CEC_STATUS_TX_TRANSFERRING  (1<<1)
#define CEC_STATUS_TX_DONE          (1<<2)
#define CEC_STATUS_TX_ERROR         (1<<3)
#define CEC_STATUS_TX_BYTES         (0xFF<<8)
#define CEC_STATUS_RX_RUNNING       (1<<16)
#define CEC_STATUS_RX_RECEIVING     (1<<17)
#define CEC_STATUS_RX_DONE          (1<<18)
#define CEC_STATUS_RX_ERROR         (1<<19)
#define CEC_STATUS_RX_BCAST         (1<<20)
#define CEC_STATUS_RX_BYTES         (0xFF<<24)


/* CEC Rx buffer size */
#define CEC_RX_BUFF_SIZE            16
/* CEC Tx buffer size */
#define CEC_TX_BUFF_SIZE            16

#endif /* _LINUX_CEC_IF_H_ */
