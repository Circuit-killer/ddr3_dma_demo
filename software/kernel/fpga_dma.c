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

#define DMA_CTRL_GO (1 << 3)
#define DMA_CTRL_I_EN (1 << 4)
#define DMA_CTRL_LEEN (1 << 7)
#define DMA_CTRL_QUADWORD (1 << 11)

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
	return (status & 0x2) == 0;
}

static irqreturn_t fpga_dma_handler(int irq, void *dev_id)
{
	switch (irq) {
		case DMA_FPGA2HPS_INT:
			pr_info("Got FPGA2HPS interrupt\n");
			wake_up_interruptible(&dma_fpga2hps_wq);
			break;
		case DMA_HPS2FPGA_INT:
			pr_info("Got HPS2FPGA interrupt\n");
			wake_up_interruptible(&dma_hps2fpga_wq);
			break;
		default:
			return IRQ_NONE;
	}
	return IRQ_HANDLED;
}

static void setup_fpga2hps_dma(void *hps_virt_addr, unsigned long fpga_addr,
		unsigned long count)
{
	unsigned long hps_phys_addr = virt_to_phys(hps_virt_addr);
	unsigned long ctrl_flags = DMA_CTRL_GO | DMA_CTRL_I_EN |
		DMA_CTRL_LEEN | DMA_CTRL_QUADWORD;

	iowrite32(0, fpga_dma_iomem + DMA_FPGA2HPS_STATUS);
	iowrite32(fpga_addr, fpga_dma_iomem + DMA_FPGA2HPS_READADDR);
	iowrite32(hps_phys_addr, fpga_dma_iomem + DMA_FPGA2HPS_WRITEADDR);
	iowrite32(count, fpga_dma_iomem + DMA_FPGA2HPS_LENGTH);
	iowrite32(ctrl_flags, fpga_dma_iomem + DMA_FPGA2HPS_CTRL);
}

static void setup_hps2fpga_dma(unsigned long fpga_addr, void *hps_virt_addr,
		unsigned long count)
{
	unsigned long hps_phys_addr = virt_to_phys(hps_virt_addr);
	unsigned long ctrl_flags = DMA_CTRL_GO | DMA_CTRL_I_EN |
		DMA_CTRL_LEEN | DMA_CTRL_QUADWORD;
	iowrite32(0, fpga_dma_iomem + DMA_HPS2FPGA_STATUS);
	iowrite32(hps_phys_addr, fpga_dma_iomem + DMA_HPS2FPGA_READADDR);
	iowrite32(fpga_addr, fpga_dma_iomem + DMA_HPS2FPGA_WRITEADDR);
	iowrite32(count, fpga_dma_iomem + DMA_HPS2FPGA_LENGTH);
	iowrite32(ctrl_flags, fpga_dma_iomem + DMA_FPGA2HPS_CTRL);
}

static ssize_t fpga_dma_read(
		struct file *file, char __user *buf,
		size_t count, loff_t *f_pos)
{
	char *kbuf;
	int ret;

	pr_info("starting fpga_dma read\n");

	kbuf = kmalloc(count, GFP_KERNEL);
	if (kbuf == NULL)
		return -ENOMEM;

	ret = mutex_lock_interruptible(&dma_fpga2hps_mutex);
	if (ret < 0)
		goto fail_before_lock;

	setup_fpga2hps_dma(kbuf, *f_pos, count);

	ret = wait_event_interruptible(dma_fpga2hps_wq,
			status_done(DMA_FPGA2HPS_STATUS));
	if (ret < 0)
		goto fail_after_lock;

	ret = copy_to_user(buf, kbuf, count);
	if (ret < 0)
		goto fail_after_lock;

	*f_pos += count;
	ret = count;

fail_after_lock:
	mutex_unlock(&dma_fpga2hps_mutex);
fail_before_lock:
	kfree(kbuf);
	return ret;
}

static ssize_t fpga_dma_write(
		struct file *file, const char __user *buf,
		size_t count, loff_t *f_pos)
{
	char *kbuf;
	int ret;

	pr_info("starting fpga_dma write\n");

	kbuf = kmalloc(count, GFP_KERNEL);
	if (kbuf == NULL)
		return -ENOMEM;

	ret = copy_from_user(kbuf, buf, count);
	if (ret < 0)
		goto fail_before_lock;

	ret = mutex_lock_interruptible(&dma_fpga2hps_mutex);
	if (ret < 0)
		goto fail_before_lock;

	setup_fpga2hps_dma(kbuf, *f_pos, count);
	setup_hps2fpga_dma(*f_pos, kbuf, count);

	ret = wait_event_interruptible(dma_fpga2hps_wq,
			status_done(DMA_FPGA2HPS_STATUS));
	if (ret < 0)
		goto fail_after_lock;

	*f_pos += count;
	ret = count;

fail_after_lock:
	mutex_unlock(&dma_fpga2hps_mutex);
fail_before_lock:
	kfree(kbuf);
	return ret;
	return 0;
}

static int fpga_dma_open(struct inode *inode, struct file *filp)
{
	pr_info("opened fpga_dma\n");
	return 0;
}

static int fpga_dma_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static struct file_operations fpga_dma_fops = {
	.read = fpga_dma_read,
	.write = fpga_dma_write,
	.open = fpga_dma_open,
	.release = fpga_dma_release
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

	pr_info("initialized fpga_dma\n");

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

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Howard Zhehao Mao");
