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
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>

#include <asm/mach/arch.h>
#include <asm/mach-types.h>

#include <asm/mach/map.h>

#include <plat/s3c64xx-spi.h>
#include <plat/regs-serial.h>
#include <plat/s5pv310.h>
#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/fb.h>
#include <plat/iic.h>
#include <plat/adc.h>
#include <plat/ts.h>
#include <plat/fimg2d.h>
#include <plat/media.h>
#include <plat/regs-otg.h>
#include <plat/sdhci.h>
#include <plat/gpio-cfg.h>

#include <mach/irqs.h>
#include <mach/map.h>
#include <mach/regs-mem.h>
#include <mach/regs-clock.h>
#include <mach/media.h>
#include <mach/gpio.h>
#include <mach/spi-clocks.h>

#ifdef CONFIG_S5P_SAMSUNG_PMEM
#include <linux/android_pmem.h>
#endif

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

extern void s5p_reserve_bootmem(void);

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
		.start = S5PV310_PA_SROM1,
		.end   = S5PV310_PA_SROM1 + SZ_64K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_EINT(5),
		.end   = IRQ_EINT(5),
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device smdkv310_smsc911x = {
	.name          = "smc911x",
	.id            = -1,
	.num_resources = ARRAY_SIZE(smdkv310_smsc911x_resources),
	.resource      = smdkv310_smsc911x_resources,
};

#ifdef CONFIG_I2C_S3C2410
/* I2C0 */
static struct i2c_board_info i2c_devs0[] __initdata = {
	{ I2C_BOARD_INFO("24c128", 0x50), },	 /* Samsung S524AD0XD1 */
	{ I2C_BOARD_INFO("24c128", 0x52), },	 /* Samsung S524AD0XD1 */
};
#ifdef CONFIG_S3C_DEV_I2C1
/* I2C1 */
static struct i2c_board_info i2c_devs1[] __initdata = {
#if defined(CONFIG_SND_SOC_WM8994) || defined(CONFIG_SND_SOC_WM8994_MODULE)
	{ I2C_BOARD_INFO("wm8994", 0x1a), },
#endif
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

#ifdef CONFIG_S5P_SAMSUNG_PMEM
static struct android_pmem_platform_data pmem_pdata = {
	.name = "s5p-pmem",
	.no_allocator = 0,
	.cached = 0,
	.buffered = 0,
	.start = 0, /* will be set during proving pmem driver */
	.size = 0 /* will be set during proving pmem driver */
};

static struct platform_device s5p_pmem_device = {
	.name = "s5p-pmem",
	.id = 0,
	.dev = { .platform_data = &pmem_pdata },
};
#endif

#ifdef CONFIG_VIDEO_FIMG2D
static struct fimg2d_platdata fimg2d_data __initdata = {
	.hw_ver = 30,
	.parent_clkname = "mout_mpll",
	.clkname = "sclk_fimg2d",
	.gate_clkname = "fimg2d",
	.clkrate = 250 * 1000000,
};
#endif

#ifdef CONFIG_FB_S3C_TL2796

static struct s3c_platform_fb tl2796_data __initdata = {
	.hw_ver = 0x62,
	.clk_name = "sclk_lcd",
	.nr_wins = 5,
	.default_win = CONFIG_FB_S3C_DEFAULT_WINDOW,
	.swap = FB_SWAP_HWORD | FB_SWAP_WORD,
};

#define	LCD_BUS_NUM	1

static struct s3c64xx_spi_csinfo spi1_csi[] = {
	[0] = {
		.line = S5PV310_GPB(5),
		.set_level = gpio_set_value,
	},
};

static struct spi_board_info spi_board_info[] __initdata = {
	{
		.modalias	= "tl2796",
		.max_speed_hz	= 1200000,
		.bus_num	= LCD_BUS_NUM,
		.chip_select	= 0,
		.mode		= SPI_MODE_3,
		.controller_data = &spi1_csi[0],
	},
};
#endif

static struct platform_device *smdkv310_devices[] __initdata = {
#ifdef CONFIG_FB_S3C
	&s3c_device_fb,
#endif
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
#if defined(CONFIG_SND_S3C64XX_SOC_I2S_V4)
	&s5pv310_device_iis0,
#endif

#ifdef CONFIG_VIDEO_MFC51
	&s5p_device_mfc,
#endif

#ifdef CONFIG_S3C_DEV_RTC
	&s3c_device_rtc,
#endif

#ifdef CONFIG_S3C_DEV_HSMMC
	&s3c_device_hsmmc0,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC1
	&s3c_device_hsmmc1,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
	&s3c_device_hsmmc2,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC3
	&s3c_device_hsmmc3,
#endif

#ifdef CONFIG_S3C2410_WATCHDOG
	&s3c_device_wdt,
#endif

#ifdef CONFIG_TOUCHSCREEN_S3C2410
	&s3c_device_ts,
#endif

#ifdef CONFIG_S3C_ADC
	&s3c_device_adc,
#endif

#ifdef CONFIG_VIDEO_TV20
	&s5p_device_tvout,
	&s5p_device_cec,
	&s5p_device_hpd,
#endif

#ifdef CONFIG_USB
	&s3c_device_usb_ehci,
	&s3c_device_usb_ohci,
#endif
#ifdef CONFIG_USB_GADGET
	&s3c_device_usbgadget,
#endif
#ifdef CONFIG_S5P_SAMSUNG_PMEM
	&s5p_pmem_device,
#endif
#ifdef CONFIG_VIDEO_FIMG2D
	&s5p_device_fimg2d,
#endif

#ifdef CONFIG_FB_S3C_TL2796
	&s5pv310_device_spi1,
#endif

#ifdef CONFIG_S5P_SYSMMU
	&s5p_device_sysmmu,
#endif
};

#ifdef CONFIG_S3C_DEV_HSMMC
static struct s3c_sdhci_platdata smdkv310_hsmmc0_pdata __initdata = {
	.wp_gpio		= S5PV310_GPX0(7),
	.has_wp_gpio		= true,
	.cd_type		= S3C_SDHCI_CD_GPIO,
	.ext_cd_gpio		= S5PV310_GPK0(2),
	.ext_cd_gpio_invert	= 1,
	.max_width		= 4,
};
#endif
#ifdef CONFIG_S3C_DEV_HSMMC1
static struct s3c_sdhci_platdata smdkv310_hsmmc1_pdata __initdata = {
	.wp_gpio		= S5PV310_GPX0(7),
	.has_wp_gpio		= true,
	.cd_type		= S3C_SDHCI_CD_GPIO,
	.ext_cd_gpio		= S5PV310_GPK0(2),
	.ext_cd_gpio_invert	= 1,
	.max_width		= 4,
};
#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
static struct s3c_sdhci_platdata smdkv310_hsmmc2_pdata __initdata = {
	.wp_gpio		= S5PV310_GPX0(5),
	.has_wp_gpio		= true,
	.cd_type		= S3C_SDHCI_CD_GPIO,
	.ext_cd_gpio		= S5PV310_GPK2(2),
	.ext_cd_gpio_invert	= 1,
	.max_width		= 4,
};
#endif
#ifdef CONFIG_S3C_DEV_HSMMC3
static struct s3c_sdhci_platdata smdkv310_hsmmc3_pdata __initdata = {
	.wp_gpio		= S5PV310_GPX0(5),
	.has_wp_gpio		= true,
	.cd_type		= S3C_SDHCI_CD_GPIO,
	.ext_cd_gpio		= S5PV310_GPK2(2),
	.ext_cd_gpio_invert	= 1,
	.max_width		= 4,
};
#endif

static void __init sromc_setup(void)
{
	u32 tmp;

	tmp = __raw_readl(S5P_SROM_BW);
	tmp &= ~ (0xffff);
	tmp |= (0x9999);
	__raw_writel(tmp, S5P_SROM_BW);

	__raw_writel(0xff1ffff1,S5P_SROM_BC1);

	tmp = __raw_readl(S5P_VA_GPIO + 0x120);
	tmp &= ~(0xffffff);
	tmp |= (0x221121);
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

	s5p_reserve_bootmem();
}

#ifdef CONFIG_TOUCHSCREEN_S3C2410
static struct s3c2410_ts_mach_info s3c_ts_platform __initdata = {
	.delay			= 10000,
	.presc			= 49,
	.oversampling_shift	= 2,
};
#endif

#ifdef CONFIG_S5P_SAMSUNG_PMEM
static void __init s5p_pmem_set_platdata(void)
{
	pmem_pdata.start = s5p_get_media_memory_bank(S5P_MDEV_PMEM, 1);
	pmem_pdata.size = s5p_get_media_memsize_bank(S5P_MDEV_PMEM, 1);
}
#endif

static void __init smdkv310_machine_init(void)
{
#ifdef CONFIG_FB_S3C_TL2796
	struct clk *sclk = NULL;
	struct clk *prnt = NULL;
	struct device *spi_dev = &s5pv310_device_spi1.dev;
#endif

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

#ifdef CONFIG_FB_S3C
	s3cfb_set_platdata(NULL);
#endif

#ifdef CONFIG_TOUCHSCREEN_S3C2410
	s3c24xx_ts_set_platdata(&s3c_ts_platform);
#endif
#ifdef CONFIG_VIDEO_FIMG2D
	s5p_fimg2d_set_platdata(&fimg2d_data);
#endif
#ifdef CONFIG_S5P_SAMSUNG_PMEM
	s5p_pmem_set_platdata();
#endif

	sromc_setup();

#ifdef CONFIG_S3C_DEV_HSMMC
	s3c_sdhci0_set_platdata(&smdkv310_hsmmc0_pdata);
#endif
#ifdef CONFIG_S3C_DEV_HSMMC1
	s3c_sdhci1_set_platdata(&smdkv310_hsmmc1_pdata);
#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
	s3c_sdhci2_set_platdata(&smdkv310_hsmmc2_pdata);
#endif
#ifdef CONFIG_S3C_DEV_HSMMC3
	s3c_sdhci3_set_platdata(&smdkv310_hsmmc3_pdata);
#endif
	platform_add_devices(smdkv310_devices, ARRAY_SIZE(smdkv310_devices));

#ifdef CONFIG_FB_S3C_TL2796
	sclk = clk_get(spi_dev, "sclk_spi");
	if (IS_ERR(sclk))
		dev_err(spi_dev, "failed to get sclk for SPI-1\n");
	prnt = clk_get(spi_dev, "mout_epll");
	if (IS_ERR(prnt))
		dev_err(spi_dev, "failed to get prnt\n");
	clk_set_parent(sclk, prnt);
	clk_put(prnt);

	if (!gpio_request(S5PV310_GPB(5), "LCD_CS")) {
		gpio_direction_output(S5PV310_GPB(5), 1);
		s3c_gpio_cfgpin(S5PV310_GPB(5), S3C_GPIO_SFN(1));
		s3c_gpio_setpull(S5PV310_GPB(5), S3C_GPIO_PULL_UP);
		s5pv310_spi_set_info(LCD_BUS_NUM, S5PV310_SPI_SRCCLK_SCLK,
			ARRAY_SIZE(spi1_csi));
	}
	spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
	s3cfb_set_platdata(&tl2796_data);
#endif
}

#ifdef CONFIG_USB_SUPPORT
/* Initializes OTG Phy. */
void otg_phy_init(void)
{
	__raw_writel(__raw_readl(S5P_USBOTG_PHY_CONTROL)
		|(0x1<<0), S5P_USBOTG_PHY_CONTROL);
	__raw_writel((__raw_readl(S3C_USBOTG_PHYPWR)
		&~(0x7<<3)&~(0x1<<0)), S3C_USBOTG_PHYPWR);
	__raw_writel((__raw_readl(S3C_USBOTG_PHYCLK)
		&~(0x5<<2))|(0x3<<0), S3C_USBOTG_PHYCLK); /* PLL 24Mhz */
	__raw_writel((__raw_readl(S3C_USBOTG_RSTCON)
		&~(0x3<<1))|(0x1<<0), S3C_USBOTG_RSTCON);
	udelay(10);
	__raw_writel(__raw_readl(S3C_USBOTG_RSTCON)
		&~(0x7<<0), S3C_USBOTG_RSTCON);
	udelay(10);
}
EXPORT_SYMBOL(otg_phy_init);

/* USB Control request data struct must be located here for DMA transfer */
struct usb_ctrlrequest usb_ctrl __attribute__((aligned(8)));
EXPORT_SYMBOL(usb_ctrl);

/* OTG PHY Power Off */
void otg_phy_off(void)
{
	__raw_writel(__raw_readl(S3C_USBOTG_PHYPWR)
		|(0x3<<3), S3C_USBOTG_PHYPWR);
	__raw_writel(__raw_readl(S5P_USBOTG_PHY_CONTROL)
		&~(1<<0), S5P_USBOTG_PHY_CONTROL);
}
EXPORT_SYMBOL(otg_phy_off);

void usb_host_phy_init(void)
{
	if (__raw_readl(S5P_USBHOST_PHY_CONTROL) & (0x1<<0)) {
		printk("[usb_host_phy_init]Already power on PHY\n");
		return;
	}
	
	__raw_writel(__raw_readl(S5P_USBHOST_PHY_CONTROL)
		|(0x1<<0), S5P_USBHOST_PHY_CONTROL);

	/* floating prevention logic : disable */
	__raw_writel((__raw_readl(S3C_USBOTG_PHY1CON) | (0x1<<0)),
		S3C_USBOTG_PHY1CON);

	/* set hsic phy0,1 to normal */
	__raw_writel((__raw_readl(S3C_USBOTG_PHYPWR) & ~(0xf<<9)),
		S3C_USBOTG_PHYPWR);

	/* phy-power on */
	__raw_writel((__raw_readl(S3C_USBOTG_PHYPWR) & ~(0x7<<6)),
		S3C_USBOTG_PHYPWR);

	/* set clock source for PLL (24MHz) */
	__raw_writel((__raw_readl(S3C_USBOTG_PHYCLK) | (0x1<<7) | (0x3<<0)),
		S3C_USBOTG_PHYCLK);

	/* reset all ports of both PHY and Link */
	__raw_writel((__raw_readl(S3C_USBOTG_RSTCON) | (0x1<<6) | (0x7<<3)),
		S3C_USBOTG_RSTCON);
	udelay(10);
	__raw_writel((__raw_readl(S3C_USBOTG_RSTCON) & ~(0x1<<6) & ~(0x7<<3)),
		S3C_USBOTG_RSTCON);
	udelay(50);
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
