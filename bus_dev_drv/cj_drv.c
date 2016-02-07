#include <linux/module.h>

#include "cj_dev_model.h"

/* dummy methods */

static int my_probe(struct cj_dev *dev) {
	printk("from the driver, probe is invoked");
	return 0;
}

static int my_remove(struct cj_dev *dev) {
	printk("from the driver, remove is invoked");
	return 0;
}

static int my_suspend(struct cj_dev * dev, pm_message_t state) {
	printk("from the driver, suspend is invoked");
	return 0;
}

static int my_resume(struct cj_dev * dev) {
	printk("from the driver, resume is invoked");
	return 0;
}

static struct cj_dev_drv_op op = {
	.probe = my_probe,
	.remove = my_remove,
	.suspend = my_suspend,
	.resume = my_resume
};
static struct cj_dev_drv cj_dev0_driver;

static int __init cj_dev0_driver_init(void) {
	if (cj_dev_drv_init(&cj_dev0_driver, "cj_dev0", &op)) {
		printk(KERN_ERR "can't register the driver");
		return -1;
	}
	return register_cj_dev_drv(&cj_dev0_driver);
}

static void __exit cj_dev0_driver_exit(void) {
	unregister_cj_dev_drv(&cj_dev0_driver);
}

module_init(cj_dev0_driver_init);
module_exit(cj_dev0_driver_exit);
