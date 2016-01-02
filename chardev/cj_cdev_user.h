#ifndef CJ_CDEV_USER_H
#define CJ_CDEV_USER_H

#include <asm/ioctl.h>

#define CJ_CLEANUP_CMD 1
#define CJ_READ_CMD 2

#define CJ_NES -2                           // for ret in pcmd_arg. no enough space

struct pcmd_arg {
	unsigned int li;                    // index of list of cj_list
	char *data;                         // data that elm holds
	long *cdsize;                       // currently data size
	long max_s;                         // maximum size
	long *total_size;                   // total size in the cj_cdev
	int *ret;                           // return value. 0 for success. 
};

// for issuing ioctl
#define CJ_MAGIC 'j'

// buf should be a pointer to pcmd_arg
#define CJ_CLEANUP   _IO(CJ_MAGIC, CJ_CLEANUP_CMD)
#define CJ_READ   _IOR(CJ_MAGIC, CJ_READ_CMD, void*)

#endif /* MY_CDEV_USER_H */
