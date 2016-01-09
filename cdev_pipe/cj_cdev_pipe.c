/**
 * This is a in-memory buffer device. The size of the buffer
 *  is fixed. Anyone can open it and write data to it, or read 
 *  data from it. Non-Blocking mode is implemented. poll is 
 *  also implemented for I/O multiplexing.
 */


#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>
#include <linux/errno.h>
#include "cj_cdev_pipe"

static struct cj_cbuf *cj_cbuf;
static int init_err;

static const struct file_operations cj_cbuf_op = {
	.owner = THIS_MODULE,
	.open = cj_open,
	.read = cj_read,
	.write = cj_write,
	.poll = cj_poll;
	.release = cj_release,
	.unlocked_ioctl = cj_ioctl,
};

static int __init cdev_setup(void)
{
	printk(KERN_INFO "set up the cdev\n");

	char mod_name = "cjdev_pipe";               
	int buffer_size = 512;                      
	dev_t dev;
	int ret;

	ret = alloc_chrdev_region(&dev, 0, 1, mod_name);
	if (ret != 0) {
		printk(KERN_ERR "error code %d when allocating chrdev\n", ret);
		ret = -1;
		goto error;
	}

	// allocate the memory for cj_cbuf
	cj_cbuf = kmalloc(sizeof(struct cj_cbuf), GFP_KERNEL);
	if (!cj_cbuf) {
		printk(KERN_ERR "can not allocate a new cj_cbuf\n");
		ret = -1;
		goto error;
	}

	// allocate cdev and add it to the kernel
	cdev_init(&cj_cbuf->cdev, &cj_cbuf_op);
	ret = cdev_add(&cj_cbuf->cdev, dev, 1);
	if (ret) {
		printk(KERN_ERR "can not add the new device\n");
		ret = -1;
		goto error;
	}

	// init semaphore
	sema_init(&cj_cbuf->sem, 1);
	
	// init wait queue
	init_waitqueue_head(&cj_cbuf->rwait);
	init_waitqueue_head(&cj_cbuf->wwait);

	// init buffer and rest
	cj_cbuf->begin = kmalloc(sizeof(char)*buffer_size, GFP_KERNEL);
	if (!cj_cbuf->begin) {
		printk(KERN_ERR "can not allocate cj_cbuf->begin\n");
		ret = -1;
		goto error;
	}
	cj_cbuf->end = cj_cbuf->begin + sizeof(char)*(buffer_size-1);
	cj_cbuf->buffer_size = buffer_size;
	cj_cbuf->rp = cj_cbuf->begin;
	cj_cbuf->wp = cj_cbuf->begin;
	cj_cbuf->nreaders = 0;
	cj_cbuf->nwriters = 0;

	return ret;
error:
	init_err = -1;
	return ret;
}

static void __exit cdev_cleanup(void)
{
	kfree(cj_cbuf->begin);
	kfree(cj_cbuf);
}

module_init(cdev_setup);
module_exit(cdev_cleanup);









