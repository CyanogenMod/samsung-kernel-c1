/* linux/arch/arm/plat-s5p/include/plat/csis.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * 		http://www.samsung.com/
 *
 * S5PV210 - Platform header file for MIPI-CSI2 driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_PLAT_CSIS_H
#define __ASM_PLAT_CSIS_H __FILE__

struct platform_device;

struct s3c_platform_csis {
	const char	srclk_name[16];
	const char	clk_name[16];
	unsigned long	clk_rate;

	void		(*cfg_gpio)(void);
	void		(*cfg_phy_global)(int on);
};
#ifdef CONFIG_CPU_S5PV310
extern void s3c_csis0_set_platdata(struct s3c_platform_csis *csis);
extern void s3c_csis1_set_platdata(struct s3c_platform_csis *csis);
extern void s3c_csis0_cfg_gpio(void);
extern void s3c_csis1_cfg_gpio(void);
extern void s3c_csis0_cfg_phy_global(int on);
extern void s3c_csis1_cfg_phy_global(int on);
#else
extern void s3c_csis_set_platdata(struct s3c_platform_csis *csis);
extern void s3c_csis_cfg_gpio(void);
extern void s3c_csis_cfg_phy_global(int on);
#endif
#endif /* __ASM_PLAT_CSIS_H */
