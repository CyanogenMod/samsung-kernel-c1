/* linux/drivers/media/video/samsung/tv20/s5p_stda_grp.c
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsung.com/
 *
 * Graphic Layer ftn. file for Samsung TVOut driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/ioctl.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/uaccess.h>

#include "s5p_tv.h"
#include "s5p_stda_grp.h"

#ifdef COFIG_TVOUT_DBG
#define S5P_GRP_DEBUG 1
#endif

#ifdef S5P_GRP_DEBUG
#define GRPPRINTK(fmt, args...)	\
	printk(KERN_INFO "\t[GRP] %s: " fmt, __func__ , ## args)
#else
#define GRPPRINTK(fmt, args...)
#endif


bool s5p_grp_start(enum s5p_tv_vmx_layer vm_layer)
{
	int merr;
	struct s5p_tv_status *st = &s5ptv_status;

	if (!(st->grp_layer_enable[0] || st->grp_layer_enable[1])) {

		merr = s5p_vmx_init_status_reg(st->grp_burst,
					st->grp_endian);

		if (merr != 0)
			return false;
	}

	merr = s5p_vmx_init_layer(s5ptv_status.tvout_param.disp_mode,
				vm_layer,
				true,
				s5ptv_overlay[vm_layer].win_blending,
				s5ptv_overlay[vm_layer].win.global_alpha,
				s5ptv_overlay[vm_layer].priority,
				s5ptv_overlay[vm_layer].fb.fmt.pixelformat,
				s5ptv_overlay[vm_layer].blank_change,
				s5ptv_overlay[vm_layer].pixel_blending,
				s5ptv_overlay[vm_layer].pre_mul,
				s5ptv_overlay[vm_layer].blank_color,
				s5ptv_overlay[vm_layer].base_addr,
				s5ptv_overlay[vm_layer].fb.fmt.bytesperline,
				s5ptv_overlay[vm_layer].win.w.width,
				s5ptv_overlay[vm_layer].win.w.height,
				s5ptv_overlay[vm_layer].win.w.left,
				s5ptv_overlay[vm_layer].win.w.top,
				s5ptv_overlay[vm_layer].dst_rect.left,
				s5ptv_overlay[vm_layer].dst_rect.top,
				s5ptv_overlay[vm_layer].dst_rect.width,
				s5ptv_overlay[vm_layer].dst_rect.height);

	if (merr != 0) {
		GRPPRINTK("can't initialize layer(%d)\n\r", merr);

		return false;
	}

	s5p_vmx_start();

	st->grp_layer_enable[vm_layer] = true;

	return true;
}

bool s5p_grp_stop(enum s5p_tv_vmx_layer vm_layer)
{
	int merr;
	struct s5p_tv_status *st = &s5ptv_status;

	merr = s5p_vmx_set_layer_show(vm_layer, false);

	if (merr != 0)
		return false;

	merr = s5p_vmx_set_layer_priority(vm_layer, 0);

	if (merr != 0)
		return false;

	s5p_vmx_start();

	st->grp_layer_enable[vm_layer] = false;

	return true;
}
