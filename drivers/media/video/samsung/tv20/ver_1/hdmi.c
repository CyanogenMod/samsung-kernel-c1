/* linux/drivers/media/video/samsung/tv20/ver_1/hdmi.c
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsung.com/
 *
 * hdmi raw ftn  file for Samsung TVOut driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/slab.h>

#include <plat/clock.h>

#include <mach/regs-hdmi.h>

#include "tv_out.h"
#include "../hpd.h"

#include "hdmi_data.c"

#ifdef COFIG_TVOUT_RAW_DBG
#define S5P_HDMI_DEBUG 1
#endif

#ifdef S5P_HDMI_DEBUG
#define HDMIPRINTK(fmt, args...) \
	printk(KERN_INFO "\t\t[HDMI] %s: " fmt, __func__, ## args)
#else
#define HDMIPRINTK(fmt, args...)
#endif

#define PHY_I2C_ADDRESS       	0x70
#define PHY_REG_MODE_SET_DONE 	0x1F

hdmi_isr hdmi_isr_ftn[HDMI_IRQ_TOTAL_NUM];

static struct resource	*hdmi_mem;
void __iomem		*hdmi_base;

static struct resource	*i2c_hdmi_phy_mem;
void __iomem		*i2c_hdmi_phy_base;

static struct {
	s32    state;
	u8 	*buffer;
	s32    bytes;
} i2c_hdmi_phy_context;

spinlock_t 	lock_hdmi;

static s32 s5p_hdmi_i2c_phy_interruptwait(void)
{
	u8 status, reg;
	s32 retval = 0;

	do {
		status = readb(i2c_hdmi_phy_base + HDMI_I2C_CON);

		if (status & I2C_PEND) {
			reg = readb(i2c_hdmi_phy_base + HDMI_I2C_STAT);
			break;
		}

	} while (1);

	return retval;
}

static s32 s5p_hdmi_i2c_phy_read(u8 addr, u8 nbytes, u8 *buffer)
{
	u8 reg;
	s32 ret = 0;
	u32 proc = true;

	i2c_hdmi_phy_context.state = STATE_RX_ADDR;
	i2c_hdmi_phy_context.buffer = buffer;
	i2c_hdmi_phy_context.bytes = nbytes;

	writeb(I2C_CLK | I2C_INT | I2C_ACK, i2c_hdmi_phy_base + HDMI_I2C_CON);
	writeb(I2C_ENABLE | I2C_MODE_MRX, i2c_hdmi_phy_base + HDMI_I2C_STAT);
	writeb(addr & 0xFE, i2c_hdmi_phy_base + HDMI_I2C_DS);
	writeb(I2C_ENABLE | I2C_START | I2C_MODE_MRX,
				i2c_hdmi_phy_base + HDMI_I2C_STAT);

	while (proc) {

		if (i2c_hdmi_phy_context.state != STATE_RX_STOP) {

			if (s5p_hdmi_i2c_phy_interruptwait() != 0) {
				HDMIPRINTK("interrupt wait failed!!!\n");
				ret = EINVAL;
				break;
			}

		}

		switch (i2c_hdmi_phy_context.state) {
		case STATE_RX_DATA:
			reg = readb(i2c_hdmi_phy_base + HDMI_I2C_DS);
			*(i2c_hdmi_phy_context.buffer) = reg;

			i2c_hdmi_phy_context.buffer++;
			--(i2c_hdmi_phy_context.bytes);

			if (i2c_hdmi_phy_context.bytes == 1) {
				i2c_hdmi_phy_context.state = STATE_RX_STOP;
				writeb(I2C_CLK_PEND_INT,
					i2c_hdmi_phy_base + HDMI_I2C_CON);
			} else {
				writeb(I2C_CLK_PEND_INT | I2C_ACK,
					i2c_hdmi_phy_base + HDMI_I2C_CON);
			}

			break;

		case STATE_RX_ADDR:
			i2c_hdmi_phy_context.state = STATE_RX_DATA;

			if (i2c_hdmi_phy_context.bytes == 1) {
				i2c_hdmi_phy_context.state = STATE_RX_STOP;
				writeb(I2C_CLK_PEND_INT,
					i2c_hdmi_phy_base + HDMI_I2C_CON);
			} else {
				writeb(I2C_CLK_PEND_INT | I2C_ACK,
					i2c_hdmi_phy_base + HDMI_I2C_CON);
			}

			break;

		case STATE_RX_STOP:
			i2c_hdmi_phy_context.state = STATE_IDLE;

			reg = readb(i2c_hdmi_phy_base + HDMI_I2C_DS);

			*(i2c_hdmi_phy_context.buffer) = reg;

			writeb(I2C_MODE_MRX|I2C_ENABLE,
				i2c_hdmi_phy_base + HDMI_I2C_STAT);
			writeb(I2C_CLK_PEND_INT,
				i2c_hdmi_phy_base + HDMI_I2C_CON);
			writeb(I2C_MODE_MRX,
				i2c_hdmi_phy_base + HDMI_I2C_STAT);

			while (readb(i2c_hdmi_phy_base + HDMI_I2C_STAT) &
					I2C_START)
				msleep(1);

			proc = false;
			break;

		case STATE_IDLE:
		default:
			HDMIPRINTK("error state!!!\n");

			ret = EINVAL;

			proc = false;
			break;
		}

	}

	return ret;
}

static s32 s5p_hdmi_i2c_phy_write(u8 addr, u8 nbytes, u8 *buffer)
{
	u8 reg;
	s32 ret = 0;
	u32 proc = true;

	i2c_hdmi_phy_context.state = STATE_TX_ADDR;
	i2c_hdmi_phy_context.buffer = buffer;
	i2c_hdmi_phy_context.bytes = nbytes;

	writeb(I2C_CLK | I2C_INT | I2C_ACK, i2c_hdmi_phy_base + HDMI_I2C_CON);
	writeb(I2C_ENABLE | I2C_MODE_MTX, i2c_hdmi_phy_base + HDMI_I2C_STAT);
	writeb(addr & 0xFE, i2c_hdmi_phy_base + HDMI_I2C_DS);
	writeb(I2C_ENABLE | I2C_START | I2C_MODE_MTX,
				i2c_hdmi_phy_base + HDMI_I2C_STAT);

	while (proc) {

		if (s5p_hdmi_i2c_phy_interruptwait() != 0) {
			HDMIPRINTK("interrupt wait failed!!!\n");
			ret = EINVAL;

			break;
		}

		switch (i2c_hdmi_phy_context.state) {
		case STATE_TX_ADDR:
		case STATE_TX_DATA:
			i2c_hdmi_phy_context.state = STATE_TX_DATA;

			reg = *(i2c_hdmi_phy_context.buffer);

			writeb(reg, i2c_hdmi_phy_base + HDMI_I2C_DS);

			i2c_hdmi_phy_context.buffer++;
			--(i2c_hdmi_phy_context.bytes);

			if (i2c_hdmi_phy_context.bytes == 0) {
				i2c_hdmi_phy_context.state = STATE_TX_STOP;
				writeb(I2C_CLK_PEND_INT,
					i2c_hdmi_phy_base + HDMI_I2C_CON);
			} else {
				writeb(I2C_CLK_PEND_INT | I2C_ACK,
					i2c_hdmi_phy_base + HDMI_I2C_CON);
			}

			break;

		case STATE_TX_STOP:
			i2c_hdmi_phy_context.state = STATE_IDLE;

			writeb(I2C_MODE_MTX | I2C_ENABLE,
				i2c_hdmi_phy_base + HDMI_I2C_STAT);
			writeb(I2C_CLK_PEND_INT,
				i2c_hdmi_phy_base + HDMI_I2C_CON);
			writeb(I2C_MODE_MTX,
				i2c_hdmi_phy_base + HDMI_I2C_STAT);

			while (readb(i2c_hdmi_phy_base + HDMI_I2C_STAT) &
					I2C_START)
				msleep(1);

			proc = false;
			break;

		case STATE_IDLE:
		default:
			HDMIPRINTK("error state!!!\n");

			ret = EINVAL;

			proc = false;
			break;
		}

	}

	return ret;
}

static int s5p_hdmi_phy_down(bool on, u8 addr, u8 offset, u8 *read_buffer)
{
	u8 buff[2] = {0};

	buff[0] = addr;
	buff[1] = (on) ? (read_buffer[addr] & (~(1 << offset))) :
			(read_buffer[addr] | (1 << offset));

	if (s5p_hdmi_i2c_phy_write(PHY_I2C_ADDRESS, 2, buff) != 0)
		return EINVAL;

	return 0;
}

int s5p_hdmi_phy_power(bool on)
{
	u32 size;
	u8 *buffer;
	u8 read_buffer[0x40] = {0, };

	size = sizeof(phy_config[0][0])
		/ sizeof(phy_config[0][0][0]);

	buffer = (u8 *) phy_config[0][0];

	/* write offset */
	if (s5p_hdmi_i2c_phy_write(PHY_I2C_ADDRESS, 1, buffer) != 0)
		return EINVAL;

	/* read data */
	if (s5p_hdmi_i2c_phy_read(PHY_I2C_ADDRESS, size, read_buffer) != 0) {
		HDMIPRINTK("s5p_hdmi_i2c_phy_read failed.\n");

		return EINVAL;
	}

	/* i can't get the information about phy setting */
	if (on) {
		/* on */
		/* biaspd */
		s5p_hdmi_phy_down(true, 0x1, 0x5, read_buffer);
		/* clockgenpd */
		s5p_hdmi_phy_down(true, 0x1, 0x7, read_buffer);
		/* pllpd */
		s5p_hdmi_phy_down(true, 0x5, 0x5, read_buffer);
		/* pcgpd */
		s5p_hdmi_phy_down(true, 0x17, 0x0, read_buffer);
		/* txpd */
		s5p_hdmi_phy_down(true, 0x17, 0x1, read_buffer);
	} else {
		/* off */
		/* biaspd */
		s5p_hdmi_phy_down(false, 0x1, 0x5, read_buffer);
		/* clockgenpd */
		s5p_hdmi_phy_down(false, 0x1, 0x7, read_buffer);
		/* pllpd */
		s5p_hdmi_phy_down(false, 0x5, 0x5, read_buffer);
		/* pcgpd */
		s5p_hdmi_phy_down(false, 0x17, 0x0, read_buffer);
		/* txpd */
		s5p_hdmi_phy_down(false, 0x17, 0x1, read_buffer);
	}

	return 0;
}

static s32 s5p_hdmi_corereset(void)
{
	writeb(0x0, hdmi_base + S5P_HDMI_CORE_RSTOUT);

	mdelay(10);

	writeb(0x1, hdmi_base + S5P_HDMI_CORE_RSTOUT);

	return 0;
}

static s32 s5p_hdmi_phy_config(enum phy_freq freq, enum s5p_hdmi_color_depth cd)
{
	s32 index;
	s32 size;
	u8 buffer[32] = {0, };
	u8 reg;

	switch (cd) {
	case HDMI_CD_24:
		index = 0;
		break;

	case HDMI_CD_30:
		index = 1;
		break;

	case HDMI_CD_36:
		index = 2;
		break;

	default:
		return false;
	}

	/* i2c_hdmi init - set i2c filtering */
	buffer[0] = PHY_REG_MODE_SET_DONE;
	buffer[1] = 0x00;

	if (s5p_hdmi_i2c_phy_write(PHY_I2C_ADDRESS, 2, buffer) != 0) {
		HDMIPRINTK("s5p_hdmi_i2c_phy_write failed.\n");

		return EINVAL;
	}

	writeb(0x5, i2c_hdmi_phy_base + HDMI_I2C_LC);

	size = sizeof(phy_config[freq][index])
		/ sizeof(phy_config[freq][index][0]);

	memcpy(buffer, phy_config[freq][index], sizeof(buffer));

	if (s5p_hdmi_i2c_phy_write(PHY_I2C_ADDRESS, size, buffer) != 0)
		return EINVAL;

	/* write offset */
	buffer[0] = 0x01;

	if (s5p_hdmi_i2c_phy_write(PHY_I2C_ADDRESS, 1, buffer) != 0) {
		HDMIPRINTK("s5p_hdmi_i2c_phy_write failed.\n");

		return EINVAL;
	}

#ifdef S5P_HDMI_DEBUG
{
	int i = 0;
	u8 read_buffer[0x40] = {0, };

	/* read data */
	if (s5p_hdmi_i2c_phy_read(PHY_I2C_ADDRESS, size, read_buffer) != 0) {
		HDMIPRINTK("s5p_hdmi_i2c_phy_read failed.\n");

		return EINVAL;
	}

	HDMIPRINTK("read buffer :\n\t\t");

	for (i = 1; i < size; i++) {
		printk("0x%02x", read_buffer[i]);

		if (i % 8)
			printk(" ");
		else
			printk("\n\t\t");
	}
	printk("\n");
}
#endif
	s5p_hdmi_corereset();

	do {
		reg = readb(hdmi_base + S5P_HDMI_PHY_STATUS);
	} while (!(reg & S5P_HDMI_PHY_STATUS_READY));

	writeb(I2C_CLK_PEND_INT, i2c_hdmi_phy_base + HDMI_I2C_CON);
	/* disable */
	writeb(I2C_IDLE, i2c_hdmi_phy_base + HDMI_I2C_STAT);

	return 0;
}

static s32 s5p_hdmi_set_tg(enum s5p_hdmi_v_fmt mode)
{
	u16 temp_reg;
	u8 tc_cmd;

	temp_reg = hdmi_tg_param[mode].h_fsz;
	writeb(S5P_HDMI_SET_TG_H_FSZ_L(temp_reg),
			hdmi_base + S5P_HDMI_TG_H_FSZ_L);
	writeb(S5P_HDMI_SET_TG_H_FSZ_H(temp_reg),
			hdmi_base + S5P_HDMI_TG_H_FSZ_H);

	temp_reg = hdmi_tg_param[mode].hact_st;
	writeb(S5P_HDMI_SET_TG_HACT_ST_L(temp_reg),
			hdmi_base + S5P_HDMI_TG_HACT_ST_L);
	writeb(S5P_HDMI_SET_TG_HACT_ST_H(temp_reg),
			hdmi_base + S5P_HDMI_TG_HACT_ST_H);

	temp_reg = hdmi_tg_param[mode].hact_sz;
	writeb(S5P_HDMI_SET_TG_HACT_SZ_L(temp_reg),
			hdmi_base + S5P_HDMI_TG_HACT_SZ_L);
	writeb(S5P_HDMI_SET_TG_HACT_SZ_H(temp_reg),
			hdmi_base + S5P_HDMI_TG_HACT_SZ_H);

	temp_reg = hdmi_tg_param[mode].v_fsz;
	writeb(S5P_HDMI_SET_TG_V_FSZ_L(temp_reg),
			hdmi_base + S5P_HDMI_TG_V_FSZ_L);
	writeb(S5P_HDMI_SET_TG_V_FSZ_H(temp_reg),
			hdmi_base + S5P_HDMI_TG_V_FSZ_H);

	temp_reg = hdmi_tg_param[mode].vsync;
	writeb(S5P_HDMI_SET_TG_VSYNC_L(temp_reg),
			hdmi_base + S5P_HDMI_TG_VSYNC_L);
	writeb(S5P_HDMI_SET_TG_VSYNC_H(temp_reg),
			hdmi_base + S5P_HDMI_TG_VSYNC_H);

	temp_reg = hdmi_tg_param[mode].vsync2;
	writeb(S5P_HDMI_SET_TG_VSYNC2_L(temp_reg),
			hdmi_base + S5P_HDMI_TG_VSYNC2_L);
	writeb(S5P_HDMI_SET_TG_VSYNC2_H(temp_reg),
			hdmi_base + S5P_HDMI_TG_VSYNC2_H);

	temp_reg = hdmi_tg_param[mode].vact_st;
	writeb(S5P_HDMI_SET_TG_VACT_ST_L(temp_reg),
			hdmi_base + S5P_HDMI_TG_VACT_ST_L);
	writeb(S5P_HDMI_SET_TG_VACT_ST_H(temp_reg),
			hdmi_base + S5P_HDMI_TG_VACT_ST_H);

	temp_reg = hdmi_tg_param[mode].vact_sz;
	writeb(S5P_HDMI_SET_TG_VACT_SZ_L(temp_reg),
			hdmi_base + S5P_HDMI_TG_VACT_SZ_L);
	writeb(S5P_HDMI_SET_TG_VACT_SZ_H(temp_reg),
			hdmi_base + S5P_HDMI_TG_VACT_SZ_H);

	temp_reg = hdmi_tg_param[mode].field_chg;
	writeb(S5P_HDMI_SET_TG_FIELD_CHG_L(temp_reg),
			hdmi_base + S5P_HDMI_TG_FIELD_CHG_L);
	writeb(S5P_HDMI_SET_TG_FIELD_CHG_H(temp_reg),
			hdmi_base + S5P_HDMI_TG_FIELD_CHG_H);

	temp_reg = hdmi_tg_param[mode].vact_st2;
	writeb(S5P_HDMI_SET_TG_VACT_ST2_L(temp_reg),
			hdmi_base + S5P_HDMI_TG_VACT_ST2_L);
	writeb(S5P_HDMI_SET_TG_VACT_ST2_H(temp_reg),
			hdmi_base + S5P_HDMI_TG_VACT_ST2_H);

	temp_reg = hdmi_tg_param[mode].vsync_top_hdmi;
	writeb(S5P_HDMI_SET_TG_VSYNC_TOP_HDMI_L(temp_reg),
			hdmi_base + S5P_HDMI_TG_VSYNC_TOP_HDMI_L);
	writeb(S5P_HDMI_SET_TG_VSYNC_TOP_HDMI_H(temp_reg),
			hdmi_base + S5P_HDMI_TG_VSYNC_TOP_HDMI_H);

	temp_reg = hdmi_tg_param[mode].vsync_bot_hdmi;
	writeb(S5P_HDMI_SET_TG_VSYNC_BOT_HDMI_L(temp_reg),
			hdmi_base + S5P_HDMI_TG_VSYNC_BOT_HDMI_L);
	writeb(S5P_HDMI_SET_TG_VSYNC_BOT_HDMI_H(temp_reg),
			hdmi_base + S5P_HDMI_TG_VSYNC_BOT_HDMI_H);

	temp_reg = hdmi_tg_param[mode].field_top_hdmi;
	writeb(S5P_HDMI_SET_TG_FIELD_TOP_HDMI_L(temp_reg),
			hdmi_base + S5P_HDMI_TG_FIELD_TOP_HDMI_L);
	writeb(S5P_HDMI_SET_TG_FIELD_TOP_HDMI_H(temp_reg),
			hdmi_base + S5P_HDMI_TG_FIELD_TOP_HDMI_H);

	temp_reg = hdmi_tg_param[mode].field_bot_hdmi;
	writeb(S5P_HDMI_SET_TG_FIELD_BOT_HDMI_L(temp_reg),
			hdmi_base + S5P_HDMI_TG_FIELD_BOT_HDMI_L);
	writeb(S5P_HDMI_SET_TG_FIELD_BOT_HDMI_H(temp_reg),
			hdmi_base + S5P_HDMI_TG_FIELD_BOT_HDMI_H);

	tc_cmd = readb(hdmi_base + S5P_HDMI_TG_CMD);

	if (video_params[mode].interlaced == 1)
		tc_cmd |= S5P_HDMI_FIELD_EN;
	else
		tc_cmd &= S5P_HDMI_FIELD_DIS;

	writeb(tc_cmd, hdmi_base + S5P_HDMI_TG_CMD);

	return 0;
}

static s32 s5p_hdmi_set_clr_depth(enum s5p_hdmi_color_depth cd)
{
	switch (cd) {
	case HDMI_CD_48:
		writeb(S5P_HDMI_GCP_BYTE2_CD_48BPP,
			hdmi_base + S5P_HDMI_GCP_BYTE2);
		break;

	case HDMI_CD_36:
		writeb(S5P_HDMI_GCP_BYTE2_CD_36BPP,
			hdmi_base + S5P_HDMI_GCP_BYTE2);
		writeb(S5P_HDMI_DC_CTL_12,
			hdmi_base + S5P_HDMI_DC_CONTROL);
		break;

	case HDMI_CD_30:
		writeb(S5P_HDMI_GCP_BYTE2_CD_30BPP,
			hdmi_base + S5P_HDMI_GCP_BYTE2);
		writeb(S5P_HDMI_DC_CTL_10,
			hdmi_base + S5P_HDMI_DC_CONTROL);
		break;

	case HDMI_CD_24:
		writeb(S5P_HDMI_GCP_BYTE2_CD_24BPP,
			hdmi_base + S5P_HDMI_GCP_BYTE2);
		writeb(S5P_HDMI_DC_CTL_8,
			hdmi_base + S5P_HDMI_DC_CONTROL);
		writeb(S5P_HDMI_GCP_CON_NO_TRAN,
			hdmi_base + S5P_HDMI_GCP_CON);
		break;

	default:
		HDMIPRINTK("HDMI core does not support"
			"requested Deep Color mode\n");
		return -EINVAL;
	}

	return 0;
}

static s32 s5p_hdmi_set_video_mode(enum s5p_hdmi_v_fmt mode,
				enum s5p_hdmi_color_depth cd,
				enum s5p_tv_hdmi_pxl_aspect pxl_ratio,
				u8 *avidata)
{
	u8  temp_reg8;
	u16 temp_reg16;
	u32 temp_reg32, temp_sync2, temp_sync3;

	if (mode > (sizeof(video_params) / sizeof(struct hdmi_v_params)) ||
		(s32)mode < 0) {
		HDMIPRINTK("This video mode is not Supported\n");

		return -EINVAL;
	}

	s5p_hdmi_set_tg(mode);

	temp_reg16 = video_params[mode].h_blank;
	writeb(S5P_HDMI_SET_H_BLANK_0(temp_reg16),
			hdmi_base + S5P_HDMI_H_BLANK_0);
	writeb(S5P_HDMI_SET_H_BLANK_1(temp_reg16),
			hdmi_base + S5P_HDMI_H_BLANK_1);

	temp_reg32 = video_params[mode].v_blank;
	writeb(S5P_HDMI_SET_V_BLANK_0(temp_reg32),
			hdmi_base + S5P_HDMI_V_BLANK_0);
	writeb(S5P_HDMI_SET_V_BLANK_1(temp_reg32),
			hdmi_base + S5P_HDMI_V_BLANK_1);
	writeb(S5P_HDMI_SET_V_BLANK_2(temp_reg32),
			hdmi_base + S5P_HDMI_V_BLANK_2);

	temp_reg32 = video_params[mode].hvline;
	writeb(S5P_HDMI_SET_H_V_LINE_0(temp_reg32),
			hdmi_base + S5P_HDMI_H_V_LINE_0);
	writeb(S5P_HDMI_SET_H_V_LINE_1(temp_reg32),
			hdmi_base + S5P_HDMI_H_V_LINE_1);
	writeb(S5P_HDMI_SET_H_V_LINE_2(temp_reg32),
			hdmi_base + S5P_HDMI_H_V_LINE_2);

	writeb(video_params[mode].polarity, hdmi_base + S5P_HDMI_SYNC_MODE);

	temp_reg32 = video_params[mode].h_sync_gen;
	writeb(S5P_HDMI_SET_H_SYNC_GEN_0(temp_reg32),
			hdmi_base + S5P_HDMI_H_SYNC_GEN_0);
	writeb(S5P_HDMI_SET_H_SYNC_GEN_1(temp_reg32),
			hdmi_base + S5P_HDMI_H_SYNC_GEN_1);
	writeb(S5P_HDMI_SET_H_SYNC_GEN_2(temp_reg32),
			hdmi_base + S5P_HDMI_H_SYNC_GEN_2);

	temp_reg32 = video_params[mode].v_sync_gen;
	writeb(S5P_HDMI_SET_V_SYNC_GEN1_0(temp_reg32),
			hdmi_base + S5P_HDMI_V_SYNC_GEN_1_0);
	writeb(S5P_HDMI_SET_V_SYNC_GEN1_1(temp_reg32),
			hdmi_base + S5P_HDMI_V_SYNC_GEN_1_1);
	writeb(S5P_HDMI_SET_V_SYNC_GEN1_2(temp_reg32),
			hdmi_base + S5P_HDMI_V_SYNC_GEN_1_2);

	if (video_params[mode].interlaced) {
		temp_reg32 = video_params[mode].v_blank_f;
		temp_sync2 = video_params[mode].v_sync_gen2;
		temp_sync3 = video_params[mode].v_sync_gen3;

		writeb(S5P_HDMI_SET_V_BLANK_F_0(temp_reg32),
				hdmi_base + S5P_HDMI_V_BLANK_F_0);
		writeb(S5P_HDMI_SET_V_BLANK_F_1(temp_reg32),
				hdmi_base + S5P_HDMI_V_BLANK_F_1);
		writeb(S5P_HDMI_SET_V_BLANK_F_2(temp_reg32),
				hdmi_base + S5P_HDMI_V_BLANK_F_2);

		writeb(S5P_HDMI_SET_V_SYNC_GEN2_0(temp_sync2),
				hdmi_base + S5P_HDMI_V_SYNC_GEN_2_0);
		writeb(S5P_HDMI_SET_V_SYNC_GEN2_1(temp_sync2),
				hdmi_base + S5P_HDMI_V_SYNC_GEN_2_1);
		writeb(S5P_HDMI_SET_V_SYNC_GEN2_2(temp_sync2),
				hdmi_base + S5P_HDMI_V_SYNC_GEN_2_2);

		writeb(S5P_HDMI_SET_V_SYNC_GEN3_0(temp_sync3),
				hdmi_base + S5P_HDMI_V_SYNC_GEN_3_0);
		writeb(S5P_HDMI_SET_V_SYNC_GEN3_1(temp_sync3),
				hdmi_base + S5P_HDMI_V_SYNC_GEN_3_1);
		writeb(S5P_HDMI_SET_V_SYNC_GEN3_2(temp_sync3),
				hdmi_base + S5P_HDMI_V_SYNC_GEN_3_2);
	} else {
		/* progressive mode */
		writeb(0x00, hdmi_base + S5P_HDMI_V_BLANK_F_0);
		writeb(0x00, hdmi_base + S5P_HDMI_V_BLANK_F_1);
		writeb(0x00, hdmi_base + S5P_HDMI_V_BLANK_F_2);

		writeb(0x01, hdmi_base + S5P_HDMI_V_SYNC_GEN_2_0);
		writeb(0x10, hdmi_base + S5P_HDMI_V_SYNC_GEN_2_1);
		writeb(0x00, hdmi_base + S5P_HDMI_V_SYNC_GEN_2_2);

		writeb(0x01, hdmi_base + S5P_HDMI_V_SYNC_GEN_3_0);
		writeb(0x10, hdmi_base + S5P_HDMI_V_SYNC_GEN_3_1);
		writeb(0x00, hdmi_base + S5P_HDMI_V_SYNC_GEN_3_2);
	}

	writeb(video_params[mode].interlaced,
			hdmi_base + S5P_HDMI_INT_PRO_MODE);

	temp_reg8 = readb(hdmi_base + S5P_HDMI_CON_1);
	temp_reg8 &= ~S5P_HDMI_CON_PXL_REP_RATIO_MASK;

	if (video_params[mode].repetition) {
		temp_reg8 |= S5P_HDMI_DOUBLE_PIXEL_REPETITION;
		avidata[4] = S5P_HDMI_AVI_PIXEL_REPETITION_DOUBLE;
	} else {
		avidata[4] = 0;
	}

	writeb(temp_reg8, hdmi_base + S5P_HDMI_CON_1);

	if (pxl_ratio == HDMI_PIXEL_RATIO_16_9)
		avidata[3] = video_params[mode].avi_vic_16_9;
	else
		avidata[3] = video_params[mode].avi_vic;

	temp_reg8 = readb(hdmi_base + S5P_HDMI_AVI_BYTE2) &
			~(S5P_HDMI_AVI_PICTURE_ASPECT_4_3 |
			S5P_HDMI_AVI_PICTURE_ASPECT_16_9);

	if (pxl_ratio == HDMI_PIXEL_RATIO_16_9)
		temp_reg8 |= S5P_HDMI_AVI_PICTURE_ASPECT_16_9;
	else
		temp_reg8 |= S5P_HDMI_AVI_PICTURE_ASPECT_4_3;

	if (s5p_hdmi_set_clr_depth(cd) != 0) {
		HDMIPRINTK("[ERR] Can't set hdmi clr. depth.\n");

		return -EINVAL;
	}

	if (video_params[mode].interlaced == 1) {
		u32 gcp_con;

		gcp_con = readb(hdmi_base + S5P_HDMI_GCP_CON);
		gcp_con |=  S5P_HDMI_GCP_CON_EN_1ST_VSYNC |
				S5P_HDMI_GCP_CON_EN_2ST_VSYNC;
		writeb(gcp_con, hdmi_base + S5P_HDMI_GCP_CON);
	} else {
		u32 gcp_con;

		gcp_con = readb(hdmi_base + S5P_HDMI_GCP_CON);
		gcp_con &= (~(S5P_HDMI_GCP_CON_EN_1ST_VSYNC |
				S5P_HDMI_GCP_CON_EN_2ST_VSYNC));
		writeb(gcp_con, hdmi_base + S5P_HDMI_GCP_CON);
	}

	return 0;
}

void s5p_hdmi_sw_hpd_enable(bool enable)
{
	u8 reg;

	reg = readb(hdmi_base + S5P_HDMI_HPD);
	reg &= ~S5P_HDMI_HPD_SEL_I_HPD;

	if (enable)
		writeb(reg | S5P_HDMI_HPD_SEL_I_HPD, hdmi_base + S5P_HDMI_HPD);
	else
		writeb(reg, hdmi_base + S5P_HDMI_HPD);
}

void s5p_hdmi_set_hpd_onoff(bool on_off)
{
	u8 reg;

	HDMIPRINTK("hpd is %s\n\r", on_off ? "on" : "off");

	reg = readb(hdmi_base + S5P_HDMI_HPD);
	reg &= ~S5P_HDMI_SW_HPD_PLUGGED;

	if (on_off)
		writel(S5P_HDMI_SW_HPD_PLUGGED, hdmi_base + S5P_HDMI_HPD);
	else
		writel(S5P_HDMI_SW_HPD_UNPLUGGED, hdmi_base + S5P_HDMI_HPD);

}

#ifdef CONFIG_SND_S5P_SPDIF
static void s5p_hdmi_audio_set_config(enum s5p_tv_audio_codec_type audio_codec)
{
	u32 data_type = (audio_codec == PCM) ?
			S5P_HDMI_SPDIFIN_CFG_LINEAR_PCM_TYPE :
			(audio_codec == AC3) ?
				S5P_HDMI_SPDIFIN_CFG_NO_LINEAR_PCM_TYPE : 0xff;

	HDMIPRINTK("audio codec type = %s\n\r",
		(audio_codec & PCM) ? "PCM" :
		(audio_codec & AC3) ? "AC3" :
		(audio_codec & MP3) ? "MP3" :
		(audio_codec & WMA) ? "WMA" : "Unknown");

	/* open SPDIF path on HDMI_I2S */
	writeb(S5P_HDMI_I2S_CLK_EN, hdmi_base + S5P_HDMI_I2S_CLK_CON);
	writeb(readl(hdmi_base + S5P_HDMI_I2S_MUX_CON) |
		S5P_HDMI_I2S_CUV_I2S_ENABLE |
		S5P_HDMI_I2S_MUX_ENABLE,
		hdmi_base + S5P_HDMI_I2S_MUX_CON);
	writeb(S5P_HDMI_I2S_CH_ALL_EN, hdmi_base + S5P_HDMI_I2S_MUX_CH);
	writeb(S5P_HDMI_I2S_CUV_RL_EN, hdmi_base + S5P_HDMI_I2S_MUX_CUV);

	writeb(S5P_HDMI_SPDIFIN_CFG_FILTER_2_SAMPLE | data_type |
		S5P_HDMI_SPDIFIN_CFG_PCPD_MANUAL_SET |
		S5P_HDMI_SPDIFIN_CFG_WORD_LENGTH_M_SET |
		S5P_HDMI_SPDIFIN_CFG_U_V_C_P_REPORT |
		S5P_HDMI_SPDIFIN_CFG_BURST_SIZE_2 |
		S5P_HDMI_SPDIFIN_CFG_DATA_ALIGN_32BIT,
		hdmi_base + S5P_HDMI_SPDIFIN_CONFIG_1);

	writeb(S5P_HDMI_SPDIFIN_CFG2_NO_CLK_DIV,
		hdmi_base + S5P_HDMI_SPDIFIN_CONFIG_2);
}
#endif

static void s5p_hdmi_audio_set_acr(u32 sample_rate)
{
	u32 value_n = (sample_rate == 32000) ? 4096 :
		     (sample_rate == 44100) ? 6272 :
		     (sample_rate == 88200) ? 12544 :
		     (sample_rate == 176400) ? 25088 :
		     (sample_rate == 48000) ? 6144 :
		     (sample_rate == 96000) ? 12288 :
		     (sample_rate == 192000) ? 24576 : 0;

	u32 cts = (sample_rate == 32000) ? 27000 :
		      (sample_rate == 44100) ? 30000 :
		      (sample_rate == 88200) ? 30000 :
		      (sample_rate == 176400) ? 30000 :
		      (sample_rate == 48000) ? 27000 :
		      (sample_rate == 96000) ? 27000 :
		      (sample_rate == 192000) ? 27000 : 0;

	HDMIPRINTK("sample rate = %d\n\r", sample_rate);
	HDMIPRINTK("cts = %d\n\r", cts);

	writeb(S5P_HDMI_SET_ACR_N_0(value_n), hdmi_base + S5P_HDMI_ACR_N0);
	writeb(S5P_HDMI_SET_ACR_N_1(value_n), hdmi_base + S5P_HDMI_ACR_N1);
	writeb(S5P_HDMI_SET_ACR_N_2(value_n), hdmi_base + S5P_HDMI_ACR_N2);

	writeb(S5P_HDMI_SET_ACR_MCTS_0(cts), hdmi_base + S5P_HDMI_ACR_MCTS0);
	writeb(S5P_HDMI_SET_ACR_MCTS_1(cts), hdmi_base + S5P_HDMI_ACR_MCTS1);
	writeb(S5P_HDMI_SET_ACR_MCTS_2(cts), hdmi_base + S5P_HDMI_ACR_MCTS2);

	writeb(S5P_HDMI_SET_ACR_CTS_0(cts), hdmi_base + S5P_HDMI_ACR_CTS0);
	writeb(S5P_HDMI_SET_ACR_CTS_1(cts), hdmi_base + S5P_HDMI_ACR_CTS1);
	writeb(S5P_HDMI_SET_ACR_CTS_2(cts), hdmi_base + S5P_HDMI_ACR_CTS2);

	writeb(4, hdmi_base + S5P_HDMI_ACR_CON);
}

static void s5p_hdmi_audio_set_asp(void)
{
	writeb(S5P_HDMI_AUD_NO_DST_DOUBLE | S5P_HDMI_AUD_TYPE_SAMPLE |
		S5P_HDMI_AUD_MODE_TWO_CH | S5P_HDMI_AUD_SP_ALL_DIS,
		hdmi_base + S5P_HDMI_ASP_CON);
	
	writeb(S5P_HDMI_ASP_SP_FLAT_AUD_SAMPLE,
		hdmi_base + S5P_HDMI_ASP_SP_FLAT);

	writeb(S5P_HDMI_SPK0R_SEL_I_PCM0R | S5P_HDMI_SPK0L_SEL_I_PCM0L,
		hdmi_base + S5P_HDMI_ASP_CHCFG0);
	writeb(S5P_HDMI_SPK0R_SEL_I_PCM0R | S5P_HDMI_SPK0L_SEL_I_PCM0L,
		hdmi_base + S5P_HDMI_ASP_CHCFG1);
	writeb(S5P_HDMI_SPK0R_SEL_I_PCM0R | S5P_HDMI_SPK0L_SEL_I_PCM0L,
		hdmi_base + S5P_HDMI_ASP_CHCFG2);
	writeb(S5P_HDMI_SPK0R_SEL_I_PCM0R | S5P_HDMI_SPK0L_SEL_I_PCM0L,
		hdmi_base + S5P_HDMI_ASP_CHCFG3);
}

#ifdef CONFIG_SND_S5P_SPDIF
static void  s5p_hdmi_audio_clock_enable(void)
{
	writeb(S5P_HDMI_SPDIFIN_CLK_ON, hdmi_base + S5P_HDMI_SPDIFIN_CLK_CTRL);
	writeb(S5P_HDMI_SPDIFIN_STATUS_CHK_OP_MODE,
		hdmi_base + S5P_HDMI_SPDIFIN_OP_CTRL);
}

static void s5p_hdmi_audio_set_repetition_time(
				enum s5p_tv_audio_codec_type audio_codec,
				u32 bits, u32 frame_size_code)
{
	/* Only 4'b1011  24bit */
	u32 wl = 5 << 1 | 1;
	u32 rpt_cnt = (audio_codec == AC3) ? 1536 * 2 - 1 : 0;

	HDMIPRINTK("repetition count = %d\n\r", rpt_cnt);

	/* 24bit and manual mode */
	writeb(((rpt_cnt & 0xf) << 4) | wl,
		hdmi_base + S5P_HDMI_SPDIFIN_USER_VALUE_1);
	/* if PCM this value is 0 */
	writeb((rpt_cnt >> 4) & 0xff,
		hdmi_base + S5P_HDMI_SPDIFIN_USER_VALUE_2);
	/* if PCM this value is 0 */
	writeb(frame_size_code & 0xff,
		hdmi_base + S5P_HDMI_SPDIFIN_USER_VALUE_3);
	/* if PCM this value is 0 */
	writeb((frame_size_code >> 8) & 0xff,
		hdmi_base + S5P_HDMI_SPDIFIN_USER_VALUE_4);
}

static void s5p_hdmi_audio_irq_enable(u32 irq_en)
{
	writeb(irq_en, hdmi_base + S5P_HDMI_SPDIFIN_IRQ_MASK);
}
#endif

static void s5p_hdmi_audio_set_aui(enum s5p_tv_audio_codec_type audio_codec,
				u32 sample_rate,
				u32 bits)
{
	u8 sum_of_bits, bytes1, bytes2, bytes3, check_sum;
	u32 bit_rate;
#if 1
	u32 type = 0;
	u32 ch = (audio_codec == PCM) ? 1 : 0;
	u32 sample = 0;
	u32 bps_type = 0;
#else
	/* Ac3 16bit only */
	u32 bps = (audio_codec == PCM) ? bits : 16;

	/* AUI Packet set. */
	u32 type = (audio_codec == PCM) ? 1 : /* PCM */
			(audio_codec == AC3) ? 2 : 0;
					/* AC3 or Refer stream header */
	u32 ch = (audio_codec == PCM) ? 1 : 0;
					/* 2ch or refer to stream header */
	u32 sample = (sample_rate == 32000) ? 1 :
			(sample_rate == 44100) ? 2 :
			(sample_rate == 48000) ? 3 :
			(sample_rate == 88200) ? 4 :
			(sample_rate == 96000) ? 5 :
			(sample_rate == 176400) ? 6 :
			(sample_rate == 192000) ? 7 : 0;

	u32 bps_type = (bps == 16) ? 1 :
			(bps == 20) ? 2 :
			(bps == 24) ? 3 : 0;
#endif
	bps_type = (audio_codec == PCM) ? bps_type : 0;

	sum_of_bits = (0x84 + 0x1 + 10);

	bytes1 = (u8)((type << 4) | ch);
	bytes2 = (u8)((sample << 2) | bps_type);
	bit_rate = 256;
	bytes3 = (audio_codec == PCM) ? (u8)0 : (u8)(bit_rate / 8);

	sum_of_bits += (bytes1 + bytes2 + bytes3);
	check_sum = 256 - sum_of_bits;

	/* AUI Packet set. */
	writeb(check_sum, hdmi_base + S5P_HDMI_AUI_CHECK_SUM);
	writeb(bytes1, hdmi_base + S5P_HDMI_AUI_BYTE1);
	writeb(bytes2, hdmi_base + S5P_HDMI_AUI_BYTE2);
	writeb(bytes3, hdmi_base + S5P_HDMI_AUI_BYTE3);
	writeb(0x00, hdmi_base + S5P_HDMI_AUI_BYTE4);
	writeb(0x00, hdmi_base + S5P_HDMI_AUI_BYTE5);

	writeb(S5P_HDMI_ACP_CON_TRANS_EVERY_VSYNC,
			hdmi_base + S5P_HDMI_ACP_CON);
	writeb(1, hdmi_base + S5P_HDMI_ACP_TYPE);

	writeb(0x10, hdmi_base + S5P_HDMI_GCP_BYTE1);
}

void s5p_hdmi_video_set_bluescreen(bool en, u8 cb_b, u8 y_g, u8 cr_r)
{
	HDMIPRINTK("%d, %d, %d, %d\n\r", en, cb_b, y_g, cr_r);

	if (en) {
		writeb(S5P_HDMI_SET_BLUESCREEN_0(cb_b),
			hdmi_base + S5P_HDMI_BLUE_SCREEN_0);
		writeb(S5P_HDMI_SET_BLUESCREEN_1(y_g),
			hdmi_base + S5P_HDMI_BLUE_SCREEN_1);
		writeb(S5P_HDMI_SET_BLUESCREEN_2(cr_r),
			hdmi_base + S5P_HDMI_BLUE_SCREEN_2);
		writeb(readl(hdmi_base + S5P_HDMI_CON_0) | S5P_HDMI_BLUE_SCR_EN,
			hdmi_base + S5P_HDMI_CON_0);
	} else {
		writeb(readl(hdmi_base + S5P_HDMI_CON_0) &
			~S5P_HDMI_BLUE_SCR_EN, hdmi_base + S5P_HDMI_CON_0);
	}
}

int s5p_hdmi_init_spd(enum s5p_hdmi_transmit trans_type,
				u8 *spd_header,
				u8 *spd_data)
{
	HDMIPRINTK("%d, %d, %d\n\r", (u32)trans_type, (u32)spd_header,
					(u32)spd_data);

	writeb(trans_type, hdmi_base + S5P_HDMI_SPD_CON);
 
	if (spd_data == NULL || spd_header == NULL) {
		HDMIPRINTK("Set default SPD\n");
		writeb(S5P_HDMI_SET_SPD_HEADER(0x83),
			hdmi_base + S5P_HDMI_SPD_HEADER0);
		writeb(S5P_HDMI_SET_SPD_HEADER(0x01),
			hdmi_base + S5P_HDMI_SPD_HEADER1);
		writeb(S5P_HDMI_SET_SPD_HEADER(0x19),
			hdmi_base + S5P_HDMI_SPD_HEADER2);

		writeb(0x0, hdmi_base + S5P_HDMI_SPD_DATA0);
		writeb(S5P_HDMI_SET_SPD_DATA('S'),
			hdmi_base + S5P_HDMI_SPD_DATA1);
		writeb(S5P_HDMI_SET_SPD_DATA('A'),
			hdmi_base + S5P_HDMI_SPD_DATA2);
		writeb(S5P_HDMI_SET_SPD_DATA('M'),
			hdmi_base + S5P_HDMI_SPD_DATA3);
		writeb(S5P_HDMI_SET_SPD_DATA('S'),
			hdmi_base + S5P_HDMI_SPD_DATA4);
		writeb(S5P_HDMI_SET_SPD_DATA('U'),
			hdmi_base + S5P_HDMI_SPD_DATA5);
		writeb(S5P_HDMI_SET_SPD_DATA('N'),
			hdmi_base + S5P_HDMI_SPD_DATA6);
		writeb(S5P_HDMI_SET_SPD_DATA('G'),
			hdmi_base + S5P_HDMI_SPD_DATA7);

		writeb(0x0, hdmi_base + S5P_HDMI_SPD_DATA8);
		writeb(S5P_HDMI_SET_SPD_DATA('S'),
			hdmi_base + S5P_HDMI_SPD_DATA9);
		writeb(S5P_HDMI_SET_SPD_DATA('5'),
			hdmi_base + S5P_HDMI_SPD_DATA10);
		writeb(S5P_HDMI_SET_SPD_DATA('P'),
			hdmi_base + S5P_HDMI_SPD_DATA11);
		writeb(S5P_HDMI_SET_SPD_DATA('C'),
			hdmi_base + S5P_HDMI_SPD_DATA12);
		writeb(S5P_HDMI_SET_SPD_DATA('1'),
			hdmi_base + S5P_HDMI_SPD_DATA13);
		writeb(S5P_HDMI_SET_SPD_DATA('1'),
			hdmi_base + S5P_HDMI_SPD_DATA14);
		writeb(S5P_HDMI_SET_SPD_DATA('0'),
			hdmi_base + S5P_HDMI_SPD_DATA15);
		writeb(0x0, hdmi_base + S5P_HDMI_SPD_DATA16);
		writeb(0x0, hdmi_base + S5P_HDMI_SPD_DATA17);
		writeb(0x0, hdmi_base + S5P_HDMI_SPD_DATA18);
		writeb(0x0, hdmi_base + S5P_HDMI_SPD_DATA19);
		writeb(0x0, hdmi_base + S5P_HDMI_SPD_DATA20);
		writeb(0x0, hdmi_base + S5P_HDMI_SPD_DATA21);
		writeb(0x0, hdmi_base + S5P_HDMI_SPD_DATA22);
		writeb(0x0, hdmi_base + S5P_HDMI_SPD_DATA23);
		writeb(0x0, hdmi_base + S5P_HDMI_SPD_DATA24);
		writeb(0x0, hdmi_base + S5P_HDMI_SPD_DATA25);
		writeb(S5P_HDMI_SET_SPD_DATA(0x2),
			hdmi_base + S5P_HDMI_SPD_DATA26);
		writeb(0x0, hdmi_base + S5P_HDMI_SPD_DATA27);
	} else {
		writeb(S5P_HDMI_SET_SPD_HEADER(*(spd_header)),
			hdmi_base + S5P_HDMI_SPD_HEADER0);

		writeb(S5P_HDMI_SET_SPD_HEADER(*(spd_header + 1)),
			hdmi_base + S5P_HDMI_SPD_HEADER1);
		writeb(S5P_HDMI_SET_SPD_HEADER(*(spd_header + 2)),
			hdmi_base + S5P_HDMI_SPD_HEADER2);

		writeb(S5P_HDMI_SET_SPD_DATA(*(spd_data)),
			hdmi_base + S5P_HDMI_SPD_DATA0);
		writeb(S5P_HDMI_SET_SPD_DATA(*(spd_data + 1)),
			hdmi_base + S5P_HDMI_SPD_DATA1);
		writeb(S5P_HDMI_SET_SPD_DATA(*(spd_data + 2)),
			hdmi_base + S5P_HDMI_SPD_DATA2);
		writeb(S5P_HDMI_SET_SPD_DATA(*(spd_data + 3)),
			hdmi_base + S5P_HDMI_SPD_DATA3);
		writeb(S5P_HDMI_SET_SPD_DATA(*(spd_data + 4)),
			hdmi_base + S5P_HDMI_SPD_DATA4);
		writeb(S5P_HDMI_SET_SPD_DATA(*(spd_data + 5)),
			hdmi_base + S5P_HDMI_SPD_DATA5);
		writeb(S5P_HDMI_SET_SPD_DATA(*(spd_data + 6)),
			hdmi_base + S5P_HDMI_SPD_DATA6);
		writeb(S5P_HDMI_SET_SPD_DATA(*(spd_data + 7)),
			hdmi_base + S5P_HDMI_SPD_DATA7);
		writeb(S5P_HDMI_SET_SPD_DATA(*(spd_data + 8)),
			hdmi_base + S5P_HDMI_SPD_DATA8);
		writeb(S5P_HDMI_SET_SPD_DATA(*(spd_data + 9)),
			hdmi_base + S5P_HDMI_SPD_DATA9);
		writeb(S5P_HDMI_SET_SPD_DATA(*(spd_data + 10)),
			hdmi_base + S5P_HDMI_SPD_DATA10);
		writeb(S5P_HDMI_SET_SPD_DATA(*(spd_data + 11)),
			hdmi_base + S5P_HDMI_SPD_DATA11);
		writeb(S5P_HDMI_SET_SPD_DATA(*(spd_data + 12)),
			hdmi_base + S5P_HDMI_SPD_DATA12);
		writeb(S5P_HDMI_SET_SPD_DATA(*(spd_data + 13)),
			hdmi_base + S5P_HDMI_SPD_DATA13);
		writeb(S5P_HDMI_SET_SPD_DATA(*(spd_data + 14)),
			hdmi_base + S5P_HDMI_SPD_DATA14);
		writeb(S5P_HDMI_SET_SPD_DATA(*(spd_data + 15)),
			hdmi_base + S5P_HDMI_SPD_DATA15);
		writeb(S5P_HDMI_SET_SPD_DATA(*(spd_data + 16)),
			hdmi_base + S5P_HDMI_SPD_DATA16);
		writeb(S5P_HDMI_SET_SPD_DATA(*(spd_data + 17)),
			hdmi_base + S5P_HDMI_SPD_DATA17);
		writeb(S5P_HDMI_SET_SPD_DATA(*(spd_data + 18)),
			hdmi_base + S5P_HDMI_SPD_DATA18);
		writeb(S5P_HDMI_SET_SPD_DATA(*(spd_data + 19)),
			hdmi_base + S5P_HDMI_SPD_DATA19);
		writeb(S5P_HDMI_SET_SPD_DATA(*(spd_data + 20)),
			hdmi_base + S5P_HDMI_SPD_DATA20);
		writeb(S5P_HDMI_SET_SPD_DATA(*(spd_data + 21)),
			hdmi_base + S5P_HDMI_SPD_DATA21);
		writeb(S5P_HDMI_SET_SPD_DATA(*(spd_data + 22)),
			hdmi_base + S5P_HDMI_SPD_DATA22);
		writeb(S5P_HDMI_SET_SPD_DATA(*(spd_data + 23)),
			hdmi_base + S5P_HDMI_SPD_DATA23);
		writeb(S5P_HDMI_SET_SPD_DATA(*(spd_data + 24)),
			hdmi_base + S5P_HDMI_SPD_DATA24);
		writeb(S5P_HDMI_SET_SPD_DATA(*(spd_data + 25)),
			hdmi_base + S5P_HDMI_SPD_DATA25);
		writeb(S5P_HDMI_SET_SPD_DATA(*(spd_data + 26)),
			hdmi_base + S5P_HDMI_SPD_DATA26);
		writeb(S5P_HDMI_SET_SPD_DATA(*(spd_data + 27)),
			hdmi_base + S5P_HDMI_SPD_DATA27);
	}

	return 0;
}

#ifndef CONFIG_SND_S5P_SPDIF
static void s5p_hdmi_audio_i2s_config(enum s5p_tv_audio_codec_type audio_codec,
					u32 sample_rate, u32 bits_per_sample,
					u32 frame_size_code)
{
	u32 data_num, bit_ch, sample_frq;

	if (bits_per_sample == 20) {
		data_num = 2;
		bit_ch   = 1;
	} else if (bits_per_sample == 24) {
		data_num = 3;
		bit_ch   = 1;
	} else {
		data_num = 1;
		bit_ch   = 0;
	}

	writeb(S5P_HDMI_I2S_CH_STATUS_RELOAD,
		hdmi_base + S5P_HDMI_I2S_CH_ST_CON);

	writeb((S5P_HDMI_I2S_IN_DISABLE | S5P_HDMI_I2S_AUD_I2S |
		S5P_HDMI_I2S_CUV_I2S_ENABLE | S5P_HDMI_I2S_MUX_ENABLE),
		hdmi_base + S5P_HDMI_I2S_MUX_CON);

	writeb(S5P_HDMI_I2S_CH0_EN | S5P_HDMI_I2S_CH1_EN | S5P_HDMI_I2S_CH2_EN,
		hdmi_base + S5P_HDMI_I2S_MUX_CH);

	writeb(S5P_HDMI_I2S_CUV_RL_EN, hdmi_base + S5P_HDMI_I2S_MUX_CUV);

	sample_frq = (sample_rate == 44100) ? 0 :
			(sample_rate == 48000) ? 2 :
			(sample_rate == 32000) ? 3 :
			(sample_rate == 96000) ? 0xa : 0x0;

	/* readl(hdmi_base + S5P_HDMI_YMAX) */
	writeb(S5P_HDMI_I2S_CLK_DIS, hdmi_base + S5P_HDMI_I2S_CLK_CON);
	writeb(S5P_HDMI_I2S_CLK_EN, hdmi_base + S5P_HDMI_I2S_CLK_CON);

	writeb(readl(hdmi_base + S5P_HDMI_I2S_DSD_CON) | 0x01,
		hdmi_base + S5P_HDMI_I2S_DSD_CON);

	/* Configuration I2S input ports. Configure I2S_PIN_SEL_0~4 */
	writeb(S5P_HDMI_I2S_SEL_SCLK(5) | S5P_HDMI_I2S_SEL_LRCK(6),
		hdmi_base + S5P_HDMI_I2S_PIN_SEL_0);
	writeb(S5P_HDMI_I2S_SEL_SDATA1(1) | S5P_HDMI_I2S_SEL_SDATA2(4),
		hdmi_base + S5P_HDMI_I2S_PIN_SEL_1);
	writeb(S5P_HDMI_I2S_SEL_SDATA3(1) | S5P_HDMI_I2S_SEL_SDATA2(2),
		hdmi_base + S5P_HDMI_I2S_PIN_SEL_2);
	writeb(S5P_HDMI_I2S_SEL_DSD(0), hdmi_base + S5P_HDMI_I2S_PIN_SEL_3);

	/* I2S_CON_1 & 2 */
	writeb(S5P_HDMI_I2S_SCLK_RISING_EDGE | S5P_HDMI_I2S_L_CH_LOW_POL,
		hdmi_base + S5P_HDMI_I2S_CON_1);
	writeb(S5P_HDMI_I2S_MSB_FIRST_MODE |
		S5P_HDMI_I2S_SET_BIT_CH(bit_ch) |
		S5P_HDMI_I2S_SET_SDATA_BIT(data_num) |
		S5P_HDMI_I2S_BASIC_FORMAT,
		hdmi_base + S5P_HDMI_I2S_CON_2);

	/* Configure register related to CUV information */
	writeb(S5P_HDMI_I2S_CH_STATUS_MODE_0 |
		S5P_HDMI_I2S_2AUD_CH_WITHOUT_PREEMPH |
		S5P_HDMI_I2S_COPYRIGHT |
		S5P_HDMI_I2S_LINEAR_PCM |
		S5P_HDMI_I2S_PROF_FORMAT,
		hdmi_base + S5P_HDMI_I2S_CH_ST_0);
	writeb(S5P_HDMI_I2S_CD_PLAYER,
		hdmi_base + S5P_HDMI_I2S_CH_ST_1);
	writeb(S5P_HDMI_I2S_SET_SOURCE_NUM(0),
		hdmi_base + S5P_HDMI_I2S_CH_ST_2);
	writeb(S5P_HDMI_I2S_CLK_ACCUR_LEVEL_1 |
		S5P_HDMI_I2S_SET_SAMPLING_FREQ(sample_frq),
		hdmi_base + S5P_HDMI_I2S_CH_ST_3);
	writeb(S5P_HDMI_I2S_ORG_SAMPLING_FREQ_44_1 |
		S5P_HDMI_I2S_WORD_LENGTH_MAX24_24BITS |
		S5P_HDMI_I2S_WORD_LENGTH_MAX_24BITS,
		hdmi_base + S5P_HDMI_I2S_CH_ST_4);

	writeb(0x00, hdmi_base + S5P_HDMI_I2S_CH_ST_SH_0);
	writeb(0x00, hdmi_base + S5P_HDMI_I2S_CH_ST_SH_1);
	writeb(0x00, hdmi_base + S5P_HDMI_I2S_CH_ST_SH_2);
	writeb(0x02, hdmi_base + S5P_HDMI_I2S_CH_ST_SH_3);
	writeb(0x00, hdmi_base + S5P_HDMI_I2S_CH_ST_SH_4);
}
#endif

int s5p_hdmi_audio_init(enum s5p_tv_audio_codec_type audio_codec,
			u32 sample_rate, u32 bits, u32 frame_size_code)
{
#ifdef CONFIG_SND_S5P_SPDIF
	s5p_hdmi_audio_set_config(audio_codec);
	s5p_hdmi_audio_set_repetition_time(audio_codec, bits, frame_size_code);
	s5p_hdmi_audio_irq_enable(S5P_HDMI_SPDIFIN_IRQ_OVERFLOW_EN);
	s5p_hdmi_audio_clock_enable();

#else
	s5p_hdmi_audio_i2s_config(audio_codec, sample_rate, bits,
				frame_size_code);
#endif
	s5p_hdmi_audio_set_asp();
	s5p_hdmi_audio_set_acr(sample_rate);

	s5p_hdmi_audio_set_aui(audio_codec, sample_rate, bits);

	return 0;
}

int s5p_hdmi_video_init_display_mode(enum s5p_tv_disp_mode disp_mode,
				enum s5p_tv_o_mode out_mode, u8 *avidata)
{
	enum s5p_hdmi_v_fmt hdmi_v_fmt;
	enum s5p_tv_hdmi_pxl_aspect aspect;

	HDMIPRINTK("disp mode %d, output mode%d\n\r", disp_mode, out_mode);

	aspect = HDMI_PIXEL_RATIO_16_9;

	switch (disp_mode) {
	case TVOUT_480P_60_16_9:
		hdmi_v_fmt = v720x480p_60Hz;
		break;

	case TVOUT_480P_60_4_3:
		hdmi_v_fmt = v720x480p_60Hz;
		aspect = HDMI_PIXEL_RATIO_4_3;
		break;

	case TVOUT_480P_59:
		hdmi_v_fmt = v720x480p_59Hz;
		break;

	case TVOUT_576P_50_16_9:
		hdmi_v_fmt = v720x576p_50Hz;
		break;

	case TVOUT_576P_50_4_3:
		hdmi_v_fmt = v720x576p_50Hz;
		aspect = HDMI_PIXEL_RATIO_4_3;
		break;

	case TVOUT_720P_60:
		hdmi_v_fmt = v1280x720p_60Hz;
		break;

	case TVOUT_720P_59:
		hdmi_v_fmt = v1280x720p_59Hz;
		break;

	case TVOUT_720P_50:
		hdmi_v_fmt = v1280x720p_50Hz;
		break;

	case TVOUT_1080P_30:
		hdmi_v_fmt = v1920x1080p_30Hz;
		break;

	case TVOUT_1080P_60:
		hdmi_v_fmt = v1920x1080p_60Hz;
		break;

	case TVOUT_1080P_59:
		hdmi_v_fmt = v1920x1080p_59Hz;
		break;

	case TVOUT_1080P_50:
		hdmi_v_fmt = v1920x1080p_50Hz;
		break;

	case TVOUT_1080I_60:
		hdmi_v_fmt = v1920x1080i_60Hz;
		break;

	case TVOUT_1080I_59:
		hdmi_v_fmt = v1920x1080i_59Hz;
		break;

	case TVOUT_1080I_50:
		hdmi_v_fmt = v1920x1080i_50Hz;
		break;

	default:
		HDMIPRINTK(" invalid disp_mode parameter(%d)\n\r", disp_mode);
		return -1;
	}

	if (s5p_hdmi_phy_config(video_params[hdmi_v_fmt].pixel_clock,
			HDMI_CD_24) == EINVAL) {
		HDMIPRINTK("[ERR] s5p_hdmi_phy_config() failed.\n");

		return -1;
	}

	switch (out_mode) {
	case TVOUT_OUTPUT_HDMI_RGB:
		s5p_hdcp_hdmi_set_dvi(false);
		writeb(S5P_HDMI_PX_LMT_CTRL_RGB, hdmi_base + S5P_HDMI_CON_1);
		writeb(S5P_HDMI_VID_PREAMBLE_EN | S5P_HDMI_GUARD_BAND_EN,
			hdmi_base + S5P_HDMI_CON_2);
		writeb(S5P_HDMI_MODE_EN | S5P_HDMI_DVI_MODE_DIS,
			hdmi_base + S5P_HDMI_MODE_SEL);

		/* there's no ACP packet api */
		writeb(HDMI_DO_NOT_TANS, hdmi_base + S5P_HDMI_ACP_CON);
		writeb(HDMI_TRANS_EVERY_SYNC, hdmi_base + S5P_HDMI_AUI_CON);
		break;

	case TVOUT_OUTPUT_HDMI:
		s5p_hdcp_hdmi_set_dvi(false);
		writeb(S5P_HDMI_PX_LMT_CTRL_BYPASS, hdmi_base + S5P_HDMI_CON_1);
		writeb(S5P_HDMI_VID_PREAMBLE_EN | S5P_HDMI_GUARD_BAND_EN,
			hdmi_base + S5P_HDMI_CON_2);
		writeb(S5P_HDMI_MODE_EN | S5P_HDMI_DVI_MODE_DIS,
			hdmi_base + S5P_HDMI_MODE_SEL);

		/* there's no ACP packet api */
		writeb(HDMI_DO_NOT_TANS, hdmi_base + S5P_HDMI_ACP_CON);
		writeb(HDMI_TRANS_EVERY_SYNC, hdmi_base + S5P_HDMI_AUI_CON);
		break;

	case TVOUT_OUTPUT_DVI:
		s5p_hdcp_hdmi_set_dvi(true);
		writeb(S5P_HDMI_PX_LMT_CTRL_RGB, hdmi_base + S5P_HDMI_CON_1);
		writeb(S5P_HDMI_VID_PREAMBLE_DIS | S5P_HDMI_GUARD_BAND_DIS,
			hdmi_base + S5P_HDMI_CON_2);
		writeb(S5P_HDMI_MODE_DIS | S5P_HDMI_DVI_MODE_EN,
			hdmi_base + S5P_HDMI_MODE_SEL);

		/* disable ACP & Audio Info.frame packet */
		writeb(HDMI_DO_NOT_TANS, hdmi_base + S5P_HDMI_ACP_CON);
		writeb(HDMI_DO_NOT_TANS, hdmi_base + S5P_HDMI_AUI_CON);
		break;

	default:
		HDMIPRINTK("invalid out_mode parameter(%d)\n\r", out_mode);
		return -1;
	}

	s5p_hdmi_set_video_mode(hdmi_v_fmt, HDMI_CD_24, aspect, avidata);

	return 0;
}

void s5p_hdmi_video_init_color_range(u8 y_min, u8 y_max, u8 c_min, u8 c_max)
{
	HDMIPRINTK("%d, %d, %d, %d\n\r", y_max, y_min, c_max, c_min);

	writeb(y_max, hdmi_base + S5P_HDMI_YMAX);
	writeb(y_min, hdmi_base + S5P_HDMI_YMIN);
	writeb(c_max, hdmi_base + S5P_HDMI_CMAX);
	writeb(c_min, hdmi_base + S5P_HDMI_CMIN);
}

int s5p_hdmi_video_init_avi(enum s5p_hdmi_transmit trans_type,
					u8 check_sum, u8 *avi_data)
{
	HDMIPRINTK("%d, %d, %d\n\r", (u32)trans_type, (u32)check_sum,
		(u32)avi_data);

	writeb(trans_type, hdmi_base + S5P_HDMI_AVI_CON);

	writeb(S5P_HDMI_SET_AVI_CHECK_SUM(check_sum),
				hdmi_base + S5P_HDMI_AVI_CHECK_SUM);
	writeb(S5P_HDMI_SET_AVI_DATA(*(avi_data)),
				hdmi_base + S5P_HDMI_AVI_BYTE1);
	writeb(S5P_HDMI_SET_AVI_DATA(*(avi_data + 1)),
				hdmi_base + S5P_HDMI_AVI_BYTE2);
	writeb(S5P_HDMI_SET_AVI_DATA(*(avi_data + 2)),
				hdmi_base + S5P_HDMI_AVI_BYTE3);
	writeb(S5P_HDMI_SET_AVI_DATA(*(avi_data + 3)),
				hdmi_base + S5P_HDMI_AVI_BYTE4);
	writeb(S5P_HDMI_SET_AVI_DATA(*(avi_data + 4)),
				hdmi_base + S5P_HDMI_AVI_BYTE5);
	writeb(S5P_HDMI_SET_AVI_DATA(*(avi_data + 5)),
				hdmi_base + S5P_HDMI_AVI_BYTE6);
	writeb(S5P_HDMI_SET_AVI_DATA(*(avi_data + 6)),
				hdmi_base + S5P_HDMI_AVI_BYTE7);
	writeb(S5P_HDMI_SET_AVI_DATA(*(avi_data + 7)),
				hdmi_base + S5P_HDMI_AVI_BYTE8);
	writeb(S5P_HDMI_SET_AVI_DATA(*(avi_data + 8)),
				hdmi_base + S5P_HDMI_AVI_BYTE9);
	writeb(S5P_HDMI_SET_AVI_DATA(*(avi_data + 9)),
				hdmi_base + S5P_HDMI_AVI_BYTE10);
	writeb(S5P_HDMI_SET_AVI_DATA(*(avi_data + 10)),
				hdmi_base + S5P_HDMI_AVI_BYTE11);
	writeb(S5P_HDMI_SET_AVI_DATA(*(avi_data + 11)),
				hdmi_base + S5P_HDMI_AVI_BYTE12);
	writeb(S5P_HDMI_SET_AVI_DATA(*(avi_data + 12)),
				hdmi_base + S5P_HDMI_AVI_BYTE13);

	return 0;
}

int s5p_hdmi_video_init_mpg(enum s5p_hdmi_transmit trans_type,
					u8 check_sum, u8 *mpg_data)
{
	HDMIPRINTK("trans_type : %d, %d, %d\n\r", (u32)trans_type,
			(u32)check_sum, (u32)mpg_data);

	writeb(trans_type, hdmi_base + S5P_HDMI_MPG_CON);

	writeb(S5P_HDMI_SET_MPG_CHECK_SUM(check_sum),
	       hdmi_base + S5P_HDMI_MPG_CHECK_SUM);
	writeb(S5P_HDMI_SET_MPG_DATA(*(mpg_data)),
	       hdmi_base + S5P_HDMI_MPEG_BYTE1);
	writeb(S5P_HDMI_SET_MPG_DATA(*(mpg_data + 1)),
	       hdmi_base + S5P_HDMI_MPEG_BYTE2);
	writeb(S5P_HDMI_SET_MPG_DATA(*(mpg_data + 2)),
	       hdmi_base + S5P_HDMI_MPEG_BYTE3);
	writeb(S5P_HDMI_SET_MPG_DATA(*(mpg_data + 3)),
	       hdmi_base + S5P_HDMI_MPEG_BYTE4);
	writeb(S5P_HDMI_SET_MPG_DATA(*(mpg_data + 4)),
	       hdmi_base + S5P_HDMI_MPEG_BYTE5);

	return 0;
}

void s5p_hdmi_video_init_tg_cmd(bool time_c_e, bool bt656_sync_en, bool tg_en)
{
	u32 temp_reg = 0;

	temp_reg = readl(hdmi_base + S5P_HDMI_TG_CMD);

	if (time_c_e)
		temp_reg |= S5P_HDMI_GETSYNC_TYPE_EN;
	else
		temp_reg &= S5P_HDMI_GETSYNC_TYPE_DIS;

	if (bt656_sync_en)
		temp_reg |= S5P_HDMI_GETSYNC_EN;
	else
		temp_reg &= S5P_HDMI_GETSYNC_DIS;

	if (tg_en)
		temp_reg |= S5P_HDMI_TG_EN;
	else
		temp_reg &= S5P_HDMI_TG_DIS;

	writeb(temp_reg, hdmi_base + S5P_HDMI_TG_CMD);
}

bool s5p_hdmi_start(enum s5p_hdmi_audio_type hdmi_audio_type, bool hdcp_en,
			struct i2c_client *ddc_port)
{
	u32 temp_reg = S5P_HDMI_EN;

	HDMIPRINTK("aud type : %d, hdcp enable : %d\n\r",
		   hdmi_audio_type, hdcp_en);

	switch (hdmi_audio_type) {
	case HDMI_AUDIO_PCM:
		temp_reg |= S5P_HDMI_ASP_EN;
		break;

	case HDMI_AUDIO_NO:
		break;

	default:
		HDMIPRINTK(" invalid hdmi_audio_type(%d)\n\r",
			   hdmi_audio_type);
		return false;
	}

	s5p_hpd_set_hdmiint();

	if (hdcp_en) {
		writeb(S5P_HDMI_HDCP_ENC_DISABLE, hdmi_base + S5P_HDMI_ENC_EN);
		s5p_hdcp_hdmi_mute_en(true);
	}

	writeb(readl(hdmi_base + S5P_HDMI_CON_0) | temp_reg,
	       hdmi_base + S5P_HDMI_CON_0);

	return true;
}

void s5p_hdmi_stop(void)
{
	u32 temp = 0, result = 0;

	s5p_hdcp_stop();

	mdelay(100);

	temp = readl(hdmi_base + S5P_HDMI_CON_0);
	result = temp & S5P_HDMI_EN;

	if (result)
		writeb(temp &  ~(S5P_HDMI_EN | S5P_HDMI_ASP_EN),
			hdmi_base + S5P_HDMI_CON_0);

	do {
		temp = readl(hdmi_base + S5P_HDMI_CON_0);
	} while (temp & S5P_HDMI_EN);

	s5p_hpd_set_eint();
}

int __init s5p_hdmi_probe(struct platform_device *pdev, u32 res_num,
			u32 res_num2)
{
	struct resource *res;
	size_t	size;
	u32	reg;

	spin_lock_init(&lock_hdmi);

	res = platform_get_resource(pdev, IORESOURCE_MEM, res_num);

	if (res == NULL) {
		dev_err(&pdev->dev,
			"failed to get memory region resource\n");
		goto error;
	}

	size = (res->end - res->start) + 1;

	hdmi_mem = request_mem_region(res->start, size, pdev->name);

	if (hdmi_mem == NULL) {
		dev_err(&pdev->dev,
			"failed to get memory region\n");
		goto error;
	}

	hdmi_base = ioremap(res->start, size);

	if (hdmi_base == NULL) {
		dev_err(&pdev->dev,
			"failed to ioremap address region\n");
		goto error;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, res_num2);

	if (res == NULL) {
		dev_err(&pdev->dev,
			"failed to get memory region resource\n");
		goto error;
	}

	size = (res->end - res->start) + 1;

	i2c_hdmi_phy_mem = request_mem_region(res->start, size, pdev->name);

	if (i2c_hdmi_phy_mem == NULL) {
		dev_err(&pdev->dev,
			"failed to get memory region\n");
		goto error;
	}

	i2c_hdmi_phy_base = ioremap(res->start, size);

	if (i2c_hdmi_phy_base == NULL) {
		dev_err(&pdev->dev,
			"failed to ioremap address region\n");
		goto error;
	}

	/* PMU Block : HDMI PHY Enable */
	reg = readl(S3C_VA_SYS + 0xE804);
	reg |= (1 << 0);
	writeb(reg, S3C_VA_SYS + 0xE804);

	/* i2c_hdmi init - set i2c filtering */
	writeb(0x5, i2c_hdmi_phy_base + HDMI_I2C_LC);
	
	return 0;

error:
	return -ENOENT;
}

int __init s5p_hdmi_release(struct platform_device *pdev)
{
	iounmap(hdmi_base);

	/* remove memory region */
	if (hdmi_mem != NULL) {
		if (release_resource(hdmi_mem))
			dev_err(&pdev->dev,
				"Can't remove tvout drv !!\n");

		kfree(hdmi_mem);

		hdmi_mem = NULL;
	}

	return 0;
}

int s5p_hdmi_register_isr(hdmi_isr isr, u8 irq_num)
{
	HDMIPRINTK("Try to register ISR for IRQ number (%d)\n", irq_num);

	if (isr == NULL) {
		HDMIPRINTK("Invaild ISR\n");

		return -EINVAL;
	}

	if (irq_num > HDMI_IRQ_TOTAL_NUM) {
		HDMIPRINTK("irq_num exceeds allowed IRQ number(%d)\n",
			HDMI_IRQ_TOTAL_NUM);

		return -EINVAL;
	}

	if (hdmi_isr_ftn[irq_num]) {
		HDMIPRINTK("the %d th ISR is already registered\n",
			irq_num);
	}

	hdmi_isr_ftn[irq_num] = isr;

	HDMIPRINTK("Success to register ISR for IRQ number (%d)\n",
			irq_num);

	return 0;
}

irqreturn_t s5p_hdmi_irq(int irq, void *dev_id)
{
	u8 irq_state, irq_num;

	spin_lock_irq(&lock_hdmi);

	irq_state = readb(hdmi_base + S5P_HDMI_INTC_FLAG);

	HDMIPRINTK("S5P_HDMI_INTC_FLAG = 0x%02x\n", irq_state);

	/* Check interrupt happened */
	/* Priority of Interrupt  HDCP> I2C > Audio > CEC */
	if (irq_state) {
		irq_num = 0;

		while (irq_num < HDMI_IRQ_TOTAL_NUM) {

			if (irq_state & (1 << irq_num)) {

				if (hdmi_isr_ftn[irq_num] != NULL)
					(hdmi_isr_ftn[irq_num])(irq_num);
				else
					HDMIPRINTK(
					"No registered ISR for IRQ[%d]\n",
					irq_num);

			}

			++irq_num;
		}

	} else {
		HDMIPRINTK("Undefined IRQ happened[%x]\n", irq_state);
	}

	spin_unlock_irq(&lock_hdmi);

	return IRQ_HANDLED;
}
u8 s5p_hdmi_get_enabled_interrupt(void)
{
	u8 reg;

	reg = readb(hdmi_base + S5P_HDMI_INTC_CON);

	return reg;
}

void s5p_hdmi_enable_interrupts(enum s5p_tv_hdmi_interrrupt intr)
{
	u8 reg;

	reg = readb(hdmi_base + S5P_HDMI_INTC_CON);
	writeb(reg | (1 << intr) | S5P_HDMI_INTC_EN_GLOBAL,
		hdmi_base + S5P_HDMI_INTC_CON);
}

void s5p_hdmi_disable_interrupts(enum s5p_tv_hdmi_interrrupt intr)
{
	u8 reg;

	reg = readb(hdmi_base + S5P_HDMI_INTC_CON);
	writeb(reg & ~(1 << intr), hdmi_base + S5P_HDMI_INTC_CON);
}

void s5p_hdmi_clear_pending(enum s5p_tv_hdmi_interrrupt intr)
{
	u8 reg;

	reg = readb(hdmi_base + S5P_HDMI_INTC_FLAG);
	writeb(reg | (1 << intr), hdmi_base + S5P_HDMI_INTC_FLAG);
}

u8 s5p_hdmi_get_interrupts(void)
{
	u8 reg;

	reg = readb(hdmi_base + S5P_HDMI_INTC_FLAG);

	return reg;
}

u8 s5p_hdmi_get_hpd_status(void)
{
	u8 reg;

	reg = readb(hdmi_base + S5P_HDMI_HPD_STATUS);

	return reg;
}

void s5p_hdmi_hpd_gen(void)
{
	writeb(0xFF, hdmi_base + S5P_HDMI_HPD_GEN);
}
