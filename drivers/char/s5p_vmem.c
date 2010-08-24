/* drivers/char/s5p_vmem.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *               http://www.samsung.com
 *
 * S5P_VMEM driver for /dev/s5p-vmem
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/uaccess.h>	/* copy_from_user, copy_to_user */
#include <linux/vmalloc.h>
#include <linux/mman.h>
#include <linux/sched.h>	/* 'current' global variable */
#include <linux/slab.h>
#include <linux/vmalloc.h>	/* unmap_kernel_range, remap_vmalloc_range, */
			/* map_vm_area, vmalloc_user, struct vm_struct */
#include <linux/fs.h>

#include "s5p_vmem.h"

enum ALLOCTYPE {
	MEM_INVTYPE = -1,
	MEM_ALLOC = 0,
	MEM_ALLOC_SHARE,
	MEM_ALLOC_CACHEABLE,
	MEM_ALLOC_CACHEABLE_SHARE,
	MEM_FREE,
	MEM_FREE_SHARE,
	MEM_RESET,
	NR_ALLOCTYPE
};

char *s5p_alloctype_names[NR_ALLOCTYPE] = {
	"MEM_ALLOC",
	"MEM_ALLOC_SHARE",
	"MEM_ALLOC_CACHEABLE",
	"MEM_ALLOC_CACHEABLE_SHARE",
	"MEM_FREE",
	"MEM_FREE_SHARE",
	"MEM_RESET",
};

struct kvm_area {
	unsigned int cookie;	/* unique number. actually, pfn */
/* TODO: cookie can be 0 because it's pfn. change type of cookie to signed */
	void *start_addr;	/* the first virtual address */
	size_t size;
	int count;		/* reference count */
	struct page **pages;	/* page descriptor table if required. */
	struct kvm_area *next;
};

static int s5p_free(struct file *, struct s5p_vmem_alloc *);
static int s5p_alloc(struct file *, struct s5p_vmem_alloc *);
static int s5p_reset(struct file *, struct s5p_vmem_alloc *);

static int (*mmanfns[NR_ALLOCTYPE]) (struct file *, struct s5p_vmem_alloc *) = {
	s5p_alloc,
	s5p_alloc,
	s5p_alloc,
	s5p_alloc,
	s5p_free,
	s5p_free,
	s5p_reset,
};

static char funcmap[NR_ALLOCTYPE + 2] = {
	MEM_ALLOC,
	MEM_FREE,
	MEM_INVTYPE,			/* NEVER USE THIS */
	MEM_INVTYPE,			/* NEVER USE THIS */
	MEM_ALLOC_SHARE,
	MEM_FREE_SHARE,
	MEM_ALLOC_CACHEABLE,
	MEM_ALLOC_CACHEABLE_SHARE,
	MEM_RESET,
};

/* we actually need only one mutex because ioctl must be executed atomically
 * to avoid the following problems:
 * - removing allocated physical memory while sharing the area.
 * - modifying global variables by different calls to ioctl by other processes.
 */
static DEFINE_MUTEX(s5p_vmem_lock);
static DEFINE_MUTEX(s5p_vmem_userlock);

/* initialized by s5p_vmem_ioctl, used by s5p_vmem_mmap */
static int alloctype = MEM_INVTYPE;	/* enum ALLOCTYPE */
/* the beginning of the list is dummy
 * do not access this variable directly; use ROOTKVM and FIRSTKVM */
struct kvm_area root_kvm;

/* points to the recently accessed entry of kvm_area */
static struct kvm_area *recent_kvm_area;
static unsigned int cookie;

#define IOCTLNR2FMAPIDX(nr)	_IOC_NR(nr)
#define ISALLOCTYPE(nr)		((nr >= 0) && (nr < MEM_FREE))
#define ISFREETYPE(nr)		((nr >= MEM_FREE) && (nr < NR_ALLOCTYPE))
#define ISSHARETYPE(nr)		(nr & 1)
#define ROOTKVM			(&root_kvm)
#define FIRSTKVM		(root_kvm.next)
#define PAGEDESCSIZE(size)	((size >> PAGE_SHIFT) * sizeof(struct page *))

static struct kvm_area *createkvm(void *kvm_addr, size_t size)
{
	struct kvm_area *newarea, *cur;

	newarea = kmalloc(sizeof(struct kvm_area), GFP_KERNEL);
	if (newarea == NULL)
		return NULL;

	mutex_lock(&s5p_vmem_lock);

	newarea->start_addr = kvm_addr;
	newarea->size = size;
	newarea->count = 1;
	newarea->next = NULL;
	newarea->pages = NULL;
	newarea->cookie = virt_to_phys(kvm_addr) ^ 0xA5CF;/* simple encryption*/

	cur = ROOTKVM;
	while (cur->next != NULL)
		cur = cur->next;
	cur->next = newarea;

	mutex_unlock(&s5p_vmem_lock);

	return newarea;
}

static inline struct kvm_area *findkvm(unsigned int cookie)
{
	struct kvm_area *kvmarea;
	kvmarea = FIRSTKVM;
	while ((kvmarea != NULL) && (kvmarea->cookie != cookie))
		kvmarea = kvmarea->next;
	return kvmarea;
}

static inline unsigned int findcookie(void *addr)
{
	struct kvm_area *kvmarea;
	kvmarea = FIRSTKVM;
	while ((kvmarea != NULL) && (kvmarea->start_addr != addr))
		kvmarea = kvmarea->next;
	return (kvmarea != NULL) ? kvmarea->cookie : 0;
}

static struct kvm_area *attachkvm(unsigned int cookie)
{
	struct kvm_area *kvmarea;

	kvmarea = findkvm(cookie);

	if (kvmarea)
		kvmarea->count++;

	return kvmarea;
}

static int freekvm(unsigned int cookie)
{
	struct kvm_area *kvmarea, *rmarea;

	kvmarea = ROOTKVM;
	while ((kvmarea->next != NULL) && (kvmarea->next->cookie != cookie))
		kvmarea = kvmarea->next;

	if (kvmarea->next == NULL)
		return -1;

	mutex_lock(&s5p_vmem_lock);

	rmarea = kvmarea->next;
	kvmarea->next = rmarea->next;
	if (rmarea->pages) {
		int i;

		/* defined in /mm/vmalloc.c */
		unmap_kernel_range((unsigned long)rmarea->start_addr,
				rmarea->size);

		for (i = 0; i < (rmarea->size >> PAGE_SHIFT); i++)
			__free_page(rmarea->pages[i]);

		if (PAGEDESCSIZE(rmarea->size) > PAGE_SIZE)
			vfree(rmarea->pages);
		else
			kfree(rmarea->pages);
	} else {
		vfree(rmarea->start_addr);
	}
	kfree(rmarea);

	mutex_unlock(&s5p_vmem_lock);
	return 0;
}

int s5p_vmem_ioctl(struct inode *inode, struct file *file,
		   unsigned int cmd, unsigned long arg)
{
	/* DO NOT REFER TO GLOBAL VARIABLES IN THIS FUNCTION */

	struct s5p_vmem_alloc param;
	int result;
	int alloccmd;

	alloccmd = IOCTLNR2FMAPIDX(cmd);
	if ((alloccmd < 0) || (alloccmd > 9)) {
		printk(KERN_DEBUG
			"S5P-VMEM: Wrong allocation command number %d\n",
			alloccmd);
		return -EINVAL;
	}

	alloccmd = funcmap[alloccmd];
	if (alloccmd < 0) {
		printk(KERN_DEBUG
		    "S5P-VMEM: Wrong translated allocation command number %d\n",
			alloccmd);
		return -EINVAL;
	}

	if (alloccmd < MEM_RESET) {
		result = copy_from_user(&param, (struct s5p_vmem_alloc *)arg,
					sizeof(struct s5p_vmem_alloc));

		if (result)
			return -EFAULT;
	}

	mutex_lock(&s5p_vmem_userlock);
	alloctype = alloccmd;
	result = mmanfns[alloctype] (file, &param);
	alloctype = MEM_INVTYPE;
	mutex_unlock(&s5p_vmem_userlock);

	if (result)
		return -EFAULT;

	if (alloccmd < MEM_FREE) {
		result = copy_to_user((struct s5p_vmem_alloc *)arg, &param,
				      sizeof(struct s5p_vmem_alloc));

		if (result != 0) {
			mutex_lock(&s5p_vmem_userlock);
			/* error handling:
			 * allowed to access 'alloctype' global var. */
			alloctype = MEM_FREE | ISSHARETYPE(alloccmd);
			s5p_free(file, &param);
			alloctype = MEM_INVTYPE;
			mutex_unlock(&s5p_vmem_userlock);
		}
	}

	if (alloccmd < MEM_RESET)
		if (result != 0)
			return -EFAULT;

	return 0;
}
EXPORT_SYMBOL(s5p_vmem_ioctl);

static struct kvm_area *createallockvm(size_t size)
{
	void *virt_addr;
	virt_addr = vmalloc_user(size);
	if (!virt_addr)
		return NULL;

	recent_kvm_area = createkvm(virt_addr, size);
	if (recent_kvm_area == NULL)
		vfree(virt_addr);

	return recent_kvm_area;
}

unsigned int s5p_vmalloc(size_t size)
{
	struct kvm_area *kvmarea;
	kvmarea = createallockvm(size);
	return (kvmarea == NULL) ? 0 : kvmarea->cookie;
}
EXPORT_SYMBOL(s5p_vmalloc);

void s5p_vfree(unsigned int cookie)
{
	freekvm(cookie);
}
EXPORT_SYMBOL(s5p_vfree);

void *s5p_getaddress(unsigned int cookie)
{
	struct kvm_area *kvmarea;
	kvmarea = findkvm(cookie);
	return (kvmarea == NULL) ? NULL : kvmarea->start_addr;
}
EXPORT_SYMBOL(s5p_getaddress);

int s5p_vmem_mmap(struct file *filp, struct vm_area_struct *vma)
{
	if ((alloctype == MEM_ALLOC) || (alloctype == MEM_ALLOC_CACHEABLE))
		recent_kvm_area = createallockvm(vma->vm_end - vma->vm_start);
	else	/* alloctype == MEM_ALLOC_SHARE or MEM_ALLOC_CACHEABLE_SHARE */
		recent_kvm_area = attachkvm(cookie);

	if (recent_kvm_area == NULL)
		return -EINVAL;

	if ((alloctype == MEM_ALLOC) || (alloctype == MEM_ALLOC_SHARE))
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	vma->vm_flags |= VM_RESERVED;

	if (recent_kvm_area->pages) {
		/* We can't use remap_vmalloc_range if page frames are not
		 * allocated by vmalloc. The following code is very simillar to
		 * remap_vmalloc_range. */
		int rpfn = 0;
		unsigned long uaddr = vma->vm_start;
		unsigned long usize = vma->vm_end - vma->vm_start;

		/* below condition creates invalid mapping
		 * remap_vmalloc_range checks it internally */
		if (recent_kvm_area->size < usize)
			return -EINVAL;

		while (rpfn < (usize >> PAGE_SHIFT)) {
			if (vm_insert_page(vma, uaddr,
				recent_kvm_area->pages[rpfn]) != 0)
				return -EFAULT;

			uaddr += PAGE_SIZE;
			rpfn++;
		}
		return 0;
	} else
		return remap_vmalloc_range(vma, recent_kvm_area->start_addr, 0);
}
EXPORT_SYMBOL(s5p_vmem_mmap);

/* return 0 if successful */
static int s5p_alloc(struct file *file, struct s5p_vmem_alloc *param)
{
	cookie = param->cookie;
	/* TODO: enhance the following code to get the size of share */
	if (ISSHARETYPE(alloctype)) {
		struct kvm_area *kvmarea;
		kvmarea = findkvm(cookie);
		param->size = (kvmarea == NULL) ? 0 : kvmarea->size;
	} else if (param->size == 0) {
		return -1;
	}

	if (param->size == 0)
		return -1;

	/* unit of allocation is a page */
	param->size = PAGE_ALIGN(param->size);
	param->vir_addr = do_mmap(file, 0, param->size,
				  (PROT_READ | PROT_WRITE), MAP_SHARED, 0);
	if (param->vir_addr == -EINVAL) {
		param->vir_addr = 0;
		recent_kvm_area = NULL;
		return -1;
	}

	if ((alloctype == MEM_ALLOC) || (alloctype == MEM_ALLOC_CACHEABLE))
		param->cookie = recent_kvm_area->cookie;

	param->size = recent_kvm_area->size;

	return 0;
}

static int s5p_free(struct file *file, struct s5p_vmem_alloc *param)
{
	struct kvm_area *kvmarea = NULL;

	if (param->vir_addr == 0)
		return -1;

	kvmarea = findkvm(param->cookie);

	if (kvmarea == NULL)
		return -1;

	if (do_munmap(current->mm, param->vir_addr, param->size) < 0)
		return -1;

	if ((alloctype == MEM_FREE) && (freekvm(param->cookie) != 0))
		return -1;
	else
		kvmarea->count -= 1;

	recent_kvm_area = NULL;
	return 0;
}

/* no parameter required. pass all NULL */
static int s5p_reset(struct file *file, struct s5p_vmem_alloc *param)
{
	struct kvm_area *kvmarea = NULL;

	kvmarea = FIRSTKVM;

	mutex_lock(&s5p_vmem_lock);
	while (kvmarea) {
		struct kvm_area *temp;

		vfree(kvmarea->start_addr);

		temp = kvmarea;
		kvmarea = kvmarea->next;
		kfree(temp);
	}
	mutex_unlock(&s5p_vmem_lock);

	return 0;
}

int s5p_vmem_open(struct inode *pinode, struct file *pfile)
{
	return 0;
}
EXPORT_SYMBOL(s5p_vmem_open);

int s5p_vmem_release(struct inode *pinode, struct file *pfile)
{
	/* TODO: remove all instances of memory allocation
	 * when an opened file is closing */
	return 0;
}
EXPORT_SYMBOL(s5p_vmem_release);

/* s5p_vmem_vmemmap
 * Maps a non-linear physical page frames into a contiguous virtual memory area
 * in the kernel's address space
 * @size: size in bytes to map into the virtual address space
 * @va_start: the beginning address of the virtual memory area
 * @va_end: the past to the last address of the virtual memory area
 *
 * If @end - @start is smaller than @size, allocation and mapping will fail.
 * This returns 'cookie' of the allocated area so that users can share it.
 * Returning '0'(zero) means mapping is failed because of memory allocation
 * failure, mapping failure and so on.
 */
unsigned int s5p_vmem_vmemmap(size_t size, unsigned long va_start,
				unsigned long va_end)
{
	struct page **pages;
	struct kvm_area *kvma;	/* new virtual address area */
	unsigned int nr_pages, array_size, i;
	struct vm_struct area; /* argument for map_vm_area*/

	/* DMA and normal memory area must not be remapped */
	if ((va_start > va_end) || (va_start < VMALLOC_START))
		return 0;

	/* Desired size must not be larger than the size of supplied virtual
	 * address space */
	size = PAGE_ALIGN(size);
	if (size > (va_end - va_start))
		return 0;

	va_end = va_start + size;

	nr_pages = size >> PAGE_SHIFT;
	array_size = nr_pages * sizeof(struct page *);

	kvma = createkvm((void *)va_start, size);
	if (unlikely(!kvma))
		return 0;

	/* preparing memory buffer for page descriptors */
	/* below condition must be used when freeing this memory */
	if (array_size > PAGE_SIZE)
		pages = vmalloc(array_size);
	else
		pages = kmalloc(array_size, GFP_KERNEL);
	/* below error handling causes invoking vfree(va_start).
	 * It is ok because vfree just ignore if va_start is not found in
	 * its rb_tree */
	if (unlikely(!pages))
		goto fail;

	memset(pages, 0, array_size);
	kvma->pages = pages;

	/* page frame allocation */
	/* even though alloc_page fails in the middle of allocation and pages
	 * array is not filled enough, deallocation is always successful in
	 * freekvm() because pages array is initialized with 0.
	 */
	for (i = 0; i < nr_pages; i++) {
		struct page *page;

		page = alloc_page(GFP_KERNEL);

		if (unlikely(!page))
			goto fail;

		pages[i] = page;
	}

	/* map_vm_area just manipulates 'addr' and 'size' of vm_struct */
	/* but if map_vm_area is modified to touch other members of vm_struct,
	 * initialization of 'area' must be also changed!*/
	area.addr = (void *)va_start;
	/* map_vm_area regards all area contains a guard page */
	area.size = size + PAGE_SIZE;

	/* page table generation */
	if (map_vm_area(&area, PAGE_KERNEL, &pages) == 0)
		return kvma->cookie;
fail:
	/* Free all pages in 'pages' array and kvma */
	freekvm(kvma->cookie);
	return 0;
}
EXPORT_SYMBOL(s5p_vmem_vmemmap);

