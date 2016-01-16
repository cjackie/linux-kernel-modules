/**
 * This is for experimenting with kernel
 */


#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/fs.h>

static struct cdev *cj_cdev;

static int cj_open(struct inode *inode, struct file *filp) 
{
	printk(KERN_INFO "open is invoked\n");

	/* code here to poke around the kernel */

	return 0;
}

static int cj_release(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "release is invoked\n");

	/* or code here to poke around the kernel */

	return 0;
}

static const struct file_operations cj_op = {
	.owner = THIS_MODULE,
	.open = cj_open,
	.release = cj_release,
};

static int __init cdev_setup(void)
{
	printk(KERN_INFO "set up the cj_poke\n");

	char *mod_name = "cj_poke";               
	dev_t dev;
	int ret;

	ret = alloc_chrdev_region(&dev, 0, 1, mod_name);
	if (ret != 0) {
		printk(KERN_ERR "error code %d when allocating chrdev\n", ret);
		ret = -1;
		goto error;
	}

	cj_cdev = kmalloc(sizeof(struct cdev), GFP_KERNEL);
	if (!cj_cdev) {
		printk(KERN_ERR "no mem\n");
		ret = -1;
		goto error;
	}
	/* allocate cdev and add it to the kernel */
	cdev_init(cj_cdev, &cj_op);
	ret = cdev_add(cj_cdev, dev, 1);
	if (ret) {
		printk(KERN_ERR "can not add the new device\n");
		ret = -1;
		goto error;
	}

	return ret;
error:
	kfree(cj_cdev);
	return ret;
}

static void __exit cdev_cleanup(void)
{
	unregister_chrdev_region(cj_cdev->dev, 1);
	printk(KERN_INFO "free and exiting\n");
	return;
}

module_init(cdev_setup);
module_exit(cdev_cleanup);
