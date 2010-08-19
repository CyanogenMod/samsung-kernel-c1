/* linux/arch/arm/mach-s5p6450/setup-fimc.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Base FIMC gpio configuration
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <plat/clock.h>
#include <plat/gpio-cfg.h>
#include <plat/map-s5p.h>
#include <mach/regs-gpio.h>
#include <mach/map.h>
//#include <mach/pd.h>

struct platform_device; /* don't need the contents */

void s3c_fimc0_cfg_gpio(struct platform_device *pdev)
{
	int i = 0;

	/* CAM A port(b0010) : PCLK, VSYNC, HREF, DATA[0-4] */
	for (i=0; i < 14; i++) {
		s3c_gpio_cfgpin(S5P6450_GPQ(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5P6450_GPQ(i), S3C_GPIO_PULL_NONE);
	}
	/* note : driver strength to max is unnecessary */
}
int s3c_fimc_clk_on(struct platform_device *pdev, struct clk **clk)
{
	struct clk *sclk_fimc = NULL;
	struct clk *dout_mpll = NULL;
	struct clk *mout_mpll = NULL;
	struct clk *hclk_low = NULL;
	
	struct clk *sclk_camif = NULL;
	struct clk *dout_epll = NULL;
	struct clk *mout_epll = NULL;

	/* fimc core clock(166Mhz) */
	mout_mpll = clk_get(&pdev->dev, "mout_mpll");
	if (IS_ERR(mout_mpll)) {
		dev_err(&pdev->dev, "failed to get mout_mpll\n");
		goto err_clk1;
	}

	dout_mpll = clk_get(&pdev->dev, "dout_mpll");
	if (IS_ERR(dout_mpll)) {
		dev_err(&pdev->dev, "failed to get dout_mpll\n");
		goto err_clk2;
	}

	sclk_fimc = clk_get(&pdev->dev, "sclk_fimc");
	if (IS_ERR(sclk_fimc)) {
		dev_err(&pdev->dev, "failed to get sclk_fimc\n");
		goto err_clk3;
	}

	mout_epll = clk_get(&pdev->dev, "mout_epll");
	dout_epll = clk_get(&pdev->dev, "dout_epll");
	//clk_set_parent(dout_mpll, mout_mpll);
	//clk_set_parent(sclk_fimc, dout_mpll);
/*	clk_set_rate(dout_mpll, 400000000);*/
	clk_set_rate(sclk_fimc, 200000000);

	//clk_enable(sclk_fimc);
	clk_disable(sclk_fimc);
	/* bus clock(133Mhz) */
	hclk_low = clk_get(&pdev->dev, "fimc");
	if (IS_ERR(hclk_low)) {
		dev_err(&pdev->dev, "failed to get hclk_low\n");
		goto err_clk4;
	}
	
	clk_enable(hclk_low);
	
//	mout_epll = clk_get(&pdev->dev, "mout_epll");
//	dout_epll = clk_get(&pdev->dev, "dout_epll");
//	sclk_camif = clk_get(&pdev->dev, "sclk_camif");
/*
	clk_put(mout_mpll);
	clk_put(dout_mpll);
	clk_put(sclk_fimc);
	clk_put(hclk_low);
*/
	printk(KERN_INFO "mout_mpll : %ld\n", clk_get_rate(mout_mpll));
	printk(KERN_INFO "dout_mpll : %ld\n", clk_get_rate(dout_mpll));
	printk(KERN_INFO "sclk_fimc : %ld\n", clk_get_rate(sclk_fimc));
	printk(KERN_INFO "mout_epll : %ld\n", clk_get_rate(mout_epll));
	printk(KERN_INFO "dout_epll : %ld\n", clk_get_rate(dout_epll));
	printk(KERN_INFO "hclk_low : %ld\n", clk_get_rate(hclk_low));
	return 0;

err_clk4:
	clk_put(hclk_low);
err_clk3:
	clk_put(sclk_fimc);
err_clk2:
	clk_put(dout_mpll);
err_clk1:
	return -EINVAL;
}

int s3c_fimc_clk_off(struct platform_device *pdev, struct clk **clk)
{
	struct clk *sclk_fimc = NULL;
	struct clk *hclk_low = NULL;

	sclk_fimc = clk_get(&pdev->dev, "sclk_fimc");
	if (IS_ERR(sclk_fimc)) {
		dev_err(&pdev->dev, "failed to get sclk_fimc\n");
		goto err_clk1;
	}
	clk_disable(sclk_fimc);
	clk_put(sclk_fimc);

	/* bus clock(133Mhz) */
	hclk_low = clk_get(&pdev->dev, "fimc");
	if (IS_ERR(hclk_low)) {
		dev_err(&pdev->dev, "failed to get hclk_low\n");
		goto err_clk2;
	}
	clk_disable(hclk_low);
	clk_put(hclk_low);

	return 0;

err_clk1:
	clk_put(sclk_fimc);
err_clk2:
	clk_put(hclk_low);
	return -EINVAL;
}
