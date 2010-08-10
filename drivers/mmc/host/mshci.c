/*
 *  linux/drivers/mmc/host/sdhci.c - Secure Digital Host Controller Interface driver
 *
 *  Copyright (C) 2005-2008 Pierre Ossman, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * Thanks to the following companies for their support:
 *
 *     - JMicron (hardware and technical support)
 */
#include <linux/delay.h>
#include <linux/highmem.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/scatterlist.h>

#include <linux/leds.h>

#include <linux/mmc/host.h>

#include "mshci.h"

#define DRIVER_NAME "mshci"

#define DBG(f, x...) \
	pr_debug(DRIVER_NAME " [%s()]: " f, __func__,## x)

#define SDHC_CLK_ON 1
#define SDHC_CLK_OFF 0

static unsigned int debug_quirks = 0;

static void mshci_prepare_data(struct mshci_host *, struct mmc_data *);
static void mshci_finish_data(struct mshci_host *);

static void mshci_send_command(struct mshci_host *, struct mmc_command *);
static void mshci_finish_command(struct mshci_host *);


static void mshci_dumpregs(struct mshci_host *host)
{
	printk(KERN_DEBUG DRIVER_NAME ": ============== REGISTER DUMP ==============\n");

	printk(KERN_DEBUG DRIVER_NAME ": Sys addr: 0x%08x \n",
		mshci_readl(host, MSHCI_CTRL));
	printk(KERN_DEBUG DRIVER_NAME ": Blk size: 0x%08x\n",
		mshci_readl(host, MSHCI_PWREN));
	printk(KERN_DEBUG DRIVER_NAME ": Argument: 0x%08x\n",
		mshci_readl(host, MSHCI_CLKDIV));
	printk(KERN_DEBUG DRIVER_NAME ": Present:  0x%08x\n",
		mshci_readl(host, MSHCI_CLKSRC));
 	printk(KERN_DEBUG DRIVER_NAME ": Present:  0x%08x\n",
		mshci_readl(host, MSHCI_CLKSRC));
	printk(KERN_DEBUG DRIVER_NAME ": Present:  0x%08x\n",
		mshci_readl(host, MSHCI_CLKENA));
	printk(KERN_DEBUG DRIVER_NAME ": Present:  0x%08x\n",
		mshci_readl(host, MSHCI_TMOUT));
	printk(KERN_DEBUG DRIVER_NAME ": Present:  0x%08x\n",
		mshci_readl(host, MSHCI_CTYPE));
	printk(KERN_DEBUG DRIVER_NAME ": Present:  0x%08x\n",
		mshci_readl(host, MSHCI_BLKSIZ));
	printk(KERN_DEBUG DRIVER_NAME ": Present:  0x%08x\n",
		mshci_readl(host, MSHCI_BYTCNT));
	printk(KERN_DEBUG DRIVER_NAME ": Present:  0x%08x\n",
		mshci_readl(host, MSHCI_INTMSK));
	printk(KERN_DEBUG DRIVER_NAME ": Present:  0x%08x\n",
		mshci_readl(host, MSHCI_CMDARG));
	printk(KERN_DEBUG DRIVER_NAME ": Present:  0x%08x\n",
		mshci_readl(host, MSHCI_CMD));
	printk(KERN_DEBUG DRIVER_NAME ": Present:  0x%08x\n",
		mshci_readl(host, MSHCI_MINTSTS));
	printk(KERN_DEBUG DRIVER_NAME ": Present:  0x%08x\n",
		mshci_readl(host, MSHCI_RINTSTS));
	printk(KERN_DEBUG DRIVER_NAME ": Present:  0x%08x\n",
		mshci_readl(host, MSHCI_STATUS));
	printk(KERN_DEBUG DRIVER_NAME ": Present:  0x%08x\n",
		mshci_readl(host, MSHCI_FIFOTH));
	printk(KERN_DEBUG DRIVER_NAME ": Present:  0x%08x\n",
		mshci_readl(host, MSHCI_CDETECT));
	printk(KERN_DEBUG DRIVER_NAME ": Present:  0x%08x\n",
		mshci_readl(host, MSHCI_WRTPRT));
	printk(KERN_DEBUG DRIVER_NAME ": Present:  0x%08x\n",
		mshci_readl(host, MSHCI_GPIO));
	printk(KERN_DEBUG DRIVER_NAME ": Present:  0x%08x\n",
		mshci_readl(host, MSHCI_TCBCNT));
	printk(KERN_DEBUG DRIVER_NAME ": Present:  0x%08x\n",
		mshci_readl(host, MSHCI_TBBCNT));
	printk(KERN_DEBUG DRIVER_NAME ": Present:  0x%08x\n",
		mshci_readl(host, MSHCI_HCON));

 	printk(KERN_DEBUG DRIVER_NAME ": ===========================================\n");
}


/*****************************************************************************\
 *                                                                           *
 * Low level functions                                                       *
 *                                                                           *
\*****************************************************************************/

static void mshci_clear_set_irqs(struct mshci_host *host, u32 clear, u32 set)
{
	u32 ier;

	ier = mshci_readl(host, MSHCI_INTMSK);
	ier &= ~clear;
	ier |= set;
	mshci_writel(host, ier, MSHCI_INTMSK);
}

static void mshci_unmask_irqs(struct mshci_host *host, u32 irqs)
{
	mshci_clear_set_irqs(host, 0, irqs);
}

static void mshci_mask_irqs(struct mshci_host *host, u32 irqs)
{
	mshci_clear_set_irqs(host, irqs, 0);
}

static void mshci_set_card_detection(struct mshci_host *host, bool enable)
{
	u32 irqs = INTMSK_CDETECT;

	if (host->quirks & SDHCI_QUIRK_BROKEN_CARD_DETECTION)
		return;

	if (enable)
		mshci_unmask_irqs(host, irqs);
	else
		mshci_mask_irqs(host, irqs);
}

static void mshci_enable_card_detection(struct mshci_host *host)
{
	mshci_set_card_detection(host, true);
}

static void mshci_disable_card_detection(struct mshci_host *host)
{
	mshci_set_card_detection(host, false);
}

static void mshci_reset_ciu(struct mshci_host *host)
{
	u32 timeout = 100;
	u32 ier;

	ier = mshci_readl(host, MSHCI_CTRL);
	ier |= CTRL_RESET;

	mshci_writel(host, ier, MSHCI_CTRL);
	while (mshci_readl(host, MSHCI_CTRL) & CTRL_RESET) {
		if (timeout == 0) {
			printk(KERN_ERR "%s: Reset CTRL never completed.\n",
				mmc_hostname(host->mmc));
			mshci_dumpregs(host);
			return;
		}
		timeout--;
		mdelay(1);
	}
}

static void mshci_reset_fifo(struct mshci_host *host)
{
	u32 timeout = 100;
	u32 ier;

	ier = mshci_readl(host, MSHCI_CTRL);
	ier |= FIFO_RESET;
	
	mshci_writel(host, ier, MSHCI_CTRL);
	while (mshci_readl(host, MSHCI_CTRL) & FIFO_RESET) {
		if (timeout == 0) {
			printk(KERN_ERR "%s: Reset FIFO never completed.\n",
				mmc_hostname(host->mmc));
			mshci_dumpregs(host);
			return;
		}
		timeout--;
		mdelay(1);
	}
}

static void mshci_reset_dma(struct mshci_host *host)
{
	u32 timeout = 100;
	u32 ier;

	ier = mshci_readl(host, MSHCI_CTRL);
	ier |= DMA_RESET;

	mshci_writel(host, ier, MSHCI_CTRL);
	while (mshci_readl(host, MSHCI_CTRL) & DMA_RESET) {
		if (timeout == 0) {
			printk(KERN_ERR "%s: Reset DMA never completed.\n",
				mmc_hostname(host->mmc));
			mshci_dumpregs(host);
			return;
		}
		timeout--;
		mdelay(1);
	}
}

static void mshci_reset(struct mshci_host *host, u8 mask)
{
	u32 uninitialized_var(ier);

	if (host->quirks & SDHCI_QUIRK_NO_CARD_NO_RESET) {
		if (!(mshci_readl(host, MSHCI_CDETECT) &
			CARD_PRESENT))
			return;
	}

/* This SHOUDBE removed. 
	if (host->quirks & SDHCI_QUIRK_RESTORE_IRQS_AFTER_RESET)
		ier = mshci_readl(host, SDHCI_INT_ENABLE);
*/
	mshci_reset_ciu(host);
	mshci_reset_fifo(host);

	if (mask & MSHCI_RESET_ALL)
		mshci_reset_dma(host);

/* This SHOUDBE removed. 
	if (host->quirks & SDHCI_QUIRK_RESTORE_IRQS_AFTER_RESET)
		mshci_clear_set_irqs(host, SDHCI_INT_ALL_MASK, ier);
*/
}

static void mshci_init(struct mshci_host *host)
{
	mshci_reset(host, MSHCI_RESET_ALL);

	/* clear interrupt status */
	mshci_writel(host, INTMSK_ALL, MSHCI_RINTSTS); 

	mshci_clear_set_irqs(host, INTMSK_ALL,
		INTMSK_CDETECT | INTMSK_RE |
		INTMSK_CDONE | INTMSK_DTO | INTMSK_TXDR |
		INTMSK_RXDR | INTMSK_RCRC | INTMSK_DCRC |
		INTMSK_RTO | INTMSK_DRTO | INTMSK_HTO |
		INTMSK_HTO | INTMSK_FRUN | INTMSK_HLE |
		INTMSK_SBE | INTMSK_EBE 
		/* | INTMSK_DMA it SHOULDBE supported */);
}

static void mshci_reinit(struct mshci_host *host)
{
	mshci_init(host);
	mshci_enable_card_detection(host);
}

/*****************************************************************************\
 *                                                                           *
 * Core functions                                                            *
 *                                                                           *
\*****************************************************************************/

static void mshci_read_block_pio(struct mshci_host *host)
{
	unsigned long flags;
	size_t fifo_cnt, len, chunk;
	u32 uninitialized_var(scratch);
	u8 *buf;

	DBG("PIO reading\n");

	fifo_cnt = (mshci_readl(host,MSHCI_STATUS)&FIFO_COUNT)>>17;
	fifo_cnt *= FIFO_WIDTH;
	chunk = 0;

	local_irq_save(flags);

	while (fifo_cnt) {
		if (!sg_miter_next(&host->sg_miter))
			BUG();

		len = min(host->sg_miter.length, fifo_cnt);

		fifo_cnt -= len;
		host->sg_miter.consumed = len;

		buf = host->sg_miter.addr;

		while (len) {
			if (chunk == 0) {
				scratch = mshci_readl(host, MSHCI_FIFODAT);
				chunk = 4;
			}

			*buf = scratch & 0xFF;

			buf++;
			scratch >>= 8;
			chunk--;
			len--;
		}
	}

	sg_miter_stop(&host->sg_miter);

	local_irq_restore(flags);
}

static void mshci_write_block_pio(struct mshci_host *host)
{
	unsigned long flags;
	size_t fifo_cnt, len, chunk;
	u32 scratch;
	u8 *buf;

	DBG("PIO writing\n");

	fifo_cnt = 8;

	fifo_cnt *= FIFO_WIDTH;
	chunk = 0;
	scratch = 0;

	local_irq_save(flags);

	while (fifo_cnt) {
		if (!sg_miter_next(&host->sg_miter)) {

			/* there is a bug. 
			 * Even though transfer is complete, 
			 * TXDR interrupt occurs again.
			 * So, it has to check that it has really 
			 * no next sg buffer or just DTO interrupt 
			 * has not occur yet.
			 */
			 
			if (( host->data->blocks * host->data->blksz ) ==
					host->data_transfered )
				break; /* transfer done but DTO no yet */
			BUG();			
		}
		len = min(host->sg_miter.length, fifo_cnt);

		fifo_cnt -= len;
		host->sg_miter.consumed = len;
		host->data_transfered += len;

		buf = (host->sg_miter.addr);

		while (len) {
			scratch |= (u32)*buf << (chunk * 8);

			buf++;
			chunk++;
			len--;

			if ((chunk == 4) || ((len == 0) && (fifo_cnt == 0))) {
				mshci_writel(host, scratch, MSHCI_FIFODAT);
				chunk = 0;
				scratch = 0;
			}
		}

		
	}
	
	sg_miter_stop(&host->sg_miter);

	local_irq_restore(flags);
}

static void mshci_transfer_pio(struct mshci_host *host)
{
	BUG_ON(!host->data);

	if (host->blocks == 0)
		return;

	if (host->data->flags & MMC_DATA_READ)
		mshci_read_block_pio(host);
	else
		mshci_write_block_pio(host);

	DBG("PIO transfer complete.\n");
}

static char *mshci_kmap_atomic(struct scatterlist *sg, unsigned long *flags)
{
	local_irq_save(*flags);
	return kmap_atomic(sg_page(sg), KM_BIO_SRC_IRQ) + sg->offset;
}

static void mshci_kunmap_atomic(void *buffer, unsigned long *flags)
{
	kunmap_atomic(buffer, KM_BIO_SRC_IRQ);
	local_irq_restore(*flags);
}

static int mshci_adma_table_pre(struct mshci_host *host,
	struct mmc_data *data)
{
	int direction;

	u8 *desc;
	u8 *align;
	dma_addr_t addr;
	dma_addr_t align_addr;
	int len, offset;

	struct scatterlist *sg;
	int i;
	char *buffer;
	unsigned long flags;

	/*
	 * The spec does not specify endianness of descriptor table.
	 * We currently guess that it is LE.
	 */

	if (data->flags & MMC_DATA_READ)
		direction = DMA_FROM_DEVICE;
	else
		direction = DMA_TO_DEVICE;

	/*
	 * The ADMA descriptor table is mapped further down as we
	 * need to fill it with data first.
	 */

	host->align_addr = dma_map_single(mmc_dev(host->mmc),
		host->align_buffer, 128 * 4, direction);
	if (dma_mapping_error(mmc_dev(host->mmc), host->align_addr))
		goto fail;
	BUG_ON(host->align_addr & 0x3);

	host->sg_count = dma_map_sg(mmc_dev(host->mmc),
		data->sg, data->sg_len, direction);
	if (host->sg_count == 0)
		goto unmap_align;

	desc = host->adma_desc;
	align = host->align_buffer;

	align_addr = host->align_addr;

	for_each_sg(data->sg, sg, host->sg_count, i) {
		addr = sg_dma_address(sg);
		len = sg_dma_len(sg);

		/*
		 * The SDHCI specification states that ADMA
		 * addresses must be 32-bit aligned. If they
		 * aren't, then we use a bounce buffer for
		 * the (up to three) bytes that screw up the
		 * alignment.
		 */
		offset = (4 - (addr & 0x3)) & 0x3;
		if (offset) {
			if (data->flags & MMC_DATA_WRITE) {
				buffer = mshci_kmap_atomic(sg, &flags);
				WARN_ON(((long)buffer & PAGE_MASK) > (PAGE_SIZE - 3));
				memcpy(align, buffer, offset);
				mshci_kunmap_atomic(buffer, &flags);
			}

			desc[7] = (align_addr >> 24) & 0xff;
			desc[6] = (align_addr >> 16) & 0xff;
			desc[5] = (align_addr >> 8) & 0xff;
			desc[4] = (align_addr >> 0) & 0xff;

			BUG_ON(offset > 65536);

			desc[3] = (offset >> 8) & 0xff;
			desc[2] = (offset >> 0) & 0xff;

			desc[1] = 0x00;
			desc[0] = 0x21; /* tran, valid */

			align += 4;
			align_addr += 4;

			desc += 8;

			addr += offset;
			len -= offset;
		}

		desc[7] = (addr >> 24) & 0xff;
		desc[6] = (addr >> 16) & 0xff;
		desc[5] = (addr >> 8) & 0xff;
		desc[4] = (addr >> 0) & 0xff;

		BUG_ON(len > 65536);

		desc[3] = (len >> 8) & 0xff;
		desc[2] = (len >> 0) & 0xff;

		desc[1] = 0x00;
		desc[0] = 0x21; /* tran, valid */

		desc += 8;

		/*
		 * If this triggers then we have a calculation bug
		 * somewhere. :/
		 */
		WARN_ON((desc - host->adma_desc) > (128 * 2 + 1) * 4);
	}

	if (host->quirks & SDHCI_QUIRK_NO_ENDATTR_IN_NOPDESC) {
		/*
		* Mark the last descriptor as the terminating descriptor
		*/
		if (desc != host->adma_desc) {
			desc -= 8;
			desc[0] |= 0x2; /* end */
		}
	} else {
	/*
	 * Add a terminating entry.
	 */
	desc[7] = 0;
	desc[6] = 0;
	desc[5] = 0;
	desc[4] = 0;

	desc[3] = 0;
	desc[2] = 0;

	desc[1] = 0x00;
	desc[0] = 0x03; /* nop, end, valid */
	}

	/*
	 * Resync align buffer as we might have changed it.
	 */
	if (data->flags & MMC_DATA_WRITE) {
		dma_sync_single_for_device(mmc_dev(host->mmc),
			host->align_addr, 128 * 4, direction);
	}

	host->adma_addr = dma_map_single(mmc_dev(host->mmc),
		host->adma_desc, (128 * 2 + 1) * 4, DMA_TO_DEVICE);
	if (dma_mapping_error(mmc_dev(host->mmc), host->adma_addr))
		goto unmap_entries;
	BUG_ON(host->adma_addr & 0x3);

	return 0;

unmap_entries:
	dma_unmap_sg(mmc_dev(host->mmc), data->sg,
		data->sg_len, direction);
unmap_align:
	dma_unmap_single(mmc_dev(host->mmc), host->align_addr,
		128 * 4, direction);
fail:
	return -EINVAL;
}

static void mshci_adma_table_post(struct mshci_host *host,
	struct mmc_data *data)
{
	int direction;

	struct scatterlist *sg;
	int i, size;
	u8 *align;
	char *buffer;
	unsigned long flags;

	if (data->flags & MMC_DATA_READ)
		direction = DMA_FROM_DEVICE;
	else
		direction = DMA_TO_DEVICE;

	dma_unmap_single(mmc_dev(host->mmc), host->adma_addr,
		(128 * 2 + 1) * 4, DMA_TO_DEVICE);

	dma_unmap_single(mmc_dev(host->mmc), host->align_addr,
		128 * 4, direction);

	if (data->flags & MMC_DATA_READ) {
		dma_sync_sg_for_cpu(mmc_dev(host->mmc), data->sg,
			data->sg_len, direction);

		align = host->align_buffer;

		for_each_sg(data->sg, sg, host->sg_count, i) {
			if (sg_dma_address(sg) & 0x3) {
				size = 4 - (sg_dma_address(sg) & 0x3);

				buffer = mshci_kmap_atomic(sg, &flags);
				WARN_ON(((long)buffer & PAGE_MASK) > (PAGE_SIZE - 3));
				memcpy(buffer, align, size);
				mshci_kunmap_atomic(buffer, &flags);

				align += 4;
			}
		}
	}

	dma_unmap_sg(mmc_dev(host->mmc), data->sg,
		data->sg_len, direction);
}

static u32 mshci_calc_timeout(struct mshci_host *host, struct mmc_data *data)
{
#if 0 /* it SHOULDBE optimized */
	u8 count;
	unsigned target_timeout, current_timeout;

	/*
	 * If the host controller provides us with an incorrect timeout
	 * value, just skip the check and use 0xE.  The hardware may take
	 * longer to time out, but that's much better than having a too-short
	 * timeout value.
	 */
	if (host->quirks & SDHCI_QUIRK_BROKEN_TIMEOUT_VAL)
		return 0xE;

	/* timeout in us */
	target_timeout = data->timeout_ns / 1000 +
		data->timeout_clks / host->clock;

	if (host->quirks & SDHCI_QUIRK_DATA_TIMEOUT_USES_SDCLK)
		host->timeout_clk = host->clock / 1000;

	/*
	 * Figure out needed cycles.
	 * We do this in steps in order to fit inside a 32 bit int.
	 * The first step is the minimum timeout, which will have a
	 * minimum resolution of 6 bits:
	 * (1) 2^13*1000 > 2^22,
	 * (2) host->timeout_clk < 2^16
	 *     =>
	 *     (1) / (2) > 2^6
	 */
	count = 0;
	current_timeout = (1 << 13) * 1000 / host->timeout_clk;
	while (current_timeout < target_timeout) {
		count++;
		current_timeout <<= 1;
		if (count >= 0xF)
			break;
	}

	if (count >= 0xF) {
		printk(KERN_WARNING "%s: Too large timeout requested!\n",
			mmc_hostname(host->mmc));
		count = 0xE;
	}

	return count;
#endif

	return 0xffffffff; /* this value SHOULD be optimized */
}

static void mshci_set_transfer_irqs(struct mshci_host *host)
{
	u32 pio_irqs = DATA_STATUS;
/* it SHOULDBE implemented
	u32 dma_irqs = SDHCI_INT_DMA_END | SDHCI_INT_ADMA_ERROR;

	if (host->flags & MSHCI_REQ_USE_DMA)
		mshci_clear_set_irqs(host, pio_irqs, dma_irqs);
	else
*/
	mshci_clear_set_irqs(host, 0, pio_irqs);
}

static void mshci_prepare_data(struct mshci_host *host, struct mmc_data *data)
{
	u32 count;

	WARN_ON(host->data);

	if (data == NULL)
		return;

	/* these value SHOULD be adjusted */
	BUG_ON(data->blksz * data->blocks > 0x20000000);
	BUG_ON(data->blksz > host->mmc->max_blk_size);
	BUG_ON(data->blocks > 400000);

	host->data = data;
	host->data_early = 0;

	count = mshci_calc_timeout(host, data);
	mshci_writel(host, count, MSHCI_TMOUT);

	mshci_reset_fifo(host);
	
	if (host->flags & (MSHCI_USE_SDMA | MSHCI_USE_MDMA))
		host->flags |= MSHCI_REQ_USE_DMA;

	/*
	 * FIXME: This doesn't account for merging when mapping the
	 * scatterlist.
	 */
	if (host->flags & MSHCI_REQ_USE_DMA) {
		int broken, i;
		struct scatterlist *sg;

		broken = 0;
		if (host->flags & MSHCI_USE_MDMA) {
			if (host->quirks & SDHCI_QUIRK_32BIT_ADMA_SIZE)
				broken = 1;
		} else {
			if (host->quirks & SDHCI_QUIRK_32BIT_DMA_SIZE)
				broken = 1;
		}

		if (unlikely(broken)) {
			for_each_sg(data->sg, sg, data->sg_len, i) {
				if (sg->length & 0x3) {
					DBG("Reverting to PIO because of "
						"transfer size (%d)\n",
						sg->length);
					host->flags &= ~MSHCI_REQ_USE_DMA;
					break;
				}
			}
		}
	}

	/*
	 * The assumption here being that alignment is the same after
	 * translation to device address space.
	 */

	/* it SHOULDBE checked. Not aligned buffer is OK on DMA transfer.? */
	if (host->flags & MSHCI_REQ_USE_DMA) {
		int i;
		struct scatterlist *sg;

		for_each_sg(data->sg, sg, data->sg_len, i) {
			if (sg->offset & 0x3) {
				DBG("Reverting to PIO because of "
					"bad alignment\n");
				host->flags &= ~MSHCI_REQ_USE_DMA;
				break;
			}
		}
	}

	if (host->flags & MSHCI_REQ_USE_DMA) {
		if (host->flags & MSHCI_USE_MDMA) {
			/*
			 * IT SHOULDBE IMPLEMENTED 
			 */
		} else {
			/*
			 * IT SHOULDBE IMPLEMENTED 
			 */
		}
	}

	if (!(host->flags & MSHCI_REQ_USE_DMA)) {
		int flags;
		
		flags = SG_MITER_ATOMIC;
		if (host->data->flags & MMC_DATA_READ)
			flags |= SG_MITER_TO_SG;
		else
			flags |= SG_MITER_FROM_SG;
		
		sg_miter_start(&host->sg_miter, data->sg, data->sg_len, flags);
		host->blocks = data->blocks;

#if 0 /* It shouldbe check whether it is necessary */
		if (data->flags & MMC_DATA_WRITE) {
			unsigned long flags;
			size_t fifo_cnt, len, chunk;
			u32 scratch;
			u8 *buf;

			fifo_cnt = host->fifo_depth*4;

			while( fifo_cnt != 0 ) {
				if (!sg_miter_next(&host->sg_miter)) {
					BUG();
				}
				len = min(host->sg_miter.length, fifo_cnt);

				fifo_cnt -= len;
				host->sg_miter.consumed = len;

				buf = host->sg_miter.addr;

				while (len) {
					scratch |= (u32)*buf << (chunk * 8);

					buf++;
					chunk++;
					len--;

					if ((chunk == 4) || ((len == 0) && (fifo_cnt == 0))) {
						mshci_writel(host, scratch, MSHCI_FIFODAT);
						chunk = 0;
						scratch = 0;
					}
				}
			}
			sg_miter_stop(&host->sg_miter);
		}
#endif		
	}
	/* set transfered data as 0 */
	host->data_transfered = 0; 
	mshci_set_transfer_irqs(host);

	mshci_writel(host, data->blksz, MSHCI_BLKSIZ);
	mshci_writel(host, (data->blocks * data->blksz), MSHCI_BYTCNT);
}

static u32 mshci_set_transfer_mode(struct mshci_host *host,
	struct mmc_data *data)
{
	u32 ret=0;

	if (data == NULL) {
		return ret;
	}

	WARN_ON(!host->data);

	/* this cmd has data to transmit */
	ret |= CMD_DATA_EXP_BIT; 
	
	if (data->flags & MMC_DATA_WRITE)
		ret |= CMD_RW_BIT;
	if (data->flags & MMC_DATA_STREAM)
		ret |= CMD_TRANSMODE_BIT;

	return ret;
}

static void mshci_finish_data(struct mshci_host *host)
{
	struct mmc_data *data;

	BUG_ON(!host->data);

	data = host->data;
	host->data = NULL;

	if (host->flags & MSHCI_REQ_USE_DMA) {
		if (host->flags & MSHCI_USE_MDMA)
			mshci_adma_table_post(host, data);
		else {
			dma_unmap_sg(mmc_dev(host->mmc), data->sg,
				data->sg_len, (data->flags & MMC_DATA_READ) ?
					DMA_FROM_DEVICE : DMA_TO_DEVICE);
		}
	}

	/*
	 * The specification states that the block count register must
	 * be updated, but it does not specify at what point in the
	 * data flow. That makes the register entirely useless to read
	 * back so we have to assume that nothing made it to the card
	 * in the event of an error.
	 */
	if (data->error)
		data->bytes_xfered = 0;
	else
		data->bytes_xfered = data->blksz * data->blocks;

	if (data->stop) {
		/*
		 * The controller needs a reset of internal state machines
		 * upon error conditions.
		 */
		if (data->error) {
			mshci_reset(host, MSHCI_RESET_ALL);
		}

		mshci_send_command(host, data->stop);
	} else
		tasklet_schedule(&host->finish_tasklet);
}

static void mshci_clock_onoff(struct mshci_host *host, bool val)
{
	volatile u32 loop_count = 0x100000;

	if (val) {
		mshci_writel(host, (0x1<<0), MSHCI_CLKENA);
		mshci_writel(host, 0, MSHCI_CMD);
		mshci_writel(host, CMD_ONLY_CLK, MSHCI_CMD);
		do {
			if (!(mshci_readl(host, MSHCI_CMD) & CMD_STRT_BIT))
				break;
			loop_count--;
		} while (loop_count);
	} else { 
		mshci_writel(host, (0x0<<0), MSHCI_CLKENA);
		mshci_writel(host, 0, MSHCI_CMD);
		mshci_writel(host, CMD_ONLY_CLK, MSHCI_CMD);
		do {
			if (!(mshci_readl(host, MSHCI_CMD) & CMD_STRT_BIT))
				break;
			loop_count--;
		} while (loop_count);
	}
	if (loop_count == 0) {
		printk(KERN_ERR "%s: Clock %s has been failed.\n "
				, mmc_hostname(host->mmc),val ? "ON":"OFF");
	}
}

static void mshci_send_command(struct mshci_host *host, struct mmc_command *cmd)
{
	int flags,ret;
	
	WARN_ON(host->cmd);

	if (host->quirks & SDHCI_QUIRK_CLOCK_OFF) {
		/* clock on */
		mshci_clock_onoff(host, SDHC_CLK_ON);
	}

	/* disable interrupt before issuing cmd to the card. 
		why? SHOULD FIND OUT*/
	mshci_writel(host, (mshci_readl(host, MSHCI_CTRL) & ~INT_ENABLE), 
					MSHCI_CTRL);	

	mod_timer(&host->timer, jiffies + 10 * HZ);

	host->cmd = cmd;

	mshci_prepare_data(host, cmd->data);

	mshci_writel(host, cmd->arg, MSHCI_CMDARG);

	flags = mshci_set_transfer_mode(host, cmd->data);
	
	if ((cmd->flags & MMC_RSP_136) && (cmd->flags & MMC_RSP_BUSY)) {
		printk(KERN_ERR "%s: Unsupported response type!\n",
			mmc_hostname(host->mmc));
		cmd->error = -EINVAL;
		tasklet_schedule(&host->finish_tasklet);
		return;
	}

	if (cmd->flags & MMC_RSP_PRESENT) {
		flags |= CMD_RESP_EXP_BIT;
		if (cmd->flags & MMC_RSP_136)
			flags |= CMD_RESP_LENGTH_BIT;
	}
	if (cmd->flags & MMC_RSP_CRC)
		flags |= CMD_CHECK_CRC_BIT;
	flags |= (cmd->opcode | CMD_STRT_BIT | CMD_WAIT_PRV_DAT_BIT); 

	ret = mshci_readl(host, MSHCI_CMD);
	if (ret & CMD_STRT_BIT)
		printk(KERN_ERR "CMD busy. current cmd %d. last cmd reg 0x%x\n", 
			cmd->opcode, ret);

	mshci_writel(host, flags, MSHCI_CMD);

	/* enable interrupt once command sent to the card. 
		why? SHOULD FIND OUT*/
	mshci_writel(host, (mshci_readl(host, MSHCI_CTRL) | INT_ENABLE), 
					MSHCI_CTRL);	
}

static void mshci_finish_command(struct mshci_host *host)
{
	int i;

	BUG_ON(host->cmd == NULL);

	if (host->cmd->flags & MMC_RSP_PRESENT) {
		if (host->cmd->flags & MMC_RSP_136) {
			/* 
			 * response data are overturned. 
			 * IT SHOULD FIND OUT. it is intended or not.
			 */
			for (i = 0;i < 4;i++) {
				host->cmd->resp[0] = mshci_readl(host, MSHCI_RESP3);
				host->cmd->resp[1] = mshci_readl(host, MSHCI_RESP2);
				host->cmd->resp[2] = mshci_readl(host, MSHCI_RESP1);
				host->cmd->resp[3] = mshci_readl(host, MSHCI_RESP0);
			}
		} else {
			host->cmd->resp[0] = mshci_readl(host, MSHCI_RESP0);
		}
	}

	host->cmd->error = 0;

	/* if data interrupt occurs earlier than command interrupt */
	if (host->data && host->data_early)
		mshci_finish_data(host);

	if (!host->cmd->data)
		tasklet_schedule(&host->finish_tasklet);

	host->cmd = NULL;
}

static void mshci_set_clock(struct mshci_host *host, unsigned int clock)
{
	int div;
	volatile u32 loop_count;

	if (clock == host->clock)
		return;

	/* befor changing clock. clock needs to be off. */
	mshci_clock_onoff(host, CLK_DISABLE);
	
	if (clock == 0)
		goto out;

	if (clock >= host->max_clk) {
		div = 0;
	} else {
		for (div = 1;div < 255;div++) {
			if ((host->max_clk / (div<<1)) <= clock)
				break;
		}
	}

	mshci_writel(host, div, MSHCI_CLKDIV);

	mshci_writel(host, 0, MSHCI_CMD);
	mshci_writel(host, CMD_ONLY_CLK, MSHCI_CMD);
	loop_count = 0x10000000;

	do {
		if (!(mshci_readl(host, MSHCI_CMD) & CMD_STRT_BIT))
			break;
		loop_count--;
	} while(loop_count);
	
	if (loop_count == 0) {
		printk(KERN_ERR "%s: Changing clock has been failed.\n "
				, mmc_hostname(host->mmc));
	}
	mshci_writel(host, mshci_readl(host, MSHCI_CMD)&(~CMD_SEND_CLK_ONLY),
					MSHCI_CMD);

	mshci_clock_onoff(host, CLK_ENABLE);
	
out:
	host->clock = clock;
}

static void mshci_set_power(struct mshci_host *host, unsigned short power)
{
	u8 pwr;

	if (power == (unsigned short)-1)
		pwr = 0;
	else {
		switch (1 << power) {
		/*
		 * IT shuoldbe implemented.
		 */ 
		}
	}
	if (host->pwr == pwr)
		return;

	host->pwr = pwr;

	if (pwr == 0) {
		mshci_writel(host, 0, MSHCI_PWREN);
	} else {
		mshci_writel(host, 0x1, MSHCI_PWREN);
	}
}

/*****************************************************************************\
 *                                                                           *
 * MMC callbacks                                                             *
 *                                                                           *
\*****************************************************************************/

static void mshci_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct mshci_host *host;
	bool present;
	unsigned long flags;

	host = mmc_priv(mmc);

	spin_lock_irqsave(&host->lock, flags);

	WARN_ON(host->mrq != NULL);

	host->mrq = mrq;

	present = !(mshci_readl(host, MSHCI_CDETECT) & CARD_PRESENT);
		
	if (!present || host->flags & MSHCI_DEVICE_DEAD) { 
		host->mrq->cmd->error = -ENOMEDIUM;
		tasklet_schedule(&host->finish_tasklet);
	} else {
		mshci_send_command(host, mrq->cmd);
	}		

	mmiowb();
	spin_unlock_irqrestore(&host->lock, flags);
}

static void mshci_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct mshci_host *host;
	unsigned long flags;

	host = mmc_priv(mmc);

	spin_lock_irqsave(&host->lock, flags);

	if (host->flags & MSHCI_DEVICE_DEAD)
		goto out;

	if (ios->power_mode == MMC_POWER_OFF) {
		mshci_reinit(host);
	}

	if (host->ops->set_ios)
		host->ops->set_ios(host, ios);

	mshci_set_clock(host, ios->clock);

	if (ios->power_mode == MMC_POWER_OFF)
		mshci_set_power(host, -1);
	else 
		mshci_set_power(host, ios->vdd);

	if (ios->bus_width == MMC_BUS_WIDTH_8) 
		mshci_writel(host, (0x1<<16), MSHCI_CTYPE);
	else if (ios->bus_width == MMC_BUS_WIDTH_4)
		mshci_writel(host, (0x1<<0), MSHCI_CTYPE);
	else
		mshci_writel(host, (0x0<<0), MSHCI_CTYPE);

out:
	mmiowb();
	spin_unlock_irqrestore(&host->lock, flags);
}

static int mshci_get_ro(struct mmc_host *mmc)
{
	struct mshci_host *host;
	unsigned long flags;
	int wrtprt;

	host = mmc_priv(mmc);

	spin_lock_irqsave(&host->lock, flags);

	if (host->flags & MSHCI_DEVICE_DEAD)
		wrtprt = 0;
	else
		wrtprt = mshci_readl(host, MSHCI_WRTPRT);

	spin_unlock_irqrestore(&host->lock, flags);

	return (wrtprt & WRTPRT_ON);
}

static void mshci_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
	struct mshci_host *host;
	unsigned long flags;

	host = mmc_priv(mmc);

	spin_lock_irqsave(&host->lock, flags);

	if (host->flags & MSHCI_DEVICE_DEAD)
		goto out;

	if (enable)
		mshci_unmask_irqs(host, SDIO_INT_ENABLE);
	else
		mshci_mask_irqs(host, SDIO_INT_ENABLE);
out:
	mmiowb();

	spin_unlock_irqrestore(&host->lock, flags);
}

static struct mmc_host_ops mshci_ops = {
	.request	= mshci_request,
	.set_ios	= mshci_set_ios,
	.get_ro		= mshci_get_ro,
	.enable_sdio_irq = mshci_enable_sdio_irq,
};

/*****************************************************************************\
 *                                                                           *
 * Tasklets                                                                  *
 *                                                                           *
\*****************************************************************************/

static void mshci_tasklet_card(unsigned long param)
{
	struct mshci_host *host;
	unsigned long flags;

	host = (struct mshci_host*)param;

	spin_lock_irqsave(&host->lock, flags);

	if (mshci_readl(host, MSHCI_CDETECT) & CARD_PRESENT) {
		if (host->mrq) {
			printk(KERN_ERR "%s: Card removed during transfer!\n",
				mmc_hostname(host->mmc));
			printk(KERN_ERR "%s: Resetting controller.\n",
				mmc_hostname(host->mmc));

			mshci_reset(host, MSHCI_RESET_ALL);

			host->mrq->cmd->error = -ENOMEDIUM;
			tasklet_schedule(&host->finish_tasklet);
		}
	}

	spin_unlock_irqrestore(&host->lock, flags);

	mmc_detect_change(host->mmc, msecs_to_jiffies(200));
}

static void mshci_tasklet_finish(unsigned long param)
{
	struct mshci_host *host;
	unsigned long flags;
	struct mmc_request *mrq;

	host = (struct mshci_host*)param;

	spin_lock_irqsave(&host->lock, flags);

	del_timer(&host->timer);

	mrq = host->mrq;

	/*
	 * The controller needs a reset of internal state machines
	 * upon error conditions.
	 */
	if (!(host->flags & MSHCI_DEVICE_DEAD) &&
		(mrq->cmd->error ||
		 (mrq->data && (mrq->data->error ||
		  (mrq->data->stop && mrq->data->stop->error))) ||
		   (host->quirks & SDHCI_QUIRK_RESET_AFTER_REQUEST))) {

		/* Some controllers need this kick or reset won't work here */
		if (host->quirks & SDHCI_QUIRK_CLOCK_BEFORE_RESET) {
			unsigned int clock;

			/* This is to force an update */
			clock = host->clock;
			host->clock = 0;
			mshci_set_clock(host, clock);
		}

		/* Spec says we should do both at the same time, but Ricoh
		   controllers do not like that. */
		mshci_reset(host, MSHCI_RESET_ALL);
	}

	host->mrq = NULL;
	host->cmd = NULL;
	host->data = NULL;

	mmiowb();
	spin_unlock_irqrestore(&host->lock, flags);

	mmc_request_done(host->mmc, mrq);

	if (host->quirks & SDHCI_QUIRK_CLOCK_OFF) {
		/* clock off */
		mshci_clock_onoff(host, SDHC_CLK_OFF);
	}
}

static void mshci_timeout_timer(unsigned long data)
{
	struct mshci_host *host;
	unsigned long flags;

	host = (struct mshci_host*)data;

	spin_lock_irqsave(&host->lock, flags);

	if (host->mrq) {
		printk(KERN_ERR "%s: Timeout waiting for hardware "
			"interrupt.\n", mmc_hostname(host->mmc));
		mshci_dumpregs(host);

		if (host->data) {
			host->data->error = -ETIMEDOUT;
			mshci_finish_data(host);
		} else {
			if (host->cmd)
				host->cmd->error = -ETIMEDOUT;
			else
				host->mrq->cmd->error = -ETIMEDOUT;

			tasklet_schedule(&host->finish_tasklet);
		}
	}

	mmiowb();
	spin_unlock_irqrestore(&host->lock, flags);
}

/*****************************************************************************\
 *                                                                           *
 * Interrupt handling                                                        *
 *                                                                           *
\*****************************************************************************/

static void mshci_cmd_irq(struct mshci_host *host, u32 intmask)
{
	BUG_ON(intmask == 0);

	if (!host->cmd) {
		printk(KERN_ERR "%s: Got command interrupt 0x%08x even "
			"though no command operation was in progress.\n",
			mmc_hostname(host->mmc), (unsigned)intmask);
		mshci_dumpregs(host);
		return;
	}

	if (intmask & INTMSK_RTO)
		host->cmd->error = -ETIMEDOUT;
	else if (intmask & (INTMSK_RCRC | INTMSK_RE))
		host->cmd->error = -EILSEQ;

	if (host->cmd->error) {
		tasklet_schedule(&host->finish_tasklet);
		return;
	}

	if (intmask & INTMSK_CDONE)
		mshci_finish_command(host);
}

#ifdef DEBUG
static void mshci_show_adma_error(struct mshci_host *host)
{
	const char *name = mmc_hostname(host->mmc);
	u8 *desc = host->adma_desc;
	__le32 *dma;
	__le16 *len;
	u8 attr;

	mshci_dumpregs(host);

	while (true) {
		dma = (__le32 *)(desc + 4);
		len = (__le16 *)(desc + 2);
		attr = *desc;

		DBG("%s: %p: DMA 0x%08x, LEN 0x%04x, Attr=0x%02x\n",
		    name, desc, le32_to_cpu(*dma), le16_to_cpu(*len), attr);

		desc += 8;

		if (attr & 2)
			break;
	}
}
#else
static void mshci_show_adma_error(struct mshci_host *host) { }
#endif

static void mshci_data_irq(struct mshci_host *host, u32 intmask)
{
	BUG_ON(intmask == 0);

	if (!host->data) {
		/*
		 * The "data complete" interrupt is also used to
		 * indicate that a busy state has ended. See comment
		 * above in mshci_cmd_irq().
		 */
		if (host->cmd && (host->cmd->flags & MMC_RSP_BUSY)) {
			if (intmask & INTMSK_DTO) {
				mshci_finish_command(host);
				return;
			}
		}

		printk(KERN_ERR "%s: Got data interrupt 0x%08x even "
			"though no data operation was in progress.\n",
			mmc_hostname(host->mmc), (unsigned)intmask);
		mshci_dumpregs(host);

		return;
	}

	if (intmask & INTMSK_HTO) {
		printk(KERN_ERR "%s: Host timeout error\n", 
							mmc_hostname(host->mmc));
		host->data->error = -ETIMEDOUT;
	} else if (intmask & INTMSK_DRTO) {
		printk(KERN_ERR "%s: Data read timeout error\n", 
							mmc_hostname(host->mmc));
		host->data->error = -ETIMEDOUT;
	} else if (intmask & INTMSK_EBE) {
		printk(KERN_ERR "%s: FIFO Endbit/Write no CRC error\n", 
							mmc_hostname(host->mmc));
		host->data->error = -EIO;
	} else if (intmask & INTMSK_DCRC) {
		printk(KERN_ERR "%s: Data CRC error\n", 
							mmc_hostname(host->mmc));
		host->data->error = -EIO;
	} else if (intmask & INTMSK_FRUN) {
		printk(KERN_ERR "%s: FIFO underrun/overrun error\n", 
							mmc_hostname(host->mmc));
		host->data->error = -EIO;
	}

	if (host->data->error)
		mshci_finish_data(host);
	else {
		if (((host->data->flags & MMC_DATA_READ)&&  
				(intmask & (INTMSK_RXDR | INTMSK_DTO))) ||
				((host->data->flags & MMC_DATA_WRITE)&&  
					(intmask & (INTMSK_TXDR))))	
			mshci_transfer_pio(host);
			
		if (intmask & INTMSK_DTO) {
			if (host->cmd) {
				/*
				 * Data managed to finish before the
				 * command completed. Make sure we do
				 * things in the proper order.
				 */
				host->data_early = 1;
			} else {
				mshci_finish_data(host);
			}
		}
	}
}

static irqreturn_t mshci_irq(int irq, void *dev_id)
{
	irqreturn_t result;
	struct mshci_host* host = dev_id;
	u32 intmask;
	int cardint = 0;

	spin_lock(&host->lock);

	intmask = mshci_readl(host, MSHCI_MINTSTS);
		
	if (!intmask || intmask == 0xffffffff) {
		result = IRQ_NONE;
		goto out;
	}

	DBG("*** %s got interrupt: 0x%08x\n",
		mmc_hostname(host->mmc), intmask);

	mshci_writel(host, intmask, MSHCI_RINTSTS);

	if (intmask & (INTMSK_CDETECT))
		tasklet_schedule(&host->card_tasklet);

	intmask &= ~INTMSK_CDETECT;

	if (intmask & CMD_STATUS) {
		mshci_cmd_irq(host, intmask & CMD_STATUS);
	}

	if (intmask & DATA_STATUS) {
		mshci_data_irq(host, intmask & DATA_STATUS);
	}

	intmask &= ~(CMD_STATUS | DATA_STATUS);

	if (intmask & SDIO_INT_ENABLE)
		cardint = 1;

	intmask &= ~SDIO_INT_ENABLE;

	if (intmask) {
		printk(KERN_ERR "%s: Unexpected interrupt 0x%08x.\n",
			mmc_hostname(host->mmc), intmask);
		mshci_dumpregs(host);
	}
	
	result = IRQ_HANDLED;

	mmiowb();
out:
	spin_unlock(&host->lock);

	/*
	 * We have to delay this as it calls back into the driver.
	 */
	if (cardint)
		mmc_signal_sdio_irq(host->mmc);

	return result;
}

/*****************************************************************************\
 *                                                                           *
 * Suspend/resume                                                            *
 *                                                                           *
\*****************************************************************************/

#ifdef CONFIG_PM

int mshci_suspend_host(struct mshci_host *host, pm_message_t state)
{
	int ret;

	mshci_disable_card_detection(host);

	ret = mmc_suspend_host(host->mmc, state);
	if (ret)
		return ret;

	free_irq(host->irq, host);

	return 0;
}

EXPORT_SYMBOL_GPL(mshci_suspend_host);

int mshci_resume_host(struct mshci_host *host)
{
	int ret;

	if (host->flags & (MSHCI_USE_SDMA | MSHCI_USE_MDMA)) {
		if (host->ops->enable_dma)
			host->ops->enable_dma(host);
	}

	ret = request_irq(host->irq, mshci_irq, IRQF_SHARED,
			  mmc_hostname(host->mmc), host);
	if (ret)
		return ret;

	mshci_init(host);
	mmiowb();

	ret = mmc_resume_host(host->mmc);
	if (ret)
		return ret;

	mshci_enable_card_detection(host);

	return 0;
}

EXPORT_SYMBOL_GPL(mshci_resume_host);

#endif /* CONFIG_PM */

/*****************************************************************************\
 *                                                                           *
 * Device allocation/registration                                            *
 *                                                                           *
\*****************************************************************************/

struct mshci_host *mshci_alloc_host(struct device *dev,
	size_t priv_size)
{
	struct mmc_host *mmc;
	struct mshci_host *host;

	WARN_ON(dev == NULL);

	mmc = mmc_alloc_host(sizeof(struct mshci_host) + priv_size, dev);
	if (!mmc)
		return ERR_PTR(-ENOMEM);

	host = mmc_priv(mmc);
	host->mmc = mmc;

	return host;
}

static void mshci_fifo_init(struct mshci_host *host)
{
	int fifo_val, fifo_depth, fifo_threshold;
	
	fifo_val = mshci_readl(host, MSHCI_FIFOTH);
	fifo_depth = ((fifo_val & RX_WMARK)>>16)+1;
	fifo_threshold = fifo_depth/2;
	host->fifo_threshold = fifo_threshold;
	host->fifo_depth = fifo_threshold*2;
	
	printk(KERN_INFO "%s: FIFO WMARK FOR RX 0x%x WX 0x%x. ###########\n",
		mmc_hostname(host->mmc),fifo_depth,((fifo_val & TX_WMARK)>>16)+1  );
	
	fifo_val &= ~(RX_WMARK | TX_WMARK | MSIZE_MASK);

	fifo_val |= (fifo_threshold | (fifo_threshold<<16));
	fifo_val |= MSIZE_1;

	mshci_writel(host, fifo_val, MSHCI_FIFOTH);
}

EXPORT_SYMBOL_GPL(mshci_alloc_host);

int mshci_add_host(struct mshci_host *host)
{
	struct mmc_host *mmc;
	int ret;
	
	WARN_ON(host == NULL);
	if (host == NULL)
		return -EINVAL;

	mmc = host->mmc;

	if (debug_quirks)
		host->quirks = debug_quirks;

	mshci_reset(host, MSHCI_RESET_ALL);

	host->version = mshci_readl(host, MSHCI_VERID);

	/* This DMA flag SHOULD BE ADDED from next version. */
	/* host->flags |= MSHCI_USE_SDMA;*/

	/*
	 * If we use DMA, then it's up to the caller to set the DMA
	 * mask, but PIO does not need the hw shim so we set a new
	 * mask here in that case.
	 */
	if (!(host->flags & (MSHCI_USE_SDMA | MSHCI_USE_MDMA))) {
		host->dma_mask = DMA_BIT_MASK(64);
		mmc_dev(host->mmc)->dma_mask = &host->dma_mask;
	}

	printk(KERN_ERR "%s: Version ID 0x%x.\n",
		mmc_hostname(host->mmc), host->version);
		
	host->max_clk = CLK_SDMMC_MAX;
	
	if (host->max_clk == 0) {
		if (!host->ops->get_max_clock) {
			printk(KERN_ERR
			       "%s: Hardware doesn't specify base clock "
			       "frequency.\n", mmc_hostname(mmc));
			return -ENODEV;
		}
		host->max_clk = host->ops->get_max_clock(host);
	}

/* IT SHOULDBE implemented.
	host->timeout_clk =
		(caps & SDHCI_TIMEOUT_CLK_MASK) >> SDHCI_TIMEOUT_CLK_SHIFT;
	if (host->timeout_clk == 0) {
		if (host->ops->get_timeout_clock) {
			host->timeout_clk = host->ops->get_timeout_clock(host);
		} else if (!(host->quirks &
				SDHCI_QUIRK_DATA_TIMEOUT_USES_SDCLK)) {
			printk(KERN_ERR
			       "%s: Hardware doesn't specify timeout clock "
			       "frequency.\n", mmc_hostname(mmc));
			return -ENODEV;
		}
	}
	if (caps & SDHCI_TIMEOUT_CLK_UNIT)
		host->timeout_clk *= 1000;
*/
	/*
	 * Set host parameters.
	 */
	if(host->ops->get_ro)
		mshci_ops.get_ro = host->ops->get_ro;

	mmc->ops = &mshci_ops;
	mmc->f_min = host->max_clk / 256;
	mmc->f_max = host->max_clk;
	mmc->caps |= MMC_CAP_SDIO_IRQ;

	if (!(host->quirks & SDHCI_QUIRK_FORCE_1_BIT_DATA))
		mmc->caps |= MMC_CAP_4_BIT_DATA;

	if (host->quirks & SDHCI_QUIRK_BROKEN_CARD_DETECTION)
		mmc->caps |= MMC_CAP_NEEDS_POLL;


	mmc->ocr_avail = 0;
	mmc->ocr_avail |= MMC_VDD_32_33|MMC_VDD_33_34;
	mmc->ocr_avail |= MMC_VDD_29_30|MMC_VDD_30_31;


	if (mmc->ocr_avail == 0) {
		printk(KERN_ERR "%s: Hardware doesn't report any "
			"support voltages.\n", mmc_hostname(mmc));
		return -ENODEV;
	}

	spin_lock_init(&host->lock);

	/*
	 * Maximum number of segments. Depends on if the hardware
	 * can do scatter/gather or not.
	 */
	if (host->flags & MSHCI_USE_MDMA)
		mmc->max_hw_segs = 128;
	else if (host->flags & MSHCI_USE_SDMA)
		mmc->max_hw_segs = 1;
	else /* PIO */
		mmc->max_hw_segs = 128;
	
	mmc->max_phys_segs = 128;

	/*
	 * Maximum number of sectors in one transfer. Limited by DMA boundary
	 * size (512KiB).
	 * it SHOUDBE find out how man sector can be transfered.
	 */
	mmc->max_req_size = 0x100000  ;

	/*
	 * Maximum segment size. Could be one segment with the maximum number
	 * of bytes. When doing hardware scatter/gather, each entry cannot
	 * be larger than 64 KiB though.
	 * It SHOULDBE optimized.
	 */
	if (host->flags & MSHCI_USE_MDMA)
		mmc->max_seg_size = 65536;
	else
		mmc->max_seg_size = mmc->max_req_size;

	/*
	 * Maximum block size. This varies from controller to controller and
	 * is specified in the capabilities register.
	 * 
	*/	 
	if (host->quirks & SDHCI_QUIRK_FORCE_BLK_SZ_2048) {
		mmc->max_blk_size = 2;
	} else {
		/* from SD spec 2.0 and MMC spec 4.2, block size has been
		 * fixed to 512 byte.
		 */
		mmc->max_blk_size = 2;
	}

	mmc->max_blk_size = 512 << mmc->max_blk_size;

	/*
	 * Maximum block count.
	 */
	mmc->max_blk_count = 0xffff;

	/*
	 * Init tasklets.
	 */
	tasklet_init(&host->card_tasklet,
		mshci_tasklet_card, (unsigned long)host);
	tasklet_init(&host->finish_tasklet,
		mshci_tasklet_finish, (unsigned long)host);

	setup_timer(&host->timer, mshci_timeout_timer, (unsigned long)host);

	ret = request_irq(host->irq, mshci_irq, IRQF_SHARED,
		mmc_hostname(mmc), host);
	if (ret)
		goto untasklet;

	mshci_init(host);

	mshci_writel(host, (mshci_readl(host, MSHCI_CTRL) | INT_ENABLE), 
					MSHCI_CTRL);		

	mshci_fifo_init(host);

	/* set debounce filter value*/
	mshci_writel(host, 0xfffff, MSHCI_DEBNCE);

	/* clear card type */
	mshci_writel(host, mshci_readl(host, MSHCI_CTYPE), MSHCI_CTYPE);	

#ifdef CONFIG_MMC_DEBUG
	mshci_dumpregs(host);
#endif

	mmiowb();

	mmc_add_host(mmc);

	printk(KERN_INFO "%s: SDHCI controller on %s [%s] using %s\n",
		mmc_hostname(mmc), host->hw_name, dev_name(mmc_dev(mmc)),
		(host->flags & MSHCI_USE_MDMA) ? "ADMA" :
		(host->flags & MSHCI_USE_SDMA) ? "DMA" : "PIO");

	mshci_enable_card_detection(host);

	return 0;

untasklet:
	tasklet_kill(&host->card_tasklet);
	tasklet_kill(&host->finish_tasklet);

	return ret;
}

EXPORT_SYMBOL_GPL(mshci_add_host);

void mshci_remove_host(struct mshci_host *host, int dead)
{
	unsigned long flags;

	if (dead) {
		spin_lock_irqsave(&host->lock, flags);

		host->flags |= MSHCI_DEVICE_DEAD;

		if (host->mrq) {
			printk(KERN_ERR "%s: Controller removed during "
				" transfer!\n", mmc_hostname(host->mmc));

			host->mrq->cmd->error = -ENOMEDIUM;
			tasklet_schedule(&host->finish_tasklet);
		}

		spin_unlock_irqrestore(&host->lock, flags);
	}

	mshci_disable_card_detection(host);

	mmc_remove_host(host->mmc);

	if (!dead)
		mshci_reset(host, MSHCI_RESET_ALL);

	free_irq(host->irq, host);

	del_timer_sync(&host->timer);

	tasklet_kill(&host->card_tasklet);
	tasklet_kill(&host->finish_tasklet);

	kfree(host->adma_desc);
	kfree(host->align_buffer);

	host->adma_desc = NULL;
	host->align_buffer = NULL;
}

EXPORT_SYMBOL_GPL(mshci_remove_host);

void mshci_free_host(struct mshci_host *host)
{
	mmc_free_host(host->mmc);
}

EXPORT_SYMBOL_GPL(mshci_free_host);

/*****************************************************************************\
 *                                                                           *
 * Driver init/exit                                                          *
 *                                                                           *
\*****************************************************************************/

static int __init mshci_drv_init(void)
{
	printk(KERN_INFO DRIVER_NAME
		": Secure Digital Host Controller Interface driver\n");
	printk(KERN_INFO DRIVER_NAME ": Copyright(c) Pierre Ossman\n");

	return 0;
}

static void __exit mshci_drv_exit(void)
{
}

module_init(mshci_drv_init);
module_exit(mshci_drv_exit);

module_param(debug_quirks, uint, 0444);

MODULE_AUTHOR("Pierre Ossman <pierre@ossman.eu>");
MODULE_DESCRIPTION("Secure Digital Host Controller Interface core driver");
MODULE_LICENSE("GPL");

MODULE_PARM_DESC(debug_quirks, "Force certain quirks.");
