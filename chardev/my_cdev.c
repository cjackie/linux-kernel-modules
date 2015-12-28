#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>
#include <linux/errno.h>
#include <linux/string.h>
#include "my_cdev.h"

MODULE_LICENSE("MIT");
MODULE_AUTHOR("Chaojie Wang");
MODULE_DESCRIPTION("playing around with kernel");

static struct cj_cdev *my_cj_cdev;

static int d_open(struct inode *inode, struct file *filp) 
{
	// TODO
	return 0;
}

static int d_release(struct inode *inode, struct file *filp)
{
	// TODO
	return 0;
}

static ssize_t d_read(struct file *filp, char __user *buf, size_t len, loff_t *pos)
{
	// TODO
	return 0;
}

static ssize_t d_write(struct file *filp, const char __user *buf, size_t len, loff_t *pos)
{
	// TODO
	return 0;
}

static long d_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	// TODO;
	return 0;
}

static const struct file_operations cj_cdev_op = {
	.owner = THIS_MODULE,
	.open = d_open,
	.read = d_read,
	.write = d_write,
	.release = d_release,
	.unlocked_ioctl = d_ioctl,
};


static int __init my_cdev_setup(void)
{
	printk(KERN_INFO "set up the my cdev\n");
	// obtain a major number and a minor number
	dev_t dev;
	int ret;
	struct cdev *my_cdev = NULL;
	int dsize;
	char *cj_list_data = NULL;

	if ((ret = alloc_chrdev_region(&dev, 0, 1, "cj_cdev")) != 0) {
		printk(KERN_ERR "error code %d when allocating chrdev\n", ret);
		goto error;
	}

	// allocate the memory for cj_cdev
	my_cj_cdev = kmalloc(sizeof(struct cj_cdev), GFP_KERNEL);
	if (!my_cj_cdev) {
		printk(KERN_ERR "can not allocate a new cdev\n");
		goto free_and_exit;
	}

	// allocate cdev and add it to the kernel
	my_cdev = &my_cj_cdev->lcdev;
	cdev_init(my_cdev, &cj_cdev_op);
	ret = cdev_add(my_cdev, dev, 1);
	if (ret) {
		printk(KERN_ERR "can not add the new device\n");
		goto free_and_exit;
	}

	// init semaphore
	sema_init(&my_cj_cdev->sem, 1);
	
	// allocate data in the cj_cdev
	dsize = 512;
	my_cj_cdev->head = kmalloc(sizeof(struct cj_list), GFP_KERNEL);
	cj_list_data = kmalloc(sizeof(char)*dsize, GFP_KERNEL);
	if (!my_cj_cdev->head || !cj_list_data) {
		printk(KERN_ERR "insufficient memory for allocation\n");
		goto free_and_exit;
	}
	memset(cj_list_data, 0, dsize);
	my_cj_cdev->lsize = 1;
	my_cj_cdev->dsize = dsize;
	my_cj_cdev->head->next = NULL;
	my_cj_cdev->head->dsize = dsize;
	my_cj_cdev->head->cdsize = 0;
	my_cj_cdev->head->data = cj_list_data;
	
	return ret;

error:
	return ret;

free_and_exit:
	if (my_cj_cdev) {
		kfree(cj_list_data);
		kfree(my_cdev);
		kfree(my_cj_cdev);
	}
	return -ENOBUFS;
}

static void __exit my_cdev_cleanup(void)
{
	struct cj_list *l_ptr;
	struct cj_list *next;
	for (l_ptr = my_cj_cdev->head; !l_ptr; l_ptr = next) {
		kfree(l_ptr->data);
		next = l_ptr->next;
		kfree(l_ptr);
	}
	kfree(my_cj_cdev);
	printk(KERN_INFO "free and exiting\n");
	return;
}

module_init(my_cdev_setup);
module_exit(my_cdev_cleanup);
