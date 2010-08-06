/* drivers/char/s5p_vmem.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *               http://www.samsung.com
 *
 * S5P_VMEM driver for /dev/S5P-VMEM
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

struct kvm_area {
	unsigned int cookie;	/* unique number. actually, pfn */
/* TODO: cookie can be 0 because it's pfn. change type of cookie to signed */
	void *start_addr;	/* the first virtual address */
	size_t size;
	int count;		/* reference count */
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
	s5p_reset
};

static char funcmap[NR_ALLOCTYPE + 2] = {
	MEM_ALLOC,
	MEM_FREE,
	-1,			/* NEVER USE THIS */
	-1,			/* NEVER USE THIS */
	MEM_ALLOC_SHARE,
	MEM_FREE_SHARE,
	MEM_ALLOC_CACHEABLE,
	MEM_ALLOC_CACHEABLE_SHARE,
	MEM_RESET
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
static struct kvm_area root_kvm;

/* points to the recently accessed entry of kvm_area */
static struct kvm_area *recent_kvm_area;
static unsigned int cookie;

#define IOCTLNR2FMAPIDX(nr)	_IOC_NR(nr)
#define ISALLOCTYPE(nr)		((nr >= 0) && (nr < MEM_FREE))
#define ISFREETYPE(nr)		((nr >= MEM_FREE) && (nr < NR_ALLOCTYPE))
#define ISSHARETYPE(nr)		(nr & 1)
#define ROOTKVM			(&root_kvm)
#define FIRSTKVM		(root_kvm.next)

static struct kvm_area *createkvm(void *kvm_addr, size_t size)
{
	struct kvm_area *newarea, *cur;

	newarea = kmalloc(sizeof(struct kvm_area), GFP_ATOMIC);
	if (newarea == NULL)
		return NULL;

	mutex_lock(&s5p_vmem_lock);

	newarea->start_addr = kvm_addr;
	newarea->size = size;
	newarea->count = 1;
	newarea->next = NULL;
	newarea->cookie = vmalloc_to_pfn(kvm_addr);

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
	vfree(rmarea->start_addr);
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
	if ((alloccmd < 0) || (alloccmd > 7))
		return -EINVAL;

	alloccmd = funcmap[alloccmd];
	if (alloccmd < 0)
		return -EINVAL;

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

inline struct kvm_area *createallockvm(size_t size)
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

	/* TOOD: if remap_vmalloc_range failed detachkvm must be invoked
	 * to decrease reference count of kvm after detachkvm is implemented */
	return remap_vmalloc_range(vma, recent_kvm_area->start_addr, 0);
}
EXPORT_SYMBOL(s5p_vmem_mmap);

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
