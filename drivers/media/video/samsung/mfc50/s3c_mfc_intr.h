/* linux/drivers/media/video/samsung/mfc50/s3c_mfc_intr.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Definition of handling for interrupt based sequence
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __S3C_MFC_INTR_H
#define __S3C_MFC_INTR_H __FILE__

#include "s3c_mfc_common.h"

#define MFC_TIMEOUT	200

extern unsigned int s3c_mfc_int_type;
extern unsigned int s3c_mfc_err_type;
extern wait_queue_head_t s3c_mfc_wait_queue;
int s3c_mfc_wait_for_done(enum s3c_mfc_wait_done_type command);

#endif /* __S3C_MFC_INTR_H */
