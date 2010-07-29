/* linux/drivers/media/video/samsung/mfc50/s3c_mfc_buffer_manager.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Definition of handling for reserved memory
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __S3C_MFC_BUFFER_MANAGER_H
#define __S3C_MFC_BUFFER_MANAGER_H __FILE__

#include "s3c_mfc_interface.h"
#include "s3c_mfc_common.h"

#define MFC_MAX_PORT_NUM	(2)

struct s3c_mfc_alloc_mem_t {
	struct s3c_mfc_alloc_mem_t *prev;
	struct s3c_mfc_alloc_mem_t *next;
	unsigned int p_addr;	/* physical address */

	unsigned char *v_addr;	/* virtual address */
	unsigned char *u_addr;	/* copied virtual addr for user mode process */
	int size;		/* memory size */
	int inst_no;
	int port_no;
};

struct s3c_mfc_free_mem_t {
	struct s3c_mfc_free_mem_t *prev;
	struct s3c_mfc_free_mem_t *next;
	unsigned int start_addr;
	unsigned int size;
};

void s3c_mfc_print_list(void);
int s3c_mfc_init_buffer_manager(void);
void s3c_mfc_merge_frag(int inst_no);
enum SSBSIP_MFC_ERROR_CODE s3c_mfc_release_alloc_mem(struct s3c_mfc_inst_ctx *mfc_ctx,
						struct s3c_mfc_alloc_mem_t *node);
enum SSBSIP_MFC_ERROR_CODE s3c_mfc_get_phys_addr(struct s3c_mfc_inst_ctx *mfc_ctx,
					    union s3c_mfc_args *args);
enum SSBSIP_MFC_ERROR_CODE s3c_mfc_get_virt_addr(struct s3c_mfc_inst_ctx *mfc_ctx,
					    union s3c_mfc_args *args);
struct s3c_mfc_alloc_mem_t *s3c_mfc_get_alloc_mem_head(int port_num);
struct s3c_mfc_alloc_mem_t *s3c_mfc_get_alloc_mem_tail(int port_num);


#endif /* __S3C_MFC_BUFFER_MANAGER_H */
