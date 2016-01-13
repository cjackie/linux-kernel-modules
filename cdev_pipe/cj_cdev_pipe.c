/**
 * This is a in-memory buffer device. The size of the buffer
 *  is fixed. Anyone can open it and write data to it, or read 
 *  data from it. Non-Blocking mode is implemented. poll is 
 *  also implemented for I/O multiplexing.
 */


#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/poll.h>

#include <asm/atomic.h>

#include "cj_cdev_pipe.h"

#define cj_isfull(cbuf)  (cbuf->wp == cbuf->rp && cbuf->buffer_size != 0)
#define cj_isempty(cbuf)  (cbuf->wp == cbuf->rp && cbuf->buffer_size == 0) /* can be simplified? */

/**
 * for debuf purposes.
 * @cbuf: a pointer to struct cj_cbuf.
 */
#define print_data_struct(cbuf)						\
do {									\
	printk(KERN_INFO "begin pointer: %p;end pointer: %p\n",		\
	       cbuf->begin, cbuf->end);					\
	printk(KERN_INFO "total_size: %d\n", cbuf->total_size);		\
	char *tmp = kmalloc((cbuf->total_size+1)*sizeof(char), GFP_KERNEL); \
	if (tmp == NULL) {						\
		printk(KERN_INFO "allocation failed?!\n");		\
		break;							\
	}								\
	strncpy(tmp, cbuf->begin, cbuf->total_size);			\
	tmp[cbuf->total_size] = '\0';					\
	printk(KERN_INFO "data: \n------\n%s\n------\n", tmp);		\
	kfree(tmp);							\
} while(0)

/* not used.. */
#define cj_outbound(rwptr, endptr) ({     \
	int __ret = 0;                    \
	if (rwptr > endptr)               \
		__ret = 1;                \
	__ret;                            \
})                                        \

/* maximum number of processes permitted to access our device */
#define MAX_NUM_PROCS 10 
/* our data structure */
static struct cj_cbuf *cj_cbuf;
/* current number of processes that has access to our device  */
static atomic_t cj_nprocs = ATOMIC_INIT(MAX_NUM_PROCS-1);


static int cj_open(struct inode *inode, struct file *filp) 
{
	printk(KERN_INFO "open is invoked\n");

	if (atomic_add_negative(-1, &cj_nprocs)) {
		atomic_inc(&cj_nprocs);
		return -EBUSY;
	}
	
	filp->private_data = cj_cbuf;
	return 0;
}

static int cj_release(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "release is invoked\n");
	atomic_dec(&cj_nprocs);
	return 0;
}

ssize_t cj_read(struct file *filp, char __user *buf, size_t len, loff_t *pos)
{
	printk(KERN_INFO "read is invoked\n");

	struct cj_cbuf *cbuf = filp->private_data;

#ifdef CJ_DEBUG
	print_data_struct(cbuf);
#endif

	down(&cbuf->sem);
	while (cj_isempty(cbuf)) {
		up(&cbuf->sem);

		/* if it is in NON-BLOCKING mode */
		if (filp->f_flags & O_NONBLOCK) {
			return -EAGAIN;
		}

		if (wait_event_interruptible(cbuf->rwait, !cj_isempty(cbuf))) {
			printk(KERN_ERR "interrupted when wait for cond(1)\n");
			return -ERESTARTSYS;
		}
		down(&cbuf->sem);
	}

	int ret = 0;
	int copy_len;
	if (cbuf->rp < cbuf->wp) 
		copy_len = min((int)(cbuf->wp - cbuf->rp), sizeof(char)*len);
	else 
		copy_len = min((int)(cbuf->end - cbuf->rp) + sizeof(char), sizeof(char)*len);

	ret = copy_to_user(buf, cbuf->rp, copy_len);
	if (ret) {
		up(&cbuf->sem);
		printk(KERN_ERR "error on copying data over to the user\n");
		/* invalid exchange */
		return -EBADE;
	}


	cbuf->rp += sizeof(char)*copy_len;
#ifdef CJ_DEBUG
	if (cbuf->rp + sizeof(char) > cbuf->end)
		printk(KERN_ERR "how come? read pointer is over the bound!\n");
#endif
	cbuf->rp = (cbuf->rp > cbuf->end) ? cbuf->begin : cbuf->rp;   	       /* wrap the pointer if needed */
	up(&cbuf->sem);
	wake_up_interruptible(&cbuf->wwait);                                   	/* wake up writing queue */
	return copy_len;
}

static ssize_t cj_write(struct file *filp, const char __user *buf, size_t len, loff_t *pos) 
{
	printk(KERN_INFO "write is invoked\n");

	struct cj_cbuf *cbuf = filp->private_data;

#ifdef CJ_DEBUG
	print_data_struct(cbuf);
#endif

	down(&cbuf->sem);
	while (cj_isfull(cbuf)) {
		up(&cbuf->sem);
		
		/* if it is in NON-BLOCKING mode */
		if (filp->f_flags & O_NONBLOCK) {
			return -EAGAIN;
		}

		if (wait_event_interruptible(cbuf->wwait, !cj_isfull(cbuf))) {
			printk(KERN_ERR "interrupted when wait for cond(2)\n");
			return -ERESTARTSYS;
		}
		down(&cbuf->sem);
	}

	int ret;
	int copy_len;
	if (cbuf->wp < cbuf->rp) 
		copy_len = min((int)(cbuf->rp - cbuf->wp), sizeof(char)*len);
	else
		copy_len = min((int)(cbuf->end - cbuf->wp) + sizeof(char), sizeof(char)*len);
	
	if (copy_from_user(cbuf->wp, buf, copy_len)) {
		up(&cbuf->sem);
		printk(KERN_ERR "failed, copy from user\n");
		return -ERESTART;
	}

	cbuf->wp += sizeof(char)*copy_len;
#ifdef CJ_DEBUG
	if (cbuf->wp > cbuf->end + sizeof(char)) {
		printk(KERN_ERR "how come? write pointer is over the bound!\n");
	}
#endif
	cbuf->wp = (cbuf->wp > cbuf->end) ? cbuf->begin : cbuf->wp;
	wake_up_interruptible(&cbuf->rwait);
	return copy_len;
}

unsigned int cj_poll(struct file *filp, struct poll_table_struct *table)
{
	printk(KERN_INFO "poll is invoked\n");

	struct cj_cbuf *cbuf = filp->private_data;

	unsigned int mask = 0;
	poll_wait(filp, &cbuf->wwait, table); /* poll_wait does not block? */
	poll_wait(filp, &cbuf->rwait, table);  

	down(&cbuf->sem);
	if (!cj_isfull(cbuf)) 
		mask |= POLLIN | POLLRDNORM;
	if (!cj_isempty(cbuf))
		mask |= POLLOUT | POLLWRNORM;
	up(&cbuf->sem);
	return mask;
}

static long cj_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) 
{
	printk(KERN_INFO "ioctl is invoked\n");
	/* TODO */
	return 0;
}

static const struct file_operations cj_cbuf_op = {
	.owner = THIS_MODULE,
	.open = cj_open,
	.read = cj_read,
	.write = cj_write,
	.llseek = no_llseek,
	.poll = cj_poll,
	.release = cj_release,
	.unlocked_ioctl = cj_ioctl,
};

static int __init cdev_setup(void)
{
	printk(KERN_INFO "set up the cdev\n");
	int total_size = 512;

	char *mod_name = "cj_cdev_pipe";               
	dev_t dev;
	int ret;

	ret = alloc_chrdev_region(&dev, 0, 1, mod_name);
	if (ret != 0) {
		printk(KERN_ERR "error code %d when allocating chrdev\n", ret);
		ret = -1;
		goto error;
	}

	// allocate the memory for cj_cbuf
	cj_cbuf = kmalloc(sizeof(struct cj_cbuf), GFP_KERNEL);
	if (!cj_cbuf) {
		printk(KERN_ERR "can not allocate a new cj_cbuf\n");
		ret = -1;
		goto error;
	}

	/* allocate cdev and add it to the kernel */
	cdev_init(&cj_cbuf->cdev, &cj_cbuf_op);
	ret = cdev_add(&cj_cbuf->cdev, dev, 1);
	if (ret) {
		printk(KERN_ERR "can not add the new device\n");
		ret = -1;
		goto error;
	}

	/* init semaphore */
	sema_init(&cj_cbuf->sem, 1);
	
	/* init wait queue */
	init_waitqueue_head(&cj_cbuf->rwait);
	init_waitqueue_head(&cj_cbuf->wwait);

	/* init buffer and rest */
	cj_cbuf->begin = kmalloc(sizeof(char)*total_size, GFP_KERNEL);
	if (!cj_cbuf->begin) {
		printk(KERN_ERR "can not allocate cj_cbuf->begin\n");
		ret = -1;
		goto error;
	}
	cj_cbuf->end = cj_cbuf->begin + sizeof(char)*(total_size-1);
	cj_cbuf->buffer_size = 0;
	cj_cbuf->total_size = total_size;
	cj_cbuf->rp = cj_cbuf->begin;
	cj_cbuf->wp = cj_cbuf->begin;
	cj_cbuf->nreaders = 0;
	cj_cbuf->nwriters = 0;

	return ret;
error:
	return ret;
}

static void __exit cdev_cleanup(void)
{
	kfree(cj_cbuf->begin);
	unregister_chrdev_region(cj_cbuf->cdev.dev, 1);
	kfree(cj_cbuf);
	printk(KERN_INFO "free and exiting\n");
	return;
}

module_init(cdev_setup);
module_exit(cdev_cleanup);









