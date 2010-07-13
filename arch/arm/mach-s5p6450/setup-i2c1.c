/* linux/arch/arm/mach-s5p6450/setup-i2c0.c
 *
 * Copyright (c) 2009 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * I2C0 GPIO configuration.
 *
 * Based on plat-s3c64xx/setup-i2c0.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#if 1
#include <linux/io.h>
void __iomem *iobase;
#endif

struct platform_device; /* don't need the contents */

#include <mach/gpio.h>
#include <plat/iic.h>
#include <plat/gpio-cfg.h>

void s3c_i2c1_cfg_gpio(struct platform_device *dev)
{
#if 0
	s3c_gpio_cfgpin(S5P6450_GPR(9), S3C_GPIO_SFN(6));
	s3c_gpio_cfgpin(S5P6450_GPR(10), S3C_GPIO_SFN(6));

	s3c_gpio_setpull(S5P6450_GPR(9), S3C_GPIO_PULL_UP);
	s3c_gpio_setpull(S5P6450_GPR(10), S3C_GPIO_PULL_UP);
#else
	u32 reg_value;

	/* set iobase address */
	iobase = ioremap(0xe0308290, 0x10);

	/* set GPRCON1 register */
	reg_value = readl(iobase + 0x04);
	/* GPR[9] set to I2C_SCL1 and GPR[10] set to I2C_SDA1 */
	reg_value = ((reg_value & ~(0xFF<<8)) | (0x66<<8));
	writel(reg_value, (iobase + 0x04));

	/* set GPRPUD register */
	reg_value = readl(iobase + 0x0C);
	/* GPR[9], GPR[10] set to PULLUP_ENABLE */
	reg_value = ((reg_value & ~(0xF<<18)) | (0xA<<18));
	writel(reg_value, (iobase + 0x0C));
#endif
}
