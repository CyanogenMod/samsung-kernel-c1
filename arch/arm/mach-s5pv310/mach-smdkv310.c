/* linux/arch/arm/mach-s5pv310/mach-smdkv310.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/platform_device.h>
#include <linux/serial_core.h>
#include <linux/smsc911x.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/usb/ch9.h>

#include <asm/mach/arch.h>
#include <asm/mach-types.h>
#include <asm/hardware/cache-l2x0.h>

#include <asm/mach/map.h>

#include <plat/regs-serial.h>
#include <plat/s5pv310.h>
#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/iic.h>
#include <plat/regs-otg.h>

#include <mach/irqs.h>
#include <mach/map.h>
#include <mach/regs-mem.h>
#include <mach/regs-clock.h>

extern struct sys_timer s5pv310_timer;

/* Following are default values for UCON, ULCON and UFCON UART registers */
#define SMDKV310_UCON_DEFAULT	(S3C2410_UCON_TXILEVEL |	\
				 S3C2410_UCON_RXILEVEL |	\
				 S3C2410_UCON_TXIRQMODE |	\
				 S3C2410_UCON_RXIRQMODE |	\
				 S3C2410_UCON_RXFIFO_TOI |	\
				 S3C2443_UCON_RXERR_IRQEN)

#define SMDKV310_ULCON_DEFAULT	S3C2410_LCON_CS8

#define SMDKV310_UFCON_DEFAULT	(S3C2410_UFCON_FIFOMODE |	\
				 S5PV210_UFCON_TXTRIG4 |	\
				 S5PV210_UFCON_RXTRIG4)

extern void s5pv310_reserve_bootmem(void);

static struct s3c2410_uartcfg smdkv310_uartcfgs[] __initdata = {
	[0] = {
		.hwport		= 0,
		.flags		= 0,
		.ucon		= SMDKV310_UCON_DEFAULT,
		.ulcon		= SMDKV310_ULCON_DEFAULT,
		.ufcon		= SMDKV310_UFCON_DEFAULT,
	},
	[1] = {
		.hwport		= 1,
		.flags		= 0,
		.ucon		= SMDKV310_UCON_DEFAULT,
		.ulcon		= SMDKV310_ULCON_DEFAULT,
		.ufcon		= SMDKV310_UFCON_DEFAULT,
	},
	[2] = {
		.hwport		= 2,
		.flags		= 0,
		.ucon		= SMDKV310_UCON_DEFAULT,
		.ulcon		= SMDKV310_ULCON_DEFAULT,
		.ufcon		= SMDKV310_UFCON_DEFAULT,
	},
	[3] = {
		.hwport		= 3,
		.flags		= 0,
		.ucon		= SMDKV310_UCON_DEFAULT,
		.ulcon		= SMDKV310_ULCON_DEFAULT,
		.ufcon		= SMDKV310_UFCON_DEFAULT,
	},
};

static struct resource smdkv310_smsc911x_resources[] = {
	[0] = {
		.start = S5PV310_PA_SROM3,
		.end   = S5PV310_PA_SROM3 + SZ_64K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = EINT_NUMBER(5),
		.end   = EINT_NUMBER(5),
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device smdkv310_smsc911x = {
	.name          = "smc911x",
	.id            = -1,
	.num_resources = ARRAY_SIZE(smdkv310_smsc911x_resources),
	.resource      = &smdkv310_smsc911x_resources,
};

#ifdef CONFIG_I2C_S3C2410
/* I2C0 */
static struct i2c_board_info i2c_devs0[] __initdata = {
};
#ifdef CONFIG_S3C_DEV_I2C1
/* I2C1 */
static struct i2c_board_info i2c_devs1[] __initdata = {
	{ I2C_BOARD_INFO("24c128", 0x57), },	 /* Samsung S524AD0XD1 */
};
#endif
#ifdef CONFIG_S3C_DEV_I2C2
/* I2C2 */
static struct i2c_board_info i2c_devs2[] __initdata = {
};
#endif
#ifdef CONFIG_S3C_DEV_I2C3
/* I2C3 */
static struct i2c_board_info i2c_devs3[] __initdata = {
};
#endif
#ifdef CONFIG_S3C_DEV_I2C4
/* I2C4 */
static struct i2c_board_info i2c_devs4[] __initdata = {
};
#endif
#ifdef CONFIG_S3C_DEV_I2C5
/* I2C5 */
static struct i2c_board_info i2c_devs5[] __initdata = {
};
#endif
#ifdef CONFIG_S3C_DEV_I2C6
/* I2C6 */
static struct i2c_board_info i2c_devs6[] __initdata = {
};
#endif
#ifdef CONFIG_S3C_DEV_I2C7
/* I2C7 */
static struct i2c_board_info i2c_devs7[] __initdata = {
};
#endif
#endif

static struct platform_device *smdkv310_devices[] __initdata = {
	&smdkv310_smsc911x,
#ifdef CONFIG_I2C_S3C2410
	&s3c_device_i2c0,
#if defined(CONFIG_S3C_DEV_I2C1)
	&s3c_device_i2c1,
#endif
#if defined(CONFIG_S3C_DEV_I2C2)
	&s3c_device_i2c2,
#endif
#if defined(CONFIG_S3C_DEV_I2C3)
	&s3c_device_i2c3,
#endif
#if defined(CONFIG_S3C_DEV_I2C4)
	&s3c_device_i2c4,
#endif
#if defined(CONFIG_S3C_DEV_I2C5)
	&s3c_device_i2c5,
#endif
#if defined(CONFIG_S3C_DEV_I2C6)
	&s3c_device_i2c6,
#endif
#if defined(CONFIG_S3C_DEV_I2C7)
	&s3c_device_i2c7,
#endif
#endif

#ifdef CONFIG_VIDEO_MFC51
	&s5p_device_mfc,
#endif

#ifdef CONFIG_S3C_DEV_RTC
	&s3c_device_rtc,
#endif

#ifdef CONFIG_USB
	&s3c_device_usb_ehci,
	&s3c_device_usb_ohci,
#endif
};

static void __init sromc_setup(void)
{
	u32 tmp;

	tmp = __raw_readl(S5P_SROM_BW);
	tmp &= ~ ((0xf) << 12);
	tmp |= (S5P_SROM_BYTE_EN(3) | S5P_SROM_16WIDTH(3));
	__raw_writel(tmp, S5P_SROM_BW);

	tmp = __raw_readl(S5P_VA_GPIO + 0x120);
	tmp &= ~(0xffffff);
	tmp |= 0x222222;
	__raw_writel(tmp, (S5P_VA_GPIO + 0x120));

	__raw_writel(0x22222222, (S5P_VA_GPIO + 0x180));
	__raw_writel(0x22222222, (S5P_VA_GPIO + 0x1a0));
	__raw_writel(0x22222222, (S5P_VA_GPIO + 0x1c0));
	__raw_writel(0x22222222, (S5P_VA_GPIO + 0x1e0));
}

static void __init smdkv310_map_io(void)
{
	s5p_init_io(NULL, 0, S5P_VA_CHIPID);

#ifdef CONFIG_S5PV310_FPGA
	s3c24xx_init_clocks(10000000);
#else
	s3c24xx_init_clocks(24000000);
#endif
	s3c24xx_init_uarts(smdkv310_uartcfgs, ARRAY_SIZE(smdkv310_uartcfgs));

	/* s5pv310_reserve_bootmem(); */
}

static void __init smdkv310_machine_init(void)
{
#ifdef CONFIG_I2C_S3C2410
	s3c_i2c0_set_platdata(NULL);
	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
#ifdef CONFIG_S3C_DEV_I2C1
	s3c_i2c1_set_platdata(NULL);
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));
#endif
#ifdef CONFIG_S3C_DEV_I2C2
	s3c_i2c2_set_platdata(NULL);
	i2c_register_board_info(2, i2c_devs2, ARRAY_SIZE(i2c_devs2));
#endif
#ifdef CONFIG_S3C_DEV_I2C3
	s3c_i2c3_set_platdata(NULL);
	i2c_register_board_info(3, i2c_devs3, ARRAY_SIZE(i2c_devs3));
#endif
#ifdef CONFIG_S3C_DEV_I2C4
	s3c_i2c4_set_platdata(NULL);
	i2c_register_board_info(4, i2c_devs4, ARRAY_SIZE(i2c_devs4));
#endif
#ifdef CONFIG_S3C_DEV_I2C5
	s3c_i2c5_set_platdata(NULL);
	i2c_register_board_info(5, i2c_devs5, ARRAY_SIZE(i2c_devs5));
#endif
#ifdef CONFIG_S3C_DEV_I2C6
	s3c_i2c6_set_platdata(NULL);
	i2c_register_board_info(6, i2c_devs6, ARRAY_SIZE(i2c_devs6));
#endif
#ifdef CONFIG_S3C_DEV_I2C7
	s3c_i2c7_set_platdata(NULL);
	i2c_register_board_info(7, i2c_devs7, ARRAY_SIZE(i2c_devs7));
#endif
#endif

	sromc_setup();
#ifdef CONFIG_CACHE_L2X0
	l2x0_init(S5P_VA_L2CC, 1 << 28, 0xffffffff);
#endif
	platform_add_devices(smdkv310_devices, ARRAY_SIZE(smdkv310_devices));
}

#ifdef CONFIG_USB_SUPPORT
void usb_host_phy_init(void)
{
	struct clk *otg_clk;

	otg_clk = clk_get(NULL, "otg");
	clk_enable(otg_clk);

	if (__raw_readl(S5P_USBHOST_PHY_CONTROL) & (0x1<<0))
		return;

	__raw_writel(__raw_readl(S5P_USBHOST_PHY_CONTROL)
		|(0x1<<0), S5P_USBHOST_PHY_CONTROL);
	__raw_writel((__raw_readl(S3C_USBOTG_PHYPWR)
		&~(0x7<<6)&~(0x3<<9)&~(0x3<<11)), S3C_USBOTG_PHYPWR);
	__raw_writel((__raw_readl(S3C_USBOTG_PHYCLK)
		&~(0x1<<7))|(0x3<<0), S3C_USBOTG_PHYCLK);
	__raw_writel((__raw_readl(S3C_USBOTG_RSTCON))
		|(0x1<<9)|(0x1<<8)|(0x1<<7)|(0x1<<5)|(0x1<<4),
		S3C_USBOTG_RSTCON);
	__raw_writel(__raw_readl(S3C_USBOTG_RSTCON)
		&~(0x1<<9)&~(0x1<<8)&~(0x1<<7)&~(0x1<<5)&~(0x1<<4),
		S3C_USBOTG_RSTCON);
}
EXPORT_SYMBOL(usb_host_phy_init);

void usb_host_phy_off(void)
{
	__raw_writel(__raw_readl(S3C_USBOTG_PHYPWR)
		|(0x1<<7)|(0x1<<6), S3C_USBOTG_PHYPWR);
	__raw_writel(__raw_readl(S5P_USBHOST_PHY_CONTROL)
		&~(1<<0), S5P_USBHOST_PHY_CONTROL);
}
EXPORT_SYMBOL(usb_host_phy_off);
#endif

MACHINE_START(SMDKV310, "SMDKV310")
	/* Maintainer: Kukjin Kim <kgene.kim@samsung.com> */
	/* Maintainer: Changhwan Youn <chaos.youn@samsung.com> */
	.phys_io	= S3C_PA_UART & 0xfff00000,
	.io_pg_offst	= (((u32)S3C_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S5P_PA_SDRAM + 0x100,
	.init_irq	= s5pv310_init_irq,
	.map_io		= smdkv310_map_io,
	.init_machine	= smdkv310_machine_init,
	.timer		= &s5pv310_timer,
MACHINE_END
