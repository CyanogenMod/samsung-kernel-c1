/* linux/drivers/media/video/samsung/tv20/ddc.c
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsung.com/
 *
 * ddc i2c client driver file for Samsung TVOut driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>

#define S5P_HDCP_I2C_ADDR		0x74

const static u16 ignore[] = { I2C_CLIENT_END };
const static u16 normal_addr[] = {
	(S5P_HDCP_I2C_ADDR >> 1),
	I2C_CLIENT_END
};

const static u16 *forces[] = { NULL };

struct i2c_client *ddc_port;

int s5p_ddc_read(u8 subaddr, u8 *data, u16 len)
{
	u8 addr = subaddr;
	int ret = 0;

	struct i2c_msg msg[] = {
		[0] = {
			.addr = ddc_port->addr,
			.flags = 0,
			.len = 1,
			.buf = &addr
		},
		[1] = {
			.addr = ddc_port->addr,
			.flags = I2C_M_RD,
			.len = len,
			.buf = data
		}
	};

	if (i2c_transfer(ddc_port->adapter, msg, 2) != 2)
		ret = -EIO;

	return ret;
}

int s5p_ddc_write(u8 *data, u16 len)
{
	int ret = 0;

	if (i2c_master_send(ddc_port, (const char *) data, len) != len)
		ret = -EIO;

	return ret;
}

static int __devinit s5p_ddc_probe(struct i2c_client *client,
			const struct i2c_device_id *dev_id)
{
	int ret = 0;

	ddc_port = client;

	dev_info(&client->adapter->dev, "attached s5p_ddc "
		"into i2c adapter successfully\n");

	return ret;
}

static int s5p_ddc_remove(struct i2c_client *client)
{
	dev_info(&client->adapter->dev, "detached s5p_ddc "
		"from i2c adapter successfully\n");

	return 0;
}

static int s5p_ddc_suspend(struct i2c_client *cl, pm_message_t mesg)
{
	return 0;
};

static int s5p_ddc_resume(struct i2c_client *cl)
{
	return 0;
};

static struct i2c_device_id ddc_idtable[] = {
	{"s5p_ddc", 0},
};
MODULE_DEVICE_TABLE(i2c, ddc_idtable);

static struct i2c_driver ddc_driver = {
	.driver = {
		.name = "s5p_ddc",
	},
	.id_table 	= ddc_idtable,
	.probe 		= s5p_ddc_probe,
	.remove 	= __devexit_p(s5p_ddc_remove),
	.suspend	= s5p_ddc_suspend,
	.resume 	= s5p_ddc_resume,
};

static int __init s5p_ddc_init(void)
{
	return i2c_add_driver(&ddc_driver);
}

static void __exit s5p_ddc_exit(void)
{
	i2c_del_driver(&ddc_driver);
}

MODULE_AUTHOR("SangPil Moon <sangpil.moon@samsung.com>");
MODULE_DESCRIPTION("Driver for S5P I2C DDC devices");

MODULE_LICENSE("GPL");

module_init(s5p_ddc_init);
module_exit(s5p_ddc_exit);
