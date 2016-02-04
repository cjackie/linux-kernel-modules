#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/stat.h>
#include <linux/slab.h>

#include "cj_dev_model.h"


static int cj_bus_match(struct device *dev, struct device_driver *drv) {
	/* don't know what to check. */
	/* according to kernel doc, we need to check if dev matches with drv
	 * or not. 
	 */
	printk(KERN_INFO "cj_bus_match is invoked\n");
	
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
	.match = cj_bus_match
};
EXPORT_SYMBOL(cj_bus_type);

/* an attribute */
#define cj_bus_attr_store_data_size = 1024;
static char cj_bus_attr_store_data[cj_bus_attr_store_data_size];
static ssize_t cj_bus_show(struct bus_type *bus, char *buf) {
	ssize_t wrt_n = snprintf(buf, PAGE_SIZE, "hello from the cj_bus. extra info: %s", 
		 cj_bus_attr_store_data);

	return wrt_n;
}

static ssize_t cj_bus_store(struct bus_type *bus, const char *buf, size_t count){
	memset(cj_bus_attr_store_data, 0, cj_bus_attr_store_data_size);
	ssize_t rd_n = snprintf(buf, min(cj_bus_attr_store_data_size-1, count), "%s", buf);
	return rd_n;
}

/*
 * cj_bus_type attributes. the name will be bus_attr_hello_file created by 
 * this macro.
 */
static BUS_ATTR("hello_file", 0666, cj_bus_show, cj_bus_store);


/* bus device */
struct device cj_bus0 = {
	.parent = NULL,
	.bus = &cj_bus_type,
	.init_name = "cj_bus0"
};
EXPORT_SYMBOL(cj_bus0);
		

/**
 *  register a device under cj_bus0.
 *  @cj_dev, device
 *
 *  return 0 upon success? 
 */
int cj_dev_register(struct cj_dev *cj_dev) {
	printk("registering a new bus, with id: %d\n", bus.id);
	
	/* according to Linux/Documentation/driver-model/device.txt */
	/* set up fields */
	char *name = kmalloc(sizeof(char)*(strlen(cj_dev->name)+1), GFP_KERNEL);
	if (name == NULL) {
		return -1;
	}
	strcpy(name, cj_dev->name);

	cj_dev->dev.parent = &cj_bus0;
	cj_dev->dev.bus = &cj_bus_type;
	cj_dev->dev.init_name = name;
	return device_register(cj_dev->dev);
}
EXPORT_SYMBOL(cj_dev_register);

/**
 *  unregister a device under cj_bus0.
 */
void cj_dev_unregister(struct cj_dev *cj_dev) {
	kfree(cj_dev->name);
	cj_dev->name = NULL;
	
	device_unregister(cj_dev->dev);
}
EXPORT_SYMBOL(cj_dev_unregister);

int register_cj_dev_drv(struct cj_dev_drv drv) {
	if (drv == NULL) {
		printk(KERN_ERR "drv is NULL\n");
		return -1;
	}

	char *name = kmalloc(sizeof(char)*(strlen(drv.name)+1), GFP_KERNEL);
	
	drv->driver.name = name;
	drv->driver.probe = drv->probe;
	drv->driver.remove = drv->remove;
	drv->driver.suspend = drv->suspend;
	drv->driver.resume = drv->resume;
	drv->driver.bus = &cj_bus0;

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

	return 0;
}

static void __exit cj_bus_exit(void) {
	bus_unregister(&cj_bus_type);
}

module_init(cj_bus_init);
module_exit(cj_bus_exit);

