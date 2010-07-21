/* linux/arch/arm/mach-s5pv310/clock.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5PV310 - Clock support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/io.h>

#include <plat/cpu-freq.h>
#include <plat/clock.h>
#include <plat/cpu.h>
#include <plat/pll.h>
#include <plat/s5p-clock.h>
#include <plat/clock-clksrc.h>

#include <mach/map.h>
#include <mach/regs-clock.h>

static struct clk clk_sclk_hdmi27m = {
	.name           = "sclk_hdmi27m",
	.id             = -1,
	.rate           = 27000000,
};

static struct clk clk_sclk_hdmiphy = {
	.name		= "sclk_hdmiphy",
	.id		= -1,
};

static struct clk clk_sclk_usbphy0 = {
	.name		= "sclk_usbphy0",
	.id		= -1,
};

static struct clk clk_sclk_usbphy1 = {
	.name		= "sclk_usbphy1",
	.id		= -1,
};

/* Core list of CMU_CPU side */

static struct clksrc_clk clk_mout_apll = {
	.clk	= {
		.name		= "mout_apll",
		.id		= -1,
	},
	.sources	= &clk_src_apll,
	.reg_src	= { .reg = S5P_CLKSRC_CPU, .shift = 0, .size = 1 },
	.reg_div	= { .reg = S5P_CLKDIV_CPU, .shift = 24, .size = 3 },
};

static struct clksrc_clk clk_mout_epll = {
	.clk	= {
		.name		= "mout_epll",
		.id		= -1,
	},
	.sources	= &clk_src_epll,
	.reg_src	= { .reg = S5P_CLKSRC_TOP0, .shift = 4, .size = 1 },
};

static struct clksrc_clk clk_mout_mpll = {
	.clk = {
		.name		= "mout_mpll",
		.id		= -1,
	},
	.sources	= &clk_src_mpll,
	.reg_src	= { .reg = S5P_CLKSRC_CPU, .shift = 4, .size = 1 },
};

static struct clk *clkset_moutcore_list[] = {
	[0] = &clk_mout_apll.clk,
	[1] = &clk_mout_mpll.clk,
};

static struct clksrc_sources clkset_moutcore = {
	.sources	= clkset_moutcore_list,
	.nr_sources	= ARRAY_SIZE(clkset_moutcore_list),
};

static struct clksrc_clk clk_moutcore = {
	.clk	= {
		.name		= "moutcore",
		.id		= -1,
	},
	.sources	= &clkset_moutcore,
	.reg_src	= { .reg = S5P_CLKSRC_CPU, .shift = 16, .size = 1 },
};

static struct clksrc_clk clk_coreclk = {
	.clk	= {
		.name		= "core_clk",
		.id		= -1,
		.parent		= &clk_moutcore.clk,
	},
	.reg_div	= { .reg = S5P_CLKDIV_CPU, .shift = 0, .size = 3 },
};

static struct clksrc_clk clk_armclk = {
	.clk	= {
		.name		= "armclk",
		.id		= -1,
		.parent		= &clk_coreclk.clk,
	},
};

/* Core list of CMU_CORE side */

static struct clk *clkset_corebus_list[] = {
	[0] = &clk_mout_mpll.clk,
	[1] = &clk_mout_apll.clk,
};

static struct clksrc_sources clkset_mout_corebus = {
	.sources	= clkset_corebus_list,
	.nr_sources	= ARRAY_SIZE(clkset_corebus_list),
};

static struct clksrc_clk clk_mout_corebus = {
	.clk	= {
		.name		= "mout_corebus",
		.id		= -1,
	},
	.sources	= &clkset_mout_corebus,
	.reg_src	= { .reg = S5P_CLKSRC_CPU, .shift = 4, .size = 1 },
};

static struct clksrc_clk clk_sclk_dmc = {
	.clk	= {
		.name		= "sclk_dmc",
		.id		= -1,
		.parent		= &clk_mout_corebus.clk,
	},
	.reg_div	= { .reg = S5P_CLKDIV_CORE0, .shift = 12, .size = 3 },
};

/* Core list of CMU_TOP side */

static struct clk *clkset_aclk_top_list[] = {
	[0] = &clk_mout_mpll.clk,
	[1] = &clk_mout_apll.clk,
};

static struct clksrc_sources clkset_aclk = {
	.sources        = clkset_aclk_top_list,
	.nr_sources     = ARRAY_SIZE(clkset_aclk_top_list),
};

static struct clksrc_clk clk_aclk_200 = {
	.clk    = {
		.name           = "aclk_200",
		.id             = -1,
	},
	.sources        = &clkset_aclk,
	.reg_src        = { .reg = S5P_CLKSRC_TOP0, .shift = 12, .size = 1 },
	.reg_div	= { .reg = S5P_CLKDIV_TOP, .shift = 0, .size = 3 },
};

static struct clksrc_clk clk_aclk_100 = {
	.clk    = {
		.name           = "aclk_100",
		.id             = -1,
	},
	.sources        = &clkset_aclk,
	.reg_src        = { .reg = S5P_CLKSRC_TOP0, .shift = 16, .size = 1 },
	.reg_div	= { .reg = S5P_CLKDIV_TOP, .shift = 4, .size = 4 },
};

static struct clksrc_clk clk_aclk_160 = {
	.clk    = {
		.name           = "aclk_160",
		.id             = -1,
	},
	.sources        = &clkset_aclk,
	.reg_src        = { .reg = S5P_CLKSRC_TOP0, .shift = 20, .size = 1 },
	.reg_div	= { .reg = S5P_CLKDIV_TOP, .shift = 8, .size = 3 },
};

static struct clksrc_clk clk_aclk_133 = {
	.clk    = {
		.name           = "aclk_133",
		.id             = -1,
	},
	.sources        = &clkset_aclk,
	.reg_src        = { .reg = S5P_CLKSRC_TOP0, .shift = 24, .size = 1 },
	.reg_div	= { .reg = S5P_CLKDIV_TOP, .shift = 12, .size = 3 },
};

static struct clk *clkset_vpllsrc_list[] = {
	[0] = &clk_fin_vpll,
	[1] = &clk_sclk_hdmi27m,
};

static struct clksrc_sources clkset_vpllsrc = {
	.sources	= clkset_vpllsrc_list,
	.nr_sources	= ARRAY_SIZE(clkset_vpllsrc_list),
};

static struct clksrc_clk clk_vpllsrc = {
	.clk    = {
		.name           = "vpll_src",
		.id             = -1,
	},
	.sources        = &clkset_vpllsrc,
	.reg_src        = { .reg = S5P_CLKSRC_TOP0, .shift = 0, .size = 1 },
};

static struct clk *clkset_sclk_vpll_list[] = {
	[0] = &clk_vpllsrc.clk,
	[1] = &clk_fout_vpll,
};

static struct clksrc_sources clkset_sclk_vpll = {
	.sources        = clkset_sclk_vpll_list,
	.nr_sources     = ARRAY_SIZE(clkset_sclk_vpll_list),
};

static struct clksrc_clk clk_sclk_vpll = {
	.clk    = {
		.name           = "sclk_vpll",
		.id             = -1,
	},
	.sources        = &clkset_sclk_vpll,
	.reg_src        = { .reg = S5P_CLKSRC_TOP0, .shift = 8, .size = 1 },
};

static int s5pv310_clk_ip_cam_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP_CAM, clk, enable);
}

static int s5pv310_clk_ip_tv_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP_TV, clk, enable);
}

static int s5pv310_clk_ip_mfc_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP_MFC, clk, enable);
}

static int s5pv310_clk_ip_g3d_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP_G3D, clk, enable);
}

static int s5pv310_clk_ip_image_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP_IMAGE, clk, enable);
}

static int s5pv310_clk_ip_lcd0_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP_LCD0, clk, enable);
}

static int s5pv310_clk_ip_lcd1_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP_LCD1, clk, enable);
}

static int s5pv310_clk_ip_fsys_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP_FSYS, clk, enable);
}

static int s5pv310_clk_ip_gps_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP_GPS, clk, enable);
}

static int s5pv310_clk_ip_peril_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP_PERIL, clk, enable);
}

static int s5pv310_clk_ip_perir_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP_PERIR, clk, enable);
}

static int s5pv310_clk_sclk_cam_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_SCLK_CAM, clk, enable);
}

static int s5pv310_clk_sclk_tv_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_SCLK_TV, clk, enable);
}

static int s5pv310_clk_sclk_mfc_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_SCLK_MFC, clk, enable);
}

static int s5pv310_clk_sclk_g3d_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_SCLK_G3D, clk, enable);
}

static int s5pv310_clk_sclk_image_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_SCLK_IMAGE, clk, enable);
}

static int s5pv310_clk_sclk_lcd0_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_SCLK_LCD0, clk, enable);
}

static int s5pv310_clk_sclk_lcd1_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_SCLK_LCD1, clk, enable);
}

static int s5pv310_clk_sclk_maudio_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_SCLK_MAUDIO, clk, enable);
}

static int s5pv310_clk_sclk_fsys_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_SCLK_FSYS, clk, enable);
}

static int s5pv310_clk_sclk_peril_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_SCLK_PERIL, clk, enable);
}

static struct clk init_clocks_disable[] = {
	/* Nothing here yet */
};

static struct clk init_clocks[] = {
	{
		.name		= "uart",
		.id		= 0,
		.enable		= s5pv310_clk_ip_perir_ctrl,
		.ctrlbit	= (1 << 0),
	}, {
		.name		= "uart",
		.id		= 1,
		.enable		= s5pv310_clk_ip_perir_ctrl,
		.ctrlbit	= (1 << 1),
	}, {
		.name		= "uart",
		.id		= 2,
		.enable		= s5pv310_clk_ip_perir_ctrl,
		.ctrlbit	= (1 << 2),
	}, {
		.name		= "uart",
		.id		= 3,
		.enable		= s5pv310_clk_ip_perir_ctrl,
		.ctrlbit	= (1 << 3),
	}, {
		.name		= "pwm",
		.id		= -1,
		.enable		= s5pv310_clk_ip_perir_ctrl,
		.ctrlbit	= (1 << 24),
	}, {
		.name		= "csis",
		.id		= 0,
		.enable		= s5pv310_clk_ip_cam_ctrl,
		.ctrlbit	= (1 << 4),
	}, {
		.name		= "csis",
		.id		= 1,
		.enable		= s5pv310_clk_ip_cam_ctrl,
		.ctrlbit	= (1 << 5),
	}, {
		.name		= "fimc",
		.id		= 0,
		.enable		= s5pv310_clk_ip_cam_ctrl,
		.ctrlbit	= (1 << 0),
	}, {
		.name		= "fimc",
		.id		= 1,
		.enable		= s5pv310_clk_ip_cam_ctrl,
		.ctrlbit	= (1 << 1),
	}, {
		.name		= "fimc",
		.id		= 2,
		.enable		= s5pv310_clk_ip_cam_ctrl,
		.ctrlbit	= (1 << 2),
	}, {
		.name		= "fimc",
		.id		= 3,
		.enable		= s5pv310_clk_ip_cam_ctrl,
		.ctrlbit	= (1 << 3),
	}, {
		.name		= "fimd",
		.id		= 0,
		.enable		= s5pv310_clk_ip_lcd0_ctrl,
		.ctrlbit	= (1 << 0),
	}, {
		.name		= "fimd",
		.id		= 1,
		.enable		= s5pv310_clk_ip_lcd1_ctrl,
		.ctrlbit	= (1 << 0),
	}, {
		.name		= "hsmmc",
		.id		= 0,
		.enable		= s5pv310_clk_ip_fsys_ctrl,
		.ctrlbit	= (1 << 5),
	}, {
		.name		= "hsmmc",
		.id		= 1,
		.enable		= s5pv310_clk_ip_fsys_ctrl,
		.ctrlbit	= (1 << 6),
	}, {
		.name		= "hsmmc",
		.id		= 2,
		.enable		= s5pv310_clk_ip_fsys_ctrl,
		.ctrlbit	= (1 << 7),
	}, {
		.name		= "hsmmc",
		.id		= 3,
		.enable		= s5pv310_clk_ip_fsys_ctrl,
		.ctrlbit	= (1 << 8),
	}, {
		.name		= "hsmmc",
		.id		= 4,
		.enable		= s5pv310_clk_ip_fsys_ctrl,
		.ctrlbit	= (1 << 9),
	}, {
		.name		= "sata",
		.id		= -1,
		.enable		= s5pv310_clk_ip_fsys_ctrl,
		.ctrlbit	= (1 << 10),
	}, {
		.name		= "spi",
		.id		= 0,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 16),
	}, {
		.name		= "spi",
		.id		= 1,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 17),
	}, {
		.name		= "spi",
		.id		= 2,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 18),
	},
};

static struct clk *clkset_group_list[] = {
	[0] = &clk_ext_xtal_mux,
	[1] = &clk_xusbxti,
	[2] = &clk_sclk_hdmi27m,
	[3] = &clk_sclk_usbphy0,
	[4] = &clk_sclk_usbphy1,
	[5] = &clk_sclk_hdmiphy,
	[6] = &clk_mout_mpll.clk,
	[7] = &clk_mout_epll.clk,
	[8] = &clk_sclk_vpll.clk,
};

static struct clksrc_sources clkset_group = {
	.sources	= clkset_group_list,
	.nr_sources	= ARRAY_SIZE(clkset_group_list),
};

static struct clksrc_clk clk_sclk_mipidphy4l = {
	.clk    = {
		.name           = "sclk_mipidphy4l",
		.id             = -1,
		.enable		= s5pv310_clk_ip_lcd0_ctrl,
		.ctrlbit	= (1 << 4),
	},
	.sources        = &clkset_group,
	.reg_src        = { .reg = S5P_CLKSRC_LCD0, .shift = 12, .size = 4 },
	.reg_div	= { .reg = S5P_CLKDIV_LCD0, .shift = 16, .size = 4 },
};

static struct clksrc_clk clk_sclk_mipidphy2l = {
	.clk    = {
		.name           = "sclk_mipidphy2l",
		.id             = -1,
		.enable		= s5pv310_clk_ip_lcd1_ctrl,
		.ctrlbit	= (1 << 4),
	},
	.sources        = &clkset_group,
	.reg_src        = { .reg = S5P_CLKSRC_LCD1, .shift = 12, .size = 4 },
	.reg_div	= { .reg = S5P_CLKDIV_LCD1, .shift = 16, .size = 4 },
};

static struct clksrc_clk clksrcs[] = {
	{
		.clk	= {
			.name		= "uclk1",
			.id		= 0,
			.ctrlbit	= (1 << 0),
			.enable		= s5pv310_clk_sclk_peril_ctrl,
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_PERIL0, .shift = 0, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_PERIL0, .shift = 0, .size = 4 },
	}, {
		.clk		= {
			.name		= "uclk1",
			.id		= 1,
			.enable		= s5pv310_clk_sclk_peril_ctrl,
			.ctrlbit	= (1 << 1),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_PERIL0, .shift = 4, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_PERIL0, .shift = 4, .size = 4 },
	}, {
		.clk		= {
			.name		= "uclk1",
			.id		= 2,
			.enable		= s5pv310_clk_sclk_peril_ctrl,
			.ctrlbit	= (1 << 2),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_PERIL0, .shift = 8, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_PERIL0, .shift = 8, .size = 4 },
	}, {
		.clk		= {
			.name		= "uclk1",
			.id		= 3,
			.enable		= s5pv310_clk_sclk_peril_ctrl,
			.ctrlbit	= (1 << 3),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_PERIL0, .shift = 12, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_PERIL0, .shift = 12, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_pwm",
			.id		= -1,
			.enable		= s5pv310_clk_sclk_peril_ctrl,
			.ctrlbit	= (1 << 12),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_PERIL0, .shift = 24, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_PERIL3, .shift = 0, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_csis",
			.id		= 0,
			.enable		= s5pv310_clk_sclk_cam_ctrl,
			.ctrlbit	= (1 << 4),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_CAM, .shift = 24, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_CAM, .shift = 24, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_csis",
			.id		= 1,
			.enable		= s5pv310_clk_sclk_cam_ctrl,
			.ctrlbit	= (1 << 5),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_CAM, .shift = 28, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_CAM, .shift = 28, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_cam",
			.id		= 0,
			.enable		= s5pv310_clk_sclk_cam_ctrl,
			.ctrlbit	= (1 << 4),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_CAM, .shift = 16, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_CAM, .shift = 16, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_cam",
			.id		= 1,
			.enable		= s5pv310_clk_sclk_cam_ctrl,
			.ctrlbit	= (1 << 5),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_CAM, .shift = 20, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_CAM, .shift = 20, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_fimc",
			.id		= 0,
			.enable		= s5pv310_clk_sclk_cam_ctrl,
			.ctrlbit	= (1 << 0),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_CAM, .shift = 0, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_CAM, .shift = 0, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_fimc",
			.id		= 1,
			.enable		= s5pv310_clk_sclk_cam_ctrl,
			.ctrlbit	= (1 << 1),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_CAM, .shift = 4, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_CAM, .shift = 4, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_fimc",
			.id		= 2,
			.enable		= s5pv310_clk_sclk_cam_ctrl,
			.ctrlbit	= (1 << 2),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_CAM, .shift = 8, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_CAM, .shift = 8, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_fimc",
			.id		= 3,
			.enable		= s5pv310_clk_sclk_cam_ctrl,
			.ctrlbit	= (1 << 3),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_CAM, .shift = 12, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_CAM, .shift = 12, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_fimd",
			.id		= 0,
			.enable		= s5pv310_clk_sclk_lcd0_ctrl,
			.ctrlbit	= (1 << 0),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_LCD0, .shift = 0, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_LCD0, .shift = 0, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_fimd",
			.id		= 1,
			.enable		= s5pv310_clk_sclk_lcd1_ctrl,
			.ctrlbit	= (1 << 0),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_LCD1, .shift = 0, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_LCD1, .shift = 0, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_mipi",
			.id		= 0,
			.parent		= &clk_sclk_mipidphy4l.clk,
			.enable		= s5pv310_clk_sclk_lcd0_ctrl,
			.ctrlbit	= (1 << 3),
		},
		.reg_div = { .reg = S5P_CLKDIV_LCD0, .shift = 20, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_mipi",
			.id		= 1,
			.parent		= &clk_sclk_mipidphy2l.clk,
			.enable		= s5pv310_clk_sclk_lcd1_ctrl,
			.ctrlbit	= (1 << 3),
		},
		.reg_div = { .reg = S5P_CLKDIV_LCD0, .shift = 20, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_mmc",
			.id		= 0,
			.enable		= s5pv310_clk_sclk_fsys_ctrl,
			.ctrlbit	= (1 << 0),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_FSYS, .shift = 0, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_FSYS1, .shift = 0, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_mmc",
			.id		= 1,
			.enable		= s5pv310_clk_sclk_fsys_ctrl,
			.ctrlbit	= (1 << 1),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_FSYS, .shift = 4, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_FSYS1, .shift = 16, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_mmc",
			.id		= 2,
			.enable		= s5pv310_clk_sclk_fsys_ctrl,
			.ctrlbit	= (1 << 2),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_FSYS, .shift = 8, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_FSYS2, .shift = 0, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_mmc",
			.id		= 3,
			.enable		= s5pv310_clk_sclk_fsys_ctrl,
			.ctrlbit	= (1 << 3),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_FSYS, .shift = 12, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_FSYS2, .shift = 16, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_mmc",
			.id		= 4,
			.enable		= s5pv310_clk_sclk_fsys_ctrl,
			.ctrlbit	= (1 << 4),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_FSYS, .shift = 16, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_FSYS3, .shift = 0, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_sata",
			.id		= -1,
			.enable		= s5pv310_clk_sclk_fsys_ctrl,
			.ctrlbit	= (1 << 8),
		},
		.sources = &clkset_mout_corebus,
		.reg_src = { .reg = S5P_CLKSRC_FSYS, .shift = 24, .size = 1 },
		.reg_div = { .reg = S5P_CLKDIV_FSYS0, .shift = 20, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_spi",
			.id		= 0,
			.enable		= s5pv310_clk_sclk_peril_ctrl,
			.ctrlbit	= (1 << 6),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_PERIL1, .shift = 16, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_PERIL1, .shift = 0, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_spi",
			.id		= 1,
			.enable		= s5pv310_clk_sclk_peril_ctrl,
			.ctrlbit	= (1 << 7),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_PERIL1, .shift = 20, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_PERIL1, .shift = 16, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_spi",
			.id		= 2,
			.enable		= s5pv310_clk_sclk_peril_ctrl,
			.ctrlbit	= (1 << 8),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_PERIL1, .shift = 24, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_PERIL2, .shift = 0, .size = 4 },
	},
};

/* Clock initialisation code */
static struct clksrc_clk *sysclks[] = {
	&clk_mout_apll,
	&clk_mout_epll,
	&clk_mout_mpll,
	&clk_moutcore,
	&clk_coreclk,
	&clk_armclk,
	&clk_mout_corebus,
	&clk_sclk_dmc,
	&clk_vpllsrc,
	&clk_sclk_vpll,
	&clk_aclk_200,
	&clk_aclk_100,
	&clk_aclk_160,
	&clk_aclk_133,
	&clk_sclk_mipidphy4l,
	&clk_sclk_mipidphy2l,
};

void __init_or_cpufreq s5pv310_setup_clocks(void)
{
	struct clk *xtal_clk;
	unsigned long apll;
	unsigned long mpll;
	unsigned long epll;
	unsigned long vpll;
	unsigned long xtal;
	unsigned long armclk;
	unsigned long sclk_dmc;
	unsigned long aclk_200;
	unsigned long aclk_100;
	unsigned long aclk_160;
	unsigned long aclk_133;
	unsigned int ptr;

	printk(KERN_DEBUG "%s: registering clocks\n", __func__);

	xtal_clk = clk_get(NULL, "xtal");
	BUG_ON(IS_ERR(xtal_clk));

	xtal = clk_get_rate(xtal_clk);
	clk_put(xtal_clk);

	printk(KERN_DEBUG "%s: xtal is %ld\n", __func__, xtal);

#ifndef CONFIG_S5PV310_FPGA
	apll = s5p_get_pll45xx(xtal, __raw_readl(S5P_APLL_CON0), pll_4508);
	mpll = s5p_get_pll45xx(xtal, __raw_readl(S5P_MPLL_CON0), pll_4508);
	epll = s5p_get_pll46xx(xtal, __raw_readl(S5P_EPLL_CON0),
				__raw_readl(S5P_EPLL_CON1), pll_4500);

	vpllsrc = clk_get_rate(&clk_vpllsrc.clk);
	vpll = s5p_get_pll46xx(vpllsrc, __raw_readl(S5P_VPLL_CON0),
				__raw_readl(S5P_VPLL_CON1), pll_4502);
#else
	apll = xtal;
	mpll = xtal;
	epll = xtal;
	vpll = xtal;
#endif

	clk_fout_apll.rate = apll;
	clk_fout_mpll.rate = mpll;
	clk_fout_epll.rate = epll;
	clk_fout_vpll.rate = vpll;

	armclk = clk_get_rate(&clk_armclk.clk);
	sclk_dmc = clk_get_rate(&clk_sclk_dmc.clk);
	aclk_200 = clk_get_rate(&clk_aclk_200.clk);
	aclk_100 = clk_get_rate(&clk_aclk_100.clk);
	aclk_160 = clk_get_rate(&clk_aclk_160.clk);
	aclk_133 = clk_get_rate(&clk_aclk_133.clk);

	printk(KERN_INFO "S5PV310: ARMCLK=%ld, DMC=%ld, ACLK200=%ld\n"
			 "ACLK100=%ld, ACLK160=%ld, ACLK133=%ld\n",
			  armclk, sclk_dmc, aclk_200,
			  aclk_100,aclk_160, aclk_133);

	clk_f.rate = armclk;
	clk_h.rate = sclk_dmc;
	clk_p.rate = aclk_200;

	for (ptr = 0; ptr < ARRAY_SIZE(clksrcs); ptr++)
		s3c_set_clksrc(&clksrcs[ptr], true);
}

static struct clk *clks[] __initdata = {
	/* Nothing here yet */
};

void __init s5pv310_register_clocks(void)
{
	struct clk *clkp;
	int ret;
	int ptr;

	ret = s3c24xx_register_clocks(clks, ARRAY_SIZE(clks));
	if (ret > 0)
		printk(KERN_ERR "Failed to register %u clocks\n", ret);

	for (ptr = 0; ptr < ARRAY_SIZE(sysclks); ptr++)
		s3c_register_clksrc(sysclks[ptr], 1);

	s3c_register_clksrc(clksrcs, ARRAY_SIZE(clksrcs));
	s3c_register_clocks(init_clocks, ARRAY_SIZE(init_clocks));

	clkp = init_clocks_disable;
	for (ptr = 0; ptr < ARRAY_SIZE(init_clocks_disable); ptr++, clkp++) {
		ret = s3c24xx_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}
		(clkp->enable)(clkp, 0);
	}

	s3c_pwmclk_init();
}
