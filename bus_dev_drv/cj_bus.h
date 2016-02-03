#ifndef CJ_BUS_H
#define CJ_BUS_H

struct cj_bus_dev {
	char *name;
	struct device dev;
};

struct cj_bus_drv {
};

/* some exporting functions go here */
/* TODO */

#define to_cj_bus(dev) container_of(dev, struct cj_bus, dev)


#endif	/* CJ_BUS_H */
