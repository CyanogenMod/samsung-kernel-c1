/* linux/drivers/media/video/samsung/jpeg/jpeg_mem.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * http://www.samsung.com/
 *
 * Definition for Operation of Jpeg encoder/docoder with memory
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __JPEG_MEM_H__
#define __JPEG_MEM_H__

#define MAX_JPEG_WIDTH	3072
#define MAX_JPEG_HEIGHT	2048
#define MAX_JPEG_RES	(MAX_JPEG_WIDTH * MAX_JPEG_HEIGHT)

/* jpeg stream buf */
#define JPEG_S_BUF_SIZE	((MAX_JPEG_RES / PAGE_SIZE + 1) * PAGE_SIZE)
/* jpeg frame buf */
#define JPEG_F_BUF_SIZE	(((MAX_JPEG_RES * 3) / PAGE_SIZE + 1) * PAGE_SIZE)

#define JPEG_MAIN_START		0x00

/* for reserved memory */
struct jpeg_mem {
	/* buffer base */
	unsigned int	base;
	/* for jpeg stream data */
	unsigned int	stream_data_addr;
	unsigned int	stream_data_size;
	/* for raw data */
	unsigned int	frame_data_addr;
	unsigned int	frame_data_size;
};


unsigned long jpeg_get_stream_buf(unsigned long base);
unsigned long jpeg_get_frame_buf(unsigned long base);
void jpeg_set_stream_buf(unsigned int *str_buf, unsigned int base);
void jpeg_set_frame_buf(unsigned int *fra_buf, unsigned int base);

#endif /* __JPEG_MEM_H__ */

