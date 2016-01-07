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

#define D_TMP 

static struct cj_cdev *my_cj_cdev;
static const char *mod_name = "cj_cdev";

static int d_open(struct inode *inode, struct file *filp) 
{
	printk(KERN_INFO "open is invoked\n");

	int ret = 0;
	filp->private_data = container_of(inode->i_cdev, struct cj_cdev, cdev);
	if (!filp) {
		printk(KERN_ERR "error when finding cdev?\n");
		ret = -1;
	}

#ifdef CJ_DEBUG
	printk(KERN_INFO "cj_cdev initail_info:\n");
	printk(KERN_INFO "head: %p, dsize: %ld, cdsize: %ld\n",
	       my_cj_cdev->head, my_cj_cdev->dsize, my_cj_cdev->total_size);
	printk(KERN_INFO "cj_list: \n----\n next:%p\n dsize:%ld\n cdsize:%ld\n data:%s\n----\n",
	       my_cj_cdev->head->next, my_cj_cdev->head->dsize, my_cj_cdev->head->cdsize, 
	       my_cj_cdev->head->data);
#endif
	return ret;
}

static int d_release(struct inode *inode, struct file *filp)
{
	// Nothing need to be done here
	printk(KERN_INFO "release is invoked\n");
	return 0;
}

static loff_t d_llseek(struct file *filp, loff_t pos, int whence) {
	printk(KERN_INFO "llseek is invoked\n");

	struct cj_cdev *cj_cdev_l = filp->private_data;
	long total_size = cj_cdev_l->total_size;
	long f_pos = pos + whence;
	int ret = 0;
	if (total_size < f_pos) {
		filp->f_pos = f_pos;
	} else {
		printk(KERN_ERR "seeking a invalid position\n");
		ret = -1;
	}

	return ret;
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
	int ret = 0;
	// lock the sema
	if (down_interruptible(&cj_cdev_l->sem)) {
		printk(KERN_ERR "interrupted when acquiring the lock(read)!\n");
		ret = -1;
		goto out2;
	}

	// go to the first elm in the list where it can be read
	while (!lptr && relative_pos >= dsize) {
		remain_space -= dsize;
		relative_pos -= dsize;
		lptr = lptr->next;
#ifdef D_TMP
		printk("dsize : %li\n", dsize);
		n_read = 0;
		goto out;
#endif
	}

	// go to the relative position in that elm
	remain_space -= (relative_pos+1);
	remain_space = remain_space > 0 ? remain_space : 0;
	relative_pos = 0;
	
	// copy all variable data to the user. one element at a time
	while (len && remain_space) {
		// copy the data to the user space
		long chuck_len = lptr->cdsize-relative_pos < len ? lptr->cdsize-relative_pos : len;
		if (copy_to_user(buf, lptr->data + sizeof(char *)*relative_pos, chuck_len)) {
			printk(KERN_ERR "error when copying to user\n");
			n_read = -1;
			goto out;
		}
		n_read += chuck_len;
		remain_space -= chuck_len;
		len -= chuck_len;
		relative_pos = chuck_len + relative_pos >= dsize ? 0 : chuck_len + relative_pos;
		lptr = relative_pos == 0 ? lptr->next : lptr;
#ifdef DEBUG
		if (len < 0 || remain_space < 0) {
			printk(KERN_ERR "len or remain_space is invalid. len is %li, remain_space is %li\n",
			       len, remain_space);
			n_read = -1;
			goto out;
		}
#endif
	}
	
out:
	if (n_read > 0) 
		*pos += n_read;
	up(&cj_cdev_l->sem);
	return n_read;

out2:
	return ret;
}

static ssize_t d_write(struct file *filp, const char __user *buf, size_t len, loff_t *pos)
{
	printk(KERN_INFO "write is invoked\n");
	
	// copy the data from user space
	char *user_data = kmalloc(sizeof(char)*len, GFP_KERNEL);
	unsigned long n_write = 0;
	int ret = 0;

	if (!user_data) {
		printk(KERN_ERR "no enough space for allocating %d amount\n", (int)len);
		ret = -1;
		goto out2;
	}

	if (copy_from_user(user_data, buf, len)) {
		printk(KERN_ERR "error when getting the data from the user");
		ret = -1;
		goto out2;
	}

	struct cj_cdev *cj_cdev_l = filp->private_data;
	long total_size = cj_cdev_l->total_size;
	long dsize = cj_cdev_l->dsize;

	// allocate extra space if needed
	long n_elm_now = total_size/dsize + 1;
	long n_elm_needed = (*pos + len) / dsize + 1;
	struct cj_list **alloc_elms;
	long alloc_len = 0;
	if (n_elm_needed - n_elm_now > 0) {
		int alloc_total = n_elm_needed - n_elm_now;
		alloc_elms = kmalloc(sizeof(struct cj_list *)*alloc_total, GFP_KERNEL);
		if (!alloc_elms) {
			printk(KERN_ERR "no space for allocating array of cj_list*\n");
			ret = -1;
			goto out3;
		}
		int i;
		char *data_tmp;
		for (i = 0; i < alloc_total; ++i) {
			alloc_elms[i] = kmalloc(sizeof(struct cj_list), GFP_KERNEL);
			if (!alloc_elms[i]) {
				printk(KERN_ERR "no space for new cj_list\n");
				ret = -1;
				goto out3;
			}
			data_tmp = kmalloc(sizeof(char)*dsize, GFP_KERNEL);
			if (!data_tmp) {
				kfree(alloc_elms[i]);
				printk(KERN_ERR "no space for data\n");
				ret = -1;
				goto out3;
			}
			alloc_elms[i]->data = data_tmp;
			++alloc_len;
		}
	}
	

	// get the first ptr that can be written
	struct cj_list *lptr = cj_cdev_l->head;
	unsigned long cur_pos = *pos;
	while (cur_pos >= dsize) {
		lptr = lptr->next;
		cur_pos -= dsize;
	}
	
	// lock the sema
	if (down_interruptible(&cj_cdev_l->sem)) {
		printk(KERN_ERR "interrupted when acquiring the lock(write)\n");
		ret = -1;
		goto out3;
	}

	// copying a char at a time to kernel space memory
	unsigned long offset = 0;
	unsigned advance_cursor = 0;
	unsigned long elm_avail_i = 0;
	while (len) {
		// if end of the current chuck
		if (cur_pos == dsize) {
			if (!lptr->next) {
				// It is end of the buffer. introduce a new elm
				if (elm_avail_i >= alloc_len) {
					printk(KERN_ERR "what!?, need more allocated cj_list?\n");
					goto out3;
				}
				struct cj_list *new_elm = alloc_elms[elm_avail_i++];
				new_elm->next = NULL;
				new_elm->dsize = dsize;
				new_elm->cdsize = 0;
				memset(new_elm->data, 0, dsize);

				lptr->next = new_elm;
				lptr = new_elm;
			} else {
				lptr = lptr->next;
			}
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
	
	// exiting
	kfree(user_data);
	*pos += n_write;
	cj_cdev_l->total_size += advance_cursor;
	up(&cj_cdev_l->sem);
	return n_write;

out2:
	kfree(user_data);
	return ret;

out3:
	kfree(user_data);
	int i;
	for (i = 0; i < alloc_len; ++i) {
		kfree(alloc_elms[i]->data);
		kfree(alloc_elms[i]);
	}
	return ret;
}

static long d_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	printk(KERN_INFO "ioctl is invoked\n");

	int ret = 0;
	struct cj_cdev *cj_cdev_l = filp->private_data;
	struct cj_list *lptr = NULL;
	struct cj_list *next = NULL;
	struct pcmd_arg *arg_ptr = NULL;
	int total_i = 0;
	long data_size = 0;
	int index = 0;
	switch (cmd) {
	case CJ_CLEANUP:
		// clean up the buffer in memory, and restore to the initial state
		lptr = cj_cdev_l->head->next;
		while (lptr) {
			kfree(lptr->data);
			next = lptr->next;
			kfree(lptr);
			lptr = next;
		}

		cj_cdev_l->head->cdsize = 0;
		cj_cdev_l->head->next = NULL;
		memset(cj_cdev_l->head->data, 0, cj_cdev_l->dsize);
		cj_cdev_l->total_size = 0;

		filp->f_pos = 0;  /* reset the file position */
		break;

	case CJ_READ:
#ifdef CJ_DEBUG
		printk(KERN_INFO "cj_cdev initail_info:\n");
		printk(KERN_INFO "head: %p, dsize: %ld, cdsize: %ld\n",
		       my_cj_cdev->head, my_cj_cdev->dsize, my_cj_cdev->total_size);
		printk(KERN_INFO "cj_list: \n----\n next:%p\n dsize:%ld\n cdsize:%ld\n data:%s\n----\n",
		       my_cj_cdev->head->next, my_cj_cdev->head->dsize, my_cj_cdev->head->cdsize, 
		       my_cj_cdev->head->data);
#endif

		arg_ptr = kmalloc(sizeof(struct pcmd_arg), GFP_KERNEL);
		if (copy_from_user(arg_ptr, (void *)arg, sizeof(struct pcmd_arg))) {
			printk(KERN_ERR "can not copy pcmd_arg from the user space\n");
			ret = -1;
			goto err;
		}
		
		total_i = cj_cdev_l->total_size / cj_cdev_l->dsize;
		total_i += (cj_cdev_l->total_size % cj_cdev_l->dsize > 0) ? 1 : 0;
		if (arg_ptr->li < total_i) {
			// goto the cj_list at li
			struct cj_list *lptr = cj_cdev_l->head;
			index = arg_ptr->li;
			while (index--)
				lptr = lptr->next;
			
			data_size = (arg_ptr->max_s < lptr->cdsize) ? arg_ptr->max_s : lptr->cdsize;
			// copy the info to user space
			ret = 0;
			if (copy_to_user(arg_ptr->data, lptr->data, sizeof(char)*data_size) || 
			    copy_to_user(arg_ptr->cdsize, &lptr->cdsize, sizeof(long)) ||
			    copy_to_user(arg_ptr->total_size, &cj_cdev_l->total_size, sizeof(long)) ||
			    copy_to_user(arg_ptr->ret, &ret, sizeof(int))) {
				printk(KERN_ERR "error when copying over data to userspace\n");
				ret = -1;
				goto err;
			}

			if (copy_to_user(arg_ptr->ret, &ret, sizeof(int))) {
				printk(KERN_ERR "error when copying ret value\n");
				ret = -1;
				goto err;
			}
			
			kfree(arg_ptr);
			arg_ptr = NULL;
			ret = 0;
		}
		break;

	default:
		ret = -1;
		break;
	}

	return ret;

err:
	kfree(arg_ptr);
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
		ret = -1;
		goto error;
	}

	// allocate the memory for cj_cdev
	my_cj_cdev = kmalloc(sizeof(struct cj_cdev), GFP_KERNEL);
	if (!my_cj_cdev) {
		printk(KERN_ERR "can not allocate a new cdev\n");
		ret = -1;
		goto error;
	}

	// allocate cdev and add it to the kernel
	my_cdev = &my_cj_cdev->cdev;
	cdev_init(my_cdev, &cj_cdev_op);
	ret = cdev_add(my_cdev, dev, 1);
	if (ret) {
		printk(KERN_ERR "can not add the new device\n");
		ret = -1;
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
		ret = -1;
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
	return ret;
}

static void __exit my_cdev_cleanup(void)
{
	struct cj_list *lptr;
	struct cj_list *next;
	for (lptr = my_cj_cdev->head; lptr; lptr = next) {
		kfree(lptr->data);
		next = lptr->next;
		kfree(lptr);
	}
	unregister_chrdev_region(my_cj_cdev->cdev.dev, 1);
	kfree(my_cj_cdev);
	printk(KERN_INFO "free and exiting\n");
	return;
}

module_init(my_cdev_setup);
module_exit(my_cdev_cleanup);
