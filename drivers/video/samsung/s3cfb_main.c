/* linux/drivers/video/samsung/s3cfb-main.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Core file for Samsung Display Controller (FIMD) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/ctype.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/memory.h>
#include <plat/clock.h>
#include <plat/media.h>
#include <mach/media.h>
#include "s3cfb.h"

struct s3cfb_fimd_desc		*fbfimd;

inline struct s3cfb_global *get_fimd_global(int id)
{
	struct s3cfb_global *fbdev;

	if (id < 5)
		fbdev = fbfimd->fbdev[0];
	else
		fbdev = fbfimd->fbdev[1];

	return fbdev;
}

static irqreturn_t s3cfb_irq_frame(int irq, void *dev_id)
{
	struct s3cfb_global *fbdev[2];
	fbdev[0] = fbfimd->fbdev[0];

	s3cfb_clear_interrupt(fbdev[0]);

	fbdev[0]->wq_count++;
	wake_up(&fbdev[0]->wq);

	return IRQ_HANDLED;
}

#ifdef CONFIG_FB_S3C_TRACE_UNDERRUN
static irqreturn_t s3cfb_irq_fifo(int irq, void *dev_id)
{
	struct s3cfb_global *fbdev[2];
	fbdev[0] = fbfimd->fbdev[0];

	s3cfb_clear_interrupt(fbdev[0]);

	return IRQ_HANDLED;
}
#endif

int s3cfb_register_framebuffer(struct s3cfb_global *fbdev)
{
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);
	int ret, i;

	for (i = 0; i < pdata->nr_wins; i++) {
		ret = register_framebuffer(fbdev->fb[i]);
		if (ret) {
			dev_err(fbdev->dev, "failed to register	\
				framebuffer device\n");
			return -EINVAL;
		}
#ifndef CONFIG_FRAMEBUFFER_CONSOLE
		if (i == pdata->default_win) {
			s3cfb_check_var_window(fbdev, &fbdev->fb[i]->var,
					fbdev->fb[i]);
			s3cfb_set_par_window(fbdev, fbdev->fb[i]);
			s3cfb_draw_logo(fbdev->fb[i]);
		}
#endif
	}
	return 0;
}

static int s3cfb_sysfs_show_win_power(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct s3c_platform_fb *pdata = to_fb_plat(dev);
	struct s3cfb_window *win;
	char temp[16];
	int i;
	struct s3cfb_global *fbdev[1];
	fbdev[0] = fbfimd->fbdev[0];

	for (i = 0; i < pdata->nr_wins; i++) {
		win = fbdev[0]->fb[i]->par;
		sprintf(temp, "[fb%d] %s\n", i, win->enabled ? "on" : "off");
		strcat(buf, temp);
	}

	return strlen(buf);
}

static int s3cfb_sysfs_store_win_power(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t len)
{
	struct s3c_platform_fb *pdata = to_fb_plat(dev);
	char temp[4] = { 0, };
	const char *p = buf;
	int id, to;
	struct s3cfb_global *fbdev[1];
	fbdev[0] = fbfimd->fbdev[0];

	while (*p != '\0') {
		if (!isspace(*p))
			strncat(temp, p, 1);
		p++;
	}

	if (strlen(temp) != 2)
		return -EINVAL;

	id = simple_strtoul(temp, NULL, 10) / 10;
	to = simple_strtoul(temp, NULL, 10) % 10;

	if (id < 0 || id > pdata->nr_wins)
		return -EINVAL;

	if (to != 0 && to != 1)
		return -EINVAL;

	if (to == 0)
		s3cfb_disable_window(fbdev[0], id);
	else
		s3cfb_enable_window(fbdev[0], id);

	return len;
}

static DEVICE_ATTR(win_power, 0644,
	s3cfb_sysfs_show_win_power, s3cfb_sysfs_store_win_power);

static int s3cfb_probe(struct platform_device *pdev)
{
	struct s3c_platform_fb *pdata = NULL;
	struct resource *res = NULL;
	struct s3cfb_global *fbdev[2];
	int ret = 0;
	int i = 0;

	fbfimd = kzalloc(sizeof(struct s3cfb_fimd_desc), GFP_KERNEL);

	if (FIMD_MAX == 2)
		fbfimd->dual = 1;
	else
		fbfimd->dual = 0;

	for (i = 0; i < FIMD_MAX; i++) {
		/* global structure */
		fbdev[i] = kzalloc(sizeof(struct s3cfb_global), GFP_KERNEL);
		if (!fbdev[i]) {
			dev_err(fbdev[i]->dev, "failed to allocate for	\
				global fb structure fimd[%d]!\n", i);
			goto err0;
		}

		fbdev[i]->dev = &pdev->dev;
		s3cfb_set_lcd_info(fbdev[i]);

		/* platform_data*/
		pdata = to_fb_plat(&pdev->dev);
		if (pdata->cfg_gpio)
			pdata->cfg_gpio(pdev);

		if (pdata->clk_on)
			pdata->clk_on(pdev, &fbdev[i]->clock);

		/* io memory */
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res) {
			dev_err(fbdev[i]->dev,
				"failed to get io memory region\n");
			ret = -EINVAL;
			goto err1;
		}
		res = request_mem_region(res->start,
					res->end - res->start + 1, pdev->name);
		if (!res) {
			dev_err(fbdev[i]->dev,
				"failed to request io memory region\n");
			ret = -EINVAL;
			goto err1;
		}
		fbdev[i]->regs = ioremap(res->start, res->end - res->start + 1);
		if (!fbdev[i]->regs) {
			dev_err(fbdev[i]->dev, "failed to remap io region\n");
			ret = -EINVAL;
			goto err1;
		}

		/* irq */
		fbdev[i]->irq = platform_get_irq(pdev, 0);
		if (request_irq(fbdev[i]->irq, s3cfb_irq_frame, IRQF_SHARED,
				pdev->name, fbdev[i])) {
			dev_err(fbdev[i]->dev, "request_irq failed\n");
			ret = -EINVAL;
			goto err2;
		}
#ifdef CONFIG_FB_S3C_TRACE_UNDERRUN
		if (request_irq(platform_get_irq(pdev, 1), s3cfb_irq_fifo,
				IRQF_DISABLED, pdev->name, fbdev[i])) {
			dev_err(fbdev[i]->dev, "request_irq failed\n");
			ret = -EINVAL;
			goto err2;
		}

		s3cfb_set_fifo_interrupt(fbdev[i], 1);
		dev_info(fbdev[i]->dev, "fifo underrun trace\n");
#endif
		/* hw setting */
		s3cfb_init_global(fbdev[i]);

		/* alloc fb_info */
		if (s3cfb_alloc_framebuffer(fbdev[i])) {
			dev_err(fbdev[i]->dev, "alloc error fimd[%d]\n", i);
			goto err3;
		}

		/* register fb_info */
		if (s3cfb_register_framebuffer(fbdev[i])) {
			dev_err(fbdev[i]->dev, "register error fimd[%d]\n", i);
			goto err3;
		}

		/* enable display */
		s3cfb_set_clock(fbdev[i]);
		s3cfb_enable_window(fbdev[0], pdata->default_win);

		s3cfb_update_power_state(fbdev[i], pdata->default_win,
					FB_BLANK_UNBLANK);
		s3cfb_display_on(fbdev[i]);

		fbfimd->fbdev[i] = fbdev[i];

	}
#ifdef CONFIG_FB_S3C_LCD_INIT
	/* panel control */
	if (pdata->backlight_on)
		pdata->backlight_on(pdev);

	if (pdata->lcd_on)
		pdata->lcd_on(pdev);
#endif

	ret = device_create_file(&(pdev->dev), &dev_attr_win_power);
	if (ret < 0)
		dev_err(fbdev[0]->dev, "failed to add sysfs entries\n");

	dev_info(fbdev[0]->dev, "registered successfully\n");
	return 0;

err3:
	for (i = 0; i < FIMD_MAX; i++)
		free_irq(fbdev[i]->irq, fbdev[i]);
err2:
	for (i = 0; i < FIMD_MAX; i++)
		iounmap(fbdev[i]->regs);
err1:
	for (i = 0; i < FIMD_MAX; i++)
		pdata->clk_off(pdev, &fbdev[i]->clock);
err0:
	return ret;
}

static int s3cfb_remove(struct platform_device *pdev)
{
	struct s3c_platform_fb *pdata = to_fb_plat(&pdev->dev);
	struct s3cfb_window *win;
	struct fb_info *fb;
	struct s3cfb_global *fbdev[2];
	int i;
	int j;

	for (i = 0; i < FIMD_MAX; i++) {
		fbdev[i] = fbfimd->fbdev[i];

		free_irq(fbdev[i]->irq, fbdev[i]);
		iounmap(fbdev[i]->regs);
		pdata->clk_off(pdev, &fbdev[i]->clock);

		for (j = 0; j < pdata->nr_wins; j++) {
			fb = fbdev[i]->fb[j];

			/* free if exists */
			if (fb) {
				win = fb->par;
				if (win->id == pdata->default_win)
					s3cfb_unmap_default_video_memory(fbdev[i], fb);
				else
					s3cfb_unmap_video_memory(fbdev[i], fb);

				s3cfb_set_buffer_address(fbdev[i], j);
				framebuffer_release(fb);
			}
		}

		kfree(fbdev[i]->fb);
		kfree(fbdev[i]);
	}

	return 0;
}

#ifdef CONFIG_PM
int s3cfb_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct s3c_platform_fb *pdata = to_fb_plat(&pdev->dev);
	struct s3cfb_global *fbdev[2];
	int i;

	for (i = 0; i < FIMD_MAX; i++) {
		fbdev[i] = fbfimd->fbdev[i];

		if (atomic_read(&fbdev[i]->enabled_win) > 0) {
			/* lcd_off and backlight_off isn't needed. */
			if (fbdev[i]->lcd->deinit_ldi)
				fbdev[i]->lcd->deinit_ldi();

			s3cfb_display_off(fbdev[i]);
			pdata->clk_off(pdev, &fbdev[i]->clock);
		}

	}

	return 0;
}

int s3cfb_resume(struct platform_device *pdev)
{
	struct s3c_platform_fb *pdata = to_fb_plat(&pdev->dev);
	struct fb_info *fb;
	struct s3cfb_window *win;
	struct s3cfb_global *fbdev[2];
	int i;
	int j;

	for (i = 0; i < FIMD_MAX; i++) {
		dev_dbg(fbdev[i]->dev, "wake up from suspend fimd[%d]\n", i);

		if (pdata->cfg_gpio)
			pdata->cfg_gpio(pdev);

		if (pdata->backlight_on)
			pdata->backlight_on(pdev);
		if (pdata->lcd_on)
			pdata->lcd_on(pdev);
		if (fbdev[i]->lcd->init_ldi)
			fbdev[i]->lcd->init_ldi();

		if (pdata->backlight_off)
			pdata->backlight_off(pdev);
		if (pdata->lcd_off)
			pdata->lcd_off(pdev);
		if (fbdev[i]->lcd->deinit_ldi)
			fbdev[i]->lcd->deinit_ldi();

		if (atomic_read(&fbdev[i]->enabled_win) > 0) {
			pdata->clk_on(pdev, &fbdev[i]->clock);
			s3cfb_init_global(fbdev[i]);
			s3cfb_set_clock(fbdev[i]);

			for (j = 0; j < pdata->nr_wins; j++) {
				fb = fbdev[i]->fb[j];
				win = fb->par;
				if (win->owner == DMA_MEM_FIMD) {
					s3cfb_set_win_params(fbdev[i], win->id);
					if (win->enabled) {
						if (win->power_state == FB_BLANK_NORMAL)
							s3cfb_win_map_on(fbdev[i], win->id, 0x0);

						s3cfb_enable_window(fbdev[i], win->id);
					}
				}
			}

			s3cfb_display_on(fbdev[i]);

			if (pdata->backlight_on)
				pdata->backlight_on(pdev);

			if (pdata->lcd_on)
				pdata->lcd_on(pdev);

			if (fbdev[i]->lcd->init_ldi)
				fbdev[i]->lcd->init_ldi();
		}
	}

	return 0;
}
#else
#define s3cfb_suspend NULL
#define s3cfb_resume NULL
#endif

static struct platform_driver s3cfb_driver = {
	.probe		= s3cfb_probe,
	.remove		= s3cfb_remove,
	.suspend	= s3cfb_suspend,
	.resume		= s3cfb_resume,
	.driver		= {
		.name	= S3CFB_NAME,
		.owner	= THIS_MODULE,
	},
};

struct fb_ops s3cfb_ops = {
	.owner		= THIS_MODULE,
	.fb_open	= s3cfb_open,
	.fb_release	= s3cfb_release,
	.fb_check_var	= s3cfb_check_var,
	.fb_set_par	= s3cfb_set_par,
	.fb_setcolreg	= s3cfb_setcolreg,
	.fb_blank	= s3cfb_blank,
	.fb_pan_display	= s3cfb_pan_display,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
	.fb_cursor	= s3cfb_cursor,
	.fb_ioctl	= s3cfb_ioctl,
};

static int s3cfb_register(void)
{
	platform_driver_register(&s3cfb_driver);
	return 0;
}

static void s3cfb_unregister(void)
{
	platform_driver_unregister(&s3cfb_driver);
}

module_init(s3cfb_register);
module_exit(s3cfb_unregister);

MODULE_AUTHOR("Jingoo Han <jg1.han@samsung.com>");
MODULE_AUTHOR("Jonghun, Han <jonghun.han@samsung.com>");
MODULE_AUTHOR("Jinsung, Yang <jsgood.yang@samsung.com>");
MODULE_DESCRIPTION("Samsung Display Controller (FIMD) driver");
MODULE_LICENSE("GPL");
