/* linux/arch/arm/mach-s5p6450/mach-smdk6450.c
 *
 * Copyright (c) 2009 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/clk.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <mach/hardware.h>
#include <mach/map.h>

#include <asm/irq.h>
#include <asm/mach-types.h>

#include <plat/regs-serial.h>

#include <plat/s5p6450.h>
#include <plat/clock.h>
#include <mach/regs-clock.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/pll.h>
#include <plat/adc.h>
#include <plat/ts.h>
#include <plat/fb.h>
#include <plat/sdhci.h>

extern struct sys_timer s5p6450_timer;

#define S5P6450_UCON_DEFAULT    (S3C2410_UCON_TXILEVEL |	\
				S3C2410_UCON_RXILEVEL |		\
				S3C2410_UCON_TXIRQMODE |	\
				S3C2410_UCON_RXIRQMODE |	\
				S3C2410_UCON_RXFIFO_TOI |	\
				S3C2443_UCON_RXERR_IRQEN)

#define S5P6450_ULCON_DEFAULT	S3C2410_LCON_CS8

#define S5P6450_UFCON_DEFAULT   (S3C2410_UFCON_FIFOMODE |	\
				S3C2440_UFCON_TXTRIG16 |	\
				S3C2410_UFCON_RXTRIG8)

static struct s3c2410_uartcfg smdk6450_uartcfgs[] __initdata = {
	[0] = {
		.hwport	     = 0,
		.flags	     = 0,
		.ucon	     = S5P6450_UCON_DEFAULT,
		.ulcon	     = S5P6450_ULCON_DEFAULT,
		.ufcon	     = S5P6450_UFCON_DEFAULT,
	},
	[1] = {
		.hwport	     = 1,
		.flags	     = 0,
		.ucon	     = S5P6450_UCON_DEFAULT,
		.ulcon	     = S5P6450_ULCON_DEFAULT,
		.ufcon	     = S5P6450_UFCON_DEFAULT,
	},
	[2] = {
		.hwport	     = 2,
		.flags	     = 0,
		.ucon	     = S5P6450_UCON_DEFAULT,
		.ulcon	     = S5P6450_ULCON_DEFAULT,
		.ufcon	     = S5P6450_UFCON_DEFAULT,
	},
	[3] = {
		.hwport	     = 3,
		.flags	     = 0,
		.ucon	     = S5P6450_UCON_DEFAULT,
		.ulcon	     = S5P6450_ULCON_DEFAULT,
		.ufcon	     = S5P6450_UFCON_DEFAULT,
	},
#if CONFIG_SERIAL_SAMSUNG_UARTS > 4
	[4] = {
		.hwport	     = 4,
		.flags	     = 0,
		.ucon	     = S5P6450_UCON_DEFAULT,
		.ulcon	     = S5P6450_ULCON_DEFAULT,
		.ufcon	     = S5P6450_UFCON_DEFAULT,
	},
#endif
#if CONFIG_SERIAL_SAMSUNG_UARTS > 5 
	[5] = {
		.hwport	     = 5,
		.flags	     = 0,
		.ucon	     = S5P6450_UCON_DEFAULT,
		.ulcon	     = S5P6450_ULCON_DEFAULT,
		.ufcon	     = S5P6450_UFCON_DEFAULT,
	},
#endif
};

static struct platform_device *smdk6450_devices[] __initdata = {
#ifdef CONFIG_S3C2410_WATCHDOG
	&s3c_device_wdt,
#endif
	&s3c_device_fb,
#ifdef CONFIG_S3C_DEV_HSMMC
	&s3c_device_hsmmc0,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC1
	&s3c_device_hsmmc1,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
	&s3c_device_hsmmc2,
#endif
};

static void __init smdk6450_map_io(void)
{
	s5p_init_io(NULL, 0, S5P_SYS_ID);
	s3c24xx_init_clocks(19200000);
	//s3c24xx_init_clocks(12000000);

	s3c24xx_init_uarts(smdk6450_uartcfgs, ARRAY_SIZE(smdk6450_uartcfgs));
}

static void __init smdk6450_machine_init(void)
{
	s3cfb_set_platdata(NULL);
#ifdef CONFIG_S3C_DEV_HSMMC
	s5p6450_default_sdhci0();
#endif
#ifdef CONFIG_S3C_DEV_HSMMC1
	s5p6450_default_sdhci1();
#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
	s5p6450_default_sdhci2();
#endif
	platform_add_devices(smdk6450_devices, ARRAY_SIZE(smdk6450_devices));
}

MACHINE_START(SMDK6450, "SMDK6450")
	.phys_io	= S3C_PA_UART & 0xfff00000,
	.io_pg_offst	= (((u32)S3C_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S5P_PA_SDRAM + 0x100,

	.init_irq	= s5p6450_init_irq,
	.map_io		= smdk6450_map_io,
	.init_machine	= smdk6450_machine_init,
	.timer		= &s5p6450_timer,
MACHINE_END
