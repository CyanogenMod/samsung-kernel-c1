/* linux/drivers/media/video/samsung/mfc50/s3c_mfc_intr.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Handling for interrupt based sequence
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/io.h>

#include <mach/regs-mfc.h>

#include "s3c_mfc_intr.h"
#include "s3c_mfc_logmsg.h"
#include "s3c_mfc_common.h"
#include "s3c_mfc_memory.h"

int s3c_mfc_wait_for_done(enum s3c_mfc_wait_done_type command)
{
	unsigned int ret_val = 1;

	switch (command) {
	case R2H_CMD_FW_STATUS_RET:
	case R2H_CMD_OPEN_INSTANCE_RET:
	case R2H_CMD_SYS_INIT_RET:
	case R2H_CMD_SEQ_DONE_RET:
	case R2H_CMD_INIT_BUFFERS_RET:
	case R2H_CMD_FRAME_DONE_RET:
	case R2H_CMD_SLICE_DONE_RET:
	case R2H_CMD_CLOSE_INSTANCE_RET:
	case R2H_CMD_SLEEP_RET:
	case R2H_CMD_WAKEUP_RET:
		s3c_mfc_int_type = R2H_CMD_NONE_RET;
		if (wait_event_timeout(s3c_mfc_wait_queue,
					((s3c_mfc_int_type >= R2H_CMD_EMPTY) && \
					(s3c_mfc_int_type <= R2H_CMD_ERR_RET)),
						MFC_TIMEOUT) == 0) {
			ret_val = 0;
			mfc_err("Interrupt Time Out(%d)\n", command);
			break;
		}
		ret_val = s3c_mfc_int_type;
		s3c_mfc_int_type = R2H_CMD_NONE_RET;
		break;

	default:
		mfc_err("undefined command\n");
		ret_val = 0;
	}

	return ret_val;
}
