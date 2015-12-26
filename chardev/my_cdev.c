#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>
#include <linux/errno.h>

MODULE_LICENSE("MIT");
MODULE_AUTHOR("Chaojie Wang");
MODULE_DESCRIPTION("playing around with kernel");

static int __init my_cdev_setup(void)
{
	return 0;
}

static void __exit my_cdev_cleanup(void)
{
	return;
}

module_init(my_cdev_setup);
module_exit(my_cdev_cleanup);
