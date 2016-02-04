#include <linux/module.h>

#include "cj_dev_model.h"

static struct cj_dev cj_dev0 = {
	.name = "cj_dev0"
};

static int __init cj_dev0_int(void) {
	printk(KERN_INFO "cj_dev0_init is invoked\n");
	
	if (cj_dev_register(&cj_dev0)) {
		printk(KERN_ERR "something went wrong when registering cj_dev0\n");
		return -1;
	}

	return 0;
}

static void __exit cj_dev0_exit(void) {
	printk(KERN_INFO "cj_dev0_exit is invoked\n");
	cj_dev_unregister(&cj_dev0);
}

module_init(cj_dev0_init);
module_exit(cj_dev0_exit);







