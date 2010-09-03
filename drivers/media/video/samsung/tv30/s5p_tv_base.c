/* linux/drivers/media/video/samsung/tv20/s5p_tv_base.c
 *
 * Copyright (c) 2009 Samsung Electronics
 *		http://www.samsung.com/
 *
 * Entry file for Samsung TVOut driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/ioctl.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/uaccess.h>

#include <media/v4l2-common.h>
#include <media/v4l2-ioctl.h>
#include <mach/map.h>

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>

//#include <mach/pd.h>

#if defined(CONFIG_S5P_SYSMMU_TV)
#include <plat/sysmmu.h>
#endif

#include "ver_1/mixer.h"
#include "s5p_tv_base.h"
#include "hpd.h"
#include "s5p_stda_grp.h"
#include "s5p_tvout_ctrl.h"
#include "s5p_mixer_ctrl.h"
#include "s5p_tv_fb.h"
#include "s5p_tv_v4l2.h"

#ifdef COFIG_TVOUT_DBG
#define S5P_TV_BASE_DEBUG 1
#endif

#ifdef S5P_TV_BASE_DEBUG
#define BASEPRINTK(fmt, args...) \
	printk(KERN_INFO "[TVBASE] %s: " fmt, __func__ , ## args)
#else
#define BASEPRINTK(fmt, args...)
#endif

#define TVOUT_CLK_INIT(dev, clk, name) 					\
	do {								\
		clk = clk_get(dev, name);				\
		if (clk == NULL) { 					\
			printk(KERN_ERR					\
			"failed to find %s clock source\n", name);	\
			return -ENOENT;					\
		}							\
		clk_enable(clk)						\
	} while (0);

#define TVOUT_IRQ_INIT(x, ret, dev, num, jump, ftn, m_name) 		\
	do {								\
		x = platform_get_irq(dev, num); 			\
		if (x < 0) {						\
			printk(KERN_ERR					\
			"failed to get %s irq resource\n", m_name);	\
			ret = -ENOENT; 					\
			goto jump;					\
		}							\
		ret = request_irq(x, ftn, IRQF_DISABLED,		\
			dev->name, dev);				\
		if (ret != 0) {						\
			printk(KERN_ERR					\
			"failed to install %s irq (%d)\n", m_name, ret);\
			goto jump;					\
		}							\
	} while (0);

#define TV_CLK_GET_WITH_ERR_CHECK(clk, pdev, clk_name)			\
		do {							\
			clk = clk_get(&pdev->dev, clk_name);		\
			if (IS_ERR(clk)) {				\
				printk(KERN_ERR 			\
				"failed to find clock %s\n", clk_name);	\
				return -ENOENT;				\
			}						\
		} while (0);


static struct mutex	*mutex_for_fo;

struct s5p_tv_status 	s5ptv_status;
struct s5p_tv_vo 	s5ptv_overlay[2];

unsigned long tv_pgd;

/* external function for HDMI */
extern irqreturn_t s5p_hdmi_irq(int irq, void *dev_id);
extern int s5p_hdmi_phy_power(bool on);

/* external function for VP ctrl class */
extern void s5p_vp_ctrl_stop(void);
extern void s5p_vp_ctrl_suspend(void);
extern void s5p_vp_ctrl_resume(void);
extern int s5p_vp_ctrl_constructor(struct platform_device *pdev);
extern void s5p_vp_ctrl_destructor(void);

/* external function for TVOUT ctrl class */
extern int s5p_tvout_ctrl_constructor(struct platform_device *pdev);
extern void s5p_tvout_ctrl_destructor(void);

int s5p_tv_phy_power(bool on)
{
	if (on) {
		/* on */
		clk_enable(s5ptv_status.i2c_phy_clk);

		s5p_hdmi_phy_power(true);

	} else {
		/*
		 * for preventing hdmi hang up when restart
		 * switch to internal clk - SCLK_DAC, SCLK_PIXEL
		 */
		clk_set_parent(s5ptv_status.sclk_mixer,
					s5ptv_status.sclk_dac);
		clk_set_parent(s5ptv_status.sclk_hdmi,
					s5ptv_status.sclk_pixel);

		s5p_hdmi_phy_power(false);

		clk_disable(s5ptv_status.i2c_phy_clk);
	}

	return 0;
}

int s5p_tv_clk_gate(bool on)
{
	/* power control is not implemented yet */
	if (on) {
//		s5pv210_pd_enable("mixer_pd");
		clk_enable(s5ptv_status.mixer_clk);

//		s5pv210_pd_enable("tv_enc_pd");
//		clk_enable(s5ptv_status.tvenc_clk);

//		s5pv210_pd_enable("hdmi_pd");
//		clk_enable(s5ptv_status.hdmi_clk);
	} else {
		clk_disable(s5ptv_status.mixer_clk);
//		s5pv210_pd_disable("mixer_pd");

//		clk_disable(s5ptv_status.tvenc_clk);
//		s5pv210_pd_disable("tv_enc_pd");

//		clk_disable(s5ptv_status.hdmi_clk);
//		s5pv210_pd_disable("hdmi_pd");
	}

	return 0;
}

static int __devinit s5p_tv_clk_get(struct platform_device *pdev,
					struct s5p_tv_status *ctrl)
{
	struct clk	*ext_xtal_clk,
			*mout_vpll_src,
			*fout_vpll,
			*mout_vpll;

//	TV_CLK_GET_WITH_ERR_CHECK(ctrl->tvenc_clk,	pdev, "tvenc");
	TV_CLK_GET_WITH_ERR_CHECK(ctrl->mixer_clk,	pdev, "mixer");
//	TV_CLK_GET_WITH_ERR_CHECK(ctrl->hdmi_clk,	pdev, "hdmi");
	TV_CLK_GET_WITH_ERR_CHECK(ctrl->i2c_phy_clk,	pdev, "i2c-hdmiphy");

	TV_CLK_GET_WITH_ERR_CHECK(ctrl->sclk_dac,	pdev, "sclk_dac");
	TV_CLK_GET_WITH_ERR_CHECK(ctrl->sclk_mixer,	pdev, "sclk_mixer");
	TV_CLK_GET_WITH_ERR_CHECK(ctrl->sclk_hdmi,	pdev, "sclk_hdmi");

	TV_CLK_GET_WITH_ERR_CHECK(ctrl->sclk_pixel,	pdev, "sclk_pixel");
	TV_CLK_GET_WITH_ERR_CHECK(ctrl->sclk_hdmiphy,	pdev, "sclk_hdmiphy");

	TV_CLK_GET_WITH_ERR_CHECK(ext_xtal_clk,		pdev, "ext_xtal");
	TV_CLK_GET_WITH_ERR_CHECK(mout_vpll_src,	pdev, "vpll_src");
	TV_CLK_GET_WITH_ERR_CHECK(fout_vpll,		pdev, "fout_vpll");
	TV_CLK_GET_WITH_ERR_CHECK(mout_vpll,		pdev, "sclk_vpll");

	clk_set_parent(mout_vpll_src, ext_xtal_clk);
	clk_set_parent(mout_vpll, fout_vpll);

	/* sclk_dac's parent is fixed as mout_vpll */
	clk_set_parent(ctrl->sclk_dac, mout_vpll);

	/* It'll be moved in the future */
	clk_enable(ctrl->i2c_phy_clk);

//	clk_enable(ctrl->sclk_dac);
	clk_enable(ctrl->sclk_mixer);
//	clk_enable(ctrl->sclk_hdmi);

	clk_enable(mout_vpll_src);
	clk_enable(fout_vpll);
	clk_enable(mout_vpll);

	clk_put(ext_xtal_clk);
	clk_put(mout_vpll_src);
	clk_put(fout_vpll);
	clk_put(mout_vpll);

	return 0;
}

#ifdef CONFIG_TV_FB
static int s5p_tv_open(struct file *file)
{
	return 0;
}

static int s5p_tv_release(struct file *file)
{
	//s5ptv_status.hdmi.hdcp_en = false;
	return 0;
}

static int s5p_tv_vid_open(struct file *file)
{
	int ret = 0;

#ifdef OLD
	mutex_lock(mutex_for_fo);

	if (s5ptv_status.vp_layer_enable) {
		printk(KERN_ERR "video layer. already used !!\n");
		ret =  -EBUSY;
	}

	mutex_unlock(mutex_for_fo);
#endif

	return ret;
}

static int s5p_tv_vid_release(struct file *file)
{
	s5p_vp_ctrl_stop();

	return 0;
}
#else

static int s5p_tv_v_open(struct file *file)
{
	int ret = 0;

	mutex_lock(mutex_for_fo);

	if (s5ptv_status.tvout_output_enable) {
		BASEPRINTK("tvout drv. already used !!\n");
		ret =  -EBUSY;

		goto drv_used;
	}

	s5p_tv_clk_gate(true);
	s5p_tv_phy_power(true);

	s5p_tv_v4l2_init_param();

	mutex_unlock(mutex_for_fo);

	return 0;

drv_used:
	mutex_unlock(mutex_for_fo);

	return ret;
}

static int s5p_tv_v_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	return 0;
}

static int s5p_tv_v_write(struct file *filp, const char *buf, size_t count,
			loff_t *f_pos)
{
	return 0;
}

static int s5p_tv_v_mmap(struct file *filp, struct vm_area_struct *vma)
{
	return 0;
}

static int s5p_tv_v_release(struct file *filp)
{
	s5p_vp_ctrl_stop();
	s5p_tvout_ctrl_stop();

//	s5ptv_status.hdmi.hdcp_en = false;	//. d: sichoi
//	s5ptv_status.tvout_output_enable = false;

	s5p_tv_clk_gate(false);
	s5p_tv_phy_power(false);

	return 0;
}

static int s5p_tv_vo_open(int layer, struct file *file)
{
	int ret = 0;

	mutex_lock(mutex_for_fo);

	/* check tvout path available!! */
	if (!s5ptv_status.tvout_output_enable) {
		BASEPRINTK("check tvout start !!\n");
		ret =  -EACCES;

		goto resource_busy;
	}

	if (s5ptv_status.grp_layer_enable[layer]) {
		BASEPRINTK("grp %d layer is busy!!\n", layer);
		ret =  -EBUSY;

		goto resource_busy;
	}

	/* set layer info.!! */
	s5ptv_overlay[layer].index = layer;

	/* set file private data.!! */
	file->private_data = &s5ptv_overlay[layer];

	mutex_unlock(mutex_for_fo);

	return 0;

resource_busy:
	mutex_unlock(mutex_for_fo);

	return ret;
}

static int s5p_tv_vo_release(int layer, struct file *filp)
{
	s5p_mixer_ctrl_grp_stop(layer);

	return 0;
}

static int s5p_tv_vo0_open(struct file *file)
{
	s5p_tv_vo_open(0, file);

	return 0;
}

static int s5p_tv_vo0_release(struct file *file)
{
	s5p_tv_vo_release(0, file);

	return 0;
}

static int s5p_tv_vo1_open(struct file *file)
{
	s5p_tv_vo_open(1, file);

	return 0;
}

static int s5p_tv_vo1_release(struct file *file)
{
	s5p_tv_vo_release(1, file);

	return 0;
}
#endif

static struct v4l2_file_operations s5p_tv_fops = {
	.owner		= THIS_MODULE,
	.open		= s5p_tv_open,
	.ioctl		= s5p_tv_ioctl,
	.release	= s5p_tv_release
};

static struct v4l2_file_operations s5p_tv_vid_fops = {
	.owner		= THIS_MODULE,
	.open		= s5p_tv_vid_open,
	.ioctl		= video_ioctl2,
	.release	= s5p_tv_vid_release
};

static void s5p_tv_vdev_release(struct video_device *vdev)
{
	kfree(vdev);
}

struct video_device s5p_tvout[] = {
	[0] = {
		.name		= "S5P TVOUT ctrl",
		.fops		= &s5p_tv_fops,
		.ioctl_ops	= &s5p_tv_v4l2_ops,
		.release	= s5p_tv_vdev_release,
		.minor		= TVOUT_MINOR_TVOUT,
		.tvnorms	= V4L2_STD_ALL_HD,
	},
	[1] = {
		.name		= "S5P TVOUT for Video",
		.fops		= &s5p_tv_vid_fops,
		.ioctl_ops	= &s5p_tv_v4l2_vid_ops,
		.release	= s5p_tv_vdev_release,
		.minor		= TVOUT_MINOR_VID,
		.tvnorms	= V4L2_STD_ALL_HD,
	}
};

void s5p_tv_kobject_uevent(void)
{
	int hpd_state = s5p_hpd_get_state();

	if (hpd_state) {
		BASEPRINTK(KERN_ERR "Event] Send UEvent = %d\n", hpd_state);
		kobject_uevent(&(s5p_tvout[0].dev.kobj), KOBJ_ONLINE);
		kobject_uevent(&(s5p_tvout[1].dev.kobj), KOBJ_ONLINE);
	} else {
		BASEPRINTK(KERN_ERR "Event] Send UEvent = %d\n", hpd_state);
		kobject_uevent(&(s5p_tvout[0].dev.kobj), KOBJ_OFFLINE);
		kobject_uevent(&(s5p_tvout[1].dev.kobj), KOBJ_OFFLINE);
	}
}

static int __devinit s5p_tv_probe(struct platform_device *pdev)
{
	int 	ret = 0;
	int 	i;

	s5ptv_status.dev_fb = &pdev->dev;

#if defined(CONFIG_S5P_SYSMMU_TV) && defined(CONFIG_S5P_VMEM)
	sysmmu_on(SYSMMU_TV);
	printk("sysmmu on\n");
	sysmmu_set_tablebase_pgd(SYSMMU_TV, __pa(swapper_pg_dir));
#endif

	s5p_vp_ctrl_constructor(pdev);
	s5p_tvout_ctrl_constructor(pdev);
	s5p_mixer_ctrl_constructor(pdev);

	s5p_hdcp_init();

	s5p_tv_clk_get(pdev, &s5ptv_status);

	/* interrupt */
	//TVOUT_IRQ_INIT(irq_num, ret, pdev, 0, out, s5p_mixer_irq, "mixer");

	/* v4l2 video device registration */
	for (i = 0; i < ARRAY_SIZE(s5p_tvout); i++) {
		s5ptv_status.video_dev[i] = &s5p_tvout[i];

		if (video_register_device(s5ptv_status.video_dev[i],
				VFL_TYPE_GRABBER, s5p_tvout[i].minor) != 0) {
			dev_err(&pdev->dev,
				"Couldn't register tvout driver.\n");

			return 0;
		}
	}

#ifdef CONFIG_TV_FB
	mutex_init(&s5ptv_status.fb_lock);

	s5ptv_status.tvout_param.disp = TVOUT_720P_60;
	s5ptv_status.tvout_param.out  = TVOUT_HDMI;

#ifndef CONFIG_USER_ALLOC_TVOUT
	s5p_tv_clk_gate(true);
	s5p_tv_phy_power(true);
	s5p_tvout_ctrl_start(
		s5ptv_status.tvout_param.disp, s5ptv_status.tvout_param.out);
#endif

	/* prepare memory */
	if (s5p_tv_fb_alloc_framebuffer(&pdev->dev))
		goto err_alloc;

	if (s5p_tv_fb_register_framebuffer(&pdev->dev))
		goto err_alloc;

#endif

	mutex_for_fo = kmalloc(sizeof(struct mutex), GFP_KERNEL);

	if (mutex_for_fo == NULL) {
		dev_err(&pdev->dev,
			"failed to create mutex handle\n");

		goto out;
	}

	mutex_init(mutex_for_fo);

	return 0;

#ifdef CONFIG_TV_FB
err_alloc:
#endif

out:
	printk(KERN_ERR "not found (%d). \n", ret);

	return ret;
}

static int s5p_tv_remove(struct platform_device *pdev)
{
#if defined(CONFIG_S5P_SYSMMU_TV) && defined(CONFIG_S5P_VMEM)
	sysmmu_off(SYSMMU_TV);
	printk("sysmmu off\n");
#endif
	s5p_vp_ctrl_destructor();
	s5p_tvout_ctrl_destructor();
	s5p_mixer_ctrl_destructor();

//	clk_disable(s5ptv_status.tvenc_clk);
	clk_disable(s5ptv_status.mixer_clk);
//	clk_disable(s5ptv_status.hdmi_clk);
	clk_disable(s5ptv_status.sclk_hdmi);
	clk_disable(s5ptv_status.sclk_mixer);
//	clk_disable(s5ptv_status.sclk_dac);

//	clk_put(s5ptv_status.tvenc_clk);
	clk_put(s5ptv_status.mixer_clk);
//	clk_put(s5ptv_status.hdmi_clk);
	clk_put(s5ptv_status.sclk_hdmi);
	clk_put(s5ptv_status.sclk_mixer);
	clk_put(s5ptv_status.sclk_dac);
	clk_put(s5ptv_status.sclk_pixel);
	clk_put(s5ptv_status.sclk_hdmiphy);

	free_irq(IRQ_MIXER, pdev);
//	free_irq(IRQ_HDMI, pdev);
//	free_irq(IRQ_TVENC, pdev);

	mutex_destroy(mutex_for_fo);

	return 0;
}

#ifdef CONFIG_PM
static int s5p_tv_suspend(struct platform_device *dev, pm_message_t state)
{
#ifndef OLD
	s5p_vp_ctrl_suspend();
	s5p_mixer_ctrl_suspend(); /* mixer suspend will be implemented soon */
	s5p_tvout_ctrl_suspend();
#else
	/* video layer stop */
	if (s5ptv_status.vp_layer_enable) {
		s5p_vlayer_stop();
		s5ptv_status.vp_layer_enable = true;
	}

	/* grp0 layer stop */
	if (s5ptv_status.grp_layer_enable[0]) {
		s5p_mixer_ctrl_grp_stop(MIXER_GPR0_LAYER);
		s5ptv_status.grp_layer_enable[MIXER_GPR0_LAYER] = true;
	}

	/* grp1 layer stop */
	if (s5ptv_status.grp_layer_enable[1]) {
		s5p_mixer_ctrl_grp_stop(MIXER_GPR1_LAYER);
		s5ptv_status.grp_layer_enable[MIXER_GPR0_LAYER] = true;
	}

	/* tv off */
	if (s5ptv_status.tvout_output_enable) {
		s5p_tvout_ctrl_stop();
		s5ptv_status.tvout_output_enable = true;

		/* clk & power off */
		s5p_tv_clk_gate(false);
		s5p_tv_phy_power(false);
	}
#endif
	return 0;
}

static int s5p_tv_resume(struct platform_device *dev)
{
#ifndef OLD
	s5p_vp_ctrl_resume();
	s5p_mixer_ctrl_resume(); /* mixer suspend will be implemented soon */
	s5p_tvout_ctrl_resume();
#else
	/* tv on */
	if (s5ptv_status.tvout_output_enable) {

		/* clk & power on */
		s5p_tv_clk_gate(true);
		s5p_tv_phy_power(true);

		s5p_tvout_ctrl_start(
			s5ptv_status.tvout_param.disp,
			s5ptv_status.tvout_param.out);
	}

	/* video layer start */
	if (s5ptv_status.vp_layer_enable)
		s5p_vlayer_start();

	/* grp0 layer start */
	if (s5ptv_status.grp_layer_enable[0])
		s5p_mixer_ctrl_grp_start(MIXER_GPR0_LAYER);

	/* grp1 layer start */
	if (s5ptv_status.grp_layer_enable[1])
		s5p_mixer_ctrl_grp_start(MIXER_GPR1_LAYER);

#ifdef CONFIG_TV_FB
	if (s5ptv_status.tvout_output_enable) {
		//s5p_mixer_ctrl_display_on(5);
		/* s5ptv_status.fb will be replaced */
		s5p_tv_fb_set_par(s5ptv_status.fb);
	}
#endif
#endif
	return 0;
}
#else
#define s5p_tv_suspend NULL
#define s5p_tv_resume NULL
#endif

static struct platform_driver s5p_tv_driver = {
	.probe		= s5p_tv_probe,
	.remove		= s5p_tv_remove,
	.suspend	= s5p_tv_suspend,
	.resume		= s5p_tv_resume,
	.driver		= {
		.name	= "s5p-tv",
		.owner	= THIS_MODULE,
	},
};

static char banner[] __initdata =
	KERN_INFO "S5P TV Driver v2.0 (c) 2010 Samsung Electronics\n";

static int __init s5p_tv_init(void)
{
	int ret;

	printk(banner);

	ret = platform_driver_register(&s5p_tv_driver);

	if (ret) {
		printk(KERN_ERR "Platform Device Register Failed %d\n", ret);

		return -1;
	}

	return 0;
}

static void __exit s5p_tv_exit(void)
{
	platform_driver_unregister(&s5p_tv_driver);
}

late_initcall(s5p_tv_init);
module_exit(s5p_tv_exit);

MODULE_AUTHOR("SangPil Moon");
MODULE_DESCRIPTION("S5P TV driver");
MODULE_LICENSE("GPL");
