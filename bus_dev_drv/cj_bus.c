#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/stat.h>


static int cj_bus_match(struct device *dev, struct device_driver *drv) {
	/* don't know how to check. */
	/* according to kernel doc, we need to check if dev and drv
	 *  can be handle by this bus or not?
	 */
	return 1;
}

static struct bus_type cj_bus_type = {
	.name = "cj_bus",
	.match = cj_bus_match
};

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

/* cj_bus_dev */
/* TODO */

/* cj_bus_drv */
/* TODO */

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

	/* TODO */
}

static void __exit cj_bus_exit(void) {
	/* TODO */
}


module_init(cj_bus_init);
module_exit(cj_bus_exit);

