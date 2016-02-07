#ifndef CJ_DEV_MODEL_H
#define CJ_DEV_MODEL_H

#include <linux/device.h>

extern struct bus_type cj_bus_type;
extern struct device cj_dev_root;

/* kinda like inherent in OOP */
/**
 * @name, should be unique and will be used for finding matching driver
 * @dev, device
 */
struct cj_dev {
	char *name;
	long id;
	struct device dev;
};
#define to_cj_dev(device) container_of(device, struct cj_dev, dev)

extern int cj_dev_init(struct cj_dev *cj_dev, char *name, long id);
extern int cj_dev_register(struct cj_dev *cj_dev);
extern void cj_dev_unregister(struct cj_dev *cj_dev);


/* kinda like inherent in OOP */

/**
 * @name, should be unique for drivers. For finding matching device.
 * @probe, same probe in device_driver.
 * @remove, same ...
 * @suspend, same ...
 * @resume, same ...
 * @driver, driver
 */
struct cj_dev_drv_op {
	int (*probe) (struct cj_dev *dev);
	int (*remove) (struct cj_dev *dev);
	int (*suspend) (struct cj_dev *dev, pm_message_t state);
	int (*resume) (struct cj_dev *dev);
};
struct cj_dev_drv {
	char *name;
	struct cj_dev_drv_op *op;
	struct device_driver driver;
};
#define to_cj_dev_drv(device_driver) container_of(device_driver, struct cj_dev_drv, driver);

extern int cj_dev_drv_init(struct cj_dev_drv *drv, char *name, struct cj_dev_drv_op *op);
extern int register_cj_dev_drv(struct cj_dev_drv *drv);
extern void unregister_cj_dev_drv(struct cj_dev_drv *drv);

#endif	/* CJ_DEV_MODEL_H */
