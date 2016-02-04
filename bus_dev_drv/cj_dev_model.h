#ifndef CJ_DEV_MODEL_H
#define CJ_DEV_MODEL_H

/* kinda like inherent in OOP */
struct cj_dev {
	char *name;
	struct device dev;
};
#define to_cj_dev(dev) container_of(dev, struct cj_dev, dev)

extern struct bus_type cj_bus_type;
extern struct device cj_bus0;

extern int cj_dev_register(struct cj_dev *cj_dev);
extern void cj_dev_unregister(struct cj_dev *cj_dev);

/* kinda like inherent in OOP */
struct cj_dev_drv {
	char *name;
	int (*probe) (struct cj_dev *cj_dev);
	int (*remove) (struct cj_dev *dev);
	int (*suspend) (struct device * dev, pm_message_t state);
	int (*resume) (struct device * dev);
	struct device_driver driver;
};
#define to_cj_dev_drv(dev) container_of(dev, struct cj_dev_drv, driver);

extern int register_cj_dev_drv(struct cj_dev_drv *drv);
extern void unregister_cj_dev_drv(struct cj_dev_drv *drv);


#endif	/* CJ_DEV_MODEL_H */
