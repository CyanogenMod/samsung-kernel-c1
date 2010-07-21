/* linux/drivers/media/video/samsung/tv20/ver_1/tv_power.c
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsung.com/
 *
 * power raw ftn  file for Samsung TVOut driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/io.h>

#include <mach/map.h>
#include <mach/regs-clock.h>


#include "tv_out.h"

#ifdef COFIG_TVOUT_RAW_DBG
#define S5P_TVOUT_PM_DEBUG 1
#endif

#ifdef S5P_TVOUT_PM_DEBUG
#define TVPMPRINTK(fmt, args...) \
	printk(KERN_INFO "\t\t[TVPM] %s: " fmt, __func__ , ## args)
#else
#define TVPMPRINTK(fmt, args...)
#endif

/* NORMAL_CFG */
#define TVPWR_SUBSYSTEM_ACTIVE (1<<4)
#define TVPWR_SUBSYSTEM_LP     (0<<4)

/* MTC_STABLE */
#define TVPWR_MTC_COUNTER_CLEAR(a) (((~0xf)<<16)&a)
#define TVPWR_MTC_COUNTER_SET(a)   ((0xf&a)<<16)

/* BLK_PWR_STAT */
#define TVPWR_TV_BLOCK_STATUS(a)    ((0x1<<4)&a)

void s5p_tv_powerset_dac_onoff(unsigned short on)
{
	TVPMPRINTK("(%d)\n\r", on);

	if (on)
		writel(S5P_DAC_ENABLE, S5P_DAC_CONTROL);
	else
		writel(S5P_DAC_DISABLE, S5P_DAC_CONTROL);
}

void s5p_tv_power_on(void)
{
	TVPMPRINTK("0x%08x\n\r", readl(S3C_VA_SYS + 0xE804));

	writel(readl(S3C_VA_SYS + 0xE804) | 0x1, S3C_VA_SYS + 0xE804);
	writel(readl(S5P_NORMAL_CFG) | TVPWR_SUBSYSTEM_ACTIVE, S5P_NORMAL_CFG);

	while (!TVPWR_TV_BLOCK_STATUS(readl(S5P_BLK_PWR_STAT)))
		msleep(1);
}

void s5p_tv_power_off(void)
{
	TVPMPRINTK("()\n\r");

	s5p_tv_powerset_dac_onoff(0);
	writel(readl(S5P_NORMAL_CFG) & ~TVPWR_SUBSYSTEM_ACTIVE, S5P_NORMAL_CFG);

	while (TVPWR_TV_BLOCK_STATUS(readl(S5P_BLK_PWR_STAT)))
		msleep(1);
}
