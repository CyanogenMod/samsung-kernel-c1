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
#include <linux/vmalloc.h>

#include <mach/media.h>
#include <plat/media.h>
#include "jpeg_mem.h"

#if defined(CONFIG_S5P_SYSMMU_JPEG) && defined(CONFIG_S5P_VMEM)
unsigned int s_cookie;	/* for stream buffer */
unsigned int f_cookie;	/* for frame buffer */
#endif

int jpeg_init_mem(unsigned int *base)
{
#if defined(CONFIG_S5P_SYSMMU_JPEG)
#if !defined(CONFIG_S5P_VMEM)
	unsigned char *addr;
	addr = vmalloc(JPEG_MEM_SIZE);
	if (addr == NULL)
		return -1;
	*base = (unsigned int)addr;
#endif /* CONFIG_S5P_VMEM */
#else
	*base = s5p_get_media_memory_bank(S5P_MDEV_JPEG, 0);
#endif /* CONFIG_S5P_SYSMMU_JPEG */
	return 0;
}

int jpeg_mem_free(void)
{
#if defined(CONFIG_S5P_SYSMMU_JPEG) && defined(CONFIG_S5P_VMEM)
	if (s_cookie != 0) {
		s5p_vfree(s_cookie);
		s_cookie = 0;
	}
	if (f_cookie != 0) {
		s5p_vfree(f_cookie);
		f_cookie = 0;
	}
#endif
	return 0;
}

unsigned long jpeg_get_stream_buf(unsigned long arg)
{
#if defined(CONFIG_S5P_SYSMMU_JPEG) && defined(CONFIG_S5P_VMEM)
	arg = ((arg / PAGE_SIZE + 1) * PAGE_SIZE);
	s_cookie = (unsigned int)s5p_vmalloc(arg);
		if (s_cookie == 0)
			return -1;
	return (unsigned long)s_cookie;
#else
	return arg + JPEG_MAIN_START;
#endif
}

unsigned long jpeg_get_frame_buf(unsigned long arg)
{
#if defined(CONFIG_S5P_SYSMMU_JPEG) && defined(CONFIG_S5P_VMEM)
	arg = ((arg / PAGE_SIZE + 1) * PAGE_SIZE);
	f_cookie = (unsigned int)s5p_vmalloc(arg);
		if (f_cookie == 0)
			return -1;
	return (unsigned long)f_cookie;
#else
	return arg + JPEG_S_BUF_SIZE;
#endif
}

void jpeg_set_stream_buf(unsigned int *str_buf, unsigned int base)
{
#if defined(CONFIG_S5P_SYSMMU_JPEG) && defined(CONFIG_S5P_VMEM)
	*str_buf = (unsigned int)s5p_getaddress(s_cookie);
#else
	*str_buf = base;
#endif
}

void jpeg_set_frame_buf(unsigned int *fra_buf, unsigned int base)
{
#if defined(CONFIG_S5P_SYSMMU_JPEG) && defined(CONFIG_S5P_VMEM)
	*fra_buf = (unsigned int)s5p_getaddress(f_cookie);
#else
	*fra_buf = base + JPEG_S_BUF_SIZE;
#endif
}

