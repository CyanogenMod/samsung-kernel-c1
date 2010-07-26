/* linux/drivers/media/video/samsung/tv20/ver_1/hdcp.c
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsung.com/
 *
 * hdcp raw ftn  file for Samsung TVOut driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/io.h>

#include <plat/gpio-cfg.h>

#include <mach/regs-gpio.h>
#include <mach/gpio.h>
#include <mach/regs-hdmi.h>

#include "tv_out.h"
#include "../ddc.h"

/* for Operation check */
#ifdef COFIG_TVOUT_RAW_DBG
#define S5P_HDCP_DEBUG 1
#define S5P_HDCP_I2C_DEBUG 1
#define S5P_HDCP_AUTH_DEBUG 1
#endif

#ifdef S5P_HDCP_DEBUG
#define HDCPPRINTK(fmt, args...) \
	printk(KERN_INFO "\t\t[HDCP] %s: " fmt, __func__ , ## args)
#else
#define HDCPPRINTK(fmt, args...)
#endif

/* for authentication key check */
#ifdef S5P_HDCP_AUTH_DEBUG
#define AUTHPRINTK(fmt, args...) \
	printk("\t\t\t[AUTHKEY] %s: " fmt, __func__ , ## args)
#else
#define AUTHPRINTK(fmt, args...)
#endif

static bool sw_reset;
static bool is_dvi;
static bool av_mute;
static bool audio_en;

static struct s5p_hdcp_info hdcp_info = {
	.is_repeater 	= false,
	.time_out	= 0,
	.hdcp_enable	= false,
	.client		= NULL,
	.event 		= HDCP_EVENT_STOP,
	.auth_status	= NOT_AUTHENTICATED,

};


void s5p_hdcp_hdmi_set_audio(bool en)
{
	if (en)
		audio_en = true;
	else
		audio_en = false;
}

int s5p_hdcp_hdmi_set_dvi(bool en)
{
	if (en)
		is_dvi = true;
	else
		is_dvi = false;

	return 0;
}

int s5p_hdcp_hdmi_set_mute(bool en)
{
	if (en)
		av_mute = true;
	else
		av_mute = false;

	return 0;
}

int s5p_hdcp_hdmi_get_mute(void)
{
	return av_mute ? true : false;
}

int s5p_hdcp_hdmi_audio_enable(bool en)
{
	u8 reg;

	if (!is_dvi) {
		reg = readl(hdmi_base + S5P_HDMI_CON_0);

		if (en) {
			reg |= S5P_HDMI_ASP_EN;
			writel(HDMI_TRANS_EVERY_SYNC,
					hdmi_base + S5P_HDMI_AUI_CON);
		} else {
			reg &= ~S5P_HDMI_ASP_EN;
			writel(HDMI_DO_NOT_TANS, hdmi_base + S5P_HDMI_AUI_CON);
		}

		writel(reg, hdmi_base + S5P_HDMI_CON_0);
	}

	return 0;
}

void s5p_hdcp_hdmi_mute_en(bool en)
{
	if (!av_mute) {
		if (en) {
			s5p_hdmi_video_set_bluescreen(true, 0, 0, 0);
			s5p_hdcp_hdmi_audio_enable(false);
		} else {
			s5p_hdmi_video_set_bluescreen(false, 0, 0, 0);
			if (audio_en)
				s5p_hdcp_hdmi_audio_enable(true);
		}

	}
}

static bool s5p_hdcp_write_an(void)
{
	int ret = 0;
	u8 an[AN_SIZE+1];

	an[0] = HDCP_An;
	an[1] = readb(hdmi_base + S5P_HDMI_HDCP_An_0_0);
	an[2] = readb(hdmi_base + S5P_HDMI_HDCP_An_0_1);
	an[3] = readb(hdmi_base + S5P_HDMI_HDCP_An_0_2);
	an[4] = readb(hdmi_base + S5P_HDMI_HDCP_An_0_3);
	an[5] = readb(hdmi_base + S5P_HDMI_HDCP_An_1_0);
	an[6] = readb(hdmi_base + S5P_HDMI_HDCP_An_1_1);
	an[7] = readb(hdmi_base + S5P_HDMI_HDCP_An_1_2);
	an[8] = readb(hdmi_base + S5P_HDMI_HDCP_An_1_3);

	ret = s5p_ddc_write(an, AN_SIZE + 1);

	if (ret < 0)
		HDCPPRINTK("Can't write an data through i2c bus\n");

#ifdef S5P_HDCP_AUTH_DEBUG
	{
		u16 i = 0;

		for (i = 1; i < AN_SIZE + 1; i++)
			AUTHPRINTK("HDCPAn[%d]: 0x%x \n", i, an[i]);

	}
#endif

	return (ret < 0) ? false : true;
}

static bool s5p_hdcp_write_aksv(void)
{
	int ret = 0;
	u8 aksv[AKSV_SIZE+1];

	aksv[0] = HDCP_Aksv;
	aksv[1] = readb(hdmi_base + S5P_HDMI_HDCP_AKSV_0_0);
	aksv[2] = readb(hdmi_base + S5P_HDMI_HDCP_AKSV_0_1);
	aksv[3] = readb(hdmi_base + S5P_HDMI_HDCP_AKSV_0_2);
	aksv[4] = readb(hdmi_base + S5P_HDMI_HDCP_AKSV_0_3);
	aksv[5] = readb(hdmi_base + S5P_HDMI_HDCP_AKSV_1);

	if (aksv[1] == 0 && aksv[2] == 0 && aksv[3] == 0 &&
					aksv[4] == 0 && aksv[5] == 0)
		return false;

	ret = s5p_ddc_write(aksv, AKSV_SIZE + 1);

	if (ret < 0)
		HDCPPRINTK("Can't write aksv data through i2c bus\n");

#ifdef S5P_HDCP_AUTH_DEBUG
	{
		u16 i = 0;

		for (i = 1; i < AKSV_SIZE + 1; i++)
			AUTHPRINTK("HDCPAksv[%d]: 0x%x\n", i, aksv[i]);

	}
#endif

	return (ret < 0) ? false : true;
}

static bool s5p_hdcp_read_bcaps(void)
{
	int ret = 0;
	u8 bcaps[BCAPS_SIZE] = {0};

	ret = s5p_ddc_read(HDCP_Bcaps, bcaps, BCAPS_SIZE);

	if (ret < 0) {
		HDCPPRINTK("Can't read bcaps data from i2c bus\n");

		return false;
	}

	writel(bcaps[0], hdmi_base + S5P_HDMI_HDCP_BCAPS);

	HDCPPRINTK("BCAPS(from i2c) : 0x%08x\n", bcaps[0]);

	if (bcaps[0] & S5P_HDMI_HDCP_BCAPS_REPEATER)
		hdcp_info.is_repeater = true;
	else
		hdcp_info.is_repeater = false;

	HDCPPRINTK("attached device type :  %s !! \n\r",
		   hdcp_info.is_repeater ? "REPEATER" : "SINK");
	HDCPPRINTK("BCAPS(from sfr) = 0x%08x\n",
		readl(hdmi_base + S5P_HDMI_HDCP_BCAPS));

	return true;
}

static bool s5p_hdcp_read_again_bksv(void)
{
	u8 bk_sv[BKSV_SIZE] = {0, 0, 0, 0, 0};
	u8 i = 0;
	u8 j = 0;
	u32 no_one = 0;
	u32 no_zero = 0;
	u32 result = 0;
	int ret = 0;

	ret = s5p_ddc_read(HDCP_Bksv, bk_sv, BKSV_SIZE);

	if (ret < 0) {
		HDCPPRINTK("Can't read bk_sv data from i2c bus\n");

		return false;
	}

#ifdef S5P_HDCP_AUTH_DEBUG
	for (i = 0; i < BKSV_SIZE; i++)
		AUTHPRINTK("i2c read : Bksv[%d]: 0x%x\n", i, bk_sv[i]);
#endif

	for (i = 0; i < BKSV_SIZE; i++) {

		for (j = 0; j < 8; j++) {
			result = bk_sv[i] & (0x1 << j);

			if (result == 0)
				no_zero += 1;
			else
				no_one += 1;
		}

	}

	if ((no_zero == 20) && (no_one == 20)) {
		HDCPPRINTK("Suucess: no_zero, and no_one is 20\n");

		writel(bk_sv[0], hdmi_base + S5P_HDMI_HDCP_BKSV_0_0);
		writel(bk_sv[1], hdmi_base + S5P_HDMI_HDCP_BKSV_0_1);
		writel(bk_sv[2], hdmi_base + S5P_HDMI_HDCP_BKSV_0_2);
		writel(bk_sv[3], hdmi_base + S5P_HDMI_HDCP_BKSV_0_3);
		writel(bk_sv[4], hdmi_base + S5P_HDMI_HDCP_BKSV_1);

#ifdef S5P_HDCP_AUTH_DEBUG
		for (i = 0; i < BKSV_SIZE; i++)
			AUTHPRINTK("set reg : Bksv[%d]: 0x%x\n", i, bk_sv[i]);
#endif

		return true;
	} else {
		HDCPPRINTK("Failed: no_zero or no_one is NOT 20\n");

		return false;
	}
}

static bool s5p_hdcp_read_bksv(void)
{
	u8 bk_sv[BKSV_SIZE] = {0, 0, 0, 0, 0};
	int i = 0;
	int j = 0;
	u32 no_one = 0;
	u32 no_zero = 0;
	u32 result = 0;
	u32 count = 0;
	int ret = 0;

	ret = s5p_ddc_read(HDCP_Bksv, bk_sv, BKSV_SIZE);

	if (ret < 0) {
		HDCPPRINTK("Can't read bk_sv data from i2c bus\n");

		return false;
	}

#ifdef S5P_HDCP_AUTH_DEBUG
	for (i = 0; i < BKSV_SIZE; i++)
		AUTHPRINTK("i2c read : Bksv[%d]: 0x%x\n", i, bk_sv[i]);
#endif

	for (i = 0; i < BKSV_SIZE; i++) {

		for (j = 0; j < 8; j++) {
			result = bk_sv[i] & (0x1 << j);

			if (result == 0)
				no_zero++;
			else
				no_one++;
		}

	}

	if ((no_zero == 20) && (no_one == 20)) {
		writel(bk_sv[0], hdmi_base + S5P_HDMI_HDCP_BKSV_0_0);
		writel(bk_sv[1], hdmi_base + S5P_HDMI_HDCP_BKSV_0_1);
		writel(bk_sv[2], hdmi_base + S5P_HDMI_HDCP_BKSV_0_2);
		writel(bk_sv[3], hdmi_base + S5P_HDMI_HDCP_BKSV_0_3);
		writel(bk_sv[4], hdmi_base + S5P_HDMI_HDCP_BKSV_1);

#ifdef S5P_HDCP_AUTH_DEBUG
		for (i = 0; i < BKSV_SIZE; i++)
			AUTHPRINTK("set reg : Bksv[%d]: 0x%x\n", i, bk_sv[i]);
#endif

		HDCPPRINTK("Success: no_zero, and no_one is 20\n");
	} else {
		HDCPPRINTK("Failed: no_zero or no_one is NOT 20\n");

		while (!s5p_hdcp_read_again_bksv()) {
			count++;
			mdelay(200);

			if (count == 14)
				return false;
		}

	}

	return true;
}

static bool s5p_hdcp_compare_r_val(void)
{
	int ret = 0;
	u8 ri[2] = {0, 0};
	u8 rj[2] = {0, 0};
	u16 i;

	for (i = 0; i < R_VAL_RETRY_CNT; i++) {

		if (hdcp_info.auth_status < AKSV_WRITE_DONE) {
			ret = false;

			break;
		}

		ri[0] = readl(hdmi_base + S5P_HDMI_HDCP_Ri_0);
		ri[1] = readl(hdmi_base + S5P_HDMI_HDCP_Ri_1);

		ret = s5p_ddc_read(HDCP_Ri, rj, 2);

		if (ret < 0) {
			HDCPPRINTK("Can't read r data from i2c bus\n");

			return false;
		}

#ifdef S5P_HDCP_AUTH_DEBUG
		AUTHPRINTK("retries :: %d\n", i);
		printk(KERN_INFO "\t\t\t Rx(ddc) ->");
		printk(KERN_INFO "rj[0]: 0x%02x, rj[1]: 0x%02x\n",
			rj[0], rj[1]);
		printk(KERN_INFO "\t\t\t Tx(register) ->");
		printk(KERN_INFO "ri[0]: 0x%02x, ri[1]: 0x%02x\n",
			ri[0], ri[1]);
#endif

		if ((ri[0] == rj[0]) && (ri[1] == rj[1]) && (ri[0] | ri[1])) {
			writel(S5P_HDMI_HDCP_Ri_MATCH_RESULT_Y,
				hdmi_base + S5P_HDMI_HDCP_CHECK_RESULT);
			HDCPPRINTK("R0, R0' is matched!!\n");
			ret = true;

			break;
		} else {
			writel(S5P_HDMI_HDCP_Ri_MATCH_RESULT_N,
				hdmi_base + S5P_HDMI_HDCP_CHECK_RESULT);
			HDCPPRINTK("R0, R0' is not matched!!\n");
			ret = false;
		}

		ri[0] = 0;
		ri[1] = 0;
		rj[0] = 0;
		rj[1] = 0;
	}

	if (!ret) {
		hdcp_info.event 	= HDCP_EVENT_STOP;
		hdcp_info.auth_status 	= NOT_AUTHENTICATED;
	}

	return ret ? true : false;
}

static void s5p_hdcp_reset_authentication(void)
{
	u8 reg;

	spin_lock_irq(&hdcp_info.reset_lock);

	hdcp_info.time_out 	= INFINITE;
	hdcp_info.event 	= HDCP_EVENT_STOP;
	hdcp_info.auth_status 	= NOT_AUTHENTICATED;

	writeb(0x0, hdmi_base + S5P_HDMI_HDCP_CTRL1);
	writeb(0x0, hdmi_base + S5P_HDMI_HDCP_CTRL2);
	s5p_hdcp_hdmi_mute_en(true);

	HDCPPRINTK("Stop Encryption by reset!!\n");
	writeb(S5P_HDMI_HDCP_ENC_DISABLE, hdmi_base + S5P_HDMI_ENC_EN);

	HDCPPRINTK("Now reset authentication\n");

	reg = readb(hdmi_base + S5P_HDMI_STATUS_EN);
	reg &= S5P_HDMI_INT_DIS_ALL;
	writeb(reg, hdmi_base + S5P_HDMI_STATUS_EN);

	writeb(S5P_HDMI_HDCP_CLR_ALL_RESULTS,
				hdmi_base + S5P_HDMI_HDCP_CHECK_RESULT);

	sw_reset = true;
	reg = s5p_hdmi_get_enabled_interrupt();

	s5p_hdmi_disable_interrupts(HDMI_IRQ_HPD_PLUG);
	s5p_hdmi_disable_interrupts(HDMI_IRQ_HPD_UNPLUG);
	s5p_hdmi_sw_hpd_enable(true);
	s5p_hdmi_set_hpd_onoff(false);
	s5p_hdmi_set_hpd_onoff(true);
	s5p_hdmi_sw_hpd_enable(false);

	if (reg & 1<<HDMI_IRQ_HPD_PLUG)
		s5p_hdmi_enable_interrupts(HDMI_IRQ_HPD_PLUG);
	if (reg & 1<<HDMI_IRQ_HPD_UNPLUG)
		s5p_hdmi_enable_interrupts(HDMI_IRQ_HPD_UNPLUG);

	sw_reset = false;

	writel(S5P_HDMI_HDCP_CLR_ALL_RESULTS,
		hdmi_base + S5P_HDMI_HDCP_CHECK_RESULT);

	reg = readb(hdmi_base + S5P_HDMI_STATUS_EN);
	reg |= S5P_HDMI_WTFORACTIVERX_INT_OCC |
		S5P_HDMI_WATCHDOG_INT_OCC |
		S5P_HDMI_WRITE_INT_OCC |
		S5P_HDMI_UPDATE_RI_INT_OCC;
	writeb(reg, hdmi_base + S5P_HDMI_STATUS_EN);
	writeb(S5P_HDMI_HDCP_CP_DESIRED_EN, hdmi_base + S5P_HDMI_HDCP_CTRL1);

	spin_unlock_irq(&hdcp_info.reset_lock);
}

static int s5p_hdcp_loadkey(void)
{
	u8 status;

	writeb(S5P_HDMI_EFUSE_CTRL_HDCP_KEY_READ,
				hdmi_base + S5P_HDMI_EFUSE_CTRL);

	do {
		status = readb(hdmi_base + S5P_HDMI_EFUSE_STATUS);
	} while (!(status & S5P_HDMI_EFUSE_ECC_DONE));

	if (readb(hdmi_base + S5P_HDMI_EFUSE_STATUS) &
					S5P_HDMI_EFUSE_ECC_FAIL) {
		HDCPPRINTK("Can't load key from fuse ctrl.\n");

		return -EINVAL;
	}

	return 0;
}

static void s5p_hdmi_start_encryption(void)
{
	u32 time_out = 100;

	if (readl(hdmi_base + S5P_HDMI_HDCP_CHECK_RESULT) ==
				S5P_HDMI_HDCP_Ri_MATCH_RESULT_Y) {

		while (time_out) {

			if (readl(hdmi_base + S5P_HDMI_SYS_STATUS)
					& S5P_HDMI_AUTHEN_ACK_AUTH) {
				writel(S5P_HDMI_HDCP_ENC_ENABLE,
						hdmi_base + S5P_HDMI_ENC_EN);
				HDCPPRINTK("Encryption start!!\n");
				s5p_hdcp_hdmi_mute_en(false);

				break;
			} else {
				time_out--;
				mdelay(1);
			}

		}
	} else {
		writel(S5P_HDMI_HDCP_ENC_DISABLE, hdmi_base + S5P_HDMI_ENC_EN);
		s5p_hdcp_hdmi_mute_en(true);
		HDCPPRINTK("Encryption stop!!\n");
	}
}

static int s5p_hdmi_check_repeater(void)
{
	int ret = 0;
	u8 i = 0;
	u16 j = 0;
	u8 bcaps[BCAPS_SIZE] = {0};
	u8 status[BSTATUS_SIZE] = {0, 0};
	u8 rx_v[SHA_1_HASH_SIZE] = {0};
	u8 ksv_list[HDCP_MAX_DEVS * HDCP_KSV_SIZE] = {0};
	u32 dev_cnt;
	u32 stat;
	bool ksv_fifo_ready = false;

	memset(rx_v, 0x0, SHA_1_HASH_SIZE);
	memset(ksv_list, 0x0, HDCP_MAX_DEVS*HDCP_KSV_SIZE);

	while (j <= 50) {
		ret = s5p_ddc_read(HDCP_Bcaps,
				bcaps, BCAPS_SIZE);

		if (ret < 0) {
			HDCPPRINTK("Can't read bcaps data from i2c bus\n");

			return false;
		}

		if (bcaps[0] & KSV_FIFO_READY) {
			HDCPPRINTK("ksv fifo is ready\n");
			ksv_fifo_ready = true;
			writel(bcaps[0], hdmi_base + S5P_HDMI_HDCP_BCAPS);

			break;
		} else {
			HDCPPRINTK("ksv fifo is not ready\n");
			ksv_fifo_ready = false;
			mdelay(100);
			j++;
		}

		bcaps[0] = 0;
	}

	if (!ksv_fifo_ready)
		return REPEATER_TIMEOUT_ERROR;

	ret = s5p_ddc_read(HDCP_BStatus, status, BSTATUS_SIZE);

	if (ret < 0) {
		HDCPPRINTK("Can't read status data from i2c bus\n");

		return false;
	}

	if (status[1] & MAX_CASCADE_EXCEEDED) {
		HDCPPRINTK("MAX_CASCADE_EXCEEDED\n");

		return MAX_CASCADE_EXCEEDED_ERROR;
	} else if (status[0] & MAX_DEVS_EXCEEDED) {
		HDCPPRINTK("MAX_CASCADE_EXCEEDED\n");

		return MAX_DEVS_EXCEEDED_ERROR;
	}

	writel(status[0], hdmi_base + S5P_HDMI_HDCP_BSTATUS_0);
	writel(status[1], hdmi_base + S5P_HDMI_HDCP_BSTATUS_1);

	dev_cnt = status[0] & 0x7f;

	HDCPPRINTK("status[0] :0x%08x, status[1] :0x%08x!!\n",
							status[0], status[1]);

	if (dev_cnt) {
		u32 val = 0;

		ret = s5p_ddc_read(HDCP_KSVFIFO, ksv_list,
				dev_cnt * HDCP_KSV_SIZE);

		if (ret < 0) {
			HDCPPRINTK("Can't read ksv fifo!!\n");

			return false;
		}

		for (i = 0; i < dev_cnt - 1; i++) {
			writel(ksv_list[(i*5) + 0],
				hdmi_base + S5P_HDMI_HDCP_RX_KSV_0_0);
			writel(ksv_list[(i*5) + 1],
				hdmi_base + S5P_HDMI_HDCP_RX_KSV_0_1);
			writel(ksv_list[(i*5) + 2],
				hdmi_base + S5P_HDMI_HDCP_RX_KSV_0_2);
			writel(ksv_list[(i*5) + 3],
				hdmi_base + S5P_HDMI_HDCP_RX_KSV_0_3);
			writel(ksv_list[(i*5) + 4],
				hdmi_base + S5P_HDMI_HDCP_RX_KSV_0_4);

			mdelay(1);

			writel(S5P_HDMI_HDCP_KSV_WRITE_DONE,
				hdmi_base + S5P_HDMI_HDCP_KSV_LIST_CON);
			mdelay(1);

			stat = readl(hdmi_base +
					S5P_HDMI_HDCP_KSV_LIST_CON);

			if (!(stat & S5P_HDMI_HDCP_KSV_READ))
				return false;

			HDCPPRINTK("HDCP_RX_KSV_1 = 0x%x\n\r",
				readl(hdmi_base +
					S5P_HDMI_HDCP_KSV_LIST_CON));
			HDCPPRINTK("i : %d, dev_cnt : %d, val = 0x%08x\n",
				i, dev_cnt, val);
		}

		writel(ksv_list[(i*5) + 0],
				hdmi_base + S5P_HDMI_HDCP_RX_KSV_0_0);
		writel(ksv_list[(i*5) + 1],
				hdmi_base + S5P_HDMI_HDCP_RX_KSV_0_1);
		writel(ksv_list[(i*5) + 2],
				hdmi_base + S5P_HDMI_HDCP_RX_KSV_0_2);
		writel(ksv_list[(i*5) + 3],
				hdmi_base + S5P_HDMI_HDCP_RX_KSV_0_3);
		writel(ksv_list[(i*5) + 4],
				hdmi_base + S5P_HDMI_HDCP_RX_KSV_0_4);

		mdelay(1);

		val = S5P_HDMI_HDCP_KSV_END | S5P_HDMI_HDCP_KSV_WRITE_DONE;
		writel(val, hdmi_base + S5P_HDMI_HDCP_KSV_LIST_CON);

		HDCPPRINTK("HDCP_RX_KSV_1 = 0x%x\n\r",
			readl(hdmi_base + S5P_HDMI_HDCP_KSV_LIST_CON));
		HDCPPRINTK("i : %d, dev_cnt : %d, val = 0x%08x\n",
			i, dev_cnt, val);
	} else {
		writel(S5P_HDMI_HDCP_KSV_LIST_EMPTY,
			hdmi_base + S5P_HDMI_HDCP_KSV_LIST_CON);
	}

	ret = s5p_ddc_read(HDCP_SHA1, rx_v, SHA_1_HASH_SIZE);

	if (ret < 0) {
		HDCPPRINTK("Can't read sha_1_hash data from i2c bus\n");

		return false;
	}

#ifdef S5P_HDCP_DEBUG
	for (i = 0; i < SHA_1_HASH_SIZE; i++)
		HDCPPRINTK("SHA_1 rx :: %x\n", rx_v[i]);
#endif

	writeb(rx_v[0], hdmi_base + S5P_HDMI_HDCP_RX_SHA1_0_0);
	writeb(rx_v[1], hdmi_base + S5P_HDMI_HDCP_RX_SHA1_0_1);
	writeb(rx_v[2], hdmi_base + S5P_HDMI_HDCP_RX_SHA1_0_2);
	writeb(rx_v[3], hdmi_base + S5P_HDMI_HDCP_RX_SHA1_0_3);
	writeb(rx_v[4], hdmi_base + S5P_HDMI_HDCP_RX_SHA1_1_0);
	writeb(rx_v[5], hdmi_base + S5P_HDMI_HDCP_RX_SHA1_1_1);
	writeb(rx_v[6], hdmi_base + S5P_HDMI_HDCP_RX_SHA1_1_2);
	writeb(rx_v[7], hdmi_base + S5P_HDMI_HDCP_RX_SHA1_1_3);
	writeb(rx_v[8], hdmi_base + S5P_HDMI_HDCP_RX_SHA1_2_0);
	writeb(rx_v[9], hdmi_base + S5P_HDMI_HDCP_RX_SHA1_2_1);
	writeb(rx_v[10], hdmi_base + S5P_HDMI_HDCP_RX_SHA1_2_2);
	writeb(rx_v[11], hdmi_base + S5P_HDMI_HDCP_RX_SHA1_2_3);
	writeb(rx_v[12], hdmi_base + S5P_HDMI_HDCP_RX_SHA1_3_0);
	writeb(rx_v[13], hdmi_base + S5P_HDMI_HDCP_RX_SHA1_3_1);
	writeb(rx_v[14], hdmi_base + S5P_HDMI_HDCP_RX_SHA1_3_2);
	writeb(rx_v[15], hdmi_base + S5P_HDMI_HDCP_RX_SHA1_3_3);
	writeb(rx_v[16], hdmi_base + S5P_HDMI_HDCP_RX_SHA1_4_0);
	writeb(rx_v[17], hdmi_base + S5P_HDMI_HDCP_RX_SHA1_4_1);
	writeb(rx_v[18], hdmi_base + S5P_HDMI_HDCP_RX_SHA1_4_2);
	writeb(rx_v[19], hdmi_base + S5P_HDMI_HDCP_RX_SHA1_4_3);

	mdelay(1);

	stat = readb(hdmi_base + S5P_HDMI_HDCP_SHA_RESULT);

	HDCPPRINTK("auth status %d\n", stat);

	if (stat & S5P_HDMI_HDCP_SHA_VALID_RD) {
		stat = readb(hdmi_base + S5P_HDMI_HDCP_SHA_RESULT);

		if (stat & S5P_HDMI_HDCP_SHA_VALID)
			ret = true;
		else
			ret = false;

	} else {
		HDCPPRINTK("SHA not ready 0x%x \n\r", stat);
		ret = false;
	}

	writeb(0x0, hdmi_base + S5P_HDMI_HDCP_SHA_RESULT);

	return ret;
}

static bool s5p_hdcp_try_read_receiver(void)
{
	u8 i = 0;
	bool ret = false;

	s5p_hdcp_hdmi_mute_en(true);

	for (i = 0; i < 400; i++) {
		mdelay(250);

		if (hdcp_info.auth_status != RECEIVER_READ_READY) {
			HDCPPRINTK("hdcp stat. changed!!"
				"failed attempt no = %d\n\r", i);

			return false;
		}

		ret = s5p_hdcp_read_bcaps();

		if (ret) {
			HDCPPRINTK("succeeded at attempt no= %d \n\r", i);

			return true;

		} else {
			HDCPPRINTK("can't read bcaps!!"
				"failed attempt no=%d\n\r", i);
		}

	}

	return false;
}

bool s5p_hdcp_stop(void)
{
	u32  sfr_val = 0;

	HDCPPRINTK("HDCP ftn. Stop!!\n");

	s5p_hdmi_disable_interrupts(HDMI_IRQ_HDCP);

	hdcp_protocol_status = 0;
	hdcp_info.time_out 	= INFINITE;
	hdcp_info.event 	= HDCP_EVENT_STOP;
	hdcp_info.auth_status 	= NOT_AUTHENTICATED;
	hdcp_info.hdcp_enable 	= false;

	sfr_val = readl(hdmi_base + S5P_HDMI_HDCP_CTRL1);
	sfr_val &= (S5P_HDMI_HDCP_ENABLE_1_1_FEATURE_DIS
			& S5P_HDMI_HDCP_CLEAR_REPEATER_TIMEOUT
			& S5P_HDMI_HDCP_EN_PJ_DIS
			& S5P_HDMI_HDCP_CP_DESIRED_DIS);
	writel(sfr_val, hdmi_base + S5P_HDMI_HDCP_CTRL1);

	s5p_hdmi_sw_hpd_enable(false);

	sfr_val = readl(hdmi_base + S5P_HDMI_STATUS_EN);
	sfr_val &= S5P_HDMI_INT_DIS_ALL;
	writel(sfr_val, hdmi_base + S5P_HDMI_STATUS_EN);

	sfr_val = readl(hdmi_base + S5P_HDMI_SYS_STATUS);
	sfr_val |= S5P_HDMI_INT_EN_ALL;
	writel(sfr_val, hdmi_base + S5P_HDMI_SYS_STATUS);

	HDCPPRINTK("Stop Encryption by Stop!!\n");
	writel(S5P_HDMI_HDCP_ENC_DISABLE, hdmi_base + S5P_HDMI_ENC_EN);
	s5p_hdcp_hdmi_mute_en(true);

	writel(S5P_HDMI_HDCP_Ri_MATCH_RESULT_N,
			hdmi_base + S5P_HDMI_HDCP_CHECK_RESULT);
	writel(S5P_HDMI_HDCP_CLR_ALL_RESULTS,
			hdmi_base + S5P_HDMI_HDCP_CHECK_RESULT);

	return true;
}

bool s5p_hdcp_start(void)
{
	u8 reg;
	u32  sfr_val;

	hdcp_info.event 	= HDCP_EVENT_STOP;
	hdcp_info.time_out 	= INFINITE;
	hdcp_info.auth_status 	= NOT_AUTHENTICATED;

	HDCPPRINTK("HDCP ftn. Start!!\n");

	sw_reset = true;
	reg = s5p_hdmi_get_enabled_interrupt();

	s5p_hdmi_disable_interrupts(HDMI_IRQ_HPD_PLUG);
	s5p_hdmi_disable_interrupts(HDMI_IRQ_HPD_UNPLUG);

	s5p_hdmi_sw_hpd_enable(true);
	s5p_hdmi_set_hpd_onoff(false);
	s5p_hdmi_set_hpd_onoff(true);
	s5p_hdmi_sw_hpd_enable(false);
	s5p_hdmi_set_hpd_onoff(false);

	if (reg & 1<<HDMI_IRQ_HPD_PLUG)
		s5p_hdmi_enable_interrupts(HDMI_IRQ_HPD_PLUG);
	if (reg & 1<<HDMI_IRQ_HPD_UNPLUG)
		s5p_hdmi_enable_interrupts(HDMI_IRQ_HPD_UNPLUG);

	sw_reset = false;
	HDCPPRINTK("Stop Encryption by Start!!\n");

	writel(S5P_HDMI_HDCP_ENC_DISABLE, hdmi_base + S5P_HDMI_ENC_EN);
	s5p_hdcp_hdmi_mute_en(true);

	hdcp_protocol_status = 1;

	if (s5p_hdcp_loadkey() < 0)
		return false;

	writel(S5P_HDMI_GCP_CON_NO_TRAN, hdmi_base + S5P_HDMI_GCP_CON);
	writel(S5P_HDMI_INT_EN_ALL, hdmi_base + S5P_HDMI_STATUS_EN);

	sfr_val = 0;
	sfr_val |= S5P_HDMI_HDCP_CP_DESIRED_EN;
	writel(sfr_val, hdmi_base + S5P_HDMI_HDCP_CTRL1);

	s5p_hdmi_enable_interrupts(HDMI_IRQ_HDCP);

	if (!s5p_hdcp_read_bcaps()) {
		HDCPPRINTK("can't read ddc port!\n");
		s5p_hdcp_reset_authentication();
	}

	hdcp_info.hdcp_enable = true;

	return true;
}

static void s5p_hdcp_bksv_start_bh(void)
{
	bool ret = false;

	HDCPPRINTK("HDCP_EVENT_READ_BKSV_START bh\n");

	hdcp_info.auth_status = RECEIVER_READ_READY;

	ret = s5p_hdcp_read_bcaps();

	if (!ret) {
		ret = s5p_hdcp_try_read_receiver();

		if (!ret) {
			HDCPPRINTK("Can't read bcaps!! retry failed!!\n"
				"\t\t\t\thdcp ftn. will be stopped\n");

			s5p_hdcp_reset_authentication();

			return;
		}

	}

	hdcp_info.auth_status = BCAPS_READ_DONE;

	ret = s5p_hdcp_read_bksv();

	if (!ret) {
		HDCPPRINTK("Can't read bksv!!"
			"hdcp ftn. will be reset\n");

		s5p_hdcp_reset_authentication();

		return;
	}

	hdcp_info.auth_status = BKSV_READ_DONE;

	HDCPPRINTK("authentication status : bksv is done (0x%08x)\n",
		hdcp_info.auth_status);
}

static void s5p_hdcp_second_auth_start_bh(void)
{
	u8 count = 0;
	int reg;
	bool ret = false;
	int ret_err;
	u32 bcaps;

	HDCPPRINTK("HDCP_EVENT_SECOND_AUTH_START bh\n");

	ret = s5p_hdcp_read_bcaps();

	if (!ret) {
		ret = s5p_hdcp_try_read_receiver();

		if (!ret) {
			HDCPPRINTK("Can't read bcaps!! retry failed!!\n"
				"\t\t\t\thdcp ftn. will be stopped\n");

			s5p_hdcp_reset_authentication();

			return;
		}

	}

	bcaps = readl(hdmi_base + S5P_HDMI_HDCP_BCAPS);
	bcaps &= (KSV_FIFO_READY);

	if (!bcaps) {

		HDCPPRINTK("ksv fifo is not ready\n");

		do {
			count++;

			ret = s5p_hdcp_read_bcaps();

			if (!ret) {

				ret = s5p_hdcp_try_read_receiver();
				if (!ret)
					s5p_hdcp_reset_authentication();

				return;
			}

			bcaps = readl(hdmi_base + S5P_HDMI_HDCP_BCAPS);
			bcaps &= (KSV_FIFO_READY);

			if (bcaps) {
				HDCPPRINTK("bcaps retries : %d\n", count);

				break;
			}

			mdelay(100);

			if (!hdcp_info.hdcp_enable) {
				s5p_hdcp_reset_authentication();

				return;
			}

		} while (count <= 50);

		if (count > 50) {
			hdcp_info.time_out = INFINITE;

			writel(readl(hdmi_base + S5P_HDMI_HDCP_CTRL1) |
				(0x1 << 2), hdmi_base + S5P_HDMI_HDCP_CTRL1);

			s5p_hdcp_reset_authentication();

			return;
		}
	}

	HDCPPRINTK("ksv fifo ready\n");

	ret_err = s5p_hdmi_check_repeater();

	if (ret_err == true) {
		u32 flag;

		hdcp_info.auth_status = SECOND_AUTHENTICATION_DONE;
		HDCPPRINTK("second authentication done!!\n");

		flag = readb(hdmi_base + S5P_HDMI_SYS_STATUS);
		HDCPPRINTK("hdcp state : %s authenticated!!\n",
			flag & S5P_HDMI_AUTHEN_ACK_AUTH ? "" : "not not");

		s5p_hdmi_start_encryption();
	} else if (ret_err == false) {
		HDCPPRINTK("repeater check error!!\n");
		s5p_hdcp_reset_authentication();
	} else {
		if (ret_err == REPEATER_ILLEGAL_DEVICE_ERROR) {
			HDCPPRINTK("illegal dev. error!!\n");
			reg = readl(hdmi_base + S5P_HDMI_HDCP_CTRL2);

			reg = 0x1;
			writel(reg, hdmi_base + S5P_HDMI_HDCP_CTRL2);

			reg = 0x0;
			writel(reg, hdmi_base + S5P_HDMI_HDCP_CTRL2);

			hdcp_info.auth_status = NOT_AUTHENTICATED;

		} else if (ret_err == REPEATER_TIMEOUT_ERROR) {
			reg = readl(hdmi_base + S5P_HDMI_HDCP_CTRL1);

			reg |= S5P_HDMI_HDCP_SET_REPEATER_TIMEOUT;
			writel(reg, hdmi_base + S5P_HDMI_HDCP_CTRL1);

			reg &= ~S5P_HDMI_HDCP_SET_REPEATER_TIMEOUT;
			writel(reg, hdmi_base + S5P_HDMI_HDCP_CTRL1);

			hdcp_info.auth_status = NOT_AUTHENTICATED;
		} else {
			HDCPPRINTK("repeater check error(MAX_EXCEEDED)!!\n");
			s5p_hdcp_reset_authentication();
		}

	}
}

static bool s5p_hdcp_s5p_hdcp_write_aksv_start_bh(void)
{
	bool ret = false;

	HDCPPRINTK("HDCP_EVENT_WRITE_AKSV_START bh\n");

	if (hdcp_info.auth_status != BKSV_READ_DONE) {
		HDCPPRINTK("bksv is not ready!!\n");

		return false;
	}

	ret = s5p_hdcp_write_an();

	if (!ret)
		return false;

	hdcp_info.auth_status = AN_WRITE_DONE;

	HDCPPRINTK("an write done!!\n");

	ret = s5p_hdcp_write_aksv();

	if (!ret)
		return false;

	mdelay(100);

	hdcp_info.auth_status = AKSV_WRITE_DONE;

	HDCPPRINTK("aksv write done!!\n");

	return ret;
}

static bool s5p_hdcp_check_ri_start_bh(void)
{
	bool ret = false;

	HDCPPRINTK("HDCP_EVENT_CHECK_RI_START bh\n");

	if (hdcp_info.auth_status == AKSV_WRITE_DONE ||
	    hdcp_info.auth_status == FIRST_AUTHENTICATION_DONE ||
	    hdcp_info.auth_status == SECOND_AUTHENTICATION_DONE) {
		ret = s5p_hdcp_compare_r_val();

		if (ret) {

			if (hdcp_info.auth_status == AKSV_WRITE_DONE) {

				if (hdcp_info.is_repeater)
					hdcp_info.auth_status
						= SECOND_AUTHENTICATION_RDY;
				else {
					hdcp_info.auth_status
						= FIRST_AUTHENTICATION_DONE;
					s5p_hdmi_start_encryption();
				}

			}

		} else {
			HDCPPRINTK("authentication reset\n");
			s5p_hdcp_reset_authentication();
		}

		HDCPPRINTK("auth_status = 0x%08x\n",
			hdcp_info.auth_status);

		return true;
	} else {
		s5p_hdcp_reset_authentication();
	}

	HDCPPRINTK("aksv_write or first/second"
		" authentication is not done\n");

	return false;
}

static void s5p_hdcp_work(void *arg)
{
	if (hdcp_info.event & (1 << HDCP_EVENT_READ_BKSV_START)) {
		s5p_hdcp_bksv_start_bh();
		hdcp_info.event &= ~(1 << HDCP_EVENT_READ_BKSV_START);
	}

	if (hdcp_info.event & (1 << HDCP_EVENT_SECOND_AUTH_START)) {
		s5p_hdcp_second_auth_start_bh();
		hdcp_info.event &= ~(1 << HDCP_EVENT_SECOND_AUTH_START);
	}

	if (hdcp_info.event & (1 << HDCP_EVENT_WRITE_AKSV_START)) {
		s5p_hdcp_s5p_hdcp_write_aksv_start_bh();
		hdcp_info.event  &= ~(1 << HDCP_EVENT_WRITE_AKSV_START);
	}

	if (hdcp_info.event & (1 << HDCP_EVENT_CHECK_RI_START)) {
		s5p_hdcp_check_ri_start_bh();
		hdcp_info.event &= ~(1 << HDCP_EVENT_CHECK_RI_START);
	}
}

irqreturn_t s5p_hdcp_irq_handler(int irq)
{
	u32 event = 0;
	u8 flag;

	event = 0;

	flag = readb(hdmi_base + S5P_HDMI_SYS_STATUS);

	HDCPPRINTK("irq_status : 0x%08x\n",
			readb(hdmi_base + S5P_HDMI_SYS_STATUS));

	HDCPPRINTK("hdcp state : %s authenticated!!\n",
		flag & S5P_HDMI_AUTHEN_ACK_AUTH ? "" : "not");

	spin_lock_irq(&hdcp_info.lock);

	if (flag & S5P_HDMI_WTFORACTIVERX_INT_OCC) {
		event |= (1 << HDCP_EVENT_READ_BKSV_START);
		writeb(flag | S5P_HDMI_WTFORACTIVERX_INT_OCC,
			 hdmi_base + S5P_HDMI_SYS_STATUS);
		writeb(0x0, hdmi_base + S5P_HDMI_HDCP_I2C_INT);
	}

	if (flag & S5P_HDMI_WRITE_INT_OCC) {
		event |= (1 << HDCP_EVENT_WRITE_AKSV_START);
		writeb(flag | S5P_HDMI_WRITE_INT_OCC,
			hdmi_base + S5P_HDMI_SYS_STATUS);
		writeb(0x0, hdmi_base + S5P_HDMI_HDCP_AN_INT);
	}

	if (flag & S5P_HDMI_UPDATE_RI_INT_OCC) {
		event |= (1 << HDCP_EVENT_CHECK_RI_START);
		writeb(flag | S5P_HDMI_UPDATE_RI_INT_OCC,
			hdmi_base + S5P_HDMI_SYS_STATUS);
		writeb(0x0, hdmi_base + S5P_HDMI_HDCP_RI_INT);
	}

	if (flag & S5P_HDMI_WATCHDOG_INT_OCC) {
		event |= (1 << HDCP_EVENT_SECOND_AUTH_START);
		writeb(flag | S5P_HDMI_WATCHDOG_INT_OCC,
			hdmi_base + S5P_HDMI_SYS_STATUS);
		writeb(0x0, hdmi_base + S5P_HDMI_HDCP_WDT_INT);
	}

	if (!event) {
		HDCPPRINTK("unknown irq.\n");

		return IRQ_HANDLED;
	}

	hdcp_info.event |= event;
	schedule_work(&hdcp_info.work);
	spin_unlock_irq(&hdcp_info.lock);

	return IRQ_HANDLED;
}

int s5p_hdcp_init(void)
{
	INIT_WORK(&hdcp_info.work, (work_func_t)s5p_hdcp_work);

	init_waitqueue_head(&hdcp_info.waitq);

	spin_lock_init(&hdcp_info.lock);
	spin_lock_init(&hdcp_info.reset_lock);

	s5p_hdmi_register_isr((hdmi_isr)s5p_hdcp_irq_handler,
		(u8)HDMI_IRQ_HDCP);

	return 0;
}

int s5p_hdcp_encrypt_stop(bool on)
{
	u32 reg;

	if (hdcp_info.hdcp_enable) {
		writeb(0x0, hdmi_base + S5P_HDMI_HDCP_I2C_INT);
		writeb(0x0, hdmi_base + S5P_HDMI_HDCP_AN_INT);
		writeb(0x0, hdmi_base + S5P_HDMI_HDCP_RI_INT);
		writeb(0x0, hdmi_base + S5P_HDMI_HDCP_WDT_INT);

		writel(S5P_HDMI_HDCP_ENC_DISABLE, hdmi_base + S5P_HDMI_ENC_EN);
		s5p_hdcp_hdmi_mute_en(true);

		if (!sw_reset) {
			reg = readl(hdmi_base + S5P_HDMI_HDCP_CTRL1);

			if (on) {
				writel(reg | S5P_HDMI_HDCP_CP_DESIRED_EN,
					hdmi_base + S5P_HDMI_HDCP_CTRL1);
				s5p_hdmi_enable_interrupts(HDMI_IRQ_HDCP);
			} else {
				hdcp_info.event
					= HDCP_EVENT_STOP;
				hdcp_info.auth_status
					= NOT_AUTHENTICATED;
				writel(reg & ~S5P_HDMI_HDCP_CP_DESIRED_EN,
					hdmi_base + S5P_HDMI_HDCP_CTRL1);
				s5p_hdmi_disable_interrupts(HDMI_IRQ_HDCP);
			}
		}

		HDCPPRINTK("Stop Encryption by HPD Event!!\n");
	}

	return 0;
}
