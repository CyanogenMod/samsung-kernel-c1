/* linux/drivers/media/video/samsung/jpeg/jpeg_dev.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * http://www.samsung.com/
 *
 * Core file for Samsung Jpeg Interface driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/signal.h>
#include <linux/ioport.h>
#include <linux/kmod.h>
#include <linux/vmalloc.h>
#include <linux/time.h>
#include <linux/clk.h>
#include <linux/semaphore.h>
#include <linux/vmalloc.h>
#include <asm/page.h>

#include <plat/media.h>
#include <plat/regs_jpeg.h>
#include <mach/media.h>
#include <mach/irqs.h>
/*#include <mach/pd.h>*/

#include "jpeg_core.h"
#include "jpeg_dev.h"

struct jpeg_control *jpeg_ctrl;

static int jpeg_open(struct inode *inode, struct file *file)
{
	int ret;
	int in_use;

	mutex_lock(&jpeg_ctrl->lock);

	in_use = atomic_read(&jpeg_ctrl->in_use);

	if (in_use > JPEG_MAX_INSTANCE) {
		ret = -EBUSY;
		goto resource_busy;
	} else {
		atomic_inc(&jpeg_ctrl->in_use);
		jpeg_info("jpeg driver opened.\n");
	}

	mutex_unlock(&jpeg_ctrl->lock);
/*
	ret = s5pv210_pd_enable("jpeg_pd");
	if (ret < 0) {
		jpeg_err("failed to enable jpeg power domain\n");
		return -EINVAL;
	}
*/
	/* clock enable */
	clk_enable(jpeg_ctrl->clk);

	file->private_data = (struct jpeg_control *)jpeg_ctrl;
	return 0;

resource_busy:
	mutex_unlock(&jpeg_ctrl->lock);
	return ret;
}

static int jpeg_release(struct inode *inode, struct file *file)
{
	int ret;

	atomic_dec(&jpeg_ctrl->in_use);

	clk_disable(jpeg_ctrl->clk);
/*
	ret = s5pv210_pd_disable("jpeg_pd");
	if (ret < 0) {
		jpeg_err("failed to disable jpeg power domain\n");
		return -EINVAL;
	}
*/

	return 0;
}

static int jpeg_ioctl(struct inode *inode, struct file *file,
					unsigned int cmd, unsigned long arg)
{
	int ret;
	struct jpeg_control	*ctrl;

	ctrl  = (struct jpeg_control *)file->private_data;
	if (!ctrl) {
		jpeg_err("jpeg invalid input argument\n");
		return -1;
	}


	switch (cmd) {

	case IOCTL_JPEG_DEC_EXE:
		ret = copy_from_user(&ctrl->dec_param,
			(struct jpeg_dec_param *)arg,
			sizeof(struct jpeg_dec_param));

		jpeg_exe_dec(ctrl);
		ret = copy_to_user((void *)arg,
			(void *) &ctrl->dec_param,
			sizeof(struct jpeg_dec_param));
		break;

	case IOCTL_JPEG_ENC_EXE:
		ret = copy_from_user(&ctrl->enc_param,
			(struct jpeg_enc_param *)arg,
			sizeof(struct jpeg_enc_param));

		jpeg_exe_enc(ctrl);
		ret = copy_to_user((void *)arg,
			(void *) &ctrl->enc_param,
			sizeof(struct jpeg_enc_param));
		break;

	case IOCTL_GET_DEC_IN_BUF:
	case IOCTL_GET_ENC_OUT_BUF:
		return jpeg_get_stream_buf(arg);

	case IOCTL_GET_DEC_OUT_BUF:
	case IOCTL_GET_ENC_IN_BUF:
		return jpeg_get_frame_buf(arg);

	case IOCTL_SET_DEC_PARAM:
		ret = copy_from_user(&ctrl->dec_param,
			(struct jpeg_dec_param *)arg,
			sizeof(struct jpeg_dec_param));

		ret = jpeg_set_dec_param(ctrl);

		break;

	case IOCTL_SET_ENC_PARAM:
		ret = copy_from_user(&ctrl->enc_param,
			(struct jpeg_enc_param *)arg,
			sizeof(struct jpeg_enc_param));

		ret = jpeg_set_enc_param(ctrl);
		break;

	default:
		break;
	}
	return 0;
}

int jpeg_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long	page_frame_no;
	unsigned long	size;
	int		ret;

	size = vma->vm_end - vma->vm_start;

	vma->vm_flags |= VM_RESERVED | VM_IO;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	page_frame_no = __phys_to_pfn(jpeg_ctrl->mem.base);
	ret = remap_pfn_range(vma, vma->vm_start, page_frame_no,
					size, vma->vm_page_prot);
	if (ret != 0) {
		jpeg_err("failed to remap jpeg pfn range.\n");
		return -ENOMEM;
	}
	return 0;
}

static const struct file_operations jpeg_fops = {
	.owner = THIS_MODULE,
	.open =	jpeg_open,
	.release = jpeg_release,
	.ioctl = jpeg_ioctl,
	.mmap =	jpeg_mmap,
};

static struct miscdevice jpeg_miscdev = {
	.minor = JPEG_MINOR_NUMBER,
	.name =	JPEG_NAME,
	.fops =	&jpeg_fops,
};

static irqreturn_t jpeg_irq(int irq, void *dev_id)
{
	unsigned int int_status;
	struct jpeg_control *ctrl = (struct jpeg_control *) dev_id;

	int_status = jpeg_int_pending(ctrl);

	if (int_status) {
		switch (int_status) {
		case 0x40:
			ctrl->irq_ret = OK_ENC_OR_DEC;
			break;
		case 0x20:
			ctrl->irq_ret = ERR_ENC_OR_DEC;
			break;
		default:
			ctrl->irq_ret = ERR_UNKNOWN;
		}

		wake_up_interruptible(&ctrl->wq);
	} else {
		ctrl->irq_ret = ERR_UNKNOWN;
		wake_up_interruptible(&ctrl->wq);
	}

	return IRQ_HANDLED;
}

static int jpeg_probe(struct platform_device *pdev)
{
	struct	resource *res;
	int	ret;

	if (!jpeg_ctrl) {
		jpeg_ctrl = kzalloc(sizeof(*jpeg_ctrl), GFP_KERNEL);
		if (!jpeg_ctrl) {
			dev_err(&pdev->dev, "%s: not enough memory\n",
				__func__);
			return -ENOMEM;
		}
	}

	jpeg_ctrl->clk = clk_get(&pdev->dev, "jpeg");
	if (IS_ERR(jpeg_ctrl->clk)) {
		jpeg_err("failed to find jpeg clock source\n");
		return -EINVAL;
	}

	clk_enable(jpeg_ctrl->clk);

	jpeg_ctrl->mem.base = s5p_get_media_memory_bank(S5P_MDEV_JPEG, 0);
	/* get memory region for jpeg driver =================================*/
	/* get resource for io memory */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		jpeg_err("failed to get jpeg memory region resource\n");
		return -EINVAL;
	}

	/* request mem region */
	res = request_mem_region(res->start,
				res->end - res->start + 1, pdev->name);
	if (!res) {
		jpeg_err("failed to request jpeg io memory region\n");
		return -EINVAL;
	}

	/* ioremap for jpeg driver register block */
	jpeg_ctrl->reg_base = ioremap(res->start, res->end - res->start + 1);

	if (!jpeg_ctrl->reg_base)
		jpeg_err("failed to remap jpeg io region\n");

	/* end get memory region for jpeg driver ====================*/

	/* enroll jpeg driver irq ===================================*/
	/* get resource for io irq */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		jpeg_err("failed to request jpeg irq resource\n");
		return -EINVAL;
	}

	jpeg_ctrl->irq_no = res->start;
	ret = request_irq(jpeg_ctrl->irq_no, (void *)jpeg_irq,
			IRQF_DISABLED, pdev->name, jpeg_ctrl);
	if (ret != 0) {
		jpeg_err("failed to jpeg request irq\n");
		return -EINVAL;
	}
	/* end enroll jpeg driver irq ================================*/

	atomic_set(&jpeg_ctrl->in_use, 0);
	mutex_init(&jpeg_ctrl->lock);
	init_waitqueue_head(&jpeg_ctrl->wq);

	ret = misc_register(&jpeg_miscdev);

	clk_disable(jpeg_ctrl->clk);

	return 0;
}

static int jpeg_remove(struct platform_device *dev)
{

	free_irq(jpeg_ctrl->irq_no, dev);
	mutex_destroy(&jpeg_ctrl->lock);
	iounmap(jpeg_ctrl->reg_base);

	kfree(jpeg_ctrl);
	misc_deregister(&jpeg_miscdev);

	return 0;
}

static int jpeg_suspend(struct platform_device *pdev, pm_message_t state)
{
	int ret;
	/* clock disable */
	clk_disable(jpeg_ctrl->clk);
/*
	ret = s5pv210_pd_disable("jpeg_pd");
	if (ret < 0) {
		jpeg_err("failed to disable jpeg power domain\n");
		return -EINVAL;
	}
*/
	return 0;
}

static int jpeg_resume(struct platform_device *pdev)
{
	int ret;
/*
	ret = s5pv210_pd_enable("jpeg_pd");
	if (ret < 0) {
		jpeg_err("failed to enable jpeg power domain\n");
		return -EINVAL;
	}
*/
	/* clock enable */
	clk_enable(jpeg_ctrl->clk);

	return 0;
}
static struct platform_driver jpeg_driver = {
	.probe		= jpeg_probe,
	.remove		= jpeg_remove,
	.suspend	= jpeg_suspend,
	.resume		= jpeg_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= JPEG_NAME,
	},
};

static int __init jpeg_init(void)
{
	printk("Initialize JPEG driver\n");

	platform_driver_register(&jpeg_driver);

	return 0;
}

static void __exit jpeg_exit(void)
{
	platform_driver_unregister(&jpeg_driver);
}

module_init(jpeg_init);
module_exit(jpeg_exit);

MODULE_AUTHOR("Hyunmin, Kwak <hyunmin.kwak@samsung.com>");
MODULE_DESCRIPTION("JPEG Codec Device Driver");
MODULE_LICENSE("GPL");

