/* linux/arch/arm/mach-s5pv310/setup-fb.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Base FIMD controller configuration
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <plat/clock.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-clock.h>
#include <mach/regs-gpio.h>
#include <linux/io.h>
#include <mach/map.h>
#include <mach/gpio.h>
/* #include <mach/pd.h> */

struct platform_device; /* don't need the contents */

void s3cfb_cfg_gpio(struct platform_device *pdev)
{
	int i;
	u32 reg;

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PV310_GPF0(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV310_GPF0(i), S3C_GPIO_PULL_NONE);
		s5p_gpio_set_drvstr(S5PV310_GPF0(i), S5P_GPIO_DRVSTR_LV4);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PV310_GPF1(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV310_GPF1(i), S3C_GPIO_PULL_NONE);
		s5p_gpio_set_drvstr(S5PV310_GPF1(i), S5P_GPIO_DRVSTR_LV4);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(S5PV310_GPF2(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV310_GPF2(i), S3C_GPIO_PULL_NONE);
		s5p_gpio_set_drvstr(S5PV310_GPF2(i), S5P_GPIO_DRVSTR_LV4);
	}

	for (i = 0; i < 4; i++) {
		s3c_gpio_cfgpin(S5PV310_GPF3(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV310_GPF3(i), S3C_GPIO_PULL_NONE);
		s5p_gpio_set_drvstr(S5PV310_GPF3(i), S5P_GPIO_DRVSTR_LV4);
	}

	/* Set FIMD0 bypass */
	reg = __raw_readl(S3C_VA_SYS + 0x0210);
	reg |= (1<<1);
	__raw_writel(reg, S3C_VA_SYS + 0x0210);
}

int s3cfb_clk_on(struct platform_device *pdev, struct clk **s3cfb_clk)
{
	struct clk *sclk = NULL;
	struct clk *mout_mpll = NULL;

	u32 rate = 0;
	int ret;

	sclk = clk_get(&pdev->dev, "sclk_fimd");
	if (IS_ERR(sclk)) {
		dev_err(&pdev->dev, "failed to get sclk for fimd\n");
		goto err_clk1;
	}

	mout_mpll = clk_get(&pdev->dev, "mout_mpll");
	if (IS_ERR(mout_mpll)) {
		dev_err(&pdev->dev, "failed to get mout_mpll\n");
		goto err_clk1;
	}

	clk_set_parent(sclk, mout_mpll);

	rate = clk_round_rate(sclk, 133400000);
	dev_dbg(&pdev->dev, "set fimd sclk rate to %d\n", rate);

	if (!rate)
		rate = 133400000;

	clk_set_rate(sclk, rate);
	dev_dbg(&pdev->dev, "set fimd sclk rate to %d\n", rate);

	clk_put(mout_mpll);

	clk_enable(sclk);

	*s3cfb_clk = sclk;

	return 0;

err_clk1:
	clk_put(mout_mpll);

	clk_put(sclk);


	return -EINVAL;

}

int s3cfb_clk_off(struct platform_device *pdev, struct clk **clk)
{
	int ret;

	clk_disable(*clk);
	clk_put(*clk);

	*clk = NULL;

	if (ret < 0)
		dev_err(&pdev->dev, "failed to disable fimd power domain\n");

	return 0;
}

void s3cfb_get_clk_name(char *clk_name)
{
	strcpy(clk_name, "sclk_fimd");
}

#ifdef CONFIG_FB_S3C_WA101S
int s3cfb_backlight_on(struct platform_device *pdev)
{
	int err;

	err = gpio_request(S5PV310_GPD0(1), "GPD0");

	if (err) {
		printk(KERN_ERR "failed to request GPD0 for "
			"lcd backlight control\n");
		return err;
	}

	gpio_direction_output(S5PV310_GPD0(1), 1);
	gpio_free(S5PV310_GPD0(1));

	return 0;
}

int s3cfb_backlight_off(struct platform_device *pdev)
{
	int err;

	err = gpio_request(S5PV310_GPD0(1), "GPD0");

	if (err) {
		printk(KERN_ERR "failed to request GPD0 for "
				"lcd backlight control\n");
		return err;
	}

	gpio_direction_output(S5PV310_GPD0(1), 0);
	gpio_free(S5PV310_GPD0(1));

	return 0;
}

int s3cfb_lcd_on(struct platform_device *pdev)
{
	return 0;
}

int s3cfb_lcd_off(struct platform_device *pdev)
{
	return 0;
}

#elif defined(CONFIG_FB_S3C_LTE480WV)
int s3cfb_backlight_on(struct platform_device *pdev)
{
#if !defined(CONFIG_BACKLIGHT_PWM)
	int err;

	err = gpio_request(S5PV310_GPD0(1), "GPD0");

	if (err) {
		printk(KERN_ERR "failed to request GPD0 for "
			"lcd backlight control\n");
		return err;
	}

	gpio_direction_output(S5PV310_GPD0(1), 1);
	gpio_free(S5PV310_GPD0(1));
#endif
	return 0;
}

int s3cfb_backlight_off(struct platform_device *pdev)
{
#if !defined(CONFIG_BACKLIGHT_PWM)
	int err;

	err = gpio_request(S5PV310_GPD0(1), "GPD0");

	if (err) {
		printk(KERN_ERR "failed to request GPD0 for "
				"lcd backlight control\n");
		return err;
	}

	gpio_direction_output(S5PV310_GPD0(1), 0);
	gpio_free(S5PV310_GPD0(1));
#endif
	return 0;
}

int s3cfb_lcd_on(struct platform_device *pdev)
{
	int err;

	err = gpio_request(S5PV310_GPX0(6), "GPX0");
	if (err) {
		printk(KERN_ERR "failed to request GPX0 for "
			"lcd reset control\n");
		return err;
	}

	gpio_direction_output(S5PV310_GPX0(6), 1);
	mdelay(100);

	gpio_set_value(S5PV310_GPX0(6), 0);
	mdelay(10);

	gpio_set_value(S5PV310_GPX0(6), 1);
	mdelay(10);

	gpio_free(S5PV310_GPX0(6));

	return 0;
}

int s3cfb_lcd_off(struct platform_device *pdev)
{
	return 0;
}
#elif defined(CONFIG_FB_S3C_HT101HD1)
int s3cfb_backlight_on(struct platform_device *pdev)
{
	int err;

	err = gpio_request(S5PV310_GPD0(0), "GPD0");
	if (err) {
		printk(KERN_ERR "failed to request GPD0 for "
				"lcd backlight control\n");
		return err;
	}

	err = gpio_request(S5PV310_GPB(2), "GPB");
	if (err) {
		printk(KERN_ERR "failed to request GPB for "
				"lcd LED_EN control\n");
		return err;
	}

	gpio_direction_output(S5PV310_GPD0(0), 1); /* BL pwm High */

	gpio_direction_output(S5PV310_GPB(2), 1); /* LED_EN (SPI1_MOSI) */

	gpio_free(S5PV310_GPD0(0));
	gpio_free(S5PV310_GPB(2));

	return 0;
}

int s3cfb_backlight_off(struct platform_device *pdev)
{
#if !defined(CONFIG_BACKLIGHT_PWM)
	int err;

	err = gpio_request(S5PV310_GPD0(0), "GPD0");

	if (err) {
		printk(KERN_ERR "failed to request GPD0 for "
				"lcd backlight control\n");
		return err;
	}

	err = gpio_request(S5PV310_GPB(2), "GPB");
	if (err) {
		printk(KERN_ERR "failed to request GPB for "
				"lcd LED_EN control\n");
		return err;
	}

	gpio_direction_output(S5PV310_GPD0(3), 0);
	gpio_direction_output(S5PV310_GPB(2), 0);

	gpio_free(S5PV310_GPD0(0));
	gpio_free(S5PV310_GPB(2));
#endif
	return 0;
}

int s3cfb_lcd_on(struct platform_device *pdev)
{
	int err;

	err = gpio_request(S5PV310_GPH0(1), "GPH0");
	if (err) {
		printk(KERN_ERR "failed to request GPH0 for "
			"lcd reset control\n");
		return err;
	}

	gpio_direction_output(S5PV310_GPH0(1), 1);

	gpio_set_value(S5PV310_GPH0(1), 0);

	gpio_set_value(S5PV310_GPH0(1), 1);

	gpio_free(S5PV310_GPH0(1));

	return 0;
}

int s3cfb_lcd_off(struct platform_device *pdev)
{
	return 0;
}
#else
int s3cfb_backlight_on(struct platform_device *pdev)
{
	return 0;
}

int s3cfb_backlight_off(struct platform_device *pdev)
{
	return 0;
}

int s3cfb_lcd_on(struct platform_device *pdev)
{
	return 0;
}

int s3cfb_lcd_off(struct platform_device *pdev)
{
	return 0;
}
#endif
