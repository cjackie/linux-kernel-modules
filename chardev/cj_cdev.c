#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/moduleparam.h>
#include <linux/syscalls.h>
#include "cj_cdev.h"
#include "cj_cdev_user.h"

MODULE_LICENSE("MIT");
MODULE_AUTHOR("Chaojie Wang");
MODULE_DESCRIPTION("playing around with kernel");

static struct cj_cdev *my_cj_cdev;
static const char *mod_name = "cj_cdev";

static int d_open(struct inode *inode, struct file *filp) 
{
	printk(KERN_INFO "open is invoked\n");

	filp->private_data = container_of(inode->i_cdev, struct cj_cdev, cdev);
	if (!filp) {
		printk(KERN_ERR "error when finding cdev?\n");
		return -1;
	}
	return 0;
}

static int d_release(struct inode *inode, struct file *filp)
{
	// Nothing need to be done
	printk(KERN_INFO "release is invoked\n");
	return 0;
}

static loff_t d_llseek(struct file *filp, loff_t pos, int whence) 
{
	printk(KERN_INFO "llseek is invoked\n");

	struct cj_cdev *cj_cdev_l = filp->private_data;
	long total_size = cj_cdev_l->total_size;
	long f_pos = (long) pos + whence;
	if (total_size < f_pos) {
		filp->f_pos = f_pos;
	} else {
		printk(KERN_ERR "seeking a invalid position\n");
		return -1;
	}

	return 0;
}

static ssize_t d_read(struct file *filp, char __user *buf, size_t len, loff_t *pos)
{
	printk(KERN_INFO "read is invoked\n");
	
	struct cj_cdev *cj_cdev_l = filp->private_data;
	long dsize = cj_cdev_l->dsize;
	long relative_pos = *pos;
	long remain_space = cj_cdev_l->total_size;
	unsigned long n_read = 0;
	struct cj_list *lptr = cj_cdev_l->head;
	// lock the sema
	if (down_interruptible(&cj_cdev_l->sem)) {
		printk(KERN_ERR "interrupted when acquiring the lock(read)!\n");
		return -1;
	}
	// go to the first elm in the list where it can be read
	while (!lptr && relative_pos >= dsize) {
		remain_space -= lptr->cdsize;
		relative_pos -= lptr->cdsize;
		lptr = lptr->next;
	}
	
	// copy all variable data to the user. one element at a time
	while (len && remain_space) {
		// copy the data to the user space
		long chuck_len = dsize-relative_pos > len ? dsize-relative_pos : len;
		if (copy_to_user(buf, lptr->data + sizeof(char *)*relative_pos, chuck_len)) {
			printk(KERN_ERR "error when copying to user\n");
			n_read = -1;
			goto out;
		}
		remain_space -= chuck_len;
		len -= chuck_len;
		relative_pos = 0;
		lptr = lptr->next;
	}
	
out:
	if (n_read > 0) 
		*pos += n_read;
	up(&cj_cdev_l->sem);
	return n_read;
}

static ssize_t d_write(struct file *filp, const char __user *buf, size_t len, loff_t *pos)
{
	printk(KERN_INFO "write is invoked\n");
	
	// copy the data from user space
	char *user_data = kmalloc(sizeof(char *)*len, GFP_KERNEL);
	unsigned n_write = 0;
	if (!user_data) {
		printk(KERN_ERR "no enough space for allocating %d amount\n", (int)len);
		goto out;
	}

	if (copy_from_user(user_data, buf, len)) {
		printk(KERN_ERR "error when getting the data from the user");
		goto out;
	}

	// get the first ptr that can be written
	struct cj_cdev *cj_cdev_l = filp->private_data;
	long total_size = cj_cdev_l->total_size;
	long dsize = cj_cdev_l->dsize;
	struct cj_list *lptr = cj_cdev_l->head;
	unsigned long cur_pos = *pos;
	while (cur_pos >= dsize) {
		lptr = lptr->next;
		cur_pos -= dsize;
	}
	
	// lock the sema
	if (down_interruptible(&cj_cdev_l->sem)) {
		printk(KERN_ERR "interrupted when acquiring the lock(write)\n");
		return -1;
	}

	// copying a char at a time to our memory
	unsigned long offset = 0;
	unsigned advance_cursor = 0;
	while (len) {
		// if end of the current chuck
		if (cur_pos == dsize) {
			struct cj_list *new_elm = NULL;
			new_elm = kmalloc(sizeof(struct cj_list), GFP_KERNEL);
			if (!new_elm) {
				printk(KERN_ERR "can't allocate new cj_list\n");
				goto out;
			}
			memset(new_elm, 0, sizeof(struct cj_list));
			new_elm->dsize = dsize;
			new_elm->next = NULL;
			lptr->next = new_elm;
			lptr = new_elm;
			cur_pos = 0;
		}
		// copy over data, one char at a time
		lptr->data[cur_pos] = user_data[offset];
		if (cur_pos == lptr->cdsize) {
			lptr->cdsize += 1;
			++advance_cursor;
		}
		++cur_pos;
		++offset;
		++n_write;
		--len;
	}

out:
	kfree(user_data);
	*pos += n_write;
	cj_cdev_l->total_size += advance_cursor;
	up(&cj_cdev_l->sem);
	return n_write;
}

static long d_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	printk(KERN_INFO "ioctl is invoked\n");

	int ret = 0;
	struct cj_cdev *cj_cdev_l = filp->private_data;
	struct cj_list *lptr = NULL;
	struct cj_list *next = NULL;
	switch (cmd) {
	case CLEANUP_CMD:
		// clean up the buffer in memory, and restore to the initial state
		for (lptr = cj_cdev_l->head->next; !lptr; lptr = next) {
			kfree(lptr->data);
			next = lptr->next;
			kfree(lptr);
		}
		cj_cdev_l->head->cdsize = 0;
		cj_cdev_l->head->next = NULL;
		memset(cj_cdev_l->head->data, 0, cj_cdev_l->dsize);
		cj_cdev_l->total_size = 0;
		break;
	case PRINT_CMD:
		// print relevent data to a console
		// TODO write to filp?
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}

static const struct file_operations cj_cdev_op = {
	.owner = THIS_MODULE,
	.llseek = d_llseek,
	.open = d_open,
	.read = d_read,
	.write = d_write,
	.release = d_release,
	.unlocked_ioctl = d_ioctl,
};


static int __init my_cdev_setup(void)
{
	printk(KERN_INFO "set up the my cdev\n");
	// obtain a major number and a minor number
	dev_t dev;
	int ret;
	struct cdev *my_cdev = NULL;
	int dsize;
	char *cj_list_data = NULL;

	if ((ret = alloc_chrdev_region(&dev, 0, 1, mod_name)) != 0) {
		printk(KERN_ERR "error code %d when allocating chrdev\n", ret);
		goto error;
	}

	// allocate the memory for cj_cdev
	my_cj_cdev = kmalloc(sizeof(struct cj_cdev), GFP_KERNEL);
	if (!my_cj_cdev) {
		printk(KERN_ERR "can not allocate a new cdev\n");
		goto error;
	}

	// allocate cdev and add it to the kernel
	my_cdev = &my_cj_cdev->cdev;
	cdev_init(my_cdev, &cj_cdev_op);
	ret = cdev_add(my_cdev, dev, 1);
	if (ret) {
		printk(KERN_ERR "can not add the new device\n");
		goto error;
	}

	// init semaphore
	sema_init(&my_cj_cdev->sem, 1);
	
	// allocate data in the cj_cdev
	dsize = 512;
	my_cj_cdev->head = kmalloc(sizeof(struct cj_list), GFP_KERNEL);
	cj_list_data = kmalloc(sizeof(char)*dsize, GFP_KERNEL);
	if (!my_cj_cdev->head || !cj_list_data) {
		printk(KERN_ERR "insufficient memory for allocation\n");
		goto error;
	}
	memset(cj_list_data, 0, dsize);
	my_cj_cdev->dsize = dsize;
	my_cj_cdev->total_size = 0;
	my_cj_cdev->head->next = NULL;
	my_cj_cdev->head->dsize = dsize;
	my_cj_cdev->head->cdsize = 0;
	my_cj_cdev->head->data = cj_list_data;
	
	return ret;
error:
	return -1;
}

static void __exit my_cdev_cleanup(void)
{
	struct cj_list *l_ptr;
	struct cj_list *next;
	for (l_ptr = my_cj_cdev->head; !l_ptr; l_ptr = next) {
		kfree(l_ptr->data);
		next = l_ptr->next;
		kfree(l_ptr);
	}
	unregister_chrdev_region(my_cj_cdev->cdev.dev, 1);
	kfree(my_cj_cdev);
	printk(KERN_INFO "free and exiting\n");
	return;
}

module_init(my_cdev_setup);
module_exit(my_cdev_cleanup);
