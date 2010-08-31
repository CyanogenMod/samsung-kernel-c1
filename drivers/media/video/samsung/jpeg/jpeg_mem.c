/* linux/drivers/media/video/samsung/jpeg/jpeg_mem.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * http://www.samsung.com/
 *
 * Managent memory of the jpeg driver for encoder/docoder.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <asm/page.h>
#include "jpeg_mem.h"

unsigned long jpeg_get_stream_buf(unsigned long base)
{
	return base + JPEG_MAIN_START;
}

unsigned long jpeg_get_frame_buf(unsigned long base)
{
	return base + JPEG_S_BUF_SIZE;
}

void jpeg_set_stream_buf(unsigned int *str_buf, unsigned int base)
{
	*str_buf = base;
}

void jpeg_set_frame_buf(unsigned int *fra_buf, unsigned int base)
{
	*fra_buf = base + JPEG_S_BUF_SIZE;
}

