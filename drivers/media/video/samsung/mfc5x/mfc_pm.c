/*
 * linux/drivers/media/video/samsung/mfc5x/mfc_pm.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Power management module for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/clk.h>

#include "mfc_dev.h"
#include "mfc_log.h"

#if defined(CONFIG_ARCH_S5PV210)
#include <plat/clock.h>
#include <mach/pd.h>

#define MFC_CLK_NAME	"mfc"
#define MFC_PD_NAME	"mfc_pd"

static struct mfc_pm pm;

int mfc_init_pm(struct mfc_dev *mfcdev)
{
	int ret = 0;

	sprintf(mfcdev->pm.clk_name, "%s", MFC_CLK_NAME);
	sprintf(mfcdev->pm.pd_name, "%s", MFC_PD_NAME);

	memcpy(&pm, &mfcdev->pm, sizeof(struct mfc_pm));

	pm.clock = clk_get(mfcdev->device, pm.clk_name);

	if (IS_ERR(pm.clock))
		ret = -ENOENT;

	return ret;
}

int mfc_clock_on(void)
{
	return clk_enable(pm.clock);
}

int mfc_clock_off(void)
{
	clk_disable(pm.clock);

	return 0;
}

int mfc_power_on(void)
{
	return s5pv210_pd_enable(pm.pd_name);
}

int mfc_power_off(void)
{
	return s5pv210_pd_disable(pm.pd_name);
}

#elif defined(CONFIG_ARCH_S5PV310)

#define MFC_PARENT_CLK_NAME	"mout_mfc0"
#define MFC_CLKNAME		"sclk_mfc"
#define MFC_GATE_CLK_NAME	"mfc"

static struct mfc_pm pm;

int mfc_init_pm(struct mfc_dev *mfcdev)
{
	struct clk *parent, *sclk;
	int ret = 0;

	memcpy(&pm, &mfcdev->pm, sizeof(struct mfc_pm));

	parent = clk_get(mfcdev->device, MFC_PARENT_CLK_NAME);
	if (IS_ERR(parent)) {
		printk(KERN_ERR "failed to get parent clock\n");
		ret = -ENOENT;
		goto err_p_clk;
	}

	sclk = clk_get(mfcdev->device, MFC_CLKNAME);
	if (IS_ERR(sclk)) {
		printk(KERN_ERR "failed to get source clock\n");
		ret = -ENOENT;
		goto err_s_clk;
	}

	clk_set_parent(sclk, parent);
	clk_set_rate(sclk, 200 * 1000000);

	/* clock for gating */
	pm.clock = clk_get(mfcdev->device, MFC_GATE_CLK_NAME);
	if (IS_ERR(pm.clock)) {
		printk(KERN_ERR "failed to get clock-gating control\n");
		ret = -ENOENT;
		goto err_g_clk;
	}

	return 0;

err_g_clk:
	clk_put(sclk);
err_s_clk:
	clk_put(parent);
err_p_clk:
	return ret;
}

int mfc_clock_on(void)
{
	return clk_enable(pm.clock);
}

int mfc_clock_off(void)
{
	clk_disable(pm.clock);

	return 0;
}

int mfc_power_on(void)
{
	return 0;
}

int mfc_power_off(void)
{
	return 0;
}

#else

int mfc_init_pm(struct mfc_dev *mfcdev)
{
	return -1;
}

int mfc_clock_on(void)
{
	return -1;
}

int mfc_clock_off(void)
{
	return -1;
}

int mfc_power_on(void)
{
	return -1;
}

int mfc_power_off(void)
{
	return -1;
}

#endif
