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

#include <asm/mach/arch.h>
#include <asm/mach-types.h>
#include <asm/hardware/cache-l2x0.h>

#include <asm/mach/map.h>

#include <plat/regs-serial.h>
#include <plat/s5pv310.h>
#include <plat/cpu.h>

#include <mach/irqs.h>
#include <mach/map.h>
#include <mach/regs-mem.h>

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

static struct platform_device *smdkv310_devices[] __initdata = {
	&smdkv310_smsc911x,
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
}

static void __init smdkv310_machine_init(void)
{
	sromc_setup();
#ifdef CONFIG_CACHE_L2X0
	l2x0_init(S5P_VA_L2CC, 1 << 28, 0xffffffff);
#endif
	platform_add_devices(smdkv310_devices, ARRAY_SIZE(smdkv310_devices));
}

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
