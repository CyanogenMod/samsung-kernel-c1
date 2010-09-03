/* linux/drivers/media/video/samsung/tv20/s5p_tvout_ctrl.c
 *
 * Copyright (c) 2009 Samsung Electronics
 *		http://www.samsung.com/
 *
 * Tvout ctrl class for Samsung TVOUT driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

#include <plat/clock.h>

#include "ver_1/mixer.h"
#include "ver_1/tvout_ver_1.h"
#include "s5p_tvout_ctrl.h"
#include "s5p_mixer_ctrl.h"

#ifdef COFIG_TVOUT_DBG
#define S5P_TVOUT_CTRL_DEBUG 1
#endif

#ifdef S5P_TVOUT_CTRL_DEBUG
#define TVOUTIFPRINTK(fmt, args...) \
	printk(KERN_INFO "\t[TVOUT_IF] %s: " fmt, __func__ , ## args)
#else
#define TVOUTIFPRINTK(fmt, args...)
#endif

/* start: external functions for SDO */
#include "ver_1/sdo.h"

extern int s5p_sdo_ctrl_start(enum s5p_tv_disp_mode disp);
extern void s5p_sdo_ctrl_stop(void);
extern int s5p_sdo_ctrl_constructor(struct platform_device *pdev);
extern void s5p_sdo_ctrl_destructor(void);
/* end: external functions for SDO */

/* start: external functions for HDMI */
#include "ver_1/hdmi.h"

extern int s5p_hdmi_ctrl_start(
		enum s5p_tv_disp_mode disp, enum s5p_tv_o_mode out);

extern void s5p_hdmi_ctrl_stop(void);
extern void s5p_hdmi_init(void __iomem *hdmi_addr, void __iomem *hdmi_phy_addr);
extern irqreturn_t s5p_hdmi_irq(int irq, void *dev_id);
extern int s5p_hdmi_ctrl_constructor(struct platform_device *pdev);
extern void s5p_hdmi_ctrl_destructor(void);
/* end: external functions for HDMI */

struct s5p_tvout_ctrl_private_data {
	enum s5p_tv_disp_mode	curr_std;
	enum s5p_tv_o_mode	curr_if;

	enum s5p_tv_disp_mode	new_std;
	enum s5p_tv_o_mode	new_if;

	bool			running;
};

static struct s5p_tvout_ctrl_private_data private;

int s5p_tv_map_resource_mem(
		struct platform_device *pdev,
		char *name,
		void __iomem **base,
		struct resource **res)
{
	size_t		size;
	void __iomem	*tmp_base;
	struct resource	*tmp_res;

	tmp_res = platform_get_resource_byname(pdev, IORESOURCE_MEM, name);

	if (!tmp_res)
		goto not_found;

	size = (tmp_res->end - tmp_res->start) + 1;

	tmp_res = request_mem_region(tmp_res->start, size, tmp_res->name);

	if (!tmp_res) {
		printk("%s: fail to get memory region\n", __func__);
		goto err_on_request_mem_region;
	}

	tmp_base = ioremap(tmp_res->start, size);

	if (!tmp_base) {
		printk("%s: fail to ioremap address region\n", __func__);
		goto err_on_ioremap;
	}

	*res = tmp_res;
	*base = tmp_base;
	return 0;

err_on_ioremap:
	release_resource(tmp_res);
	kfree(tmp_res);

err_on_request_mem_region:
	return -ENXIO;
	
not_found:
	printk("%s: fail to get IORESOURCE_MEM for %s\n", __func__, name);
	return -ENODEV;
}

void s5p_tv_unmap_resource_mem(void __iomem *base, struct resource *res)
{
	if (!base)
		iounmap(base);

	if (!res) {
		release_resource(res);
		kfree(res);
	}
}
	
static bool s5p_tvout_ctrl_init_vm_reg(
		enum s5p_tv_disp_mode disp, enum s5p_tv_o_mode out)
{
	int merr = 0;

#ifndef OLD
#else
	struct s5p_tv_status *st = &s5ptv_status;
	enum s5p_tv_o_mode out = st->tvout_param.out;
	enum s5p_tv_disp_mode disp = st->tvout_param.disp;
#endif
	merr = s5p_mixer_ctrl_init_status_reg();

	if (merr != 0)
		return false;

	merr = s5p_mixer_init_display_mode(disp, out);

	if (merr != 0)
		return false;

	s5p_mixer_ctrl_init_bg_color();

	switch (out) {
	case TVOUT_COMPOSITE:
		s5p_mixer_init_csc_coef_default(MIXER_CSC_601_FR);
		break;

	case TVOUT_HDMI_RGB:
	case TVOUT_HDMI:
	case TVOUT_DVI:
		switch (disp) {
		case TVOUT_NTSC_M:
		case TVOUT_PAL_BDGHI:
		case TVOUT_PAL_M:
		case TVOUT_PAL_N:
		case TVOUT_PAL_NC:
		case TVOUT_PAL_60:
		case TVOUT_NTSC_443:
			break;

		case TVOUT_480P_60_16_9:
		case TVOUT_480P_60_4_3:
		case TVOUT_480P_59:
		case TVOUT_576P_50_16_9:
		case TVOUT_576P_50_4_3:
			s5p_mixer_init_csc_coef_default(
				MIXER_CSC_601_FR);
			break;

		case TVOUT_720P_60:
		case TVOUT_720P_50:
		case TVOUT_720P_59:
		case TVOUT_1080I_60:
		case TVOUT_1080I_59:
		case TVOUT_1080I_50:
		case TVOUT_1080P_60:
		case TVOUT_1080P_30:
		case TVOUT_1080P_59:
		case TVOUT_1080P_50:
			s5p_mixer_init_csc_coef_default(
				MIXER_CSC_709_FR);
			break;
		default:
			break;
		}

		break;

	default:
		TVOUTIFPRINTK("invalid tvout_param.out parameter(%d)\n\r",
			      out);
		return false;
	}

	s5p_mixer_start();

	return true;
}

static void s5p_tvout_ctrl_init_private(void)
{
	private.running = false;
}

/*
 * TV cut off sequence
 * VP stop -> Mixer stop -> HDMI stop -> HDMI TG stop
 * Above sequence should be satisfied.
 */
static bool s5p_tvout_ctrl_internal_stop(void)
{
	/* how to control the clock path on stop time ??? */
//	struct s5p_tv_status *st = &s5ptv_status;

//	TVOUTIFPRINTK("tvout sub sys. stopped!!\n");

	s5p_mixer_stop();

//	switch (st->tvout_param.out) {
	switch (private.curr_if) {
	case TVOUT_COMPOSITE:
//		if (st->tvout_output_enable)
			s5p_sdo_ctrl_stop();
		break;

	case TVOUT_DVI:
	case TVOUT_HDMI:
	case TVOUT_HDMI_RGB:
//		if (st->tvout_output_enable)
			s5p_hdmi_ctrl_stop();
		break;

	default:
		tv_err("invalid out parameter(%d)\n\r", private.curr_if);
//		TVOUTIFPRINTK("invalid out parameter(%d)\n\r",
//			st->tvout_param.out);
		return false;
	}

	return true;
}

bool s5p_tvout_ctrl_get_std_if(
		enum s5p_tv_disp_mode *std,
		enum s5p_tv_o_mode *inf)
{
	if (!private.running)
		goto not_running;

	*std = private.curr_std;
	*inf = private.curr_if;

	return true;

not_running:
		return false;
	}

bool s5p_tvout_ctrl_set_std_if(
		enum s5p_tv_disp_mode std,
		enum s5p_tv_o_mode inf)
{
	private.new_std = std;
	private.new_if = inf;

	return true;
}

bool s5p_tvout_ctrl_start(
		enum s5p_tv_disp_mode std,
		enum s5p_tv_o_mode inf)
{
	struct s5p_tv_status *st = &s5ptv_status;
//	enum s5p_tv_o_mode out = st->tvout_param.out;

	if (private.running &&
		(std == private.curr_std) &&
		(inf == private.curr_if))
		goto cannot_change;

	switch (inf) {
	case TVOUT_COMPOSITE:
	case TVOUT_HDMI:
	case TVOUT_HDMI_RGB:
	case TVOUT_DVI:
		break;
	default:
		tv_err("invalid out parameter(%d)\n\r",	inf);
		goto cannot_change;
	}

	s5p_mixer_set_underflow_interrupt_enable(MIXER_VIDEO_LAYER, false);
	s5p_mixer_set_underflow_interrupt_enable(MIXER_GPR0_LAYER, false);
	s5p_mixer_set_underflow_interrupt_enable(MIXER_GPR1_LAYER, false);

	/* how to control the clock path on stop time ??? */
	if (private.running)
		s5p_tvout_ctrl_internal_stop();

	/* Clear All Interrupt Pending */
	s5p_mixer_clear_pend_all();

	switch (inf) {
	case TVOUT_COMPOSITE:
		clk_set_parent(st->sclk_mixer, st->sclk_dac);

		if (!s5p_tvout_ctrl_init_vm_reg(std, inf))
			return false;

		if (0 != s5p_sdo_ctrl_start(std))
			return false;

		break;

	case TVOUT_HDMI:
	case TVOUT_HDMI_RGB:
	case TVOUT_DVI:
		clk_set_parent(st->sclk_mixer, st->sclk_hdmi);
		clk_set_parent(st->sclk_hdmi, st->sclk_hdmiphy);

		if (!s5p_tvout_ctrl_init_vm_reg(std, inf))
			return false;

		if (0 != s5p_hdmi_ctrl_start(std, inf))
			return false;
		break;
	}

	st->tvout_output_enable = true;

	private.running = true;
	private.curr_std = std;
	private.curr_if = inf;

	s5p_mixer_set_underflow_interrupt_enable(MIXER_VIDEO_LAYER, true);
	s5p_mixer_set_underflow_interrupt_enable(MIXER_GPR0_LAYER, true);
	s5p_mixer_set_underflow_interrupt_enable(MIXER_GPR1_LAYER, true);

	/* Clear All Interrupt Pending */
	s5p_mixer_clear_pend_all();

	return true;
	
cannot_change:
	return false;
}

void s5p_tvout_ctrl_stop(void)
{
	struct s5p_tv_status *st = &s5ptv_status;

	if (private.running) {
		s5p_tvout_ctrl_internal_stop();

		st->tvout_output_enable = false;
		private.running = false;
	}
}

int s5p_tvout_ctrl_constructor(struct platform_device *pdev)
{
	if (s5p_sdo_ctrl_constructor(pdev))
		goto err;

	if (s5p_hdmi_ctrl_constructor(pdev))
		goto err;

	s5p_tvout_ctrl_init_private();

	return 0;

err:
	return -1;
}

void s5p_tvout_ctrl_destructor(void)
{
	s5p_sdo_ctrl_destructor();
	s5p_hdmi_ctrl_destructor();
}

void s5p_tvout_ctrl_suspend(void)
{
//	s5p_sdo_ctrl_suspend();
//	s5p_hdmi_ctrl_suspend();
}

void s5p_tvout_ctrl_resume(void)
{
//	s5p_sdo_ctrl_resume();
//	s5p_hdmi_ctrl_resume();
}
