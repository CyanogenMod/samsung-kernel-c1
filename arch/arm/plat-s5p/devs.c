/* linux/arch/arm/plat-s5p/devs.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Base S5P platform device definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/io.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>
#include <mach/hardware.h>
#include <mach/map.h>
#include <mach/dma.h>

#include <plat/devs.h>
#include <plat/gpio-cfg.h>
#include <plat/irqs.h>
#include <plat/fb.h>
#include <plat/fimc.h>
#include <plat/csis.h>
#include <plat/fimg2d.h>

#if defined(CONFIG_VIDEO_MFC51) || defined(CONFIG_VIDEO_MFC50)
static struct resource s5p_mfc_resources[] = {
	[0] = {
		.start	= S5P_PA_MFC,
		.end	= S5P_PA_MFC + S5P_SZ_MFC - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_MFC,
		.end	= IRQ_MFC,
		.flags	= IORESOURCE_IRQ,
	}
};

struct platform_device s5p_device_mfc = {
	.name		= "mfc",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s5p_mfc_resources),
	.resource	= s5p_mfc_resources,
};
#endif

#ifdef CONFIG_FB_S3C
static struct resource s3cfb_resource[] = {
	[0] = {
		.start	= S5P_PA_LCD,
		.end	= S5P_PA_LCD + S5P_SZ_LCD - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_LCD1,
		.end	= IRQ_LCD1,
		.flags	= IORESOURCE_IRQ,
	},
	[2] = {
		.start	= IRQ_LCD0,
		.end	= IRQ_LCD0,
		.flags	= IORESOURCE_IRQ,
	},
};

static u64 fb_dma_mask = 0xffffffffUL;

struct platform_device s3c_device_fb = {
	.name		= "s3cfb",
#if defined(CONFIG_ARCH_S5PV310)
	.id		= 0,
#else
	.id		= -1,
#endif
	.num_resources	= ARRAY_SIZE(s3cfb_resource),
	.resource	= s3cfb_resource,
	.dev		= {
		.dma_mask		= &fb_dma_mask,
		.coherent_dma_mask	= 0xffffffffUL
	}
};

static struct s3c_platform_fb default_fb_data __initdata = {
	.hw_ver	= 0x62,
	.nr_wins	= 5,
#if defined(CONFIG_FB_S3C_DEFAULT_WINDOW)
	.default_win	= CONFIG_FB_S3C_DEFAULT_WINDOW,
#else
	.default_win	= 0,
#endif
	.swap		= FB_SWAP_WORD | FB_SWAP_HWORD,
};

void __init s3cfb_set_platdata(struct s3c_platform_fb *pd)
{
	struct s3c_platform_fb *npd;
	int i;

	if (!pd)
		pd = &default_fb_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fb), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		for (i = 0; i < npd->nr_wins; i++)
			npd->nr_buffers[i] = 1;

#if defined(CONFIG_FB_S3C_NR_BUFFERS)
		npd->nr_buffers[npd->default_win] = CONFIG_FB_S3C_NR_BUFFERS;
#else
		npd->nr_buffers[npd->default_win] = 1;
#endif

		s3cfb_get_clk_name(npd->clk_name);
		npd->cfg_gpio = s3cfb_cfg_gpio;
		npd->backlight_on = s3cfb_backlight_on;
		npd->backlight_off = s3cfb_backlight_off;
		npd->lcd_on = s3cfb_lcd_on;
		npd->lcd_off = s3cfb_lcd_off;
		npd->clk_on = s3cfb_clk_on;
		npd->clk_off = s3cfb_clk_off;

		s3c_device_fb.dev.platform_data = npd;
	}
}
#endif

#ifdef CONFIG_VIDEO_FIMC
static struct resource s3c_fimc0_resource[] = {
	[0] = {
		.start	= S5P_PA_FIMC0,
		.end	= S5P_PA_FIMC0 + S5P_SZ_FIMC0 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_FIMC0,
		.end	= IRQ_FIMC0,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_fimc0 = {
	.name		= "s3c-fimc",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s3c_fimc0_resource),
	.resource	= s3c_fimc0_resource,
};

static struct s3c_platform_fimc default_fimc0_data __initdata = {
	.default_cam	= CAMERA_PAR_A,
#if defined(CONFIG_CPU_S5PV210_EVT1)
	.hw_ver	= 0x45,
#elif defined(CONFIG_CPU_S5P6450)
	.hw_ver = 0x50,
#elif defined(CONFIG_CPU_S5PV310)
	.hw_ver = 0x51,
#else
	.hw_ver	= 0x43,
#endif
};

void __init s3c_fimc0_set_platdata(struct s3c_platform_fimc *pd)
{
	struct s3c_platform_fimc *npd;

	if (!pd)
		pd = &default_fimc0_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fimc), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		if (!npd->cfg_gpio)
			npd->cfg_gpio = s3c_fimc0_cfg_gpio;

		if (!npd->clk_on)
			npd->clk_on = s3c_fimc_clk_on;

		if (!npd->clk_off)
			npd->clk_off = s3c_fimc_clk_off;
#if defined(CONFIG_CPU_S5PV210_EVT1)
		npd->hw_ver = 0x45;
#elif defined(CONFIG_CPU_S5P6450)
		npd->hw_ver = 0x50;
#elif defined(CONFIG_CPU_S5PV310)
		npd->hw_ver = 0x51;
#else
		npd->hw_ver = 0x43;
#endif

		s3c_device_fimc0.dev.platform_data = npd;
	}
}

#if !defined(CONFIG_CPU_S5P6450)
static struct resource s3c_fimc1_resource[] = {
	[0] = {
		.start	= S5P_PA_FIMC1,
		.end	= S5P_PA_FIMC1 + S5P_SZ_FIMC1 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_FIMC1,
		.end	= IRQ_FIMC1,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_fimc1 = {
	.name		= "s3c-fimc",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(s3c_fimc1_resource),
	.resource	= s3c_fimc1_resource,
};

static struct s3c_platform_fimc default_fimc1_data __initdata = {
	.default_cam	= CAMERA_PAR_A,
#if defined(CONFIG_CPU_S5PV210_EVT1)
	.hw_ver	= 0x50,
#elif defined(CONFIG_CPU_S5PV310)
	.hw_ver = 0x51,
#else
	.hw_ver	= 0x43,
#endif
};

void __init s3c_fimc1_set_platdata(struct s3c_platform_fimc *pd)
{
	struct s3c_platform_fimc *npd;

	if (!pd)
		pd = &default_fimc1_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fimc), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		if (!npd->cfg_gpio)
			npd->cfg_gpio = s3c_fimc1_cfg_gpio;

		if (!npd->clk_on)
			npd->clk_on = s3c_fimc_clk_on;

		if (!npd->clk_off)
			npd->clk_off = s3c_fimc_clk_off;
#if defined(CONFIG_CPU_S5PV210_EVT1)
		npd->hw_ver = 0x50;
#elif defined(CONFIG_CPU_S5PV310)
		npd->hw_ver = 0x51;
#else
		npd->hw_ver = 0x43;
#endif

		s3c_device_fimc1.dev.platform_data = npd;
	}
}

static struct resource s3c_fimc2_resource[] = {
	[0] = {
		.start	= S5P_PA_FIMC2,
		.end	= S5P_PA_FIMC2 + S5P_SZ_FIMC2 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_FIMC2,
		.end	= IRQ_FIMC2,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_fimc2 = {
	.name		= "s3c-fimc",
	.id		= 2,
	.num_resources	= ARRAY_SIZE(s3c_fimc2_resource),
	.resource	= s3c_fimc2_resource,
};

static struct s3c_platform_fimc default_fimc2_data __initdata = {
	.default_cam	= CAMERA_PAR_A,
#if defined(CONFIG_CPU_S5PV210_EVT1)
	.hw_ver	= 0x45,
#elif defined(CONFIG_CPU_S5PV310)
	.hw_ver	= 0x51,
#else
	.hw_ver	= 0x43,
#endif
};

void __init s3c_fimc2_set_platdata(struct s3c_platform_fimc *pd)
{
	struct s3c_platform_fimc *npd;

	if (!pd)
		pd = &default_fimc2_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fimc), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		if (!npd->cfg_gpio)
			npd->cfg_gpio = s3c_fimc2_cfg_gpio;

		if (!npd->clk_on)
			npd->clk_on = s3c_fimc_clk_on;

		if (!npd->clk_off)
			npd->clk_off = s3c_fimc_clk_off;
#if defined(CONFIG_CPU_S5PV210_EVT1)
		npd->hw_ver = 0x45;
#elif defined(CONFIG_CPU_S5PV310)
		npd->hw_ver = 0x51;
#else
		npd->hw_ver = 0x43;
#endif

		s3c_device_fimc2.dev.platform_data = npd;
	}
}

#endif
#if defined(CONFIG_CPU_S5PV310)
static struct resource s3c_fimc3_resource[] = {
	[0] = {
		.start	= S5P_PA_FIMC3,
		.end	= S5P_PA_FIMC3 + S5P_SZ_FIMC3 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_FIMC3,
		.end	= IRQ_FIMC3,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_fimc3 = {
	.name		= "s3c-fimc",
	.id		= 3,
	.num_resources	= ARRAY_SIZE(s3c_fimc3_resource),
	.resource	= s3c_fimc3_resource,
};

static struct s3c_platform_fimc default_fimc3_data __initdata = {
	.default_cam	= CAMERA_PAR_A,
	.hw_ver	= 0x51,
};

void __init s3c_fimc3_set_platdata(struct s3c_platform_fimc *pd)
{
	struct s3c_platform_fimc *npd;

	if (!pd)
		pd = &default_fimc3_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fimc), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		if (!npd->cfg_gpio)
			npd->cfg_gpio = s3c_fimc3_cfg_gpio;

		if (!npd->clk_on)
			npd->clk_on = s3c_fimc_clk_on;

		if (!npd->clk_off)
			npd->clk_off = s3c_fimc_clk_off;
		npd->hw_ver = 0x51;

		s3c_device_fimc3.dev.platform_data = npd;
	}
}
#endif
#ifdef CONFIG_VIDEO_FIMC_MIPI
#if defined(CONFIG_CPU_S5PV310)
static struct resource s3c_csis0_resource[] = {
	[0] = {
		.start	= S5P_PA_CSIS0,
		.end	= S5P_PA_CSIS0 + S5P_SZ_CSIS0 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_MIPICSI0,
		.end	= IRQ_MIPICSI0,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_csis0 = {
	.name		= "s3c-csis",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s3c_csis0_resource),
	.resource	= s3c_csis0_resource,
};

static struct s3c_platform_csis default_csis0_data __initdata = {
	.srclk_name	= "mout_mpll",
	.clk_name	= "sclk_csis",
	.clk_rate	= 166000000,
};

void __init s3c_csis0_set_platdata(struct s3c_platform_csis *pd)
{
	struct s3c_platform_csis *npd;

	if (!pd)
		pd = &default_csis0_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_csis), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);

	npd->cfg_gpio = s3c_csis0_cfg_gpio;
	npd->cfg_phy_global = s3c_csis0_cfg_phy_global;
	s3c_device_csis0.dev.platform_data = npd;
}
static struct resource s3c_csis1_resource[] = {
	[0] = {
		.start	= S5P_PA_CSIS1,
		.end	= S5P_PA_CSIS1 + S5P_SZ_CSIS0 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_MIPICSI1,
		.end	= IRQ_MIPICSI1,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_csis1 = {
	.name		= "s3c-csis",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s3c_csis1_resource),
	.resource	= s3c_csis1_resource,
};

static struct s3c_platform_csis default_csis1_data __initdata = {
	.srclk_name	= "mout_mpll",
	.clk_name	= "sclk_csis",
	.clk_rate	= 166000000,
};

void __init s3c_csis1_set_platdata(struct s3c_platform_csis *pd)
{
	struct s3c_platform_csis *npd;

	if (!pd)
		pd = &default_csis1_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_csis), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);

	npd->cfg_gpio = s3c_csis1_cfg_gpio;
	npd->cfg_phy_global = s3c_csis1_cfg_phy_global;
	s3c_device_csis1.dev.platform_data = npd;
}
#else
static struct resource s3c_csis_resource[] = {
	[0] = {
		.start	= S5P_PA_CSIS,
		.end	= S5P_PA_CSIS + S5P_SZ_CSIS - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_MIPICSI,
		.end	= IRQ_MIPICSI,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_csis = {
	.name		= "s3c-csis",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s3c_csis_resource),
	.resource	= s3c_csis_resource,
};

static struct s3c_platform_csis default_csis_data __initdata = {
	.srclk_name	= "mout_mpll",
	.clk_name	= "sclk_csis",
	.clk_rate	= 166000000,
};

void __init s3c_csis_set_platdata(struct s3c_platform_csis *pd)
{
	struct s3c_platform_csis *npd;

	if (!pd)
		pd = &default_csis_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_csis), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);

	npd->cfg_gpio = s3c_csis_cfg_gpio;
	npd->cfg_phy_global = s3c_csis_cfg_phy_global;

	s3c_device_csis.dev.platform_data = npd;
}
#endif
#endif
#endif /* CONFIG_VIDEO_FIMC */

#ifdef CONFIG_VIDEO_JPEG_V2
/* JPEG controller  */
static struct resource s3c_jpeg_resource[] = {
	[0] = {
		.start = S5PV210_PA_JPEG,
		.end   = S5PV210_PA_JPEG + S5PV210_SZ_JPEG - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_JPEG,
		.end   = IRQ_JPEG,
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device s3c_device_jpeg = {
	.name             = "s3c-jpg",
	.id               = -1,
	.num_resources    = ARRAY_SIZE(s3c_jpeg_resource),
	.resource         = s3c_jpeg_resource,
};
EXPORT_SYMBOL(s3c_device_jpeg);
#endif /* CONFIG_VIDEO_JPEG_V2 */

#ifdef CONFIG_VIDEO_ROTATOR
/* rotator interface */
static struct resource s5p_rotator_resource[] = {
	[0] = {
		.start = S5P_PA_ROTATOR,
		.end   = S5P_PA_ROTATOR + S5P_SZ_ROTATOR - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_ROTATOR,
		.end   = IRQ_ROTATOR,
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device s5p_device_rotator = {
	.name		= "s5p-rotator",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s5p_rotator_resource),
	.resource	= s5p_rotator_resource
};
EXPORT_SYMBOL(s5p_device_rotator);
#endif

#ifdef CONFIG_VIDEO_TV20
/* TVOUT interface */
static struct resource s5p_tv_resources[] = {
	[0] = {
		.start  = S5P_PA_TVENC,
		.end    = S5P_PA_TVENC + S5P_SZ_TVENC - 1,
		.flags  = IORESOURCE_MEM,
		.name	= "s5p-sdo"
	},
	[1] = {
		.start  = S5P_PA_VP,
		.end    = S5P_PA_VP + S5P_SZ_VP - 1,
		.flags  = IORESOURCE_MEM,
		.name	= "s5p-vp"
	},
	[2] = {
		.start  = S5P_PA_MIXER,
		.end    = S5P_PA_MIXER + S5P_SZ_MIXER - 1,
		.flags  = IORESOURCE_MEM,
		.name	= "s5p-mixer"
	},
	[3] = {
		.start  = S5P_PA_HDMI,
		.end    = S5P_PA_HDMI + S5P_SZ_HDMI - 1,
		.flags  = IORESOURCE_MEM,
		.name	= "s5p-hdmi"
	},
	[4] = {
		.start  = S5P_I2C_HDMI_PHY,
		.end    = S5P_I2C_HDMI_PHY + S5P_I2C_HDMI_SZ_PHY - 1,
		.flags  = IORESOURCE_MEM,
		.name	= "s5p-i2c-hdmi-phy"
	},
	[5] = {
		.start  = IRQ_MIXER,
		.end    = IRQ_MIXER,
		.flags  = IORESOURCE_IRQ,
		.name	= "s5p-mixer"
	},
	[6] = {
		.start  = IRQ_HDMI,
		.end    = IRQ_HDMI,
		.flags  = IORESOURCE_IRQ,
		.name	= "s5p-hdmi"
	},
	[7] = {
		.start  = IRQ_TVENC,
		.end    = IRQ_TVENC,
		.flags  = IORESOURCE_IRQ,
		.name	= "s5p-sdo"
	},
};

struct platform_device s5p_device_tvout = {
	.name           = "s5p-tv",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(s5p_tv_resources),
	.resource       = s5p_tv_resources,
};
EXPORT_SYMBOL(s5p_device_tvout);

/* CEC */
static struct resource s5p_cec_resources[] = {
	[0] = {
		.start  = S5P_PA_CEC,
		.end    = S5P_PA_CEC + S5P_SZ_CEC - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_CEC,
		.end    = IRQ_CEC,
		.flags  = IORESOURCE_IRQ,
	},
};

struct platform_device s5p_device_cec = {
	.name           = "s5p-cec",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(s5p_cec_resources),
	.resource       = s5p_cec_resources,
};
EXPORT_SYMBOL(s5p_device_cec);

/* HPD */
struct platform_device s5p_device_hpd = {
	.name           = "s5p-hpd",
	.id             = -1,
};
EXPORT_SYMBOL(s5p_device_hpd);
#endif

#ifdef CONFIG_USB_SUPPORT
#ifdef CONFIG_USB_ARCH_HAS_EHCI
 /* USB Host Controlle EHCI registrations */
static struct resource s3c_usb__ehci_resource[] = {
	[0] = {
		.start = S5P_PA_USB_EHCI,
		.end   = S5P_PA_USB_EHCI  + S5P_SZ_USB_EHCI - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_UHOST,
		.end   = IRQ_UHOST,
		.flags = IORESOURCE_IRQ,
	}
};

static u64 s3c_device_usb_ehci_dmamask = 0xffffffffUL;

struct platform_device s3c_device_usb_ehci = {
	.name		= "s5p-ehci",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_usb__ehci_resource),
	.resource	= s3c_usb__ehci_resource,
	.dev		= {
		.dma_mask = &s3c_device_usb_ehci_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	}
};
EXPORT_SYMBOL(s3c_device_usb_ehci);
#endif /* CONFIG_USB_ARCH_HAS_EHCI */

#ifdef CONFIG_USB_ARCH_HAS_OHCI
/* USB Host Controlle OHCI registrations */
static struct resource s3c_usb__ohci_resource[] = {
	[0] = {
		.start = S5P_PA_USB_OHCI,
		.end   = S5P_PA_USB_OHCI  + S5P_SZ_USB_OHCI - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_UHOST,
		.end   = IRQ_UHOST,
		.flags = IORESOURCE_IRQ,
	}
};

static u64 s3c_device_usb_ohci_dmamask = 0xffffffffUL;

struct platform_device s3c_device_usb_ohci = {
	.name		= "s5p-ohci",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_usb__ohci_resource),
	.resource	= s3c_usb__ohci_resource,
	.dev		= {
		.dma_mask = &s3c_device_usb_ohci_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	}
};
EXPORT_SYMBOL(s3c_device_usb_ohci);
#endif /* CONFIG_USB_ARCH_HAS_EHCI */

/* USB Device (Gadget)*/
static struct resource s3c_usbgadget_resource[] = {
	[0] = {
		.start	= S5P_PA_OTG,
		.end	= S5P_PA_OTG + S5P_SZ_OTG - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_OTG,
		.end	= IRQ_OTG,
		.flags	= IORESOURCE_IRQ,
	}
};

struct platform_device s3c_device_usbgadget = {
	.name		= "s3c-usbgadget",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_usbgadget_resource),
	.resource	= s3c_usbgadget_resource,
};
EXPORT_SYMBOL(s3c_device_usbgadget);
#endif

#ifdef CONFIG_VIDEO_FIMG2D
static struct resource s5p_fimg2d_resource[] = {
	[0] = {
		.start	= S5P_PA_FIMG2D,
		.end	= S5P_PA_FIMG2D + S5P_SZ_FIMG2D - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_FIMG2D,
		.end	= IRQ_FIMG2D,
		.flags	= IORESOURCE_IRQ,
	}
};

struct platform_device s5p_device_fimg2d = {
	.name		= "s5p-fimg2d",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s5p_fimg2d_resource),
	.resource	= s5p_fimg2d_resource
};
EXPORT_SYMBOL(s5p_device_fimg2d);

static struct fimg2d_platdata default_fimg2d_data __initdata = {
	.parent_clkname = "mout_g2d0",
	.clkname = "sclk_fimg2d",
	.gate_clkname = "fimg2d",
	.clkrate = 250 * 1000000,
};

void __init s5p_fimg2d_set_platdata(struct fimg2d_platdata *pd)
{
	struct fimg2d_platdata *npd;

	if (!pd)
		pd = &default_fimg2d_data;

	npd = kmemdup(pd, sizeof(*pd), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "no memory for fimg2d platform data\n");
	else
		s5p_device_fimg2d.dev.platform_data = npd;
}
#endif

#ifdef CONFIG_S5P_SYSMMU
/* SYSMMU Device */
static struct resource s5p_sysmmu_resource[] = {
	[0] = {
		.start	= S5P_PA_SYSMMU_MDMA,
		.end	= S5P_PA_SYSMMU_MDMA + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_SYSMMU_MDMA0_0,
		.end	= IRQ_SYSMMU_MDMA0_0,
		.flags	= IORESOURCE_IRQ,
	},
	[2] = {
		.start	= S5P_PA_SYSMMU_SSS,
		.end	= S5P_PA_SYSMMU_SSS + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[3] = {
		.start	= IRQ_SYSMMU_SSS_0,
		.end	= IRQ_SYSMMU_SSS_0,
		.flags	= IORESOURCE_IRQ,
	},
	[4] = {
		.start	= S5P_PA_SYSMMU_FIMC0,
		.end	= S5P_PA_SYSMMU_FIMC0 + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[5] = {
		.start	= IRQ_SYSMMU_FIMC0_0,
		.end	= IRQ_SYSMMU_FIMC0_0,
		.flags	= IORESOURCE_IRQ,
	},
	[6] = {
		.start	= S5P_PA_SYSMMU_FIMC1,
		.end	= S5P_PA_SYSMMU_FIMC1 + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[7] = {
		.start	= IRQ_SYSMMU_FIMC1_0,
		.end	= IRQ_SYSMMU_FIMC1_0,
		.flags	= IORESOURCE_IRQ,
	},
	[8] = {
		.start	= S5P_PA_SYSMMU_FIMC2,
		.end	= S5P_PA_SYSMMU_FIMC2 + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[9] = {
		.start	= IRQ_SYSMMU_FIMC2_0,
		.end	= IRQ_SYSMMU_FIMC2_0,
		.flags	= IORESOURCE_IRQ,
	},
	[10] = {
		.start	= S5P_PA_SYSMMU_FIMC3,
		.end	= S5P_PA_SYSMMU_FIMC3 + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[11] = {
		.start	= IRQ_SYSMMU_FIMC3_0,
		.end	= IRQ_SYSMMU_FIMC3_0,
		.flags	= IORESOURCE_IRQ,
	},
	[12] = {
		.start	= S5P_PA_SYSMMU_JPEG,
		.end	= S5P_PA_SYSMMU_JPEG + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[13] = {
		.start	= IRQ_SYSMMU_JPEG_0,
		.end	= IRQ_SYSMMU_JPEG_0,
		.flags	= IORESOURCE_IRQ,
	},
	[14] = {
		.start	= S5P_PA_SYSMMU_FIMD0,
		.end	= S5P_PA_SYSMMU_FIMD0 + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[15] = {
		.start	= IRQ_SYSMMU_LCD0_M0_0,
		.end	= IRQ_SYSMMU_LCD0_M0_0,
		.flags	= IORESOURCE_IRQ,
	},
	[16] = {
		.start	= S5P_PA_SYSMMU_FIMD1,
		.end	= S5P_PA_SYSMMU_FIMD1 + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[17] = {
		.start	= IRQ_SYSMMU_LCD1_M1_0,
		.end	= IRQ_SYSMMU_LCD1_M1_0,
		.flags	= IORESOURCE_IRQ,
	},
	[18] = {
		.start	= S5P_PA_SYSMMU_PCIe,
		.end	= S5P_PA_SYSMMU_PCIe + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[19] = {
		.start	= IRQ_SYSMMU_PCIE_0,
		.end	= IRQ_SYSMMU_PCIE_0,
		.flags	= IORESOURCE_IRQ,
	},
	[20] = {
		.start	= S5P_PA_SYSMMU_G2D,
		.end	= S5P_PA_SYSMMU_G2D + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[21] = {
		.start	= IRQ_SYSMMU_2D_0,
		.end	= IRQ_SYSMMU_2D_0,
		.flags	= IORESOURCE_IRQ,
	},
	[22] = {
		.start	= S5P_PA_SYSMMU_ROTATOR,
		.end	= S5P_PA_SYSMMU_ROTATOR + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[23] = {
		.start	= IRQ_SYSMMU_ROTATOR_0,
		.end	= IRQ_SYSMMU_ROTATOR_0,
		.flags	= IORESOURCE_IRQ,
	},
	[24] = {
		.start	= S5P_PA_SYSMMU_MDMA2,
		.end	= S5P_PA_SYSMMU_MDMA2 + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[25] = {
		.start	= IRQ_SYSMMU_MDMA1_0,
		.end	= IRQ_SYSMMU_MDMA1_0,
		.flags	= IORESOURCE_IRQ,
	},
	[26] = {
		.start	= S5P_PA_SYSMMU_TV,
		.end	= S5P_PA_SYSMMU_TV + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[27] = {
		.start	= IRQ_SYSMMU_TV_M0_0,
		.end	= IRQ_SYSMMU_TV_M0_0,
		.flags	= IORESOURCE_IRQ,
	},
	[28] = {
		.start	= S5P_PA_SYSMMU_MFC_L,
		.end	= S5P_PA_SYSMMU_MFC_L + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[29] = {
		.start	= IRQ_SYSMMU_MFC_M0_0,
		.end	= IRQ_SYSMMU_MFC_M0_0,
		.flags	= IORESOURCE_IRQ,
	},
	[30] = {
		.start	= S5P_PA_SYSMMU_MFC_R,
		.end	= S5P_PA_SYSMMU_MFC_R + S5P_SZ_SYSMMU - 1,
		.flags	= IORESOURCE_MEM,
	},
	[31] = {
		.start	= IRQ_SYSMMU_MFC_M1_0,
		.end	= IRQ_SYSMMU_MFC_M1_0,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s5p_device_sysmmu = {
	.name		= "s5p-sysmmu",
	.id		= 32,
	.num_resources	= ARRAY_SIZE(s5p_sysmmu_resource),
	.resource	= s5p_sysmmu_resource,
};
EXPORT_SYMBOL(s5p_device_sysmmu);
#endif
