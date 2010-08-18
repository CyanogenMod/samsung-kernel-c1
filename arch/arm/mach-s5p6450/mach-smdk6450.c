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
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/usb/ch9.h>
#include <linux/spi/spi.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <mach/spi-clocks.h>
#include <mach/hardware.h>
#include <mach/map.h>
#include <mach/gpio.h>

#include <asm/irq.h>
#include <asm/mach-types.h>

#include <plat/s3c64xx-spi.h>
#include <plat/regs-serial.h>

#include <plat/s5p6450.h>
#include <plat/regs-otg.h>
#include <plat/clock.h>
#include <mach/regs-clock.h>
#include <mach/regs-sys.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/iic.h>
#include <plat/pll.h>
#include <plat/adc.h>
#include <plat/ts.h>
#include <plat/fb.h>
#include <plat/mshci.h>
#include <plat/sdhci.h>
#include <plat/gpio-cfg.h>
#include <plat/pm.h>

#include <linux/mfd/s5m8751/core.h>

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

#if defined(CONFIG_MFD_S5M8751)
/* SYS, EXT */
static struct regulator_init_data s5m8751_ldo1_data = {
	.constraints = {
		.name = "PVDD_SYS/PVDD_EXT",
		.min_uV = 3300000,
		.max_uV = 3300000,
		.min_uA = 0,
		.max_uA = 300000,
		.always_on = 1,
		.apply_uV = 1,
		.state_mem = {
			.uV = 0,
			.mode = REGULATOR_MODE_STANDBY,
			.enabled = 1,
		}
	},
};

/* LCD */
static struct regulator_init_data s5m8751_ldo2_data = {
	.constraints = {
		.name = "PVDD_LCD",
		.min_uV = 3300000,
		.max_uV = 3300000,
		.min_uA = 0,
		.max_uA = 150000,
		.always_on = 1,
		.apply_uV = 1,
		.state_mem = {
			.uV = 0,
			.mode = REGULATOR_MODE_STANDBY,
			.enabled = 0,
		}
	},
};

/* ALIVE */
static struct regulator_init_data s5m8751_ldo3_data = {
	.constraints = {
		.name = "PVDD_ALIVE",
		.min_uV = 1100000,
		.max_uV = 1100000,
		.min_uA = 0,
		.max_uA = 150000,
		.always_on = 1,
		.apply_uV = 1,
		.state_mem = {
			.uV = 0,
			.mode = REGULATOR_MODE_STANDBY,
			.enabled = 1,
		}
	},
};

/* PLL */
static struct regulator_init_data s5m8751_ldo4_data = {
	.constraints = {
		.name = "PVDD_PLL",
		.min_uV = 1100000,
		.max_uV = 1100000,
		.min_uA = 0,
		.max_uA = 300000,
		.always_on = 1,
		.apply_uV = 1,
		.state_mem = {
			.uV = 0,
			.mode = REGULATOR_MODE_STANDBY,
			.enabled = 0,
		}
	},
};

/* MEM/SS */
static struct regulator_init_data s5m8751_ldo5_data = {
	.constraints = {
		.name = "PVDD_MEM/SS",
		.min_uV = 1800000,
		.max_uV = 1800000,
		.min_uA = 0,
		.max_uA = 300000,
		.always_on = 1,
		.apply_uV = 1,
		.state_mem = {
			.uV = 0,
			.mode = REGULATOR_MODE_STANDBY,
			.enabled = 1,
		}
	},
};

/* RTC */
static struct regulator_init_data s5m8751_ldo6_data = {
	.constraints = {
		.name = "PVDD_RTC",
		.min_uV = 3300000,
		.max_uV = 3300000,
		.min_uA = 0,
		.max_uA = 150000,
		.always_on = 1,
		.apply_uV = 1,
		.state_mem = {
			.uV = 0,
			.mode = REGULATOR_MODE_STANDBY,
			.enabled = 0,
		}
	},
};

/* GPS */
static struct regulator_init_data s5m8751_buck1_1_data = {
	.constraints = {
		.name = "PVDD_GPS",
		.min_uV = 1800000,
		.max_uV = 1800000,
		.min_uA = 0,
		.max_uA = 400000,
		.always_on = 1,
		.apply_uV = 1,
		.state_mem = {
			.uV = 0,
			.mode = REGULATOR_MODE_STANDBY,
			.enabled = 0,
		}
	},
};

static struct regulator_init_data s5m8751_buck1_2_data = {
	.constraints = {
		.name = "PVDD_GPS",
		.min_uV = 3300000,
		.max_uV = 3300000,
		.min_uA = 0,
		.max_uA = 400000,
		.always_on = 1,
		.apply_uV = 1,
		.state_mem = {
			.uV = 0,
			.mode = REGULATOR_MODE_STANDBY,
			.enabled = 0,
		}
	},
};

static struct regulator_consumer_supply buck2_1_consumers[] = {
	{
		.supply		= "vddarm1",
	},
};

static struct regulator_consumer_supply buck2_2_consumers[] = {
	{
		.supply		= "vddarm2",
	},
};

/* ARM/INT */
static struct regulator_init_data s5m8751_buck2_1_data = {
	.constraints = {
		.name = "PVDD_ARM/PVDD_INT",
		.min_uV = 950000,
		.max_uV = 1300000,
		.min_uA = 0,
		.max_uA = 1000000,
		.always_on = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
		.state_mem = {
			.uV = 0,
			.mode = REGULATOR_MODE_STANDBY,
			.enabled = 0,
		}
	},
	.num_consumer_supplies	= ARRAY_SIZE(buck2_1_consumers),
	.consumer_supplies	= buck2_1_consumers,
};

static struct regulator_init_data s5m8751_buck2_2_data = {
	.constraints = {
		.name = "PVDD_ARM/PVDD_INT",
		.min_uV = 950000,
		.max_uV = 1300000,
		.min_uA = 0,
		.max_uA = 1000000,
		.always_on = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
		.state_mem = {
			.uV = 0,
			.mode = REGULATOR_MODE_STANDBY,
			.enabled = 0,
		}
	},
	.num_consumer_supplies	= ARRAY_SIZE(buck2_2_consumers),
	.consumer_supplies	= buck2_2_consumers,
};

int s5m8751_gpio_irq_init(void)
{
	unsigned int gpio_irq;

	gpio_irq = S5P6450_GPN(7);
	s3c_gpio_cfgpin(gpio_irq, S3C_GPIO_SFN(2));
	s3c_gpio_setpull(gpio_irq, S3C_GPIO_PULL_UP);

	return gpio_irq;
}

static int smdk6450_s5m8751_init(struct s5m8751 *s5m8751)
{
	s5m8751_gpio_irq_init();

	s5m8751_register_regulator(s5m8751, S5M8751_LDO1, &s5m8751_ldo1_data);
	s5m8751_register_regulator(s5m8751, S5M8751_LDO2, &s5m8751_ldo2_data);
	s5m8751_register_regulator(s5m8751, S5M8751_LDO3, &s5m8751_ldo3_data);
	s5m8751_register_regulator(s5m8751, S5M8751_LDO4, &s5m8751_ldo4_data);
	s5m8751_register_regulator(s5m8751, S5M8751_LDO5, &s5m8751_ldo5_data);
	s5m8751_register_regulator(s5m8751, S5M8751_LDO6, &s5m8751_ldo6_data);
	s5m8751_register_regulator(s5m8751, S5M8751_BUCK1_1,
							 &s5m8751_buck1_1_data);
	s5m8751_register_regulator(s5m8751, S5M8751_BUCK1_2,
							 &s5m8751_buck1_2_data);
	s5m8751_register_regulator(s5m8751, S5M8751_BUCK2_1,
							 &s5m8751_buck2_1_data);
	s5m8751_register_regulator(s5m8751, S5M8751_BUCK2_2,
							 &s5m8751_buck2_2_data);

	return 0;
}

static struct s5m8751_platform_data __initdata smdk6450_s5m8751_pdata = {
	.init = smdk6450_s5m8751_init,
};
#endif

static struct i2c_board_info i2c_devs0[] __initdata = {
	{ I2C_BOARD_INFO("24c08", 0x50), },/* Samsung KS24C080C EEPROM */
#ifdef CONFIG_SND_SOC_WM8580
	{ I2C_BOARD_INFO("wm8580", 0x1b), },
#endif
};

static struct i2c_board_info i2c_devs1[] __initdata = {
	{ I2C_BOARD_INFO("24c128", 0x57), },/* Samsung S524AD0XD1 EEPROM */
#if defined(CONFIG_MFD_S5M8751)
/* S5M8751_SLAVE_ADDRESS >> 1 = 0xD0 >> 1 = 0x68 */
	{ I2C_BOARD_INFO("s5m8751", (0xD0 >> 1)),
	.platform_data = &smdk6450_s5m8751_pdata,
	.irq = S5P_IRQ(7),
	},
#endif
};

#ifdef CONFIG_S3C64XX_DEV_SPI

#define SMDK_MMCSPI_CS 0

static struct s3c64xx_spi_csinfo smdk_spi0_csi[] = {
	[SMDK_MMCSPI_CS] = {
		.line = S5P6450_GPC(3),
		.set_level = gpio_set_value,
		.fb_delay = 0x0,
	},
};

static struct s3c64xx_spi_csinfo smdk_spi1_csi[] = {
	[SMDK_MMCSPI_CS] = {
		.line = S5P6450_GPC(7),
		.set_level = gpio_set_value,
		.fb_delay = 0x0,
	},
};

static struct spi_board_info s3c_spi_devs[] __initdata = {
	{
		.modalias	 = "spidev", /* MMC SPI */
		.mode		 = SPI_MODE_0,	/* CPOL=0, CPHA=0 */
		.max_speed_hz    = 10000000,
		/* Connected to SPI-0 as 1st Slave */
		.bus_num	 = 0,
		.chip_select	 = 0,
		.controller_data = &smdk_spi0_csi[SMDK_MMCSPI_CS],
	},
	{
		.modalias	 = "spidev", /* MMC SPI */
		.mode		 = SPI_MODE_0,	/* CPOL=0, CPHA=0 */
		.max_speed_hz    = 10000000,
		/* Connected to SPI-1 as 1st Slave */
		.bus_num	 = 1,
		.chip_select	 = 0,
		.controller_data = &smdk_spi1_csi[SMDK_MMCSPI_CS],
	},
};

#endif

static struct platform_device *smdk6450_devices[] __initdata = {
#ifdef CONFIG_S3C2410_WATCHDOG
	&s3c_device_wdt,
#endif
#ifdef CONFIG_TOUCHSCREEN_S3C2410
	&s3c_device_ts,
#endif
#ifdef CONFIG_S3C_ADC
	&s3c_device_adc,
#endif
#ifdef CONFIG_FB_S3C_V2
	&s3c_device_fb,
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
#ifdef CONFIG_S3C_DEV_MSHC
	&s3c_device_mshci,
#endif
	&s3c_device_i2c0,
	&s3c_device_i2c1,
#ifdef CONFIG_S3C_DEV_RTC
	&s3c_device_rtc,
#endif
#ifdef CONFIG_USB_GADGET
	&s3c_device_usbgadget,
#endif
#ifdef CONFIG_USB
	&s3c_device_usb_ehci,
	&s3c_device_usb_ohci,
#endif
#ifdef CONFIG_SND_S3C64XX_SOC_I2S_V4
	&s5p6450_device_iis0,
#endif
#ifdef CONFIG_S3C_DEV_GIB
	&s3c_device_gib, 
#endif
#ifdef	CONFIG_S3C64XX_DEV_SPI
	&s5p6450_device_spi0,
	&s5p6450_device_spi1,
#endif
};

#ifdef CONFIG_S3C_DEV_HSMMC
static struct s3c_sdhci_platdata smdk6450_hsmmc0_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_NONE,
	.max_width		= 4,
};
#endif
#ifdef CONFIG_S3C_DEV_HSMMC1
static struct s3c_sdhci_platdata smdk6450_hsmmc1_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_INTERNAL,
	.max_width		= 4,
};
#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
static struct s3c_sdhci_platdata smdk6450_hsmmc2_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_NONE,
	.max_width		= 4,
};
#endif

static void __init smdk6450_map_io(void)
{
	s5p_init_io(NULL, 0, S5P_SYS_ID);
	s3c24xx_init_clocks(19200000);

	s3c24xx_init_uarts(smdk6450_uartcfgs, ARRAY_SIZE(smdk6450_uartcfgs));
}
static struct s3c2410_ts_mach_info s3c_ts_platform __initdata = {
	.delay			= 10000,
	.presc			= 49,
	.oversampling_shift	= 2,
};

static void __init smdk6450_machine_init(void)
{
#ifdef CONFIG_FB_S3C_V2
	s3cfb_set_platdata(NULL);
#endif
#ifdef CONFIG_S3C_DEV_MSHC
	s5p6450_default_mshci();
#endif
#ifdef CONFIG_TOUCHSCREEN_S3C2410
	s3c24xx_ts_set_platdata(&s3c_ts_platform);
#endif

	s3c_i2c0_set_platdata(NULL);
	s3c_i2c1_set_platdata(NULL);
	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));

	/* spi */
#ifdef CONFIG_S3C64XX_DEV_SPI
	if (!gpio_request(S5P6450_GPC(3), "SPI_CS0")) {
		gpio_direction_output(S5P6450_GPC(3), 1);
		s3c_gpio_cfgpin(S5P6450_GPC(3), S3C_GPIO_SFN(1));
		s3c_gpio_setpull(S5P6450_GPC(3), S3C_GPIO_PULL_UP);
		s5p6450_spi_set_info(0, S5P6450_SPI_SRCCLK_PCLK,
			ARRAY_SIZE(smdk_spi0_csi));
	}
	if (!gpio_request(S5P6450_GPC(7), "SPI_CS1")) {
		gpio_direction_output(S5P6450_GPC(7), 1);
		s3c_gpio_cfgpin(S5P6450_GPC(7), S3C_GPIO_SFN(1));
		s3c_gpio_setpull(S5P6450_GPC(7), S3C_GPIO_PULL_UP);
		s5p6450_spi_set_info(1, S5P6450_SPI_SRCCLK_PCLK,
			ARRAY_SIZE(smdk_spi1_csi));
	}
	spi_register_board_info(s3c_spi_devs, ARRAY_SIZE(s3c_spi_devs));
#endif

#ifdef CONFIG_PM
	s3c_pm_init();
#endif
#ifdef CONFIG_S3C_DEV_HSMMC
	s3c_sdhci0_set_platdata(&smdk6450_hsmmc0_pdata);
#endif
#ifdef CONFIG_S3C_DEV_HSMMC1
	s3c_sdhci1_set_platdata(&smdk6450_hsmmc1_pdata);
#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
	s3c_sdhci2_set_platdata(&smdk6450_hsmmc2_pdata);
#endif

	platform_add_devices(smdk6450_devices, ARRAY_SIZE(smdk6450_devices));
}

#ifdef CONFIG_USB_SUPPORT
void usb_host_phy_init(void)
{
	struct clk *otg_clk;

	otg_clk = clk_get(NULL, "usbotg");
	clk_enable(otg_clk);

	if (readl(S5P_OTHERS) & (0x1<<16))
		return;

	__raw_writel(__raw_readl(S5P_OTHERS)
		|(0x1<<16), S5P_OTHERS);
	__raw_writel((__raw_readl(S3C_USBOTG_PHYPWR)
		&~(0x1<<7)&~(0x1<<6))|(0x1<<8)|(0x1<<5), S3C_USBOTG_PHYPWR);
	__raw_writel((__raw_readl(S3C_USBOTG_PHYCLK)
		&~(0x1<<7))|(0x1<<0), S3C_USBOTG_PHYCLK);
	__raw_writel((__raw_readl(S3C_USBOTG_RSTCON))
		|(0x1<<4)|(0x1<<3), S3C_USBOTG_RSTCON);
	__raw_writel(__raw_readl(S3C_USBOTG_RSTCON)
		&~(0x1<<4)&~(0x1<<3), S3C_USBOTG_RSTCON);
}
EXPORT_SYMBOL(usb_host_phy_init);

void usb_host_phy_off(void)
{
	__raw_writel(__raw_readl(S3C_USBOTG_PHYPWR)
		|(0x1<<7)|(0x1<<6), S3C_USBOTG_PHYPWR);
	__raw_writel(__raw_readl(S5P_OTHERS)
		&~(1<<16), S5P_OTHERS);
}
EXPORT_SYMBOL(usb_host_phy_off);

/* Initializes OTG Phy. */
void otg_phy_init(void)
{
	/*USB PHY0 Enable */
	writel(readl(S5P_OTHERS)|(0x1<<17),
		S5P_OTHERS);
	writel((readl(S3C_USBOTG_PHYPWR)&~(0x3<<3)&~(0x1<<0))|(0x1<<5),
		S3C_USBOTG_PHYPWR);
	writel((readl(S3C_USBOTG_PHYCLK)&~(0x5<<2))|(0x1<<0),
		S3C_USBOTG_PHYCLK);
	writel((readl(S3C_USBOTG_RSTCON)&~(0x3<<1))|(0x1<<0),
		S3C_USBOTG_RSTCON);
	udelay(10);
	writel(readl(S3C_USBOTG_RSTCON)&~(0x7<<0), S3C_USBOTG_RSTCON);
	udelay(10);
}
EXPORT_SYMBOL(otg_phy_init);

/* USB Control request data struct must be located here for DMA transfer */
struct usb_ctrlrequest usb_ctrl __attribute__((aligned(8)));
EXPORT_SYMBOL(usb_ctrl);

/* OTG PHY Power Off */
void otg_phy_off(void)
{
	writel(readl(S3C_USBOTG_PHYPWR)|(0x3<<3), S3C_USBOTG_PHYPWR);
	writel(readl(S5P_OTHERS)&~(1<<17), S5P_OTHERS);
}
EXPORT_SYMBOL(otg_phy_off);
#endif

MACHINE_START(SMDK6450, "SMDK6450")
	.phys_io	= S3C_PA_UART & 0xfff00000,
	.io_pg_offst	= (((u32)S3C_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S5P_PA_SDRAM + 0x100,

	.init_irq	= s5p6450_init_irq,
	.map_io		= smdk6450_map_io,
	.init_machine	= smdk6450_machine_init,
	.timer		= &s5p6450_timer,
MACHINE_END
