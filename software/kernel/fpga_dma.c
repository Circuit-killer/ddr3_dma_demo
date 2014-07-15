#include <linux/module.h>
#include <linux/init.h>

#include <asm/io.h>
#include <asm/uaccess.h>

#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/sem.h>
#include <linux/slab.h>

#include <linux/wait.h>
#include <linux/sched.h>

#define LWHPS2FPGA_BRIDGE_BASE 0xff200000
#define LWHPS2FPGA_BRIDGE_LEN  0x40

#define FPGA_INT_BASE 72
#define FPGA_INT_FLAGS 0
#define DMA_FPGA2HPS_INT (FPGA_INT_BASE + 1)
#define DMA_HPS2FPGA_INT FPGA_INT_BASE

#define DMA_FPGA2HPS_BASE 0x00
#define DMA_HPS2FPGA_BASE 0x20

#define DMA_FPGA2HPS_STATUS DMA_FPGA2HPS_BASE
#define DMA_FPGA2HPS_READADDR (DMA_FPGA2HPS_BASE | 0x4)
#define DMA_FPGA2HPS_WRITEADDR (DMA_FPGA2HPS_BASE | 0x8)
#define DMA_FPGA2HPS_LENGTH (DMA_FPGA2HPS_BASE | 0xc)
#define DMA_FPGA2HPS_CTRL (DMA_FPGA2HPS_BASE | 0x18)

#define DMA_HPS2FPGA_STATUS DMA_HPS2FPGA_BASE
#define DMA_HPS2FPGA_READADDR (DMA_HPS2FPGA_BASE | 0x4)
#define DMA_HPS2FPGA_WRITEADDR (DMA_HPS2FPGA_BASE | 0x8)
#define DMA_HPS2FPGA_LENGTH (DMA_HPS2FPGA_BASE | 0xc)
#define DMA_HPS2FPGA_CTRL (DMA_HPS2FPGA_BASE | 0x18)

#define MAJOR_NUMBER 240
#define DEVICE_NAME "fpga_dma"

static void *fpga_dma_iomem;

static struct {
} interrupt_ctrl;

static DEFINE_MUTEX(dma_hps2fpga_mutex);
static DEFINE_MUTEX(dma_fpga2hps_mutex);

static DECLARE_WAIT_QUEUE_HEAD(dma_hps2fpga_wq);
static DECLARE_WAIT_QUEUE_HEAD(dma_fpga2hps_wq);

static int status_done(unsigned long status_addr)
{
	int status = ioread32(fpga_dma_iomem + status_addr);
	return status & 0x1;
}

static irqreturn_t fpga_dma_handler(int irq, void *dev_id)
{
	switch (irq) {
		case DMA_FPGA2HPS_INT:
			wake_up_interruptible(&dma_fpga2hps_wq);
			break;
		case DMA_HPS2FPGA_INT:
			wake_up_interruptible(&dma_hps2fpga_wq);
			break;
		default:
			return IRQ_NONE;
	}
	return IRQ_HANDLED;
}

static ssize_t fpga_dma_read(
		struct file *file, char __user *buf,
		size_t count, loff_t *f_pos)
{
	char *kbuf;
	int ret;

	kbuf = kmalloc(count, GFP_KERNEL);
	if (kbuf == NULL)
		return -ENOMEM;

	ret = mutex_lock_interruptible(&dma_fpga2hps_mutex);
	if (ret < 0)
		goto fail_lock;

	ret = wait_event_interruptible(dma_fpga2hps_wq,
			status_done(DMA_FPGA2HPS_STATUS));
	if (ret < 0)
		goto fail_wait;

fail_wait:
	mutex_unlock(&dma_fpga2hps_mutex);
fail_lock:
	kfree(kbuf);
	return ret;
}

static ssize_t fpga_dma_write(
		struct file *file, const char __user *buf,
		size_t count, loff_t *f_pos)
{
	return 0;
}

static struct file_operations fpga_dma_fops = {
	.read = fpga_dma_read,
	.write = fpga_dma_write,
};

static int fpga_dma_init(void)
{
	int ret;
	struct resource *resource;

	ret = register_chrdev(MAJOR_NUMBER, DEVICE_NAME, &fpga_dma_fops);
	if (ret < 0)
		goto fail_chrdev;

	resource = request_mem_region(
			LWHPS2FPGA_BRIDGE_BASE,
			LWHPS2FPGA_BRIDGE_LEN,
			DEVICE_NAME);
	if (resource == NULL) {
		ret = -EBUSY;
		goto fail_request_mem;
	}

	fpga_dma_iomem = ioremap_nocache(
			LWHPS2FPGA_BRIDGE_BASE, LWHPS2FPGA_BRIDGE_LEN);
	if (fpga_dma_iomem == NULL) {
		ret = -EFAULT;
		goto fail_ioremap;
	}

	ret = request_irq(DMA_FPGA2HPS_INT, fpga_dma_handler,
			FPGA_INT_FLAGS, DEVICE_NAME, &interrupt_ctrl);
	if (ret < 0)
		goto fail_req_irq0;

	ret = request_irq(DMA_HPS2FPGA_INT, fpga_dma_handler,
			FPGA_INT_FLAGS, DEVICE_NAME, &interrupt_ctrl);
	if (ret < 0)
		goto fail_req_irq1;

	return 0;

fail_req_irq1:
	free_irq(DMA_FPGA2HPS_INT, &interrupt_ctrl);
fail_req_irq0:
	iounmap(fpga_dma_iomem);
fail_ioremap:
	release_mem_region(LWHPS2FPGA_BRIDGE_BASE, LWHPS2FPGA_BRIDGE_LEN);
fail_request_mem:
	unregister_chrdev(MAJOR_NUMBER, DEVICE_NAME);
fail_chrdev:
	return ret;
}

static void fpga_dma_exit(void)
{
	free_irq(DMA_HPS2FPGA_INT, &interrupt_ctrl);
	free_irq(DMA_FPGA2HPS_INT, &interrupt_ctrl);
	iounmap(fpga_dma_iomem);
	release_mem_region(LWHPS2FPGA_BRIDGE_BASE, LWHPS2FPGA_BRIDGE_LEN);
	unregister_chrdev(MAJOR_NUMBER, DEVICE_NAME);
}

module_init(fpga_dma_init);
module_exit(fpga_dma_exit);
