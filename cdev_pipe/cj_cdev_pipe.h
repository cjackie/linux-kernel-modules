#ifndef CJ_CDEV_PIPE_H
#define CJ_CDEV_PIPE_H

/* for debug */
#define CJ_DEBUG 

/* circular buffer */
struct cj_cbuf {
	char *begin, *end;           /* begin and end of the buffer */
	int buffer_size;             /* current size of the buffer */
	int total_size;              /* total size of the buffer */
	char *rp, *wp;               /* read pointer and write pointer */
	int nreaders, nwriters;      /* number of readers and number of writers */
	wait_queue_head_t rwait;     /* wait queue for readers */
	wait_queue_head_t wwait;     /* wait queue for writers */
	struct semaphore sem;        /* semaphore */
	struct cdev cdev;            /* cdev */
};


#endif /* CJ_CDEV_PIPE_H */
