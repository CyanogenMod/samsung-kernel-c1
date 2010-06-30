/* linux/drivers/serial/s5pv310.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Based on drivers/serial/s5pv210.c
 *
 * Driver for Samsung S5PV310 SoC UARTs.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/serial_core.h>

#include <plat/regs-serial.h>
#include "samsung.h"

static int s5pv310_serial_setsource(struct uart_port *port,
				    struct s3c24xx_uart_clksrc *clk)
{
	/* for s5pv310, serial clock source is only uclk1 */
	return 0;
};

static int s5pv310_serial_getsource(struct uart_port *port,
				    struct s3c24xx_uart_clksrc *clk)
{
	/* for s5pv310, serial clock source is only uclk1 */
	clk->divisor = 1;
#ifdef CONFIG_S5PV310_FPGA
	clk->name = "xtal";
#else
	clk->name = "uclk1";
#endif
	return 0;
};

static int s5pv310_serial_resetport(struct uart_port *port,
				    struct s3c2410_uartcfg *cfg)
{
	wr_regl(port, S3C2410_UCON,  cfg->ucon);
	wr_regl(port, S3C2410_ULCON, cfg->ulcon);

	/* reset both fifos */
	wr_regl(port, S3C2410_UFCON, cfg->ufcon | S3C2410_UFCON_RESETBOTH);
	wr_regl(port, S3C2410_UFCON, cfg->ufcon);

	return 0;
}

#define S5PV310_UART_DEFAULT_INFO(fifo_size)			\
		.name		= "Samsung S5PV310 UART",	\
		.type		= PORT_S3C6400,			\
		.fifosize	= fifo_size,			\
		.has_fracval	= 1,				\
		.rx_fifomask	= S5PV210_UFSTAT_RXMASK,	\
		.rx_fifoshift	= S5PV210_UFSTAT_RXSHIFT,	\
		.rx_fifofull	= S5PV210_UFSTAT_RXFULL,	\
		.tx_fifofull	= S5PV210_UFSTAT_TXFULL,	\
		.tx_fifomask	= S5PV210_UFSTAT_TXMASK,	\
		.tx_fifoshift	= S5PV210_UFSTAT_TXSHIFT,	\
		.get_clksrc	= s5pv310_serial_getsource,	\
		.set_clksrc	= s5pv310_serial_setsource,	\
		.reset_port	= s5pv310_serial_resetport

static struct s3c24xx_uart_info s5p_port_fifo256 = {
	S5PV310_UART_DEFAULT_INFO(256),
};

static struct s3c24xx_uart_info s5p_port_fifo64 = {
	S5PV310_UART_DEFAULT_INFO(64),
};

static struct s3c24xx_uart_info s5p_port_fifo16 = {
	S5PV310_UART_DEFAULT_INFO(16),
};

struct s3c24xx_uart_info *s5p_uart_inf[] = {
	[0] = &s5p_port_fifo256,
	[1] = &s5p_port_fifo64,
	[2] = &s5p_port_fifo16,
	[3] = &s5p_port_fifo16,
};

/* device management */
static int s5p_serial_probe(struct platform_device *pdev)
{
	return s3c24xx_serial_probe(pdev, s5p_uart_inf[pdev->id]);
}

static struct platform_driver s5p_serial_driver = {
	.probe		= s5p_serial_probe,
	.remove		= __devexit_p(s3c24xx_serial_remove),
	.driver		= {
		.name	= "s5pv310-uart",
		.owner	= THIS_MODULE,
	},
};

static int __init s5pv310_serial_console_init(void)
{
	return s3c24xx_serial_initconsole(&s5p_serial_driver, s5p_uart_inf);
}

console_initcall(s5pv310_serial_console_init);

static int __init s5p_serial_init(void)
{
	return s3c24xx_serial_init(&s5p_serial_driver, *s5p_uart_inf);
}

static void __exit s5p_serial_exit(void)
{
	platform_driver_unregister(&s5p_serial_driver);
}

module_init(s5p_serial_init);
module_exit(s5p_serial_exit);

MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:s5pv310-uart");
MODULE_DESCRIPTION("Samsung S5PV310 UART Driver support");
MODULE_AUTHOR("Changhwan Youn <chaos.youn@samsung.com>");
