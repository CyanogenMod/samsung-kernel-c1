/* linux/drivers/media/video/samsung/mfc50/s3c_mfc_buffer_manager.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Handling for reserved memory
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/types.h>

#include <asm/io.h>
#include <asm/uaccess.h>

#include <plat/media.h>

#include "s3c_mfc_buffer_manager.h"
#include "s3c_mfc_errorno.h"
#include "s3c_mfc_logmsg.h"
#include "s3c_mfc_memory.h"

static struct s3c_mfc_alloc_mem_t *s3c_mfc_alloc_mem_head[MFC_MAX_PORT_NUM];
static struct s3c_mfc_alloc_mem_t *s3c_mfc_alloc_mem_tail[MFC_MAX_PORT_NUM];
static struct s3c_mfc_free_mem_t *s3c_mfc_free_mem_head[MFC_MAX_PORT_NUM];
static struct s3c_mfc_free_mem_t *s3c_mfc_free_mem_tail[MFC_MAX_PORT_NUM];

/* insert node ahead of s3c_mfc_alloc_mem_head */
static void s3c_mfc_insert_node_to_alloc_list(struct s3c_mfc_alloc_mem_t *node,
					      int inst_no, int port_no)
{
	mfc_debug("[%d]instance, [%d]port (p_addr : 0x%08x size:%d)\n",
		  inst_no, port_no, node->p_addr, node->size);
	node->next = s3c_mfc_alloc_mem_head[port_no];
	node->prev = s3c_mfc_alloc_mem_head[port_no]->prev;
	s3c_mfc_alloc_mem_head[port_no]->prev->next = node;
	s3c_mfc_alloc_mem_head[port_no]->prev = node;
	s3c_mfc_alloc_mem_head[port_no] = node;
}

void s3c_mfc_print_list(void)
{
	struct s3c_mfc_alloc_mem_t *node1;
	struct s3c_mfc_free_mem_t *node2;
	int count = 0;
	unsigned int p_addr;
	int port_no;

	for (port_no = 0; port_no < MFC_MAX_PORT_NUM; port_no++) {

		for (node1 = s3c_mfc_alloc_mem_head[port_no];
		     node1 != s3c_mfc_alloc_mem_tail[port_no];
		     node1 = node1->next) {
			p_addr = (unsigned int)node1->p_addr;

			printk
			    (KERN_INFO
			    "s3c_mfc_print_list [AllocList][%d] inst_no : %d \
			    port_no : %d  p_addr : 0x%08x v_addr: 0x%08x \
			    size:%d\n",
			    count++, node1->inst_no, node1->port_no, p_addr,
			     (unsigned int)node1->v_addr, node1->size);

		}

		count = 0;
		for (node2 = s3c_mfc_free_mem_head[port_no];
		     node2 != s3c_mfc_free_mem_tail[port_no];
		     node2 = node2->next) {
			printk
			    (KERN_INFO
			    "s3c_mfc_print_list [FreeList][%d] \
			    startAddr : 0x%08x size:%d\n",
			     count++, node2->start_addr, node2->size);
		}

	}
}

static void s3c_mfc_insert_first_node_to_free_list(struct s3c_mfc_free_mem_t *node,
						   int inst_no, int port_no)
{
	mfc_debug("[%d]instance(startAddr : 0x%08x size:%d)\n",
		  inst_no, node->start_addr, node->size);

	node->next = s3c_mfc_free_mem_head[port_no];
	node->prev = s3c_mfc_free_mem_head[port_no]->prev;
	s3c_mfc_free_mem_head[port_no]->prev->next = node;
	s3c_mfc_free_mem_head[port_no]->prev = node;
	s3c_mfc_free_mem_head[port_no] = node;
}

/* insert node ahead of s3c_mfc_free_mem_head */
static void s3c_mfc_insert_node_to_free_list(struct s3c_mfc_free_mem_t *node,
					     int inst_no, int port_no)
{
	struct s3c_mfc_free_mem_t *itr_node;

	mfc_debug("[%d]instance, [%d]port (startAddr : 0x%08x size:%d)\n",
		  inst_no, port_no, node->start_addr, node->size);

	itr_node = s3c_mfc_free_mem_head[port_no];

	while (itr_node != s3c_mfc_free_mem_tail[port_no]) {

		if (itr_node->start_addr >= node->start_addr) {
			/* head */
			if (itr_node == s3c_mfc_free_mem_head[port_no]) {
				node->next = s3c_mfc_free_mem_head[port_no];
				node->prev =
				    s3c_mfc_free_mem_head[port_no]->prev;
				s3c_mfc_free_mem_head[port_no]->prev->next =
				    node;
				s3c_mfc_free_mem_head[port_no]->prev = node;
				s3c_mfc_free_mem_head[port_no] = node;
				break;
			} else {	/* mid */
				node->next = itr_node;
				node->prev = itr_node->prev;
				itr_node->prev->next = node;
				itr_node->prev = node;
				break;
			}
		}

		itr_node = itr_node->next;
	}

	/* tail */
	if (itr_node == s3c_mfc_free_mem_tail[port_no]) {
		node->next = s3c_mfc_free_mem_tail[port_no];
		node->prev = s3c_mfc_free_mem_tail[port_no]->prev;
		s3c_mfc_free_mem_tail[port_no]->prev->next = node;
		s3c_mfc_free_mem_tail[port_no]->prev = node;
	}
}

static void s3c_mfc_del_node_from_alloc_list(struct s3c_mfc_alloc_mem_t *node,
					     int inst_no, int port_no)
{
	mfc_debug("[%d]instance, [%d]port (p_addr : 0x%08x size:%d)\n", \
		  inst_no, port_no, node->p_addr, node->size);

	if (node == s3c_mfc_alloc_mem_tail[port_no]) {
		mfc_info("InValid node\n");
		return;
	}

	if (node == s3c_mfc_alloc_mem_head[port_no])
		s3c_mfc_alloc_mem_head[port_no] = node->next;

	node->prev->next = node->next;
	node->next->prev = node->prev;

	kfree(node);
}

static void s3c_mfc_del_node_from_free_list(struct s3c_mfc_free_mem_t *node,
					    int inst_no, int port_no)
{
	mfc_debug
	    ("[%d]s3c_mfc_del_node_from_free_list \
	    (startAddr : 0x%08x size:%d)\n",
	     inst_no, node->start_addr, node->size);
	if (node == s3c_mfc_free_mem_tail[port_no]) {
		mfc_err("InValid node\n");
		return;
	}

	if (node == s3c_mfc_free_mem_head[port_no])
		s3c_mfc_free_mem_head[port_no] = node->next;

	node->prev->next = node->next;
	node->next->prev = node->prev;

	kfree(node);
}

/* Remove Fragmentation in FreeMemList */
void s3c_mfc_merge_frag(int inst_no)
{
	struct s3c_mfc_free_mem_t *node1, *node2;
	int port_no;

	for (port_no = 0; port_no < MFC_MAX_PORT_NUM; port_no++) {

		node1 = s3c_mfc_free_mem_head[port_no];

		while (node1 != s3c_mfc_free_mem_tail[port_no]) {
			node2 = s3c_mfc_free_mem_head[port_no];
			while (node2 != s3c_mfc_free_mem_tail[port_no]) {
				if (node1->start_addr + node1->size ==
				    node2->start_addr) {
					node1->size += node2->size;
					mfc_debug
					    ("find merge area !! \
					    (node1->start_addr + node1->size \
					    == node2->start_addr)\n");
					s3c_mfc_del_node_from_free_list(
					node2, inst_no, port_no);
					break;
				} else if (node1->start_addr ==
					   node2->start_addr + node2->size) {
					mfc_debug
					    ("find merge area !! \
					    (node1->start_addr == \
					    node2->start_addr + \
					    node2->size)\n");
					node1->start_addr = node2->start_addr;
					node1->size += node2->size;
					s3c_mfc_del_node_from_free_list(
					node2, inst_no, port_no);
					break;
				}
				node2 = node2->next;
			}
			node1 = node1->next;
		}
	}
}

static unsigned int s3c_mfc_get_mem_area(int allocSize, int inst_no,
					 int port_no)
{
	struct s3c_mfc_free_mem_t *node, *match_node = NULL;
	unsigned int allocAddr = 0;

	mfc_debug("request Size : %ld\n", (long int)allocSize);

	if (s3c_mfc_free_mem_head[port_no] == s3c_mfc_free_mem_tail[port_no]) {
		mfc_err("all memory is gone\n");
		return allocAddr;
	}

	/* find best chunk of memory */
	for (node = s3c_mfc_free_mem_head[port_no];
	     node != s3c_mfc_free_mem_tail[port_no]; node = node->next) {
		if (match_node != NULL) {
			if ((node->size >= allocSize)
			    && (node->size < match_node->size))
				match_node = node;
		} else {
			if (node->size >= allocSize)
				match_node = node;
		}
	}

	if (match_node != NULL) {
		mfc_debug("match : startAddr(0x%08x) size(%ld)\n",
			  match_node->start_addr, (long int)match_node->size);
	}

	/* rearange FreeMemArea */
	if (match_node != NULL) {
		allocAddr = match_node->start_addr;
		match_node->start_addr += allocSize;
		match_node->size -= allocSize;

		if (match_node->size == 0)	/* delete match_node. */
			s3c_mfc_del_node_from_free_list(match_node, inst_no,
							port_no);

		return allocAddr;
	} else {
		mfc_debug("there is no suitable chunk\n");
		return 0;
	}

	return allocAddr;
}

int s3c_mfc_init_buffer_manager(void)
{
	struct s3c_mfc_free_mem_t *free_node;
	struct s3c_mfc_alloc_mem_t *alloc_node;
	int port_no;

	for (port_no = 0; port_no < MFC_MAX_PORT_NUM; port_no++) {

		/* init alloc list, if(s3c_mfc_alloc_mem_head ==
		 * s3c_mfc_alloc_mem_tail) then, the list is NULL
		 */
		alloc_node =
		    (struct s3c_mfc_alloc_mem_t *) kmalloc(sizeof(struct s3c_mfc_alloc_mem_t),
						    GFP_KERNEL);
		memset(alloc_node, 0x00, sizeof(struct s3c_mfc_alloc_mem_t));
		alloc_node->next = alloc_node;
		alloc_node->prev = alloc_node;
		s3c_mfc_alloc_mem_head[port_no] = alloc_node;
		s3c_mfc_alloc_mem_tail[port_no] =
		    s3c_mfc_alloc_mem_head[port_no];

		/* init free list, if(s3c_mfc_free_mem_head ==
		 * s3c_mfc_free_mem_tail) then, the list is NULL
		 */
		free_node =
		    (struct s3c_mfc_free_mem_t *) kmalloc(sizeof(struct s3c_mfc_free_mem_t),
						   GFP_KERNEL);
		memset(free_node, 0x00, sizeof(struct s3c_mfc_free_mem_t));
		free_node->next = free_node;
		free_node->prev = free_node;
		s3c_mfc_free_mem_head[port_no] = free_node;
		s3c_mfc_free_mem_tail[port_no] = s3c_mfc_free_mem_head[port_no];

		/* init free head node */
		free_node =
		    (struct s3c_mfc_free_mem_t *) kmalloc(sizeof(struct s3c_mfc_free_mem_t),
						   GFP_KERNEL);
		memset(free_node, 0x00, sizeof(struct s3c_mfc_free_mem_t));
		free_node->start_addr =
		    (port_no) ? s3c_mfc_get_dpb_luma_buf_phys_addr() :
		    s3c_mfc_get_data_buf_phys_addr();
		free_node->size =
		    (port_no) ? s3c_mfc_get_dpb_luma_buf_phys_size() :
		s3c_mfc_get_data_buf_phys_size();

		s3c_mfc_insert_first_node_to_free_list(free_node, -1, port_no);
	}

	return 0;
}

/* Releae memory */
enum SSBSIP_MFC_ERROR_CODE s3c_mfc_release_alloc_mem(struct s3c_mfc_inst_ctx *mfc_ctx,
						struct s3c_mfc_alloc_mem_t *node)
{
	int ret;

	struct s3c_mfc_free_mem_t *free_node;

	free_node =
	    (struct s3c_mfc_free_mem_t *) kmalloc(sizeof(struct s3c_mfc_free_mem_t),
					   GFP_KERNEL);

	free_node->start_addr = node->p_addr;

	free_node->size = node->size;
	s3c_mfc_insert_node_to_free_list(free_node, mfc_ctx->mem_inst_no,
					 mfc_ctx->port_no);

	/* Delete from AllocMem list */
	s3c_mfc_del_node_from_alloc_list(node, mfc_ctx->mem_inst_no,
					 mfc_ctx->port_no);

	ret = MFC_RET_OK;

	return ret;
}

enum SSBSIP_MFC_ERROR_CODE s3c_mfc_get_phys_addr(struct s3c_mfc_inst_ctx *mfc_ctx,
					    union s3c_mfc_args *args)
{
	int ret;
	struct s3c_mfc_alloc_mem_t *node;
	struct s3c_mfc_get_phys_addr_arg_t *codec_get_phy_addr_arg =
	    (struct s3c_mfc_get_phys_addr_arg_t *) args;
	int port_no = mfc_ctx->port_no;

	for (node = s3c_mfc_alloc_mem_head[port_no];
	     node != s3c_mfc_alloc_mem_tail[port_no]; node = node->next) {
		if (node->u_addr ==
		    (unsigned char *)codec_get_phy_addr_arg->u_addr)
			break;
	}

	if (node == s3c_mfc_alloc_mem_tail[port_no]) {
		mfc_err("invalid virtual address(0x%x)\r\n",
			codec_get_phy_addr_arg->u_addr);
		ret = MFC_RET_MEM_INVALID_ADDR_FAIL;
		goto out_getphysaddr;
	}

	codec_get_phy_addr_arg->p_addr = node->p_addr;

	ret = MFC_RET_OK;

out_getphysaddr:
	return ret;
}

enum SSBSIP_MFC_ERROR_CODE s3c_mfc_get_virt_addr(struct s3c_mfc_inst_ctx *mfc_ctx,
					    union s3c_mfc_args *args)
{
	int ret;
	int inst_no = mfc_ctx->mem_inst_no;
	int port_no = mfc_ctx->port_no;
	unsigned int p_startAddr;
	struct s3c_mfc_mem_alloc_arg_t *in_param;
	struct s3c_mfc_alloc_mem_t *p_allocMem;

	in_param = (struct s3c_mfc_mem_alloc_arg_t *) args;

	/* if user request area, allocate from reserved area */
	p_startAddr =
	    s3c_mfc_get_mem_area((int)in_param->buff_size, inst_no, port_no);
	mfc_debug("p_startAddr = 0x%X\n\r", p_startAddr);

	if (!p_startAddr) {
		mfc_debug("There is no more memory\n\r");
		in_param->out_addr = -1;
		ret = MFC_RET_MEM_ALLOC_FAIL;
		goto out_getcodecviraddr;
	}

	p_allocMem =
	    (struct s3c_mfc_alloc_mem_t *) kmalloc(sizeof(struct s3c_mfc_alloc_mem_t),
					    GFP_KERNEL);
	memset((void *)p_allocMem, 0x00, sizeof(struct s3c_mfc_alloc_mem_t));

	p_allocMem->p_addr = p_startAddr;
	if (port_no) {		/* port 1 */
		p_allocMem->v_addr =
		    (unsigned char *)(s3c_mfc_get_dpb_luma_buf_virt_addr() +
				      (p_allocMem->p_addr -
				       s3c_mfc_get_dpb_luma_buf_phys_addr()));
		p_allocMem->u_addr =
		    (unsigned char *)(in_param->mapped_addr +
				      s3c_mfc_get_data_buf_phys_size()  +
				      (p_allocMem->p_addr -
				      s3c_mfc_get_dpb_luma_buf_phys_addr()));
	} else {	/* Check whether p_allocMem->v_addr is aligned 4KB */
		p_allocMem->v_addr =
		    (unsigned char *)(s3c_mfc_get_data_buf_virt_addr() +
				      (p_allocMem->p_addr -
				       s3c_mfc_get_data_buf_phys_addr()));
		p_allocMem->u_addr =
		    (unsigned char *)(in_param->mapped_addr +
				      (p_allocMem->p_addr -
				       s3c_mfc_get_data_buf_phys_addr()));
	}

	if (p_allocMem->v_addr == NULL) {
		mfc_debug("Mapping Failed [PA:0x%08x]\n\r",
			  p_allocMem->p_addr);
		ret = MFC_RET_MEM_MAPPING_FAIL;
		goto out_getcodecviraddr;
	}

	in_param->out_addr = (unsigned int)p_allocMem->u_addr;
	mfc_debug("u_addr : 0x%x v_addr : 0x%x p_addr : 0x%x\n",
		  (unsigned int)p_allocMem->u_addr,
		  (unsigned int)p_allocMem->v_addr,
		  (unsigned int)p_allocMem->p_addr);

	p_allocMem->size = (int)in_param->buff_size;
	p_allocMem->inst_no = inst_no;
	p_allocMem->port_no = port_no;

	s3c_mfc_insert_node_to_alloc_list(p_allocMem, inst_no, port_no);
	ret = MFC_RET_OK;

out_getcodecviraddr:
	return ret;
}

struct s3c_mfc_alloc_mem_t *s3c_mfc_get_alloc_mem_head(int port_num)
{
	struct s3c_mfc_alloc_mem_t *alloc_mem_head;

	alloc_mem_head = s3c_mfc_alloc_mem_head[port_num];

	return alloc_mem_head;
}

struct s3c_mfc_alloc_mem_t *s3c_mfc_get_alloc_mem_tail(int port_num)
{
	struct s3c_mfc_alloc_mem_t *alloc_mem_tail;

	alloc_mem_tail = s3c_mfc_alloc_mem_tail[port_num];

	return alloc_mem_tail;
}

