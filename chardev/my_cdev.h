#ifndef MY_CDEV_H
#define MY_CDEV_H

struct cj_list {
	struct cj_list *next;    /* pointing to the next element */
	long dsize;              /* max size of the data */
	long cdsize;             /* space that has been filled up */
	char *data;              /* data pointer */
}

struct cj_cdev {
	struct cj_list *head;     /* link list */
	long lsize;               /* number of elements in the list */
	long dsize;               /* max size of the data content in each list */
	struct semaphore sem;     /* mutual exclusion semaphore     */
	struct cdev cdev;         /* Char device structure      */
}

#endif /* MY_CDEV_H */
