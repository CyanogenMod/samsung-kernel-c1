/*
 * Touch driver
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/fs.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/wakelock.h>
#include <linux/earlysuspend.h>
#include <linux/suspend.h>
#endif

#include <asm/system.h>
#include <asm/gpio.h>
#include <plat/gpio-cfg.h>

#include <mach/irqs.h>
#include <mach/regs-gpio.h>

#include "s5pc210_ts.h"
#include "s5pc210_ts_gpio_i2c.h"
#include "s5pc210_ts_sysfs.h"

#define CONFIG_DEBUG_S5PV310_TS_MSG

s5pv310_ts_t	s5pv310_ts;
static void s5pv310_ts_timer_start(void);
static void s5pv310_ts_timer_irq(unsigned long arg);
static void s5pv310_ts_process_data(touch_process_data_t *ts_data);
static void s5pv310_ts_get_data(void);

irqreturn_t s5pv310_ts_irq(int irq, void *dev_id);

static int s5pv310_ts_open(struct input_dev *dev);
static void s5pv310_ts_close(struct input_dev *dev);

static void s5pv310_ts_release_device(struct device *dev);

#ifdef CONFIG_HAS_EARLYSUSPEND
static void s5pv310_ts_early_suspend(struct early_suspend *h);
static void s5pv310_ts_late_resume(struct early_suspend *h);
#else
static int s5pv310_ts_resume(struct platform_device *dev);
static int s5pv310_ts_suspend(struct platform_device *dev, pm_message_t state);
#endif

/* static void s5pv310_ts_reset(void); */
static void s5pv310_ts_config(unsigned char state);

static int __devinit s5pv310_ts_probe (struct platform_device *pdev);
static int __devexit    s5pv310_ts_remove (struct platform_device *pdev);

static int __init s5pv310_ts_init (void);
static void __exit s5pv310_ts_exit (void);

static struct platform_driver s5pv310_ts_platform_device_driver = {
	.probe          = s5pv310_ts_probe,
	.remove         = s5pv310_ts_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend        = s5pv310_ts_suspend,
	.resume         = s5pv310_ts_resume,
#endif
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= S5PV310_TS_DEVICE_NAME,
	},
};
static struct platform_device s5pv310_ts_platform_device = {
	.name           = S5PV310_TS_DEVICE_NAME,
	.id             = -1,
	.num_resources  = 0,
	.dev		= {
		.release	= s5pv310_ts_release_device,
	},
};

static void s5pv310_ts_timer_irq(unsigned long arg)
{
	/*  Acc data read */
	write_seqlock(&s5pv310_ts.lock);

	s5pv310_ts_get_data();

	//s5pv310_ts_timer_start();

	write_sequnlock(&s5pv310_ts.lock);
}

static void s5pv310_ts_timer_start(void)
{
	init_timer(&s5pv310_ts.penup_timer);
	s5pv310_ts.penup_timer.data = (unsigned long)&s5pv310_ts.penup_timer;
	s5pv310_ts.penup_timer.function = s5pv310_ts_timer_irq;

	switch(s5pv310_ts.sampling_rate){
	default	:
		s5pv310_ts.sampling_rate = 0;
		s5pv310_ts.penup_timer.expires = jiffies + PERIOD_10MS;
		break;
	case 1:
		s5pv310_ts.penup_timer.expires = jiffies + PERIOD_20MS;
		break;
	case 2:
		s5pv310_ts.penup_timer.expires = jiffies + PERIOD_50MS;
		break;
	}

	add_timer(&s5pv310_ts.penup_timer);
}

static void s5pv310_ts_process_data(touch_process_data_t *ts_data)
{
	int i = 0;
	/* read address setup */
	s5pv310_ts_write(0x00, NULL, 0x00);

	/* Acc data read */
	write_seqlock(&s5pv310_ts.lock);
	s5pv310_ts_read(&s5pv310_ts.rd[0], 10);

	write_sequnlock(&s5pv310_ts.lock);

	ts_data->finger_cnt = s5pv310_ts.rd[0] & 0x03;

	if((ts_data->x1 = ((s5pv310_ts.rd[3] << 8) | s5pv310_ts.rd[2]))){
		ts_data->x1 = (ts_data->x1 * 133) / 100;
#ifdef CONFIG_S5PV310_TS_FLIP
#else
		/* flip X & resize */
		ts_data->x1 = TS_ABS_MAX_X - ts_data->x1;
#endif
	}

	/* resize */
	if((ts_data->y1 = ((s5pv310_ts.rd[5] << 8) | s5pv310_ts.rd[4]))){
		ts_data->y1 = (ts_data->y1 * 128 ) / 100;
#ifdef CONFIG_S5PV310_TS_FLIP
		/* flip Y & resize */
		ts_data->y1 = TS_ABS_MAX_Y - ts_data->y1;
#else
#endif
	}
	if(ts_data->finger_cnt > 1) {
		/* flip X & resize */
		if((ts_data->x2 = ((s5pv310_ts.rd[7] << 8) | s5pv310_ts.rd[6]))) {
			ts_data->x2 = (ts_data->x2 * 133) / 100;
#ifdef CONFIG_S5PV310_TS_FLIP
#else
			ts_data->x2 = TS_ABS_MAX_X - ts_data->x2;
#endif
		}
		/* resize */
		if((ts_data->y2 = ((s5pv310_ts.rd[9] << 8) | s5pv310_ts.rd[8]))){

			ts_data->y2 = (ts_data->y2 * 128 ) / 100;
#ifdef CONFIG_S5PV310_TS_FLIP
			/* flip Y & resize */
			ts_data->y2 = TS_ABS_MAX_Y - ts_data->y2;
#else
#endif

		}
	}
}

static void s5pv310_ts_get_data(void)
{
	touch_process_data_t	ts_data;

	s5pv310_ts_process_data(&ts_data);
#if defined(CONFIG_DEBUG_S5PV310_TS_MSG)
	printk("x1: %d, y1: %d\n",ts_data.x1,ts_data.y1);
	printk("x2: %d, y2: %d\n",ts_data.x2,ts_data.y2);
#endif

	if(ts_data.finger_cnt > 0 && ts_data.finger_cnt < 3)	{
		s5pv310_ts.x 	= ts_data.x1;
		s5pv310_ts.y	= ts_data.y1;
		/* press */
		input_report_abs(s5pv310_ts.driver, ABS_MT_TOUCH_MAJOR, 200);
		input_report_abs(s5pv310_ts.driver, ABS_MT_WIDTH_MAJOR, 10);
		input_report_abs(s5pv310_ts.driver, ABS_MT_POSITION_X, s5pv310_ts.x);
		input_report_abs(s5pv310_ts.driver, ABS_MT_POSITION_Y, s5pv310_ts.y);
		input_mt_sync(s5pv310_ts.driver);

		if(ts_data.finger_cnt > 1)	{
			s5pv310_ts.x 	= ts_data.x2;
			s5pv310_ts.y	= ts_data.y2;
			 /* press */
			input_report_abs(s5pv310_ts.driver, ABS_MT_TOUCH_MAJOR, 200);  			input_report_abs(s5pv310_ts.driver, ABS_MT_WIDTH_MAJOR, 10);
			input_report_abs(s5pv310_ts.driver, ABS_MT_POSITION_X, s5pv310_ts.x);
			input_report_abs(s5pv310_ts.driver, ABS_MT_POSITION_Y, s5pv310_ts.y);
			input_mt_sync(s5pv310_ts.driver);
		}

		input_sync(s5pv310_ts.driver);
	}
	else	{
		 /* press */
		input_report_abs(s5pv310_ts.driver, ABS_MT_TOUCH_MAJOR, 0);
		input_report_abs(s5pv310_ts.driver, ABS_MT_WIDTH_MAJOR, 10);
		input_report_abs(s5pv310_ts.driver, ABS_MT_POSITION_X, s5pv310_ts.x);
		input_report_abs(s5pv310_ts.driver, ABS_MT_POSITION_Y, s5pv310_ts.y);
		input_mt_sync(s5pv310_ts.driver);
		input_sync(s5pv310_ts.driver);
	}
}

irqreturn_t s5pv310_ts_irq(int irq, void *dev_id)
{
	unsigned long flags;

	local_irq_save(flags);
	local_irq_disable();
	s5pv310_ts_get_data();
	local_irq_restore(flags);
	return	IRQ_HANDLED;
}

static int s5pv310_ts_open (struct input_dev *dev)
{
#if defined(CONFIG_DEBUG_S5PV310_TS_MSG)
	printk("%s\n", __FUNCTION__);
#endif

	return	0;
}

static void s5pv310_ts_close (struct input_dev *dev)
{
#if defined(CONFIG_DEBUG_S5PV310_TS_MSG)
	printk("%s\n", __FUNCTION__);
#endif
}

static void s5pv310_ts_release_device (struct device *dev)
{
#if defined(CONFIG_DEBUG_S5PV310_TS_MSG)
	printk("%s\n", __FUNCTION__);
#endif
}

#ifdef	CONFIG_HAS_EARLYSUSPEND
static void		s5pv310_ts_late_resume	(struct early_suspend *h)
#else
static 	int		s5pv310_ts_resume			(struct platform_device *dev)
#endif
{
	s5pv310_ts_config(TOUCH_STATE_RESUME);

	/* interrupt enable */
	enable_irq(S5PV310_TS_IRQ);

#ifndef	CONFIG_HAS_EARLYSUSPEND
	return	0;
#endif
}
#ifdef	CONFIG_HAS_EARLYSUSPEND
static void s5pv310_ts_early_suspend (struct early_suspend *h)
#else
static 	int s5pv310_ts_suspend (struct platform_device *dev, pm_message_t state)
#endif
{
	unsigned char	wdata;

	wdata = 0x00;
	s5pv310_ts_write(MODULE_POWERMODE, &wdata, 1);
	mdelay(10);

	/* INT_mode : disable interrupt */
	wdata = 0x00;
	s5pv310_ts_write(MODULE_INTMODE, &wdata, 1);
	mdelay(10);

	/* Touchscreen enter freeze mode : */
	wdata = 0x03;
	s5pv310_ts_write(MODULE_POWERMODE, &wdata, 1);
	mdelay(10);

	/* interrupt disable */
	disable_irq(S5PV310_TS_IRQ);

#ifndef	CONFIG_HAS_EARLYSUSPEND
	return	0;
#endif
}

/*  static void s5pv310_ts_reset(void)
{
	if(gpio_request(TS_RESET_OUT,"TS_RESET_OUT"))	{
		printk("%s : request port error!\n", __FUNCTION__);
	}

	gpio_direction_output(TS_RESET_OUT, 1);
	s3c_gpio_setpull(TS_RESET_OUT, S3C_GPIO_PULL_DOWN);

	gpio_set_value(TS_RESET_OUT, 0);
	mdelay(10);
	gpio_set_value(TS_RESET_OUT, 1);
	mdelay(10);
	gpio_set_value(TS_RESET_OUT, 0);
	mdelay(10);

	gpio_free(TS_RESET_OUT);
}*/

static void s5pv310_ts_config(unsigned char state)
{
	unsigned char	wdata;

	/* s5pv310_ts_reset(); */
	s5pv310_ts_port_init();
	mdelay(500);

	/* Touchscreen Active mode */
	wdata = 0x00;
	s5pv310_ts_write(MODULE_POWERMODE, &wdata, 1);
	mdelay(10);

	if(state == TOUCH_STATE_BOOT)	{

		/* INT_mode : disable interrupt */
		wdata = 0x00;
		s5pv310_ts_write(MODULE_INTMODE, &wdata, 1);

		if(!request_irq(S5PV310_TS_IRQ, s5pv310_ts_irq, IRQF_DISABLED, "s5pc210-Touch IRQ", (void *)&s5pv310_ts))
			printk("HKC1XX TOUCH request_irq = %d\r\n" , S5PV310_TS_IRQ);
		else
			printk("HKC1XX TOUCH request_irq = %d error!! \r\n", S5PV310_TS_IRQ);

		if (gpio_is_valid(TS_ATTB))
		{

			if(gpio_request(TS_ATTB, "TS_ATTB")) {
				printk(KERN_ERR "failed to request GPH1 for TS_ATTB..\n");
			}
			/*		gpio_direction_input(TS_ATTB);
					s3c_gpio_setpull(TS_ATTB, S3C_GPIO_PULL_NONE);
					gpio_free(TS_ATTB);
			*/
		}


		s3c_gpio_cfgpin(TS_ATTB, (0xf << 20));
		s3c_gpio_cfgpin(EINT_43CON, ~((0x7) << 20));
		s3c_gpio_setpull(TS_ATTB, S3C_GPIO_PULL_DOWN);
//		s3c_gpio_setpull(TS_ATTB, S3C_GPIO_PULL_NONE);

		set_irq_type(S5PV310_TS_IRQ, IRQ_TYPE_EDGE_RISING);
//		set_irq_type(S5PV310_TS_IRQ, IRQ_TYPE_LEVEL_LOW);

		/* seqlock init */
		seqlock_init(&s5pv310_ts.lock);

		 s5pv310_ts.seq = 0;

	}
	else {
		/* INT_mode : disable interrupt, low-active, finger moving */
		wdata = 0x01;
		s5pv310_ts_write(MODULE_INTMODE, &wdata, 1);
		mdelay(10);
		/* INT_mode : enable interrupt, low-active, finger moving */
		wdata = 0x09;
//		wdata = 0x08;
		s5pv310_ts_write(MODULE_INTMODE, &wdata, 1);
		mdelay(10);
	}
}

static int __devinit s5pv310_ts_probe (struct platform_device *pdev)
{
	int rc;
	unsigned char	wdata;

	/* struct init */
	memset(&s5pv310_ts, 0x00, sizeof(s5pv310_ts_t));

	/* create sys_fs */
	if((rc = s5pv310_ts_sysfs_create(pdev)))	{
		printk("%s : sysfs_create_group fail!!\n", __FUNCTION__);
		return	rc;
	}

	s5pv310_ts.driver = input_allocate_device();

	if(!(s5pv310_ts.driver))	{
		printk("ERROR! : %s cdev_alloc() error!!! no memory!!\n", __FUNCTION__);
		s5pv310_ts_sysfs_remove(pdev);
		return -ENOMEM;
	}

	s5pv310_ts.driver->name 	= S5PV310_TS_DEVICE_NAME;
	s5pv310_ts.driver->phys 	= "s5pv310_ts/input1";
	s5pv310_ts.driver->open 	= s5pv310_ts_open;
	s5pv310_ts.driver->close	= s5pv310_ts_close;

	s5pv310_ts.driver->id.bustype	= BUS_HOST;
	s5pv310_ts.driver->id.vendor 	= 0x16B4;
	s5pv310_ts.driver->id.product	= 0x0702;
	s5pv310_ts.driver->id.version	= 0x0001;

	set_bit(EV_ABS, s5pv310_ts.driver->evbit);

	/* multi touch */
	input_set_abs_params(s5pv310_ts.driver, ABS_MT_POSITION_X, TS_ABS_MIN_X, TS_ABS_MAX_X,	0, 0);
	input_set_abs_params(s5pv310_ts.driver, ABS_MT_POSITION_Y, TS_ABS_MIN_Y, TS_ABS_MAX_Y,	0, 0);
	input_set_abs_params(s5pv310_ts.driver, ABS_MT_TOUCH_MAJOR, 0, 255, 2, 0);
	input_set_abs_params(s5pv310_ts.driver, ABS_MT_WIDTH_MAJOR, 0,  15, 2, 0);

	if(input_register_device(s5pv310_ts.driver))	{
		printk("S5PC210 TOUCH input register device fail!!\n");

		s5pv310_ts_sysfs_remove(pdev);
		input_free_device(s5pv310_ts.driver);		return	-ENODEV;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	s5pv310_ts.power.suspend 	= s5pv310_ts_early_suspend;
	s5pv310_ts.power.resume 	= s5pv310_ts_late_resume;
	s5pv310_ts.power.level 	= EARLY_SUSPEND_LEVEL_DISABLE_FB-1;
	/* if, is in USER_SLEEP status and no active auto expiring wake lock */
	/* if (has_wake_lock(WAKE_LOCK_SUSPEND) == 0 && get_suspend_state() == PM_SUSPEND_ON) */
	register_early_suspend(&s5pv310_ts.power);
#endif

	s5pv310_ts_config(TOUCH_STATE_BOOT);


	/*  INT_mode : disable interrupt */
	wdata = 0x00;
	s5pv310_ts_write(MODULE_INTMODE, &wdata, 1);

	/*  touch calibration */
	wdata =0x03;
	/*  set mode */
	s5pv310_ts_write(MODULE_CALIBRATION, &wdata, 1);
	mdelay(500);

	/*  INT_mode : enable interrupt, low-active, periodically*/
	wdata = 0x09;
//	wdata = 0x08;
	s5pv310_ts_write(MODULE_INTMODE, &wdata, 1);

	printk("SMDKC210(MT) Multi-Touch driver initialized!! Ver 1.0\n");

	return	0;
}

static int __devexit s5pv310_ts_remove (struct platform_device *pdev)
{
#if defined(CONFIG_DEBUG_S5PV310_TS_MSG)
	printk("%s\n", __FUNCTION__);
#endif

	free_irq(S5PV310_TS_IRQ, (void *)&s5pv310_ts);

	s5pv310_ts_sysfs_remove(pdev);

	input_unregister_device(s5pv310_ts.driver);

	return  0;
}

static int __init s5pv310_ts_init(void)
{
	int ret = platform_driver_register(&s5pv310_ts_platform_device_driver);

	if(!ret)        {
		ret = platform_device_register(&s5pv310_ts_platform_device);

#if	defined(CONFIG_DEBUG_S5PV310_TS_MSG)
		printk("platform_driver_register %d \n", ret);
#endif

		if(ret)	platform_driver_unregister(&s5pv310_ts_platform_device_driver);
	}
	return ret;
}

static void __exit s5pv310_ts_exit (void)
{
#if defined(CONFIG_DEBUG_S5PV310_TS_MSG)
	printk("%s\n",__FUNCTION__);
#endif

	platform_device_unregister(&s5pv310_ts_platform_device);
	platform_driver_unregister(&s5pv310_ts_platform_device_driver);
}
module_init(s5pv310_ts_init);
module_exit(s5pv310_ts_exit);

MODULE_AUTHOR("HardKernel");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("10.1\" WXGA Touch panel for S5PC210 Board");
