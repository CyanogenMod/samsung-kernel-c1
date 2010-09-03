/* linux/drivers/media/video/samsung/tv20/ver_1/hdmi.c
 *
 * Copyright (c) 2009 Samsung Electronics
 *		http://www.samsung.com/
 *
 * Functions for HDMI of Samsung TVOut driver
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
#include <mach/map.h>

#include "mixer.h"
#include "tvout_ver_1.h"
#include "../hpd.h"

#include "hdmi.h"
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

#define I2C_ACK				(1 << 7)
#define I2C_INT				(1 << 5)
#define I2C_PEND			(1 << 4)
#define I2C_INT_CLEAR			(0 << 4)
#define I2C_CLK				(0x41)
#define I2C_CLK_PEND_INT		(I2C_CLK | I2C_INT_CLEAR | I2C_INT)
#define I2C_ENABLE			(1 << 4)
#define I2C_START			(1 << 5)
#define I2C_MODE_MTX			0xC0
#define I2C_MODE_MRX			0x80
#define I2C_IDLE			0

#define STATE_IDLE 			0
#define STATE_TX_EDDC_SEGADDR		1
#define STATE_TX_EDDC_SEGNUM		2
#define STATE_TX_DDC_ADDR		3
#define STATE_TX_DDC_OFFSET		4
#define STATE_RX_DDC_ADDR		5
#define STATE_RX_DDC_DATA		6
#define STATE_RX_ADDR			7
#define STATE_RX_DATA			8
#define STATE_TX_ADDR			9
#define STATE_TX_DATA			10
#define STATE_TX_STOP			11
#define STATE_RX_STOP			12

#define PHY_I2C_ADDRESS       	0x70
#define PHY_REG_MODE_SET_DONE 	0x1F

static struct {
	s32    state;
	u8 	*buffer;
	s32    bytes;
} i2c_hdmi_phy_context;

enum {
	PCLK = 0,
	MUX,
	NO_OF_CLK
};

enum {
	HDMI = 0,
	HDMI_PHY,
	NO_OF_MEM_RES
};

struct s5p_hdmi_ctrl_private_data {
	struct s5p_hdmi_bluescreen 	blue_screen;
	struct s5p_hdmi_color_range 	color_r;

	struct s5p_hdmi_infoframe 	avi;
	struct s5p_hdmi_infoframe 	mpg;
	struct s5p_hdmi_infoframe	spd;

	u8				avi_byte[13];
	u8				mpg_byte[5];
	u8				spd_header[3];
	u8				spd_data[28];

	struct s5p_hdmi_tg 		tg;

	enum s5p_hdmi_audio_type 	audio;
	bool				hpd_status;
	bool				hdcp_en;

	bool				running;

	char			*pow_name;
	struct s5p_tv_clk_info	clk[NO_OF_CLK];
	struct reg_mem_info	reg_mem[NO_OF_MEM_RES];
	struct irq_info		irq;
};

const u8 spd[] = { 	
			0x0, 'S', 'A', 'M', 'S', 'U', 'N', 'G',
			0x0, 'S', '5', 'P', 'V', '2', '1', '0',
			0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
			0x2, 0x0
		};

#define HDMI_IRQ_TOTAL_NUM 6

hdmi_isr hdmi_isr_ftn[HDMI_IRQ_TOTAL_NUM];

spinlock_t 	lock_hdmi;

static bool is_dvi;
static bool av_mute;
static bool audio_en;

/* start: external functions for HPD */
bool s5p_hdcp_stop(void);
/* end: external functions for HPD */

/* private data area */
void __iomem		*hdmi_base;
void __iomem		*i2c_hdmi_phy_base;

irqreturn_t s5p_hdmi_irq(int irq, void *dev_id);

struct s5p_hdmi_ctrl_private_data s5p_hdmi_ctrl_private = {
	.pow_name = "hdmi_pd",

	.clk[PCLK] = {
		.name		= "hdmi",
		.ptr		= NULL
	},
	.clk[MUX] = {
		.name		="sclk_hdmi",
		.ptr		= NULL
	},

	.reg_mem[HDMI] = {
		.name		= "s5p-hdmi",
		.res		= NULL,
		.base		= NULL
	},
	.reg_mem[HDMI_PHY] = {
		.name		= "s5p-i2c-hdmi-phy",
		.res		= NULL,
		.base		= NULL
	},

	.irq = {
		.name		= "s5p-hdmi",
		.handler	= s5p_hdmi_irq,
		.no		= -1
	}
};


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
#else
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

	hdmi_write_32(value_n, hdmi_base + S5P_HDMI_ACR_N0);
	hdmi_write_32(cts, hdmi_base + S5P_HDMI_ACR_MCTS0);
	hdmi_write_32(cts, hdmi_base + S5P_HDMI_ACR_CTS0);

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

static u8 s5p_hdmi_checksum(int size, u8 *data)
{
	u32 i, sum = 0;

	for (i = 0; i < size; i++)
		sum += (u32)(data[i]);

	return (u8)(0x100 - ((0x91 + sum) & 0xff));
}

static s32 s5p_hdmi_corereset(void)
{
	writeb(0x0, hdmi_base + S5P_HDMI_CORE_RSTOUT);

	mdelay(10);

	writeb(0x1, hdmi_base + S5P_HDMI_CORE_RSTOUT);

	return 0;
}

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

	if (s5p_hdmi_i2c_phy_write(PHY_I2C_ADDRESS, 1, buffer) != 0)
		return EINVAL;

	if (s5p_hdmi_i2c_phy_read(PHY_I2C_ADDRESS, size, read_buffer) != 0) {
		HDMIPRINTK("s5p_hdmi_i2c_phy_read failed.\n");

		return EINVAL;
	}

	if (on) {
		s5p_hdmi_phy_down(true, 0x1, 0x5, read_buffer);
		s5p_hdmi_phy_down(true, 0x1, 0x7, read_buffer);
		s5p_hdmi_phy_down(true, 0x5, 0x5, read_buffer);
		s5p_hdmi_phy_down(true, 0x17, 0x0, read_buffer);
		s5p_hdmi_phy_down(true, 0x17, 0x1, read_buffer);
	} else {
		s5p_hdmi_phy_down(false, 0x1, 0x5, read_buffer);
		s5p_hdmi_phy_down(false, 0x1, 0x7, read_buffer);
		s5p_hdmi_phy_down(false, 0x5, 0x5, read_buffer);
		s5p_hdmi_phy_down(false, 0x17, 0x0, read_buffer);
		s5p_hdmi_phy_down(false, 0x17, 0x1, read_buffer);
	}

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

	buffer[0] = PHY_REG_MODE_SET_DONE;
	buffer[1] = 0x00;

	if (s5p_hdmi_i2c_phy_write(PHY_I2C_ADDRESS, 2, buffer) != 0) {
		HDMIPRINTK("s5p_hdmi_i2c_phy_write failed.\n");

		return -1;
	}

	writeb(0x5, i2c_hdmi_phy_base + HDMI_I2C_LC);

	size = sizeof(phy_config[freq][index])
		/ sizeof(phy_config[freq][index][0]);

	memcpy(buffer, phy_config[freq][index], sizeof(buffer));

	if (s5p_hdmi_i2c_phy_write(PHY_I2C_ADDRESS, size, buffer) != 0)
		return -1;

	buffer[0] = 0x01;

	if (s5p_hdmi_i2c_phy_write(PHY_I2C_ADDRESS, 1, buffer) != 0) {
		HDMIPRINTK("s5p_hdmi_i2c_phy_write failed.\n");
		return -1;
	}

#ifdef S5P_HDMI_DEBUG
{
	int i = 0;
	u8 read_buffer[0x40] = {0, };

	/* read data */
	if (s5p_hdmi_i2c_phy_read(PHY_I2C_ADDRESS, size, read_buffer) != 0) {
		HDMIPRINTK("s5p_hdmi_i2c_phy_read failed.\n");

		return -1;
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
	writeb(I2C_IDLE, i2c_hdmi_phy_base + HDMI_I2C_STAT);

	return 0;
}

static s32 s5p_hdmi_set_tg(enum s5p_hdmi_v_fmt mode)
{
	u8 tg;

	struct hdmi_tg_param p = tg_param[mode];
	struct hdmi_v_params v = video_params[mode];

	hdmi_write_16(p.h_fsz, hdmi_base + S5P_HDMI_TG_H_FSZ_L);
	hdmi_write_16(p.hact_st, hdmi_base + S5P_HDMI_TG_HACT_ST_L);
	hdmi_write_16(p.hact_sz, hdmi_base + S5P_HDMI_TG_HACT_SZ_L);

	hdmi_write_16(p.v_fsz, hdmi_base + S5P_HDMI_TG_V_FSZ_L);
	hdmi_write_16(p.vsync, hdmi_base + S5P_HDMI_TG_VSYNC_L);
	hdmi_write_16(p.vsync2, hdmi_base + S5P_HDMI_TG_VSYNC2_L);
	hdmi_write_16(p.vact_st, hdmi_base + S5P_HDMI_TG_VACT_ST_L);
	hdmi_write_16(p.vact_sz, hdmi_base + S5P_HDMI_TG_VACT_SZ_L);

	hdmi_write_16(p.field_chg, hdmi_base + S5P_HDMI_TG_FIELD_CHG_L);

	hdmi_write_16(p.vact_st2, hdmi_base + S5P_HDMI_TG_VACT_ST2_L);

	hdmi_write_16(p.vsync_top, hdmi_base + S5P_HDMI_TG_VSYNC_TOP_HDMI_L);
	hdmi_write_16(p.vsync_bot, hdmi_base + S5P_HDMI_TG_VSYNC_BOT_HDMI_L);
	hdmi_write_16(p.field_top, hdmi_base + S5P_HDMI_TG_FIELD_TOP_HDMI_L);
	hdmi_write_16(p.field_bot, hdmi_base + S5P_HDMI_TG_FIELD_BOT_HDMI_L);

	tg = readb(hdmi_base + S5P_HDMI_TG_CMD);

	hdmi_bit_set(v.interlaced, tg, S5P_HDMI_FIELD);

	writeb(tg, hdmi_base + S5P_HDMI_TG_CMD);

	return 0;
}

static s32 s5p_hdmi_set_clr_depth(enum s5p_hdmi_color_depth cd)
{
	u8 gcp, gcp_con, dc; 

	gcp_con = HDMI_TRANS_EVERY_SYNC;

	switch (cd) {
	case HDMI_CD_48:
		gcp = S5P_HDMI_GCP_BYTE2_CD_48BPP;
		break;

	case HDMI_CD_36:
		gcp = S5P_HDMI_GCP_BYTE2_CD_36BPP;
		dc = S5P_HDMI_DC_CTL_12;
		break;

	case HDMI_CD_30:
		gcp = S5P_HDMI_GCP_BYTE2_CD_30BPP;
		dc = S5P_HDMI_DC_CTL_10;
		break;

	case HDMI_CD_24:
		gcp = S5P_HDMI_GCP_BYTE2_CD_24BPP;
		gcp_con = HDMI_DO_NOT_TANS;
		dc = S5P_HDMI_DC_CTL_8;
		break;

	default:
		HDMIPRINTK("HDMI core does not support"
			"requested Deep Color mode\n");
		return -1;
	}

	writeb(gcp, hdmi_base + S5P_HDMI_GCP_BYTE2);
	writeb(gcp_con,	hdmi_base + S5P_HDMI_GCP_CON);
	writeb(dc, hdmi_base + S5P_HDMI_DC_CONTROL);

	return 0;
}

static s32 s5p_hdmi_set_video(enum s5p_hdmi_v_fmt mode,
				enum s5p_hdmi_pxl_aspect pxl_ratio,
				u8 *avidata)
{
	u8  reg8;
	u32 gcp_con;

	struct hdmi_v_params v = video_params[mode];

	if (mode > (sizeof(video_params) / sizeof(struct hdmi_v_params)) ||
		(s32)mode < 0) {
		HDMIPRINTK("This video mode is not Supported\n");

		return -1;
	}

	writeb(v.polarity, hdmi_base + S5P_HDMI_SYNC_MODE);
	writeb(v.interlaced, hdmi_base + S5P_HDMI_INT_PRO_MODE);

	hdmi_write_16(v.h_blank, hdmi_base + S5P_HDMI_H_BLANK_0);
	hdmi_write_32(v.v_blank, hdmi_base + S5P_HDMI_V_BLANK_0);

	hdmi_write_32(v.hvline, hdmi_base + S5P_HDMI_H_V_LINE_0);

	hdmi_write_32(v.h_sync_gen, hdmi_base + S5P_HDMI_H_SYNC_GEN_0);
	hdmi_write_32(v.v_sync_gen, hdmi_base + S5P_HDMI_V_SYNC_GEN_1_0);

	if (v.interlaced) {
		hdmi_write_32(v.v_blank_f,
			hdmi_base + S5P_HDMI_V_BLANK_F_0);
		hdmi_write_32(v.v_sync_gen2,
			hdmi_base + S5P_HDMI_V_SYNC_GEN_2_0);
		hdmi_write_32(v.v_sync_gen3,
			hdmi_base + S5P_HDMI_V_SYNC_GEN_3_0);
	} else {
		hdmi_write_32(0x0, hdmi_base + S5P_HDMI_V_BLANK_F_0);
		hdmi_write_32(0x1001, hdmi_base + S5P_HDMI_V_SYNC_GEN_2_0);
		hdmi_write_32(0x1001, hdmi_base + S5P_HDMI_V_SYNC_GEN_3_0);
	}

	reg8 = readb(hdmi_base + S5P_HDMI_CON_1);
	reg8 &= ~S5P_HDMI_CON_PXL_REP_RATIO_MASK;

	if (v.repetition) {
		reg8 |= S5P_HDMI_DOUBLE_PIXEL_REPETITION;
		avidata[4] = S5P_HDMI_AVI_PIXEL_REPETITION_DOUBLE;
	} else {
		avidata[4] = 0;
	}

	writeb(reg8, hdmi_base + S5P_HDMI_CON_1);

	if (pxl_ratio == HDMI_PXL_RATIO_16_9)
		avidata[3] = v.avi_vic_16_9;
	else
		avidata[3] = v.avi_vic;

	if (v.interlaced == 1) {
		gcp_con = readb(hdmi_base + S5P_HDMI_GCP_CON);
		gcp_con |=  S5P_HDMI_GCP_CON_EN_1ST_VSYNC |
				S5P_HDMI_GCP_CON_EN_2ST_VSYNC;
	} else {
		gcp_con = readb(hdmi_base + S5P_HDMI_GCP_CON);
		gcp_con &= (~(S5P_HDMI_GCP_CON_EN_1ST_VSYNC |
				S5P_HDMI_GCP_CON_EN_2ST_VSYNC));
	}
	writeb(gcp_con, hdmi_base + S5P_HDMI_GCP_CON);

	return 0;
}

void s5p_hdmi_set_bluescreen_clr(u8 cb_b, u8 y_g, u8 cr_r)
{
	s5p_hdmi_ctrl_private.blue_screen.cb_b = cb_b;
	s5p_hdmi_ctrl_private.blue_screen.y_g = y_g;
	s5p_hdmi_ctrl_private.blue_screen.cr_r = cr_r;

	writeb(cb_b, hdmi_base + S5P_HDMI_BLUE_SCREEN_0);
	writeb(y_g, hdmi_base + S5P_HDMI_BLUE_SCREEN_1);
	writeb(cr_r, hdmi_base + S5P_HDMI_BLUE_SCREEN_2);
}

void s5p_hdmi_set_bluescreen(bool en)
{
	u8 reg = readl(hdmi_base + S5P_HDMI_CON_0);

	if (en)
		s5p_hdmi_ctrl_private.blue_screen.enable = true;
	else
		s5p_hdmi_ctrl_private.blue_screen.enable = false;

	hdmi_bit_set(en, reg, S5P_HDMI_BLUE_SCR_EN);

	writeb(reg, hdmi_base + S5P_HDMI_CON_0);
}

int s5p_hdmi_set_dvi(bool en)
{
	if (en)
		is_dvi = true;
	else
		is_dvi = false;

	return 0;
}

int s5p_hdmi_set_display(enum s5p_tv_disp_mode disp_mode,
				enum s5p_tv_o_mode out_mode)
{
	enum s5p_hdmi_v_fmt hdmi_v_fmt;
	enum s5p_hdmi_pxl_aspect aspect;
	enum s5p_hdmi_transmit avi_type, aui_type;
	enum s5p_hdmi_transmit acp_type;

	u8 mode;
	u8 con;
	u8 pxl;
	u8 sum;
	u8 avi[13];

	bool dvi;

	acp_type = HDMI_DO_NOT_TANS;
	aspect = HDMI_PXL_RATIO_16_9;

	switch (disp_mode) {
	case TVOUT_480P_60_16_9:
		avi[1] = AVI_PAR_16_9 | AVI_ITU601;
		hdmi_v_fmt = v720x480p_60Hz;
		break;

	case TVOUT_480P_60_4_3:
		avi[1] = AVI_PAR_4_3 | AVI_ITU601;
		hdmi_v_fmt = v720x480p_60Hz;
		aspect = HDMI_PXL_RATIO_4_3;
		break;

	case TVOUT_480P_59:
		avi[1] = AVI_PAR_4_3 | AVI_ITU601;
		hdmi_v_fmt = v720x480p_59Hz;
		break;

	case TVOUT_576P_50_16_9:
		avi[1] = AVI_PAR_16_9 | AVI_ITU601;
		hdmi_v_fmt = v720x576p_50Hz;
		break;

	case TVOUT_576P_50_4_3:
		avi[1] = AVI_PAR_4_3 | AVI_ITU601;
		hdmi_v_fmt = v720x576p_50Hz;
		aspect = HDMI_PXL_RATIO_4_3;
		break;

	case TVOUT_720P_60:
		avi[1] = AVI_PAR_16_9 | AVI_ITU709;
		hdmi_v_fmt = v1280x720p_60Hz;
		break;

	case TVOUT_720P_59:
		avi[1] = AVI_PAR_16_9 | AVI_ITU709;
		hdmi_v_fmt = v1280x720p_59Hz;
		break;

	case TVOUT_720P_50:
		avi[1] = AVI_PAR_16_9 | AVI_ITU709;
		hdmi_v_fmt = v1280x720p_50Hz;
		break;

	case TVOUT_1080P_30:
		avi[1] = AVI_PAR_16_9 | AVI_ITU709;
		hdmi_v_fmt = v1920x1080p_30Hz;
		break;

	case TVOUT_1080P_60:
		avi[1] = AVI_PAR_16_9 | AVI_ITU709;
		hdmi_v_fmt = v1920x1080p_60Hz;
		break;

	case TVOUT_1080P_59:
		avi[1] = AVI_PAR_16_9 | AVI_ITU709;
		hdmi_v_fmt = v1920x1080p_59Hz;
		break;

	case TVOUT_1080P_50:
		avi[1] = AVI_PAR_16_9 | AVI_ITU709;
		hdmi_v_fmt = v1920x1080p_50Hz;
		break;

	case TVOUT_1080I_60:
		avi[1] = AVI_PAR_16_9 | AVI_ITU709;
		hdmi_v_fmt = v1920x1080i_60Hz;
		break;

	case TVOUT_1080I_59:
		avi[1] = AVI_PAR_16_9 | AVI_ITU709;
		hdmi_v_fmt = v1920x1080i_59Hz;
		break;

	case TVOUT_1080I_50:
		avi[1] = AVI_PAR_16_9 | AVI_ITU709;
		hdmi_v_fmt = v1920x1080i_50Hz;
		break;

	default:
		HDMIPRINTK(" invalid disp_mode parameter(%d)\n\r", disp_mode);
		return -1;
	}

	avi[1] |= AVI_SAME_WITH_PICTURE_AR;

	if (s5p_hdmi_phy_config(video_params[hdmi_v_fmt].pixel_clock,
			HDMI_CD_24) < 0) {
		HDMIPRINTK("[ERR] s5p_hdmi_phy_config() failed.\n");
		return -1;
	}

	switch (out_mode) {
	case TVOUT_HDMI_RGB:
		avi_type = HDMI_TRANS_EVERY_SYNC;
		aui_type = HDMI_TRANS_EVERY_SYNC;

		avi[0] 	= AVI_RGB_IF;
		con 	= S5P_HDMI_VID_PREAMBLE_EN | S5P_HDMI_GUARD_BAND_EN;
		mode 	= S5P_HDMI_MODE_EN | S5P_HDMI_DVI_MODE_DIS;
		pxl 	= S5P_HDMI_PX_LMT_CTRL_RGB;
		dvi 	= false;

		break;

	case TVOUT_HDMI:
		avi_type = HDMI_TRANS_EVERY_SYNC;
		aui_type = HDMI_TRANS_EVERY_SYNC;

		avi[0] 	= AVI_YCBCR444_IF;
		con 	= S5P_HDMI_VID_PREAMBLE_EN | S5P_HDMI_GUARD_BAND_EN;
		mode	= S5P_HDMI_MODE_EN | S5P_HDMI_DVI_MODE_DIS;
		pxl 	= S5P_HDMI_PX_LMT_CTRL_BYPASS;
		dvi 	= false;

		break;

	case TVOUT_DVI:
		avi_type = HDMI_DO_NOT_TANS;
		aui_type = HDMI_DO_NOT_TANS;

		avi[0] 	= AVI_RGB_IF;
		con 	= S5P_HDMI_VID_PREAMBLE_DIS | S5P_HDMI_GUARD_BAND_DIS;
		mode 	= S5P_HDMI_MODE_DIS | S5P_HDMI_DVI_MODE_EN;
		pxl 	= S5P_HDMI_PX_LMT_CTRL_RGB;
		dvi 	= true;

		break;

	default:
		HDMIPRINTK("invalid out_mode parameter(%d)\n\r", out_mode);
		return -1;
	}

	writeb(pxl, hdmi_base + S5P_HDMI_CON_1);
	writeb(con, hdmi_base + S5P_HDMI_CON_2);
	writeb(mode, hdmi_base + S5P_HDMI_MODE_SEL);

	writeb(acp_type, hdmi_base + S5P_HDMI_ACP_CON);
	writeb(aui_type, hdmi_base + S5P_HDMI_AUI_CON);
	writeb(avi_type, hdmi_base + S5P_HDMI_AVI_CON);

	s5p_hdmi_set_dvi(dvi);
	s5p_hdmi_set_tg(hdmi_v_fmt);
	s5p_hdmi_set_clr_depth(HDMI_CD_24);
	s5p_hdmi_set_video(hdmi_v_fmt, aspect, avi);

	sum = s5p_hdmi_checksum(S5P_HDMI_AVI_SZ, avi);
	writeb(sum, hdmi_base + S5P_HDMI_AVI_CHECK_SUM);
	hdmi_write_l(avi, hdmi_base, S5P_HDMI_AVI_DATA, S5P_HDMI_AVI_SZ);

	return 0;
}

void s5p_hdmi_clr_range(u8 y_min, u8 y_max, u8 c_min, u8 c_max)
{
	writeb(y_max, hdmi_base + S5P_HDMI_YMAX);
	writeb(y_min, hdmi_base + S5P_HDMI_YMIN);
	writeb(c_max, hdmi_base + S5P_HDMI_CMAX);
	writeb(c_min, hdmi_base + S5P_HDMI_CMIN);
}

int s5p_hdmi_set_mpg(enum s5p_hdmi_transmit type, u8 *data)
{
	u8 sum;

	sum = s5p_hdmi_checksum(S5P_HDMI_MPG_SZ, data);

	writeb(type, hdmi_base + S5P_HDMI_MPG_CON);

	writeb(sum, hdmi_base + S5P_HDMI_MPG_CHECK_SUM);

	hdmi_write_l(data, hdmi_base, S5P_HDMI_MPG_DATA, S5P_HDMI_MPG_SZ);

	return 0;
}

int s5p_hdmi_set_spd(enum s5p_hdmi_transmit type, u8 *spd_data)
{
	u8 *buff;
	writeb(type, hdmi_base + S5P_HDMI_SPD_CON);

	hdmi_write_32(0x190183, hdmi_base + S5P_HDMI_SPD_HEADER);

	if (spd_data == NULL) {
		HDMIPRINTK("Set default SPD\n");
		buff = (u8 *) spd;
	} else
		buff = spd_data;

	hdmi_write_l(buff, hdmi_base, S5P_HDMI_SPD_DATA, S5P_HDMI_SPD_SZ);

	return 0;
}

void s5p_hdmi_set_tg_cmd(bool time, bool bt656, bool tg)
{
	u8 reg = 0;

	reg = readb(hdmi_base + S5P_HDMI_TG_CMD);

	hdmi_bit_set(time, reg, S5P_HDMI_GETSYNC_TYPE);
	hdmi_bit_set(bt656, reg, S5P_HDMI_GETSYNC);
	hdmi_bit_set(tg, reg, S5P_HDMI_TG);

	writeb(reg, hdmi_base + S5P_HDMI_TG_CMD);
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

void s5p_hdmi_enable_interrupts(enum s5p_hdmi_interrrupt intr)
{
	u8 reg;

	reg = readb(hdmi_base + S5P_HDMI_INTC_CON);
	writeb(reg | (1 << intr) | S5P_HDMI_INTC_EN_GLOBAL,
		hdmi_base + S5P_HDMI_INTC_CON);
}

void s5p_hdmi_disable_interrupts(enum s5p_hdmi_interrrupt intr)
{
	u8 reg;

	reg = readb(hdmi_base + S5P_HDMI_INTC_CON);
	writeb(reg & ~(1 << intr), hdmi_base + S5P_HDMI_INTC_CON);
}

void s5p_hdmi_clear_pending(enum s5p_hdmi_interrrupt intr)
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

void s5p_hdmi_set_audio(bool en)
{
	if (en)
		audio_en = true;
	else
		audio_en = false;
}

int s5p_hdmi_set_mute(bool en)
{
	if (en)
		av_mute = true;
	else
		av_mute = false;

	return 0;
}

int s5p_hdmi_get_mute(void)
{
	return av_mute ? true : false;
}

int s5p_hdmi_audio_enable(bool en)
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

void s5p_hdmi_mute(bool en)
{
	if (!av_mute) {
		if (en) {
			s5p_hdmi_set_bluescreen(true);
			s5p_hdmi_audio_enable(false);
		} else {
			s5p_hdmi_set_bluescreen(false);
			if (audio_en)
				s5p_hdmi_audio_enable(true);
		}

	}
}

void s5p_hdmi_init(void __iomem *hdmi_addr, void __iomem *hdmi_phy_addr)
{
	u32	reg;

	hdmi_base = hdmi_addr;
	i2c_hdmi_phy_base = hdmi_phy_addr;

	spin_lock_init(&lock_hdmi);

	/* It'll be moved to another file in the future */
#if 1 /* s5pv310 */
	/* HDMI PHY enable */
	reg = readl(S5PV310_VA_PMU + 0x0700);
	reg |= (1 << 0);
	writeb(reg, S5PV310_VA_PMU + 0x0700);
#else /* s5pv210 */
	reg = readl(S3C_VA_SYS + 0xE804);
	reg |= (1 << 0);
	writeb(reg, S3C_VA_SYS + 0xE804);
#endif

	writeb(0x5, i2c_hdmi_phy_base + HDMI_I2C_LC);
}

static void s5p_hdmi_ctrl_init_private(void)
{
	struct s5p_hdmi_ctrl_private_data *st = &s5p_hdmi_ctrl_private;

	st->blue_screen.enable = false;
	st->blue_screen.cb_b = 0;
	st->blue_screen.y_g  = 0;
	st->blue_screen.cr_r = 0;

	st->color_r.y_min = 1;
	st->color_r.y_max = 254;
	st->color_r.c_min = 1;
	st->color_r.c_max = 254;
	st->avi.type = HDMI_DO_NOT_TANS;
	st->avi.data = st->avi_byte;
	st->mpg.type = HDMI_DO_NOT_TANS;
	st->mpg.data = st->mpg_byte;
	memset((void *)(st->avi_byte), 0, 13);
	memset((void *)(st->mpg_byte), 0, 5);
	st->tg.correction_en= false;
	st->tg.bt656_en = false;
	st->tg.tg_en = false;

	st->spd.type = HDMI_TRANS_EVERY_SYNC;
	st->spd.data = NULL;

	st->hdcp_en = false;
	st->audio = HDMI_AUDIO_PCM;

	st->running = false;
}

static bool s5p_hdmi_ctrl_set_reg(
		enum s5p_tv_disp_mode disp, enum s5p_tv_o_mode out)
{
	u32 reg;

	struct s5p_hdmi_ctrl_private_data	*hdmi = &s5p_hdmi_ctrl_private;

	struct s5p_hdmi_infoframe	*mpg = &hdmi->mpg;
	struct s5p_hdmi_infoframe	*spd = &hdmi->spd;

	struct s5p_hdmi_color_range	*cr = &hdmi->color_r;

	struct s5p_hdmi_tg		*tg = &hdmi->tg;

	s5p_hdmi_set_display(disp, out);

	s5p_hdmi_clr_range(cr->y_min, cr->y_max, cr->c_min, cr->c_max);

	s5p_hdmi_set_mpg(mpg->type, mpg->data);
	s5p_hdmi_set_spd(spd->type, spd->data);

	reg = S5P_HDMI_EN;

	switch (hdmi->audio) {
	case HDMI_AUDIO_PCM:
		s5p_hdmi_audio_init(PCM, 44100, 16, 0);
		reg |= S5P_HDMI_ASP_EN;
		break;

	case HDMI_AUDIO_NO:
		break;

	default:
		HDMIPRINTK("invalid hdmi_audio_type(%d)\n\r", audio);
		return false;
	}

	s5p_hpd_set_hdmiint();

	if (hdmi->hdcp_en) {
		writeb(S5P_HDMI_HDCP_ENC_DISABLE, hdmi_base + S5P_HDMI_ENC_EN);
		s5p_hdmi_mute(true);
	}

	writeb(readl(hdmi_base + S5P_HDMI_CON_0) | reg,
		hdmi_base + S5P_HDMI_CON_0);

	tg->tg_en = 1;
	s5p_hdmi_set_tg_cmd(tg->correction_en, tg->bt656_en, tg->tg_en);

	return true;
}

void s5p_hdmi_ctrl_internal_stop(void)
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

	s5p_hdmi_set_tg_cmd(
		s5p_hdmi_ctrl_private.tg.correction_en,
		s5p_hdmi_ctrl_private.tg.bt656_en,
		s5p_hdmi_ctrl_private.tg.tg_en);

	mdelay(100);
}

static void s5p_hdmi_ctrl_clock(bool on)
{
	/* power control function is not implemented yet */
	if (on) {
		//s5pv210_pd_enable(s5p_hdmi_ctrl_private.pow_name);
		clk_enable(s5p_hdmi_ctrl_private.clk[MUX].ptr);
		clk_enable(s5p_hdmi_ctrl_private.clk[PCLK].ptr);
	} else {
		clk_disable(s5p_hdmi_ctrl_private.clk[PCLK].ptr);
		clk_disable(s5p_hdmi_ctrl_private.clk[MUX].ptr);
		//s5pv210_pd_disable(s5p_hdmi_ctrl_private.pow_name);
	}
	
	mdelay(50);
}

void s5p_hdmi_ctrl_stop(void)
{
	if (s5p_hdmi_ctrl_private.running) {
		s5p_hdmi_ctrl_internal_stop();
		s5p_hdmi_ctrl_clock(0);

		s5p_hdmi_ctrl_private.running = false;
	}
}

int s5p_hdmi_ctrl_start(enum s5p_tv_disp_mode disp, enum s5p_tv_o_mode out)
{
	struct s5p_hdmi_ctrl_private_data	*hdmi = &s5p_hdmi_ctrl_private;
	struct s5p_hdmi_bluescreen	*br = &hdmi->blue_screen;

	switch (out) {
	case TVOUT_DVI:
		s5p_hdmi_ctrl_private.audio = HDMI_AUDIO_NO;
		s5p_hdmi_ctrl_private.spd.type = HDMI_DO_NOT_TANS;

	case TVOUT_HDMI:
	case TVOUT_HDMI_RGB:
		break;

	default:
		HDMIPRINTK("invalid out(%d) for HDMISDO\n",
			out);
		goto err_on_s5p_hdmi_start;
	}

	if (s5p_hdmi_ctrl_private.running)
		s5p_hdmi_ctrl_internal_stop();
	else {
		s5p_hdmi_ctrl_clock(1);

		hdmi->running = true;
	}

	s5p_hdmi_set_bluescreen_clr(br->cb_b, br->y_g, br->cr_r);
	s5p_hdmi_set_bluescreen(br->enable);

	if (!s5p_hdmi_ctrl_set_reg(disp, out))
		goto err_on_s5p_hdmi_start;

	return 0;

err_on_s5p_hdmi_start:
	return -1;
}

extern int s5p_tv_map_resource_mem(
		struct platform_device *pdev,
		char *name,
		void __iomem **base,
		struct resource **res);
extern void s5p_tv_unmap_resource_mem(void __iomem *base, struct resource *res);

int s5p_hdmi_ctrl_constructor(struct platform_device *pdev)
{
	int ret;
	int res_i, clk_i, j;

	for (res_i = 0; res_i < NO_OF_MEM_RES; res_i++) {
		ret = s5p_tv_map_resource_mem(
			pdev,
			s5p_hdmi_ctrl_private.reg_mem[res_i].name,
			&(s5p_hdmi_ctrl_private.reg_mem[res_i].base),
			&(s5p_hdmi_ctrl_private.reg_mem[res_i].res));

		if (ret)
			goto err_on_res;
	}

	for (clk_i = PCLK; clk_i < NO_OF_CLK; clk_i++) {
		s5p_hdmi_ctrl_private.clk[clk_i].ptr =
			clk_get(&pdev->dev,
				s5p_hdmi_ctrl_private.clk[clk_i].name);

		if (IS_ERR(s5p_hdmi_ctrl_private.clk[clk_i].ptr)) {
			printk(KERN_ERR	"Failed to find clock %s\n",
				s5p_hdmi_ctrl_private.clk[clk_i].name);
			ret = -ENOENT;
			goto err_on_clk;
		}
	}

	s5p_hdmi_ctrl_private.irq.no =
		platform_get_irq_byname(pdev, s5p_hdmi_ctrl_private.irq.name);

	if (s5p_hdmi_ctrl_private.irq.no < 0) {
		printk("Failed to call platform_get_irq_byname() for %s\n",
			s5p_hdmi_ctrl_private.irq.name);
		ret = s5p_hdmi_ctrl_private.irq.no;
		goto err_on_irq;
	}

	ret = request_irq(
			s5p_hdmi_ctrl_private.irq.no,
			s5p_hdmi_ctrl_private.irq.handler,
			IRQF_DISABLED,
			s5p_hdmi_ctrl_private.irq.name,
			NULL);
	if (ret) {
		printk("Failed to call request_irq() for %s\n",
			s5p_hdmi_ctrl_private.irq.name);
		goto err_on_irq;
	}

	
	s5p_hdmi_ctrl_init_private();
	s5p_hdmi_init(
		s5p_hdmi_ctrl_private.reg_mem[HDMI].base,
		s5p_hdmi_ctrl_private.reg_mem[HDMI_PHY].base);

	return 0;

err_on_irq:
err_on_clk:
	for (j = 0; j < clk_i; j++)
		clk_put(s5p_hdmi_ctrl_private.clk[j].ptr);

err_on_res:
	for (j = 0; j < res_i; j++)
		s5p_tv_unmap_resource_mem(
			s5p_hdmi_ctrl_private.reg_mem[j].base,
			s5p_hdmi_ctrl_private.reg_mem[j].res);

	return ret;	
}

void s5p_hdmi_ctrl_destructor(void)
{
	int i;

	if (s5p_hdmi_ctrl_private.irq.no >= 0)
		free_irq(s5p_hdmi_ctrl_private.irq.no, NULL);

	for (i = 0; i < NO_OF_MEM_RES; i++)
		s5p_tv_unmap_resource_mem(
			s5p_hdmi_ctrl_private.reg_mem[i].base,
			s5p_hdmi_ctrl_private.reg_mem[i].res);

	for (i = PCLK; i < NO_OF_CLK; i++)
		if (s5p_hdmi_ctrl_private.clk[i].ptr) {
			if (s5p_hdmi_ctrl_private.running)
				clk_disable(s5p_hdmi_ctrl_private.clk[i].ptr);
			clk_put(s5p_hdmi_ctrl_private.clk[i].ptr);
		}
}

void s5p_hdmi_ctrl_suspend(void)
{
}

void s5p_hdmi_ctrl_resume(void)
{
}
