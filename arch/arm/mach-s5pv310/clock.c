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
#include <mach/regs-audss.h>

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

static struct clk clk_audiocdclk0 = {
	.name		= "audiocdclk",
	.id		= 0,
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

static int s5pv310_clk_audss_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_AUDSS, clk, enable);
}

static int s5pv310_clksrc_mask_top_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKSRC_MASK_TOP, clk, enable);
}

static int s5pv310_clksrc_mask_cam_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKSRC_MASK_CAM, clk, enable);
}

static int s5pv310_clksrc_mask_tv_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKSRC_MASK_TV, clk, enable);
}

static int s5pv310_clksrc_mask_lcd0_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKSRC_MASK_LCD0, clk, enable);
}

static int s5pv310_clksrc_mask_lcd1_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKSRC_MASK_LCD1, clk, enable);
}

static int s5pv310_clksrc_mask_maudio_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKSRC_MASK_MAUDIO, clk, enable);
}

static int s5pv310_clksrc_mask_fsys_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKSRC_MASK_FSYS, clk, enable);
}

static int s5pv310_clksrc_mask_peril0_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKSRC_MASK_PERIL0, clk, enable);
}

static int s5pv310_clksrc_mask_peril1_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKSRC_MASK_PERIL1, clk, enable);
}

/* Core list of CMU_CPU side */

static struct clksrc_clk clk_mout_apll = {
	.clk	= {
		.name		= "mout_apll",
		.id		= -1,
	},
	.sources	= &clk_src_apll,
	.reg_src	= { .reg = S5P_CLKSRC_CPU, .shift = 0, .size = 1 },
};

static struct clksrc_clk clk_sclk_apll = {
	.clk	= {
		.name		= "sclk_apll",
		.id		= -1,
		.parent		= &clk_mout_apll.clk,
	},
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
	.reg_src	= { .reg = S5P_CLKSRC_CPU, .shift = 8, .size = 1 },
};

static struct clk *clkset_moutcore_list[] = {
	[0] = &clk_sclk_apll.clk,
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
	[1] = &clk_sclk_apll.clk,
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
	.reg_src	= { .reg = S5P_CLKSRC_CORE, .shift = 4, .size = 1 },
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
	[1] = &clk_sclk_apll.clk,
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
		.enable		= s5pv310_clksrc_mask_top_ctrl,
		.ctrlbit	= (1 << 0),
	},
	.sources        = &clkset_vpllsrc,
	.reg_src        = { .reg = S5P_CLKSRC_TOP1, .shift = 0, .size = 1 },
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

static struct clk init_clocks_disable[] = {
	/* Nothing here yet */
};

static struct clk init_clocks[] = {
	{
		.name		= "uart",
		.id		= 0,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 0),
	}, {
		.name		= "uart",
		.id		= 1,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 1),
	}, {
		.name		= "uart",
		.id		= 2,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 2),
	}, {
		.name		= "uart",
		.id		= 3,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 3),
	}, {
		.name		= "uart",
		.id		= 4,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 4),
	}, {
		.name		= "uart",
		.id		= 5,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 5),
	}, {
		.name		= "timers",
		.id		= -1,
		.parent		= &clk_aclk_100.clk,
		.enable		= s5pv310_clk_ip_peril_ctrl,
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
                .name           = "iis",
                .id             = 1,
                .enable         = s5pv310_clk_ip_peril_ctrl,
                .ctrlbit        = (1 << 21),
        }, {
                .name           = "iis",
                .id             = 0,
                .enable         = s5pv310_clk_ip_peril_ctrl,
                .ctrlbit        = (1 << 20),
        }, {
                .name           = "iis",
                .id             = -1,
                .enable         = s5pv310_clk_audss_ctrl,
                .ctrlbit                = S5P_AUDSS_CLKGATE_I2SSPECIAL,
        }, {
		.name		= "sata",
		.id		= -1,
		.enable		= s5pv310_clk_ip_fsys_ctrl,
		.ctrlbit	= (1 << 10),
	}, {
		.name		= "adc",
		.id		= -1,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 15),
	}, {
		.name		= "watchdog",
		.id		= -1,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 14),
	}, {
		.name		= "usbhost",
		.id		= -1,
		.enable		= s5pv310_clk_ip_fsys_ctrl ,
		.ctrlbit	= (1 << 12),
	}, {
		.name		= "usbotg",
		.id		= -1,
		.enable		= s5pv310_clk_ip_fsys_ctrl,
		.ctrlbit	= (1 << 13),
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
	}, {
		.name		= "fimg2d",
		.id		= -1,
		.enable		= s5pv310_clk_ip_image_ctrl,
		.ctrlbit	= (1 << 0),
	}, {
		.name		= "gps",
		.id		= -1,
		.enable		= s5pv310_clk_ip_gps_ctrl,
		.ctrlbit	= (1 << 0),
	}, {
		.name		= "i2c",
		.id		= 0,
		.parent		= &clk_aclk_100.clk,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 6),
	}, {
		.name		= "i2c",
		.id		= 1,
		.parent		= &clk_aclk_100.clk,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 7),
	}, {
		.name		= "i2c",
		.id		= 2,
		.parent		= &clk_aclk_100.clk,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 8),
	}, {
		.name		= "i2c",
		.id		= 3,
		.parent		= &clk_aclk_100.clk,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 9),
	}, {
		.name		= "i2c",
		.id		= 4,
		.parent		= &clk_aclk_100.clk,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 10),
	}, {
		.name		= "i2c",
		.id		= 5,
		.parent		= &clk_aclk_100.clk,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 11),
	}, {
		.name		= "i2c",
		.id		= 6,
		.parent		= &clk_aclk_100.clk,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 12),
	}, {
		.name		= "i2c",
		.id		= 7,
		.parent		= &clk_aclk_100.clk,
		.enable		= s5pv310_clk_ip_peril_ctrl,
		.ctrlbit	= (1 << 13),
	},
};

static struct clk *clkset_sclk_audio0_list[] = {
	[0] = &clk_ext_xtal_mux,
	[1] = &clk_audiocdclk0,
	[2] = &clk_sclk_hdmi27m,
	[3] = &clk_sclk_usbphy0,
	[4] = &clk_mout_mpll.clk,
	[5] = &clk_mout_epll.clk,
	[6] = &clk_sclk_vpll.clk,
};

static struct clksrc_sources clkset_sclk_audio0 = {
	.sources	= clkset_sclk_audio0_list,
	.nr_sources	= ARRAY_SIZE(clkset_sclk_audio0_list),
};

static struct clksrc_clk clk_sclk_audio0 = {
	.clk		= {
		.name		= "sclk_audio",
		.id		= 0,
		.enable		= s5pv310_clksrc_mask_maudio_ctrl,
		.ctrlbit	= (1 << 0),
	},
	.sources = &clkset_sclk_audio0,
	.reg_src = { .reg = S5P_CLKSRC_MAUDIO, .shift = 0, .size = 4 },
	.reg_div = { .reg = S5P_CLKDIV_MAUDIO, .shift = 0, .size = 4 },
};

static struct clk *clkset_mout_audss_list[] = {
	NULL,
	&clk_fout_epll,
};

static struct clksrc_sources clkset_mout_audss = {
	.sources	= clkset_mout_audss_list,
	.nr_sources	= ARRAY_SIZE(clkset_mout_audss_list),
};

static struct clksrc_clk clk_mout_audss = {
	.clk		= {
		.name		= "mout_audss",
		.id		= -1,
	},
	.sources	= &clkset_mout_audss,
	.reg_src	= { .reg = S5P_CLKSRC_AUDSS, .shift = 0, .size = 1 },
};

static struct clk *clkset_sclk_audss_list[] = {
	&clk_mout_audss.clk,
	&clk_audiocdclk0,
	&clk_sclk_audio0.clk,
};

static struct clksrc_sources clkset_sclk_audss = {
	.sources	= clkset_sclk_audss_list,
	.nr_sources	= ARRAY_SIZE(clkset_sclk_audss_list),
};

static struct clksrc_clk clk_sclk_audss = {
	.clk		= {
		.name		= "audio-bus",
		.id		= -1,
		.enable		= s5pv310_clk_audss_ctrl,
		.ctrlbit	= S5P_AUDSS_CLKGATE_I2SBUS,
	},
	.sources	= &clkset_sclk_audss,
	.reg_src	= { .reg = S5P_CLKSRC_AUDSS, .shift = 2, .size = 2 },
	.reg_div	= { .reg = S5P_CLKDIV_AUDSS, .shift = 4, .size = 8 },
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

static struct clk *clkset_mout_g2d0_list[] = {
	[0] = &clk_mout_mpll.clk,
	[1] = &clk_sclk_apll.clk,
};

static struct clksrc_sources clkset_mout_g2d0 = {
	.sources	= clkset_mout_g2d0_list,
	.nr_sources	= ARRAY_SIZE(clkset_mout_g2d0_list),
};

static struct clksrc_clk clk_mout_g2d0 = {
	.clk	= {
		.name		= "mout_g2d0",
		.id		= -1,
	},
	.sources	= &clkset_mout_g2d0,
	.reg_src	= { .reg = S5P_CLKSRC_IMAGE, .shift = 0, .size = 1 },
};

static struct clk *clkset_mout_g2d1_list[] = {
	[0] = &clk_mout_epll.clk,
	[1] = &clk_sclk_vpll.clk,
};

static struct clksrc_sources clkset_mout_g2d1 = {
	.sources	= clkset_mout_g2d1_list,
	.nr_sources	= ARRAY_SIZE(clkset_mout_g2d1_list),
};

static struct clksrc_clk clk_mout_g2d1 = {
	.clk	= {
		.name		= "mout_g2d1",
		.id		= -1,
	},
	.sources	= &clkset_mout_g2d1,
	.reg_src	= { .reg = S5P_CLKSRC_IMAGE, .shift = 4, .size = 1 },
};

static struct clk *clkset_mout_g2d_list[] = {
	[0] = &clk_mout_g2d0.clk,
	[1] = &clk_mout_g2d1.clk,
};

static struct clksrc_sources clkset_mout_g2d = {
	.sources	= clkset_mout_g2d_list,
	.nr_sources	= ARRAY_SIZE(clkset_mout_g2d_list),
};

static struct clksrc_clk clk_dout_mmc0 = {
	.clk		= {
		.name		= "dout_mmc0",
		.id		= -1,
	},
	.sources = &clkset_group,
	.reg_src = { .reg = S5P_CLKSRC_FSYS, .shift = 0, .size = 4 },
	.reg_div = { .reg = S5P_CLKDIV_FSYS1, .shift = 0, .size = 4 },
};

static struct clksrc_clk clk_dout_mmc1 = {
	.clk		= {
		.name		= "dout_mmc1",
		.id		= -1,
	},
	.sources = &clkset_group,
	.reg_src = { .reg = S5P_CLKSRC_FSYS, .shift = 4, .size = 4 },
	.reg_div = { .reg = S5P_CLKDIV_FSYS1, .shift = 16, .size = 4 },
};

static struct clksrc_clk clk_dout_mmc2 = {
	.clk		= {
		.name		= "dout_mmc2",
		.id		= -1,
	},
	.sources = &clkset_group,
	.reg_src = { .reg = S5P_CLKSRC_FSYS, .shift = 8, .size = 4 },
	.reg_div = { .reg = S5P_CLKDIV_FSYS2, .shift = 0, .size = 4 },
};

static struct clksrc_clk clk_dout_mmc3 = {
	.clk		= {
		.name		= "dout_mmc3",
		.id		= -1,
	},
	.sources = &clkset_group,
	.reg_src = { .reg = S5P_CLKSRC_FSYS, .shift = 12, .size = 4 },
	.reg_div = { .reg = S5P_CLKDIV_FSYS2, .shift = 16, .size = 4 },
};

static struct clksrc_clk clk_dout_mmc4 = {
	.clk		= {
		.name		= "dout_mmc4",
		.id		= -1,
	},
	.sources = &clkset_group,
	.reg_src = { .reg = S5P_CLKSRC_FSYS, .shift = 16, .size = 4 },
	.reg_div = { .reg = S5P_CLKDIV_FSYS3, .shift = 0, .size = 4 },
};


static struct clksrc_clk clksrcs[] = {
	{
		.clk	= {
			.name		= "uclk1",
			.id		= 0,
			.ctrlbit	= (1 << 0),
			.enable		= s5pv310_clksrc_mask_peril0_ctrl,
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_PERIL0, .shift = 0, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_PERIL0, .shift = 0, .size = 4 },
	}, {
		.clk		= {
			.name		= "uclk1",
			.id		= 1,
			.enable		= s5pv310_clksrc_mask_peril0_ctrl,
			.ctrlbit	= (1 << 4),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_PERIL0, .shift = 4, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_PERIL0, .shift = 4, .size = 4 },
	}, {
		.clk		= {
			.name		= "uclk1",
			.id		= 2,
			.enable		= s5pv310_clksrc_mask_peril0_ctrl,
			.ctrlbit	= (1 << 8),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_PERIL0, .shift = 8, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_PERIL0, .shift = 8, .size = 4 },
	}, {
		.clk		= {
			.name		= "uclk1",
			.id		= 3,
			.enable		= s5pv310_clksrc_mask_peril0_ctrl,
			.ctrlbit	= (1 << 12),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_PERIL0, .shift = 12, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_PERIL0, .shift = 12, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_pwm",
			.id		= -1,
			.enable		= s5pv310_clksrc_mask_peril0_ctrl,
			.ctrlbit	= (1 << 24),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_PERIL0, .shift = 24, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_PERIL3, .shift = 0, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_csis",
			.id		= 0,
			.enable		= s5pv310_clksrc_mask_cam_ctrl,
			.ctrlbit	= (1 << 24),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_CAM, .shift = 24, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_CAM, .shift = 24, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_csis",
			.id		= 1,
			.enable		= s5pv310_clksrc_mask_cam_ctrl,
			.ctrlbit	= (1 << 28),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_CAM, .shift = 28, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_CAM, .shift = 28, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_cam",
			.id		= 0,
			.enable		= s5pv310_clksrc_mask_cam_ctrl,
			.ctrlbit	= (1 << 16),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_CAM, .shift = 16, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_CAM, .shift = 16, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_cam",
			.id		= 1,
			.enable		= s5pv310_clksrc_mask_cam_ctrl,
			.ctrlbit	= (1 << 20),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_CAM, .shift = 20, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_CAM, .shift = 20, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_fimc",
			.id		= 0,
			.enable		= s5pv310_clksrc_mask_cam_ctrl,
			.ctrlbit	= (1 << 0),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_CAM, .shift = 0, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_CAM, .shift = 0, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_fimc",
			.id		= 1,
			.enable		= s5pv310_clksrc_mask_cam_ctrl,
			.ctrlbit	= (1 << 4),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_CAM, .shift = 4, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_CAM, .shift = 4, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_fimc",
			.id		= 2,
			.enable		= s5pv310_clksrc_mask_cam_ctrl,
			.ctrlbit	= (1 << 8),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_CAM, .shift = 8, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_CAM, .shift = 8, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_fimc",
			.id		= 3,
			.enable		= s5pv310_clksrc_mask_cam_ctrl,
			.ctrlbit	= (1 << 12),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_CAM, .shift = 12, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_CAM, .shift = 12, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_fimd",
			.id		= 0,
			.enable		= s5pv310_clksrc_mask_lcd0_ctrl,
			.ctrlbit	= (1 << 0),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_LCD0, .shift = 0, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_LCD0, .shift = 0, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_fimd",
			.id		= 1,
			.enable		= s5pv310_clksrc_mask_lcd1_ctrl,
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
			.enable		= s5pv310_clksrc_mask_lcd0_ctrl,
			.ctrlbit	= (1 << 12),
		},
		.reg_div = { .reg = S5P_CLKDIV_LCD0, .shift = 20, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_mipi",
			.id		= 1,
			.parent		= &clk_sclk_mipidphy2l.clk,
			.enable		= s5pv310_clksrc_mask_lcd1_ctrl,
			.ctrlbit	= (1 << 12),
		},
		.reg_div = { .reg = S5P_CLKDIV_LCD1, .shift = 20, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_mmc",
			.id		= 0,
			.parent		= &clk_dout_mmc0.clk,
			.enable		= s5pv310_clksrc_mask_fsys_ctrl,
			.ctrlbit	= (1 << 0),
		},
		.reg_div = { .reg = S5P_CLKDIV_FSYS1, .shift = 8, .size = 8 },
	}, {
		.clk		= {
			.name		= "sclk_mmc",
			.id		= 1,
			.parent         = &clk_dout_mmc1.clk,
			.enable		= s5pv310_clksrc_mask_fsys_ctrl,
			.ctrlbit	= (1 << 4),
		},
		.reg_div = { .reg = S5P_CLKDIV_FSYS1, .shift = 24, .size = 8 },
	}, {
		.clk		= {
			.name		= "sclk_mmc",
			.id		= 2,
			.parent         = &clk_dout_mmc2.clk,
			.enable		= s5pv310_clksrc_mask_fsys_ctrl,
			.ctrlbit	= (1 << 8),
		},
		.reg_div = { .reg = S5P_CLKDIV_FSYS2, .shift = 8, .size = 8 },
	}, {
		.clk		= {
			.name		= "sclk_mmc",
			.id		= 3,
			.parent         = &clk_dout_mmc3.clk,
			.enable		= s5pv310_clksrc_mask_fsys_ctrl,
			.ctrlbit	= (1 << 12),
		},
		.reg_div = { .reg = S5P_CLKDIV_FSYS2, .shift = 24, .size = 8 },
	}, {
		.clk		= {
			.name		= "sclk_mmc",
			.id		= 4,
			.parent         = &clk_dout_mmc4.clk,
			.enable		= s5pv310_clksrc_mask_fsys_ctrl,
			.ctrlbit	= (1 << 16),
		},
		.reg_div = { .reg = S5P_CLKDIV_FSYS3, .shift = 8, .size = 8 },
	}, {
		.clk		= {
			.name		= "sclk_sata",
			.id		= -1,
			.enable		= s5pv310_clksrc_mask_fsys_ctrl,
			.ctrlbit	= (1 << 24),
		},
		.sources = &clkset_mout_corebus,
		.reg_src = { .reg = S5P_CLKSRC_FSYS, .shift = 24, .size = 1 },
		.reg_div = { .reg = S5P_CLKDIV_FSYS0, .shift = 20, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_spi",
			.id		= 0,
			.enable		= s5pv310_clksrc_mask_peril1_ctrl,
			.ctrlbit	= (1 << 16),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_PERIL1, .shift = 16, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_PERIL1, .shift = 0, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_spi",
			.id		= 1,
			.enable		= s5pv310_clksrc_mask_peril1_ctrl,
			.ctrlbit	= (1 << 20),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_PERIL1, .shift = 20, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_PERIL1, .shift = 16, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_spi",
			.id		= 2,
			.enable		= s5pv310_clksrc_mask_peril1_ctrl,
			.ctrlbit	= (1 << 24),
		},
		.sources = &clkset_group,
		.reg_src = { .reg = S5P_CLKSRC_PERIL1, .shift = 24, .size = 4 },
		.reg_div = { .reg = S5P_CLKDIV_PERIL2, .shift = 0, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_fimg2d",
			.id		= -1,
		},
		.sources = &clkset_mout_g2d,
		.reg_src = { .reg = S5P_CLKSRC_IMAGE, .shift = 8, .size = 1 },
		.reg_div = { .reg = S5P_CLKDIV_IMAGE, .shift = 0, .size = 4 },
	},
};

/* Clock initialisation code */
static struct clksrc_clk *sysclks[] = {
	&clk_mout_apll,
	&clk_sclk_apll,
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
	&clk_dout_mmc0,
	&clk_dout_mmc1,
	&clk_dout_mmc2,
	&clk_dout_mmc3,
	&clk_dout_mmc4,
	&clk_mout_audss,
	&clk_sclk_audss,
	&clk_mout_g2d0,
	&clk_mout_g2d1,
};

static int s5pv310_epll_enable(struct clk *clk, int enable)
{
	unsigned int ctrlbit = clk->ctrlbit;
	unsigned int epll_con = __raw_readl(S5P_EPLL_CON0) & ~ctrlbit;

	if (enable)
		__raw_writel(epll_con | ctrlbit, S5P_EPLL_CON0);
	else
		__raw_writel(epll_con, S5P_EPLL_CON0);

	return 0;
}

static unsigned long s5pv310_epll_get_rate(struct clk *clk)
{
	return clk->rate;
}

static u32 epll_div[][6] = {
	{  48000000, 0, 48, 3, 3, 0 },
	{  96000000, 0, 48, 3, 2, 0 },
	{ 144000000, 1, 72, 3, 2, 0 },
	{ 192000000, 0, 48, 3, 1, 0 },
	{ 288000000, 1, 72, 3, 1, 0 },
	{  84000000, 0, 42, 3, 2, 0 },
	{  50000000, 0, 50, 3, 3, 0 },
	{  80000000, 1, 80, 3, 3, 0 },
	{  32750000, 1, 65, 3, 4, 35127 },
	{  32768000, 1, 65, 3, 4, 35127 },
	{  49152000, 0, 49, 3, 3, 9961 },
	{  67737600, 1, 67, 3, 3, 48366 },
	{  73728000, 1, 73, 3, 3, 47710 },
	{  45158400, 0, 45, 3, 3, 10381 },
	{  45000000, 0, 45, 3, 3, 10355 },
	{  45158000, 0, 45, 3, 3, 10355 },
	{  49125000, 0, 49, 3, 3, 9961 },
	{  67738000, 1, 67, 3, 3, 48366 },
	{  73800000, 1, 73, 3, 3, 47710 },
	{  36000000, 1, 32, 3, 4, 0 },
	{  60000000, 1, 60, 3, 3, 0 },
	{  72000000, 1, 72, 3, 3, 0 },
};

static int s5pv310_epll_set_rate(struct clk *clk, unsigned long rate)
{
	unsigned int epll_con, epll_con_k;
	unsigned int i;

	/* Return if nothing changed */
	if (clk->rate == rate)
		return 0;

	epll_con = __raw_readl(S5P_EPLL_CON0);
	epll_con_k = __raw_readl(S5P_EPLL_CON1);

	epll_con_k &= ~(PLL90XX_KDIV_MASK);
	epll_con &= ~(PLL90XX_VDIV_MASK << 27 | \
			PLL90XX_MDIV_MASK << PLL90XX_MDIV_SHIFT |   \
			PLL90XX_PDIV_MASK << PLL90XX_PDIV_SHIFT | \
			PLL90XX_SDIV_MASK << PLL90XX_SDIV_SHIFT);

	for (i = 0; i < ARRAY_SIZE(epll_div); i++) {
		if (epll_div[i][0] == rate) {
			epll_con_k |= epll_div[i][5] << 0;
			epll_con |= epll_div[i][1] << 27;
			epll_con |= epll_div[i][2] << PLL90XX_MDIV_SHIFT;
			epll_con |= epll_div[i][3] << PLL90XX_PDIV_SHIFT;
			epll_con |= epll_div[i][4] << PLL90XX_SDIV_SHIFT;
			break;
		}
	}

	if (i == ARRAY_SIZE(epll_div)) {
		printk(KERN_ERR "%s: Invalid Clock EPLL Frequency\n",
				__func__);
		return -EINVAL;
	}

	__raw_writel(epll_con, S5P_EPLL_CON0);
	__raw_writel(epll_con_k, S5P_EPLL_CON1);

	clk->rate = rate;

	return 0;
}

static struct clk_ops s5pv310_epll_ops = {
	.get_rate = s5pv310_epll_get_rate,
	.set_rate = s5pv310_epll_set_rate,
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
	unsigned long vpllsrc;

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
				__raw_readl(S5P_EPLL_CON1), pll_4600);

	vpllsrc = clk_get_rate(&clk_vpllsrc.clk);
	vpll = s5p_get_pll46xx(vpllsrc, __raw_readl(S5P_VPLL_CON0),
				__raw_readl(S5P_VPLL_CON1), pll_4650);
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
			  aclk_100, aclk_160, aclk_133);

	clk_f.rate = armclk;
	clk_h.rate = sclk_dmc;
	clk_p.rate = aclk_100;

	for (ptr = 0; ptr < ARRAY_SIZE(clksrcs); ptr++)
		s3c_set_clksrc(&clksrcs[ptr], true);

	clk_fout_epll.enable = s5pv310_epll_enable;
	clk_fout_epll.ops = &s5pv310_epll_ops;

	clk_set_parent(&clk_sclk_audio0.clk, &clk_mout_epll.clk);
	clk_set_parent(&clk_sclk_audss.clk, &clk_mout_audss.clk);
	clk_set_parent(&clk_mout_audss.clk, &clk_fout_epll);
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
