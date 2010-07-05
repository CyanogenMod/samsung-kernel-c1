/* linux/arch/arm/mach-s5pv310/include/mach/regs-mfc.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5PV310 - Definition for MFC register
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_REGS_FIMV_H
#define __ASM_ARCH_REGS_FIMV_H __FILE__

#define S3C_FIMVREG(x)			(x)

#define S3C_FIMV_START_ADDR		S3C_FIMVREG(0x0000)
#define S3C_FIMV_END_ADDR		S3C_FIMVREG(0xe008)

#define S3C_FIMV_SW_RESET		S3C_FIMVREG(0x0000)
#define S3C_FIMV_RISC_HOST_INT		S3C_FIMVREG(0x0008)

/* Command from HOST to RISC */
#define S3C_FIMV_HOST2RISC_CMD		S3C_FIMVREG(0x0030)
#define S3C_FIMV_HOST2RISC_ARG1		S3C_FIMVREG(0x0034)
#define S3C_FIMV_HOST2RISC_ARG2		S3C_FIMVREG(0x0038)
#define S3C_FIMV_HOST2RISC_ARG3		S3C_FIMVREG(0x003c)
#define S3C_FIMV_HOST2RISC_ARG4		S3C_FIMVREG(0x0040)

/* Command from RISC to HOST */
#define S3C_FIMV_RISC2HOST_CMD		S3C_FIMVREG(0x0044)
#define S3C_FIMV_RISC2HOST_ARG1		S3C_FIMVREG(0x0048)
#define S3C_FIMV_RISC2HOST_ARG2		S3C_FIMVREG(0x004c)
#define S3C_FIMV_RISC2HOST_ARG3		S3C_FIMVREG(0x0050)
#define S3C_FIMV_RISC2HOST_ARG4		S3C_FIMVREG(0x0054)

#define S3C_FIMV_FW_VERSION		S3C_FIMVREG(0x0058)
#define S3C_FIMV_SYS_MEM_SZ		S3C_FIMVREG(0x005c)
#define S3C_FIMV_FW_STATUS		S3C_FIMVREG(0x0080)

/* Memory controller register */
#define S3C_FIMV_MC_DRAMBASE_ADR_A	S3C_FIMVREG(0x0508)
#define S3C_FIMV_MC_DRAMBASE_ADR_B	S3C_FIMVREG(0x050c)
#define S3C_FIMV_MC_STATUS		S3C_FIMVREG(0x0510)
#define S3C_FIMV_MC_RS_IBASE		S3C_FIMVREG(0x0514)

/* Common register */
#define S3C_FIMV_SYS_MEM_ADR		S3C_FIMVREG(0x0600) /* firmware buffer */
#define S3C_FIMV_CPB_BUF_ADR		S3C_FIMVREG(0x0604) /* stream buffer */
#define S3C_FIMV_DESC_BUF_ADR		S3C_FIMVREG(0x0608) /* descriptor buffer */
#define S3C_FIMV_LUMA_ADR		S3C_FIMVREG(0x0700) /* Luma0 ~ Luma18 */
#define S3C_FIMV_CHROMA_ADR		S3C_FIMVREG(0x0600) /* Chroma0 ~ Chroma18 */

/* H264 decoding */
#define S3C_FIMV_VERT_NB_MV_ADR		S3C_FIMVREG(0x068c) /* vertical neighbor motion vector */
#define S3C_FIMV_VERT_NB_IP_ADR		S3C_FIMVREG(0x0690) /* neighbor pixels for intra pred */
#define S3C_FIMV_MV_ADR			S3C_FIMVREG(0x0780) /* H264 motion vector */

/* H263/MPEG4/MPEG2/VC-1/ decoding */
#define S3C_FIMV_NB_DCAC_ADR		S3C_FIMVREG(0x068c) /* neighbor AC/DC coeff. buffer */
#define S3C_FIMV_UP_NB_MV_ADR		S3C_FIMVREG(0x0690) /* upper neighbor motion vector buffer */
#define S3C_FIMV_SA_MV_ADR		S3C_FIMVREG(0x0694) /* subseq. anchor motion vector buffer */
#define S3C_FIMV_OT_LINE_ADR		S3C_FIMVREG(0x0698) /* overlap transform line buffer */
#define S3C_FIMV_BITPLANE3_ADR		S3C_FIMVREG(0x069c) /* bitplane3 addr */
#define S3C_FIMV_BITPLANE2_ADR		S3C_FIMVREG(0x06a0) /* bitplane2 addr */
#define S3C_FIMV_BITPLANE1_ADR		S3C_FIMVREG(0x06a4) /* bitplane1 addr */
#define S3C_FIMV_SP_ADR			S3C_FIMVREG(0x06a8) /* syntax parser addr */

/* Encoder register */
#define S3C_FIMV_ENC_UP_MV_ADR		S3C_FIMVREG(0x0600) /* upper motion vector addr */
#define S3C_FIMV_ENC_COZERO_FLAG_ADR	S3C_FIMVREG(0x0610) /* direct cozero flag addr */
#define S3C_FIMV_ENC_UP_INTRA_MD_ADR	S3C_FIMVREG(0x0608) /* upper intra MD addr */

#define S3C_FIMV_ENC_UP_INTRA_PRED_ADR	S3C_FIMVREG(0x0740) /* upper intra PRED addr */

#define S3C_FIMV_ENC_NB_DCAC_ADR	S3C_FIMVREG(0x0604) /* entropy engine's neighbor inform and AC/DC coeff. */
#define S3C_FIMV_ENC_REF0_LUMA_ADR	S3C_FIMVREG(0x061c) /* ref0 Luma addr */

#define S3C_FIMV_ENC_REF0_CHROMA_ADR	S3C_FIMVREG(0x0700) /* ref0 Chroma addr */
#define S3C_FIMV_ENC_REF1_LUMA_ADR	S3C_FIMVREG(0x0620) /* ref1 Luma addr */
#define S3C_FIMV_ENC_REF1_CHROMA_ADR	S3C_FIMVREG(0x0704) /* ref1 Chroma addr */
#define S3C_FIMV_ENC_REF2_LUMA_ADR	S3C_FIMVREG(0x0710) /* ref2 Luma addr */
#define S3C_FIMV_ENC_REF2_CHROMA_ADR	S3C_FIMVREG(0x0708) /* ref2 Chroma addr */
#define S3C_FIMV_ENC_REF3_LUMA_ADR	S3C_FIMVREG(0x0714) /* ref3 Luma addr */
#define S3C_FIMV_ENC_REF3_CHROMA_ADR	S3C_FIMVREG(0x070c) /* ref3 Chroma addr */

/* Codec common register */
#define S3C_FIMV_ENC_HSIZE_PX		S3C_FIMVREG(0x0818) /* frame width at encoder */
#define S3C_FIMV_ENC_VSIZE_PX		S3C_FIMVREG(0x081c) /* frame height at encoder */
#define S3C_FIMV_ENC_PROFILE		S3C_FIMVREG(0x0830) /* profile register */
#define S3C_FIMV_ENC_PIC_STRUCT		S3C_FIMVREG(0x083c) /* picture field/frame flag */
#define S3C_FIMV_ENC_LF_CTRL		S3C_FIMVREG(0x0848) /* loop filter control */
#define S3C_FIMV_ENC_ALPHA_OFF		S3C_FIMVREG(0x084c) /* loop filter alpha offset */
#define S3C_FIMV_ENC_BETA_OFF		S3C_FIMVREG(0x0850) /* loop filter beta offset */
#define S3C_FIMV_MR_BUSIF_CTRL		S3C_FIMVREG(0x0854) /* hidden, bus interface ctrl */
#define S3C_FIMV_ENC_PXL_CACHE_CTRL	S3C_FIMVREG(0x0a00) /* pixel cache control */

#define S3C_FIMV_NUM_MASTER		S3C_FIMVREG(0x0b14) /* Number of master port */

/* Channel & stream interface register */
#define S3C_FIMV_SI_RTN_CHID		S3C_FIMVREG(0x2000) /* Return CH instance ID register */
#define S3C_FIMV_SI_CH1_INST_ID		S3C_FIMVREG(0x2040) /* codec instance ID */
#define S3C_FIMV_SI_CH2_INST_ID		S3C_FIMVREG(0x2080) /* codec instance ID */

/* Decoder */
#define S3C_FIMV_SI_VRESOL		S3C_FIMVREG(0x2004) /* vertical resolution of decoder */
#define S3C_FIMV_SI_HRESOL		S3C_FIMVREG(0x2008) /* horizontal resolution of decoder */
#define S3C_FIMV_SI_BUF_NUMBER		S3C_FIMVREG(0x200c) /* number of frames in the decoded pic */
#define S3C_FIMV_SI_DISPLAY_Y_ADR	S3C_FIMVREG(0x2010) /* luma address of displayed pic */
#define S3C_FIMV_SI_DISPLAY_C_ADR	S3C_FIMVREG(0x2014) /* chroma address of displayed pic */
#define S3C_FIMV_SI_FRM_COUNT		S3C_FIMVREG(0x2018) /* the number of frames so far decoded */
#define S3C_FIMV_SI_DISPLAY_STATUS	S3C_FIMVREG(0x201c) /* status of decoded picture */
#define S3C_FIMV_SI_FRAME_TYPE		S3C_FIMVREG(0x2020) /* frame type such as skip/I/P/B */

#define S3C_FIMV_SI_CH1_SB_ST_ADR	S3C_FIMVREG(0x2044) /* start addr of stream buf */
#define S3C_FIMV_SI_CH1_SB_FRM_SIZE	S3C_FIMVREG(0x2048) /* size of stream buf */
#define S3C_FIMV_SI_CH1_DESC_ADR	S3C_FIMVREG(0x204c) /* addr of descriptor buf */
#define S3C_FIMV_SI_CH1_CPB_SIZE	S3C_FIMVREG(0x2058) /* max size of coded pic. buf */
#define S3C_FIMV_SI_CH1_DESC_SIZE	S3C_FIMVREG(0x205c) /* max size of descriptor buf */
#define S3C_FIMV_SI_CH1_RELEASE_BUF	S3C_FIMVREG(0x2060) /* release buffer register */
#define S3C_FIMV_SI_CH1_HOST_WR_ADR	S3C_FIMVREG(0x2064) /* Shared memory address */
#define S3C_FIMV_SI_CH1_DPB_CONF_CTRL	S3C_FIMVREG(0x2068) /* DPB Configuration Control Register */

#define S3C_FIMV_SI_CH2_SB_ST_ADR	S3C_FIMVREG(0x2084) /* start addr of stream buf */
#define S3C_FIMV_SI_CH2_SB_FRM_SIZE	S3C_FIMVREG(0x2088) /* size of stream buf */
#define S3C_FIMV_SI_CH2_DESC_ADR	S3C_FIMVREG(0x208c) /* addr of descriptor buf */
#define S3C_FIMV_SI_CH2_CPB_SIZE	S3C_FIMVREG(0x2098) /* max size of coded pic. buf */
#define S3C_FIMV_SI_CH2_DESC_SIZE	S3C_FIMVREG(0x209c) /* max size of descriptor buf */
#define S3C_FIMV_SI_CH2_RELEASE_BUF	S3C_FIMVREG(0x20a0) /* release buffer register */

#define S3C_FIMV_SI_DIVX311_VRESOL	S3C_FIMVREG(0x2050) /* vertical resolution */
#define S3C_FIMV_SI_DIVX311_HRESOL	S3C_FIMVREG(0x2054) /* horizontal resolution */
#define S3C_FIMV_CRC_LUMA0		S3C_FIMVREG(0x2030) /* luma crc data per frame(or top field) */
#define S3C_FIMV_CRC_CHROMA0		S3C_FIMVREG(0x2034) /* chroma crc data per frame(or top field) */
#define S3C_FIMV_CRC_LUMA1		S3C_FIMVREG(0x2038) /* luma crc data per bottom field */
#define S3C_FIMV_CRC_CHROMA1		S3C_FIMVREG(0x203c) /* chroma crc data per bottom field */

/* Encoder */
#define S3C_FIMV_ENC_SI_STRM_SIZE	S3C_FIMVREG(0x2004) /* stream size */
#define S3C_FIMV_ENC_SI_PIC_CNT		S3C_FIMVREG(0x2008) /* picture count */
#define S3C_FIMV_ENC_SI_WRITE_PTR	S3C_FIMVREG(0x200c) /* write pointer */
#define S3C_FIMV_ENC_SI_SLICE_TYPE	S3C_FIMVREG(0x2010) /* slice type(I/P/B/IDR) */
#define S3C_FIMV_ENCODED_Y_ADDR		S3C_FIMVREG(0x2014) /* the address of the encoded luminance picture*/
#define S3C_FIMV_ENCODED_C_ADDR		S3C_FIMVREG(0x2018) /* the address of the encoded chrominance picture*/

#define S3C_FIMV_ENC_SI_CH1_SB_U_ADR	S3C_FIMVREG(0x2044) /* addr of upper stream buf */
#define S3C_FIMV_ENC_SI_CH1_SB_L_ADR	S3C_FIMVREG(0x2048) /* addr of lower stream buf */
#define S3C_FIMV_ENC_SI_CH1_SB_SIZE	S3C_FIMVREG(0x204c) /* size of stream buf */
#define S3C_FIMV_ENC_SI_CH1_CUR_Y_ADR	S3C_FIMVREG(0x2050) /* current Luma addr */
#define S3C_FIMV_ENC_SI_CH1_CUR_C_ADR	S3C_FIMVREG(0x2054) /* current Chroma addr */
#define S3C_FIMV_ENC_SI_CH1_FRAME_INS	S3C_FIMVREG(0x2058) /* frame insertion control register */
#define S3C_FIMV_ENC_SI_CH1_SLICE_ARG	S3C_FIMVREG(0x205c) /* slice argument */

#define S3C_FIMV_ENC_SI_CH2_SB_U_ADR	S3C_FIMVREG(0x2084) /* addr of upper stream buf */
#define S3C_FIMV_ENC_SI_CH2_SB_L_ADR	S3C_FIMVREG(0x2088) /* addr of lower stream buf */
#define S3C_FIMV_ENC_SI_CH2_SB_SIZE	S3C_FIMVREG(0x208c) /* size of stream buf */
#define S3C_FIMV_ENC_SI_CH2_CUR_Y_ADR	S3C_FIMVREG(0x2090) /* current Luma addr */
#define S3C_FIMV_ENC_SI_CH2_CUR_C_ADR	S3C_FIMVREG(0x2094) /* current Chroma addr */
#define S3C_FIMV_ENC_SI_CH2_FRAME_QP	S3C_FIMVREG(0x2098) /* frame QP */
#define S3C_FIMV_ENC_SI_CH2_SLICE_ARG	S3C_FIMVREG(0x209c) /* slice argument */

#define S3C_FIMV_ENC_STR_BF_U_FULL	S3C_FIMVREG(0xc004) /* upper stream buf full status */
#define S3C_FIMV_ENC_STR_BF_U_EMPTY	S3C_FIMVREG(0xc008) /* upper stream buf empty status */
#define S3C_FIMV_ENC_STR_BF_L_FULL	S3C_FIMVREG(0xc00c) /* lower stream buf full status */
#define S3C_FIMV_ENC_STR_BF_L_EMPTY	S3C_FIMVREG(0xc010) /* lower stream buf empty status */
#define S3C_FIMV_ENC_STR_STATUS		S3C_FIMVREG(0xc018) /* stream buf interrupt status */
#define S3C_FIMV_ENC_SF_EPB_ON_CTRL	S3C_FIMVREG(0xc054) /* stream control */
#define S3C_FIMV_ENC_SF_BUF_CTRL	S3C_FIMVREG(0xc058) /* buffer control */
#define S3C_FIMV_ENC_BF_MODE_CTRL	S3C_FIMVREG(0xc05c) /* fifo level control */

#define S3C_FIMV_ENC_PIC_TYPE_CTRL	S3C_FIMVREG(0xc504) /* pic type level control */
#define S3C_FIMV_ENC_B_RECON_WRITE_ON	S3C_FIMVREG(0xc508) /* B frame recon data write cotrl */
#define S3C_FIMV_ENC_MSLICE_CTRL	S3C_FIMVREG(0xc50c) /* multi slice control */
#define S3C_FIMV_ENC_MSLICE_MB		S3C_FIMVREG(0xc510) /* MB number in the one slice */
#define S3C_FIMV_ENC_MSLICE_BYTE	S3C_FIMVREG(0xc514) /* byte count number for one slice */
#define S3C_FIMV_ENC_CIR_CTRL		S3C_FIMVREG(0xc518) /* number of intra refresh MB */
#define S3C_FIMV_ENC_MAP_FOR_CUR	S3C_FIMVREG(0xc51c) /* linear or 64x32 tiled mode */
#define S3C_FIMV_ENC_PADDING_CTRL	S3C_FIMVREG(0xc520) /* padding control */
#define S3C_FIMV_ENC_INT_MASK		S3C_FIMVREG(0xc528) /* interrupt mask */

#define S3C_FIMV_ENC_RC_CONFIG		S3C_FIMVREG(0xc5a0) /* RC config */
#define S3C_FIMV_ENC_RC_BIT_RATE	S3C_FIMVREG(0xc5a8) /* bit rate */
#define S3C_FIMV_ENC_RC_QBOUND		S3C_FIMVREG(0xc5ac) /* max/min QP */
#define S3C_FIMV_ENC_RC_RPARA		S3C_FIMVREG(0xc5b0) /* rate control reaction coeff. */
#define S3C_FIMV_ENC_RC_MB_CTRL		S3C_FIMVREG(0xc5b4) /* MB adaptive scaling */

/* for MFC F/W 2010/04/09, address changed from 0xc5a4 to 0xd0d0 */
#define S3C_FIMV_ENC_RC_FRAME_RATE	S3C_FIMVREG(0xd0d0) /* frame rate */

/* Encoder for H264 */
#define S3C_FIMV_ENC_ENTRP_MODE		S3C_FIMVREG(0xd004) /* CAVLC or CABAC */
#define S3C_FIMV_ENC_H264_ALPHA_OFF	S3C_FIMVREG(0xd008) /* loop filter alpha offset */
#define S3C_FIMV_ENC_H264_BETA_OFF	S3C_FIMVREG(0xd00c) /* loop filter beta offset */
#define S3C_FIMV_ENC_H264_NUM_OF_REF	S3C_FIMVREG(0xd010) /* number of reference for P/B */
#define S3C_FIMV_ENC_H264_MDINTER_WGT	S3C_FIMVREG(0xd01c) /* inter weighted parameter */
#define S3C_FIMV_ENC_H264_MDINTRA_WGT	S3C_FIMVREG(0xd020) /* intra weighted parameter */
#define S3C_FIMV_ENC_H264_TRANS_FLAG	S3C_FIMVREG(0xd034) /* 8x8 transform flag in PPS & high profile */

/* Encoder for MPEG4 */
#define S3C_FIMV_ENC_MPEG4_QUART_PXL	S3C_FIMVREG(0xe008) /* quarter pel interpolation control */

#endif /* __ASM_ARCH_REGS_FIMV_H */
