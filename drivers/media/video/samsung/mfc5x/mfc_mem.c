/*
 * linux/drivers/media/video/samsung/mfc5x/mfc_mem.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Memory manager for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/string.h>
#include <linux/vmalloc.h>

#ifdef CONFIG_ARCH_S5PV310
#include <mach/media.h>
#endif
#include <plat/media.h>

#ifndef CONFIG_S5P_VMEM
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>

#include <asm/cacheflush.h>
#include <asm/pgtable.h>
#endif

#include "mfc_mem.h"
#include "mfc_buf.h"
#include "mfc_log.h"

static int mem_ports = -1;
static struct mfc_mem mem_infos[MFC_MAX_MEM_PORT_NUM];

#if defined(SYSMMU_MFC_ON) && !defined(CONFIG_S5P_VMEM)
static unsigned long virt2phys(unsigned long addr)
{
	pgd_t *pgd;
	pmd_t *pmd;
	pte_t *pte;

	if(!current->mm)
		current->mm = &init_mm;

	pgd = pgd_offset(current->mm, addr);

	if ((pgd_val(*pgd) & 0x1) != 0x1)
		return 0;

	pmd = pmd_offset(pgd, addr);
	pte = pte_offset_map(pmd, addr);

	/*
	mfc_dbg("0x%08lx:0x%08lx\n",
		addr, (addr & 0xfff) | (pte_val(*pte) & 0xfffff000));
	*/

	return (addr & 0xfff) | (pte_val(*pte) & 0xfffff000);
}
#endif

static int mfc_mem_addr_port(unsigned int addr)
{
	int i;
	int port = -1;

	for (i = 0; i < mem_ports; i++) {
		if ((addr >= mem_infos[i].base)
		 && (addr < (mem_infos[i].base + mem_infos[i].size))) {
			port = i;
			break;
		}
	}

	return port;
}

int mfc_mem_count(void)
{
	return mem_ports;
}

unsigned int mfc_mem_base(int port)
{
	if ((port < 0) || (port >= mem_ports))
		return 0;

	return mem_infos[port].base;
}

unsigned char *mfc_mem_addr(int port)
{
	if ((port < 0) || (port >= mem_ports))
		return 0;

	return mem_infos[port].addr;
}

unsigned int mfc_mem_data_base(int port)
{
	unsigned int addr;

	if ((port < 0) || (port >= mem_ports))
		return 0;

	if (port == 0)
		addr = mem_infos[port].base + MFC_FW_SYSTEM_SIZE;
	else
		addr = mem_infos[port].base;

	return addr;
}

unsigned int mfc_mem_data_size(int port)
{
	unsigned int size;

	if ((port < 0) || (port >= mem_ports))
		return 0;

	if (port == 0)
		size = mem_infos[port].size - MFC_FW_SYSTEM_SIZE;
	else
		size = mem_infos[port].size;

	return size;
}

unsigned int mfc_mem_data_ofs(unsigned int addr, int contig)
{
	unsigned int offset;
	int i;
	int port;

	port = mfc_mem_addr_port(addr);

	if (port < 0)
		return 0;

	offset = addr - mfc_mem_data_base(port);

	if (contig) {
		for (i = 0; i < port; i++)
			offset += mfc_mem_data_size(i);
	}

	return offset;
}

unsigned int mfc_mem_base_ofs(unsigned int addr)
{
	int port;

	port = mfc_mem_addr_port(addr);

	if (port < 0)
		return 0;

	return addr - mem_infos[port].base;
}

#ifdef SYSMMU_MFC_ON
#ifdef CONFIG_S5P_VMEM
void mfc_mem_cache_clean(const void *start_addr, unsigned long size)
{
	s5p_vmem_dmac_map_area(start_addr, size, DMA_TO_DEVICE);
}

void mfc_mem_cache_inv(const void *start_addr, unsigned long size)
{
	s5p_vmem_dmac_map_area(start_addr, size, DMA_FROM_DEVICE);
}
#else
void mfc_mem_cache_clean(const void *start_addr, unsigned long size)
{
	unsigned long paddr;
	void *cur_addr, *end_addr;

	dmac_map_area(start_addr, size, DMA_TO_DEVICE);

	cur_addr = (void *)((unsigned long)start_addr & PAGE_MASK);
	size = PAGE_ALIGN(size);
	end_addr = cur_addr + size;

	while (cur_addr < end_addr) {
		paddr = virt2phys((unsigned long)cur_addr);
		if (paddr)
			outer_clean_range(paddr, paddr + PAGE_SIZE);
		cur_addr += PAGE_SIZE;
	}

	/* FIXME: L2 operation optimization */
	/*
	unsigned long start, end, unitsize;
	unsigned long cur_addr, remain;

	dmac_map_area(start_addr, size, DMA_TO_DEVICE);

	cur_addr = (unsigned long)start_addr;
	remain = size;

	start = virt2phys(cur_addr);
	if (start & PAGE_MASK) {
		unitsize = min((start | PAGE_MASK) - start + 1, remain);
		end = start + unitsize;
		outer_clean_range(start, end);
		remain -= unitsize;
		cur_addr += unitsize;
	}

	while (remain >= PAGE_SIZE) {
		start = virt2phys(cur_addr);
		end = start + PAGE_SIZE;
		outer_clean_range(start, end);
		remain -= PAGE_SIZE;
		cur_addr += PAGE_SIZE;
	}

	if (remain) {
		start = virt2phys(cur_addr);
		end = start + remain;
		outer_clean_range(start, end);
	}
	*/

}

void mfc_mem_cache_inv(const void *start_addr, unsigned long size)
{
	unsigned long paddr;
	void *cur_addr, *end_addr;

	cur_addr = (void *)((unsigned long)start_addr & PAGE_MASK);
	size = PAGE_ALIGN(size);
	end_addr = cur_addr + size;

	while (cur_addr < end_addr) {
		paddr = virt2phys((unsigned long)cur_addr);
		if (paddr)
			outer_inv_range(paddr, paddr + PAGE_SIZE);
		cur_addr += PAGE_SIZE;
	}

	dmac_unmap_area(start_addr, size, DMA_FROM_DEVICE);

	/* FIXME: L2 operation optimization */
	/*
	unsigned long start, end, unitsize;
	unsigned long cur_addr, remain;

	cur_addr = (unsigned long)start_addr;
	remain = size;

	start = virt2phys(cur_addr);
	if (start & PAGE_MASK) {
		unitsize = min((start | PAGE_MASK) - start + 1, remain);
		end = start + unitsize;
		outer_inv_range(start, end);
		remain -= unitsize;
		cur_addr += unitsize;
	}

	while (remain >= PAGE_SIZE) {
		start = virt2phys(cur_addr);
		end = start + PAGE_SIZE;
		outer_inv_range(start, end);
		remain -= PAGE_SIZE;
		cur_addr += PAGE_SIZE;
	}

	if (remain) {
		start = virt2phys(cur_addr);
		end = start + remain;
		outer_inv_range(start, end);
	}

	dmac_unmap_area(start_addr, size, DMA_FROM_DEVICE);
	*/
}
#endif /* CONFIG_S5P_VMEM */
#else
void mfc_mem_cache_clean(const void *start_addr, unsigned long size)
{
	unsigned long paddr;

	dmac_map_area(start_addr, size, DMA_TO_DEVICE);
	/*
	 * virtual & phsical addrees mapped directly, so we can convert
	 * the address just using offset
	 */
	paddr = __pa((unsigned long)start_addr);
	outer_clean_range(paddr, paddr + size);

	/* OPT#1: kernel provide below function */
	/*
	dma_map_single(NULL, (void *)start_addr, size, DMA_TO_DEVICE);
	*/
}

void mfc_mem_cache_inv(const void *start_addr, unsigned long size)
{
	unsigned long paddr;

	paddr = __pa((unsigned long)start_addr);
	outer_inv_range(paddr, paddr + size);
	dmac_unmap_area(start_addr, size, DMA_FROM_DEVICE);

	/* OPT#1: kernel provide below function */
	/*
	dma_unmap_single(NULL, (void *)start_addr, size, DMA_FROM_DEVICE);
	*/
}
#endif /* SYSMMU_MFC_ON */

int mfc_init_mem_mgr(struct mfc_dev *dev)
{
	dma_addr_t base;
	unsigned int align_margin;
	int i;

#ifndef SYSMMU_MFC_ON
	size_t size;
#endif

	dev->mem_ports = MFC_MAX_MEM_PORT_NUM;
	memset(dev->mem_infos, 0, sizeof(dev->mem_infos));

#ifdef SYSMMU_MFC_ON
#ifdef CONFIG_S5P_VMEM
	base = MFC_FREEBASE;
	dev->mem_infos[0].base = ALIGN(base, ALIGN_128KB);
	align_margin = dev->mem_infos[0].base - base;
	dev->mem_infos[0].size = MFC_MEMSIZE_PORT_A - align_margin;
	dev->mem_infos[0].addr = (unsigned char *)dev->mem_infos[0].base;

	if (dev->mem_ports == 2) {
		base = dev->mem_infos[0].base + dev->mem_infos[0].size;
		dev->mem_infos[1].base = ALIGN(base, ALIGN_128KB);
		align_margin = dev->mem_infos[1].base - base;
		dev->mem_infos[1].size = MFC_MEMSIZE_PORT_B - align_margin;
		dev->mem_infos[1].addr = (unsigned char *)dev->mem_infos[1].base;
	}

	dev->fw.vmem_cookie = s5p_vmem_vmemmap(MFC_FW_SYSTEM_SIZE,
		dev->mem_infos[0].base,
		dev->mem_infos[0].base + MFC_FW_SYSTEM_SIZE);

	if (!dev->fw.vmem_cookie)
		return -ENOMEM;
#else
	dev->mem_infos[0].vmalloc_addr = vmalloc(MFC_MEMSIZE_PORT_A);
	if (dev->mem_infos[0].vmalloc_addr == NULL)
		return -ENOMEM;

	base = (unsigned long)dev->mem_infos[0].vmalloc_addr;
	dev->mem_infos[0].base = ALIGN(base, ALIGN_128KB);
	align_margin = dev->mem_infos[0].base - base;
	dev->mem_infos[0].size = MFC_MEMSIZE_PORT_A - align_margin;
	dev->mem_infos[0].addr = (unsigned char *)dev->mem_infos[0].base;

	if (dev->mem_ports == 2) {
		dev->mem_infos[1].vmalloc_addr = vmalloc(MFC_MEMSIZE_PORT_B);
		if (dev->mem_infos[1].vmalloc_addr == NULL) {
			vfree(dev->mem_infos[0].vmalloc_addr);
			return -ENOMEM;
		}

		base = (unsigned long)dev->mem_infos[1].vmalloc_addr;
		dev->mem_infos[1].base = ALIGN(base, ALIGN_128KB);
		align_margin = dev->mem_infos[1].base - base;
		dev->mem_infos[1].size = MFC_MEMSIZE_PORT_B - align_margin;
		dev->mem_infos[1].addr = (unsigned char *)dev->mem_infos[1].base;
	}
#endif	/* CONFIG_S5P_VMEM */
#else
	for (i = 0; i < dev->mem_ports; i++) {
#ifdef CONFIG_ARCH_S5PV310
		base = s5p_get_media_memory_bank(S5P_MDEV_MFC, i);
#else
		base = s3c_get_media_memory_bank(S3C_MDEV_MFC, i);
#endif
		if (base == 0) {
			mfc_err("failed to get reserved memory on port #%d", i);
			return -EPERM;
		}

#ifdef CONFIG_ARCH_S5PV310
		size = s5p_get_media_memsize_bank(S5P_MDEV_MFC, i);
#else
		size = s3c_get_media_memsize_bank(S3C_MDEV_MFC, i);
#endif
		if (size == 0) {
			mfc_err("failed to get reserved size on port #%d", i);
			return -EPERM;
		}

		dev->mem_infos[i].base = ALIGN(base, ALIGN_128KB);
		align_margin = dev->mem_infos[i].base - base;
		dev->mem_infos[i].size = size - align_margin;
		/* kernel direct mapped memory address */
		dev->mem_infos[i].addr = phys_to_virt(dev->mem_infos[i].base);
	}
#endif /* SYSMMU_MFC_ON */

	mem_ports = dev->mem_ports;
	for (i = 0; i < mem_ports; i++)
		memcpy(&mem_infos[i], &dev->mem_infos[i], sizeof(struct mfc_mem));

	return 0;
}

void mfc_final_mem_mgr(struct mfc_dev *dev)
{
#ifdef SYSMMU_MFC_ON
#ifdef CONFIG_S5P_VMEM
	s5p_vfree(dev->fw.vmem_cookie);
#else
	vfree(dev->mem_infos[0].vmalloc_addr);
	if (dev->mem_ports == 2)
		vfree(dev->mem_infos[1].vmalloc_addr);
#endif	/* CONFIG_S5P_VMEM */
#else
	/* no action */
#endif /* SYSMMU_MFC_ON */
}

