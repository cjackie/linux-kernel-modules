#ifndef CJ_CDEV_USER_H
#define CJ_CDEV_USER_H

#include <asm/ioctl.h>

struct pcmd_arg {
	unsigned int li;                    // index of list of cj_list
	char *data;                         // data that elm holds
	long *cdsize;                       // currently data size
	long max_s;                         // maximum size
	long *total_size;                   // total size in the cj_cdev
	int *ret;                           // return value. 0 for success. 
};

// buf should be a pointer to pcmd_arg
#define CJ_CLEANUP   _IO('j', 1)
#define CJ_READ   _IOR('j', 2, void*)

#endif /* MY_CDEV_USER_H */
