#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/stat.h>
#include <linux/slab.h>

#include "cj_dev_model.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chaojie Wang");
MODULE_DESCRIPTION("playing around with kernel");

static int cj_bus_match(struct device *dev, struct device_driver *drv) {
	/* don't know what to check. */
	/* according to kernel doc, we need to check if dev matches with drv
	 * or not. 
	 */
	printk(KERN_INFO "cj_bus_match is invoked\n");

	if (dev == NULL || drv == NULL) {
		printk(KERN_WARNING "dev or drv is NULL?\n");
		return 0;
	}

	if (dev->init_name == NULL || drv->name == NULL) {
		printk(KERN_WARNING "name in dev or drv is NULL?\n");
		return 0;
	}
	
	/* just perform a simple name checking */
	if (!strcmp(dev->init_name, drv->name)) {
		printk(KERN_INFO "name is not matched\n");
		return 0;
	}
	printk(KERN_INFO "name is matched\n");
  
	return 1;		/* match */
}

struct bus_type cj_bus_type = {
	.name = "cj_bus",
	.dev_name = "cj_bus",
	.match = cj_bus_match
};
EXPORT_SYMBOL(cj_bus_type);

/* an attribute */
#define cj_bus_attr_store_data_size 1024
static char cj_bus_attr_store_data[cj_bus_attr_store_data_size];
static ssize_t cj_bus_show(struct bus_type *bus, char *buf) {
	ssize_t wrt_n = snprintf(buf, PAGE_SIZE, "hello from the cj_bus. extra info: %s", 
		 cj_bus_attr_store_data);
	return wrt_n;
}

static ssize_t cj_bus_store(struct bus_type *bus, const char *buf, size_t count){
	memset(cj_bus_attr_store_data, 0, cj_bus_attr_store_data_size);
	ssize_t rd_n = snprintf(cj_bus_attr_store_data, 
				min(cj_bus_attr_store_data_size-1, (int)count), "%s", buf);
	return rd_n;
}

/*
 * cj_bus_type attributes. the name will be bus_attr_hello_file created by 
 * this macro.
 */
static BUS_ATTR(hello_file, 0666, cj_bus_show, cj_bus_store);

/* bus device */
struct device cj_dev_root = {
	.parent = NULL,
	.bus = &cj_bus_type,
	.init_name = "cj_dev_root"
};
EXPORT_SYMBOL(cj_dev_root);

/**
 *  register a cj_dev under cj_dev_root.
 *  @cj_dev, device
 *
 *  return 0 upon success? 
 */
int cj_dev_register(struct cj_dev *cj_dev) {
	printk("registering a cj_dev, with a name: %s\n", cj_dev->name);
	
	/* according to Linux/Documentation/driver-model/device.txt */
	/* set up fields */
	char *name = kmalloc(sizeof(char)*(strlen(cj_dev->name)+1), GFP_KERNEL);
	if (name == NULL) {
		printk(KERN_ERR "no mem\n");
		return -1;
	}
	strcpy(name, cj_dev->name);

	cj_dev->dev.parent = &cj_dev_root;
	cj_dev->dev.bus = &cj_bus_type;
	cj_dev->dev.init_name = name;
	cj_dev->dev.id = cj_dev->id;
	return device_register(&cj_dev->dev);
}
EXPORT_SYMBOL(cj_dev_register);

/**
 *  unregister a device under cj_dev_root.
 */
void cj_dev_unregister(struct cj_dev *cj_dev) {
	kfree(cj_dev->name);
	cj_dev->name = NULL;
	
	device_unregister(&cj_dev->dev);
}
EXPORT_SYMBOL(cj_dev_unregister);


static int cj_dev_drv_probe(struct device *dev) {
	/*
	 * additional checking on the device can be perform 
	 * before pass it to cj_dev_drv one.
	 */
	struct cj_dev_drv *driver = to_cj_dev_drv(dev->driver);
	struct cj_dev *device = to_cj_dev(dev);
	if (driver == NULL || device == NULL) {
		printk(KERN_ERR "in cj_dev_drv found NULL(1)?\n");
		return -1;
	}
	return driver->probe(device);
}

static int cj_dev_drv_remove(struct device *dev) {
	/*
	 * additional checking on the device can be perform 
	 * before pass it to cj_dev_drv one.
	 */
	struct cj_dev_drv *driver = to_cj_dev_drv(dev->driver);
	struct cj_dev *device = to_cj_dev(dev);
	if (driver == NULL || device == NULL) {
		printk(KERN_ERR "in cj_dev_drv found NULL(2)?\n");
		return -1;
	}
	return driver->remove(device);
}

static int cj_dev_drv_suspend(struct device *dev, pm_message_t state) {
	/*
	 * additional checking on the device can be perform 
	 * before pass it to cj_dev_drv one.
	 */
	struct cj_dev_drv *driver = to_cj_dev_drv(dev->driver);
	struct cj_dev *device = to_cj_dev(dev);
	if (driver == NULL || device == NULL) {
		printk(KERN_ERR "in cj_dev_drv found NULL(3)?\n");
		return -1;
	}
	return driver->suspend(device, state);
}

static int cj_dev_drv_resume(struct device *dev) {
	/*
	 * additional checking on the device can be perform 
	 * before pass it to cj_dev_drv one.
	 */
	struct cj_dev_drv *driver = to_cj_dev_drv(dev->driver);
	struct cj_dev *device = to_cj_dev(dev);
	if (driver == NULL || device == NULL) {
		printk(KERN_ERR "in cj_dev_drv found NULL(4)?\n");
		return -1;
	}
	return driver->resume(device);
}

int register_cj_dev_drv(struct cj_dev_drv *drv) {
	if (drv == NULL) {
		printk(KERN_ERR "drv is NULL\n");
		return -1;
	}

	char *name = kmalloc(sizeof(char)*(strlen(drv->name)+1), GFP_KERNEL);
	
	drv->driver.name = name;
	/* Type of the function is different! think about it... */
	drv->driver.probe = cj_dev_drv_probe;
	drv->driver.remove = cj_dev_drv_remove;
	drv->driver.suspend = cj_dev_drv_suspend;
	drv->driver.resume = cj_dev_drv_resume;
	drv->driver.bus = &cj_bus_type;

	return driver_register(&drv->driver);
}
EXPORT_SYMBOL(register_cj_dev_drv);

void unregister_cj_dev_drv(struct cj_dev_drv *drv) {
	kfree(drv->name);
	drv->name = NULL;

	driver_unregister(&drv->driver);
}
EXPORT_SYMBOL(unregister_cj_dev_drv);

static int __init cj_bus_init(void) {
	/* register our bus type */
	if (bus_register(&cj_bus_type)) {
		printk(KERN_ERR "something went wrong when registering our bus\n");
		return -1;
	}
	
	/* create files for our bus type */
	if (bus_create_file(&cj_bus_type, &bus_attr_hello_file)) {
		printk(KERN_ERR "failed to create a file\n");
		bus_unregister(&cj_bus_type);
		return -1;
	}

	if (device_register(&cj_dev_root)) {
		printk(KERN_ERR "failed to register root of the device tree\n");
		return -1;
	}

	return 0;
}

static void __exit cj_bus_exit(void) {
	bus_unregister(&cj_bus_type);
}

module_init(cj_bus_init);
module_exit(cj_bus_exit);

