/*
 * linux/drivers/media/video/samsung/mfc5x/mfc_buf.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Buffer manager for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/mm.h>

#include "mfc.h"
#include "mfc_mem.h"
#include "mfc_buf.h"
#include "mfc_log.h"
#include "mfc_errno.h"

#undef PRINT_BUF

static struct list_head mfc_alloc_head[MFC_MAX_MEM_PORT_NUM];
/* The free node list sorted by real address */
static struct list_head mfc_free_head[MFC_MAX_MEM_PORT_NUM];

/* FIXME: test locking, add locking mechanisim */
/*
static spinlock_t lock;
*/

static void mfc_print_buf(void)
{
#ifdef PRINT_BUF
	struct list_head *pos;
	struct mfc_alloc_buffer *alloc = NULL;
	struct mfc_free_buffer *free = NULL;
	int port, i;

	for (port = 0; port < mfc_mem_count(); port++) {
		mfc_dbg("---- port %d buffer list ----", port);

		i = 0;
		list_for_each(pos, &mfc_alloc_head[port]) {
			alloc = list_entry(pos, struct mfc_alloc_buffer, list);
			mfc_dbg("[A #%04d] addr: 0x%08x, size: %d",
				i, (unsigned int)alloc->addr, alloc->size);
			mfc_dbg("\t  real: 0x%08lx, user: 0x%08x, owner: %d",
				alloc->real, (unsigned int)alloc->user,
				alloc->owner);
#ifdef CONFIG_S5P_VMEM
			mfc_dbg("\t  vmem cookie: 0x%08x addr: 0x%08lx, size: %d",
				alloc->vmem_cookie, alloc->vmem_addr,
				alloc->vmem_size);
#endif
			i++;
		}

		i = 0;
		list_for_each(pos, &mfc_free_head[port]) {
			free = list_entry(pos, struct mfc_free_buffer, list);
			mfc_dbg("[F #%04d] addr: 0x%08lx, size: %d",
				i, free->real, free->size);
			i++;
		}
	}
#endif
}

static int mfc_put_free_buf(unsigned int addr, int size, int port)
{
	struct list_head *pos, *nxt;
	struct mfc_free_buffer *free;
	struct mfc_free_buffer *next = NULL;
	struct mfc_free_buffer *prev;
	/* 0x00: not merged, 0x01: prev merged, 0x02: next merged */
	int merged = 0x00;

	mfc_dbg("addr: 0x%08x, size: %d, port: %d\n", addr, size, port);

	list_for_each_safe(pos, nxt, &mfc_free_head[port]) {
		next = list_entry(pos, struct mfc_free_buffer, list);

		/*
		 * When the allocated address must be align without VMEM,
		 * the free buffer can be overlap
		 * previous free buffer temporaily
		 * Target buffer will be shrink after this operation
		 */
		if (addr <= next->real) {
			prev = list_entry(pos->prev, struct mfc_free_buffer, list);

			mfc_dbg("prev->addr: 0x%08lx, size: %d", prev->real, prev->size);
			/* merge previous free buffer */
			if (prev && ((prev->real + prev->size) == addr)) {
				addr  = prev->real;
				size += prev->size;

				prev->size = size;

				merged |= 0x01;
				mfc_dbg("auto merge free buffer[p]: addr: 0x%08lx, size: %d",
					prev->real, prev->size);
			}

			mfc_dbg("next->addr: 0x%08lx, size: %d", next->real, next->size);
			/* merge next free buffer */
			if ((addr + size) == next->real) {
				next->real  = addr;
				next->size += size;

				if (merged)
					prev->size = next->size;

				merged |= 0x02;
				mfc_dbg("auto merge free buffer[n]: addr: 0x%08lx, size: %d",
					next->real, next->size);
			}

			break;
		}
	}

	if (!merged) {
		free = (struct mfc_free_buffer *)
			kzalloc(sizeof(struct mfc_free_buffer), GFP_KERNEL);

		if (unlikely(free == NULL))
			return -ENOMEM;

		free->real = addr;
		free->size = size;

		list_add_tail(&free->list, pos);
	}

	/* bi-directional merged */
	else if ((merged & 0x03) == 0x03) {
		list_del(&next->list);
		kfree(next);
	}

	return 0;
}

static unsigned int mfc_get_free_buf(int size, int align, int port)
{
	struct list_head *pos, *nxt;
	struct mfc_free_buffer *free;
	struct mfc_free_buffer *match = NULL;
	int align_size = 0;
	unsigned int addr = 0;

	mfc_dbg("size: %d, align: %d, port: %d\n",
			size, align, port);

	if (list_empty(&mfc_free_head[port])) {
		mfc_err("no free node in mfc buffer\n");

		return 0;
	}

	/* find best fit area */
	list_for_each_safe(pos, nxt, &mfc_free_head[port]) {
		free = list_entry(pos, struct mfc_free_buffer, list);

#ifdef CONFIG_S5P_VMEM
		/*
		 * Align the start address.
		 * We assume the start address of free buffer aligned with 4KB
		 */
		align_size = ALIGN(align_size + size, PAGE_SIZE) - size;

		if (align > PAGE_SIZE) {
			align_size  = ALIGN(free->real, align) - free->real;
			align_size += ALIGN(align_size + size, PAGE_SIZE) - size;
		} else {
			align_size = ALIGN(align_size + size, PAGE_SIZE) - size;
		}
#else
		align_size = ALIGN(free->real, align) - free->real;
#endif
		if (free->size >= (size + align_size)) {
			if (match != NULL) {
				if (free->size < match->size)
					match = free;
			} else {
				match = free;
			}
		}
	}

	if (match != NULL) {
		addr = match->real;

#ifndef CONFIG_S5P_VMEM
		if (align_size > 0) {
			/*
			 * When the allocated address must be align without VMEM,
			 * the free buffer can be overlap
			 * previous free buffer temporaily
			 */
			if (mfc_put_free_buf(match->real, align_size, port) < 0)
				return 0;
		}
#endif
		/* change allocated buffer address & size */
		match->real += (size + align_size);
		match->size -= (size + align_size);

		if (match->size == 0) {
			list_del(&match->list);
			kfree(match);
		}
	} else {
		mfc_err("no suitable free node in mfc buffer\n");

		return 0;
	}

	return addr;
}

int mfc_init_buf(void)
{
	int port;
	int ret = 0;

	for (port = 0; port < mfc_mem_count(); port++) {
		INIT_LIST_HEAD(&mfc_alloc_head[port]);
		INIT_LIST_HEAD(&mfc_free_head[port]);

		ret = mfc_put_free_buf(mfc_mem_data_base(port),
			mfc_mem_data_size(port), port);
	}

	/*
	spin_lock_init(&lock);
	*/

	mfc_print_buf();

	return ret;
}

void mfc_merge_buf(void)
{
	struct list_head *pos, *nxt;
	struct mfc_free_buffer *n1;
	struct mfc_free_buffer *n2;
	int port;

	for (port = 0; port < mfc_mem_count(); port++) {
		list_for_each_safe(pos, nxt, &mfc_free_head[port]) {
			n1 = list_entry(pos, struct mfc_free_buffer, list);
			n2 = list_entry(nxt, struct mfc_free_buffer, list);

			mfc_dbg("merge pre: n1: 0x%08lx, n2: 0x%08lx",
				n1->real, n2->real);

			if (!list_is_last(pos, &mfc_free_head[port])) {
				if ((n1->real + n1->size) == n2->real) {
					n2->real  = n1->real;
					n2->size += n1->size;
					list_del(&n1->list);
					kfree(n1);
				}
			}

			mfc_dbg("merge aft: n1: 0x%08lx, n2: 0x%08lx, last: %d",
				n1->real, n2->real,
				list_is_last(pos, &mfc_free_head[port]));
		}
	}

	mfc_print_buf();
}

/* FIXME: port auto select */
struct mfc_alloc_buffer *_mfc_alloc_buf(
	struct mfc_inst_ctx *ctx, int size, int align, int port)
{
	unsigned int addr;
	struct mfc_alloc_buffer *alloc;
#ifdef CONFIG_S5P_VMEM
	int align_size = 0;
#endif
	/*
	unsigned long flags;
	*/

	alloc = (struct mfc_alloc_buffer *)
		kzalloc(sizeof(struct mfc_alloc_buffer), GFP_KERNEL);

	if (unlikely(alloc == NULL))
		return NULL;

	/* FIXME: right position? */
	if (port > (mfc_mem_count() - 1))
		port = mfc_mem_count() - 1;

	/*
	spin_lock_irqsave(&lock, flags);
	*/

	addr = mfc_get_free_buf(size, align, port);

	mfc_dbg("mfc_get_free_buf: 0x%08x\n", addr);

	if (!addr) {
		mfc_dbg("cannot get suitable free buffer\n");

		/*
		spin_unlock_irqrestore(&lock, flags);
		*/

		return NULL;
	}

#ifdef CONFIG_S5P_VMEM
	if (align > PAGE_SIZE) {
		align_size = ALIGN(addr, align) - addr;
		align_size += ALIGN(align_size + size, PAGE_SIZE) - size;
	} else {
		align_size = ALIGN(align_size + size, PAGE_SIZE) - size;
	}

	alloc->vmem_cookie = s5p_vmem_vmemmap(size + align_size,
			addr, addr + (size + align_size));

	if (!alloc->vmem_cookie) {
		mfc_dbg("cannot map free buffer to memory\n");

		return NULL;
	}

	alloc->vmem_addr = addr;
	alloc->vmem_size = size + align_size;
#endif
	alloc->real = ALIGN(addr, align);
	alloc->size = size;
	alloc->addr = (unsigned char *)(mfc_mem_addr(port) +
		mfc_mem_base_ofs(alloc->real));
	alloc->user = (unsigned char *)(ctx->userbase +
		mfc_mem_data_ofs(alloc->real, 1));
	alloc->owner = ctx->id;

	list_add(&alloc->list, &mfc_alloc_head[port]);

	/*
	spin_unlock_irqrestore(&lock, flags);
	*/

	mfc_print_buf();

	return alloc;
}

int
mfc_alloc_buf(struct mfc_inst_ctx *ctx, struct mfc_buf_alloc_arg *args, int port)
{
	struct mfc_alloc_buffer *alloc;

	alloc = _mfc_alloc_buf(ctx, args->size, args->align, port);

	if (unlikely(alloc == NULL))
		return MFC_MEM_ALLOC_FAIL;

	args->user = (unsigned int)alloc->user;
	args->phys = (unsigned int)alloc->real;
	args->addr = (unsigned int)alloc->addr;
#ifdef CONFIG_S5P_VMEM
	args->cookie = (unsigned int)alloc->vmem_cookie;
#endif

	return MFC_OK;
}

int _mfc_free_buf(unsigned char *addr)
{
	struct list_head *pos, *nxt;
	struct mfc_alloc_buffer *alloc;
	int port;
	int found = 0;
	/*
	unsigned long flags;
	*/

	mfc_dbg("addr: 0x%08x\n", (unsigned int)addr);

	/*
	spin_lock_irqsave(&lock, flags);
	*/

	for (port = 0; port < mfc_mem_count(); port++) {
		list_for_each_safe(pos, nxt, &mfc_alloc_head[port]) {
			alloc = list_entry(pos, struct mfc_alloc_buffer, list);

			if (alloc->addr == addr) {
				found = 1;
#ifdef CONFIG_S5P_VMEM
				if (alloc->vmem_cookie)
					s5p_vfree(alloc->vmem_cookie);

				if (mfc_put_free_buf(alloc->vmem_addr,
					alloc->vmem_size, port) < 0) {

					mfc_err("failed to add free buffer\n");
				} else {
					list_del(&alloc->list);
					kfree(alloc);
				}
#else
				if (mfc_put_free_buf(alloc->real,
					alloc->size, port) < 0) {

					mfc_err("failed to add free buffer\n");
				} else {
					list_del(&alloc->list);
					kfree(alloc);
				}
#endif
				break;
			}
		}

		if (found)
			break;
	}

	/*
	spin_unlock_irqrestore(&lock, flags);
	*/

	mfc_print_buf();

	if (found)
		return 0;

	return -1;
}

int mfc_free_buf(struct mfc_inst_ctx *ctx, unsigned char *user)
{
	unsigned char *addr;

	mfc_dbg("user addr: 0x%08x\n", (unsigned int)user);

	addr = mfc_get_buf_addr(ctx->id, user);
	if (unlikely(addr == NULL))
		return MFC_MEM_INVALID_ADDR_FAIL;

	if (_mfc_free_buf(addr) < 0)
		return MFC_MEM_INVALID_ADDR_FAIL;

	return MFC_OK;
}

/* FIXME: add MFC Buffer Type */
void mfc_free_buf_inst(int owner)
{
	struct list_head *pos, *nxt;
	int port;
	struct mfc_alloc_buffer *alloc;
	/*
	unsigned long flags;
	*/

	mfc_dbg("owner: %d\n", owner);

	/*
	spin_lock_irqsave(&lock, flags);
	*/

	for (port = 0; port < mfc_mem_count(); port++) {
		list_for_each_safe(pos, nxt, &mfc_alloc_head[port]) {
			alloc = list_entry(pos, struct mfc_alloc_buffer, list);

			if (alloc->owner == owner) {
#ifdef CONFIG_S5P_VMEM
				if (alloc->vmem_cookie)
					s5p_vfree(alloc->vmem_cookie);

				if (mfc_put_free_buf(alloc->vmem_addr,
					alloc->vmem_size, port) < 0) {

					mfc_err("failed to add free buffer\n");
				} else {
					list_del(&alloc->list);
					kfree(alloc);
				}
#else
				if (mfc_put_free_buf(alloc->real,
					alloc->size, port) < 0) {

					mfc_err("failed to add free buffer\n");
				} else {
					list_del(&alloc->list);
					kfree(alloc);
				}
#endif
			}
		}
	}

	/*
	spin_unlock_irqrestore(&lock, flags);
	*/

	mfc_print_buf();
}

unsigned long mfc_get_buf_real(int owner, unsigned char *user)
{
	struct list_head *pos, *nxt;
	int port;
	struct mfc_alloc_buffer *alloc;

	mfc_dbg("owner: %d, user: 0x%08x\n", owner, (unsigned int)user);

	for (port = 0; port < mfc_mem_count(); port++) {
		list_for_each_safe(pos, nxt, &mfc_alloc_head[port]) {
			alloc = list_entry(pos, struct mfc_alloc_buffer, list);

			if ((alloc->owner == owner)
				&& (alloc->user == user)){

				return alloc->real;
			}
		}
	}

	return 0;
}

unsigned char *mfc_get_buf_addr(int owner, unsigned char *user)
{
	struct list_head *pos, *nxt;
	int port;
	struct mfc_alloc_buffer *alloc;

	mfc_dbg("owner: %d, user: 0x%08x\n", owner, (unsigned int)user);

	for (port = 0; port < mfc_mem_count(); port++) {
		list_for_each_safe(pos, nxt, &mfc_alloc_head[port]) {
			alloc = list_entry(pos, struct mfc_alloc_buffer, list);

			if ((alloc->owner == owner)
				&& (alloc->user == user)){

				return alloc->addr;
			}
		}
	}

	return NULL;
}

unsigned char *_mfc_get_buf_addr(int owner, unsigned char *user)
{
	struct list_head *pos, *nxt;
	int port;
	struct mfc_alloc_buffer *alloc;

	mfc_dbg("owner: %d, user: 0x%08x\n", owner, (unsigned int)user);

	for (port = 0; port < mfc_mem_count(); port++) {
		list_for_each_safe(pos, nxt, &mfc_alloc_head[port]) {
			alloc = list_entry(pos, struct mfc_alloc_buffer, list);

			if ((alloc->owner == owner)
				&& ((alloc->user <= user) || ((alloc->user + alloc->size) > user))){

				return alloc->addr;
			}
		}
	}

	return NULL;
}

