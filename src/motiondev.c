#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#include "motiondev.h"
#include "motiondev_lld.h"

/* Febug framework */ 
#if (DEBUG_ENABLED != 0)
#define NUM_OF_REGS 256
static struct mdev_debug {
	unsigned short write[NUM_OF_REGS];
	unsigned short read[NUM_OF_REGS];
	char bypass_read;
} motiondev_debug;
#endif

/* Private data */
static struct cdev *motiondev_cdev;
static dev_t motiondev_major;

/* Private function prototypes */
static int motiondev_open(struct inode *inode, struct file *file);
static int motiondev_release(struct inode *inode, struct file *file);
static int motiondev_ioctl(struct inode *inode, struct file *file, unsigned int ioctl_num, unsigned long ioctl_param);

/* Control structure */
static struct file_operations file_ops = {
	.ioctl = motiondev_ioctl,
	.open = motiondev_open,
	.release = motiondev_release
};

/* Open file */
static int motiondev_open(struct inode *inode, struct file *file)
{
	/* Just for debug */
	printk(KERN_INFO "opening module\n");
	return 0;
}

/* Release file */
static int motiondev_release(struct inode *inode, struct file *file)
{
	/* Just for debug */
	printk(KERN_INFO "releasing module\n");
	return 0;
}

/* IO control access */
static int motiondev_ioctl(struct inode *inode, struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
{
	int ret, ind;
	unsigned short val, addr;
	
	/* Temp buffer */
	unsigned long buf[33];
	
	/* Requests TODO maybe add some error detection ??*/
	switch(ioctl_num) {
		/* Write data */
		case 0x4620u:
			ret = copy_from_user(buf, (unsigned long *)ioctl_param, 8);
			if(ret == 0) {
				addr = (unsigned short)(buf[0] & 0xFFFFu);
				val = (unsigned short)(buf[1] & 0xFFFFu);
#if (DEBUG_ENABLED != 0)
				if(buf[0] < NUM_OF_REGS) {
					motiondev_debug.write[addr] = val;
				}	
#endif
				motiondev_lld_write_data(addr, val);	
			}
			break;
		
		/* Read data */
		case 0x4621u:
			ret = copy_from_user(buf, (unsigned long *)ioctl_param, 8);
			if(ret == 0) {
				addr = (unsigned short)(buf[0] & 0xFFFFu);
#if (DEBUG_ENABLED != 0)
				if(motiondev_debug.bypass_read != 0) {
					val = motiondev_debug.read[addr];
				} else {
					val = motiondev_lld_read_data(addr);
				}
#else
				val = motiondev_lld_read_data(addr);
#endif
				buf[1] = (unsigned long)val;
				ret = copy_to_user((unsigned long *)ioctl_param, buf, 8);
			}
			break;
			
		/* Write motion param */	
		case 0x4622:
			/* Write from 32 to 47*/
			
			/* Write from 64 to 79 */
			
			/* Write 48 with 0*/
			
			/* Read 18 and if not 1 print error ?? */
			break;
			
		default:
			ret = -1;
			break;
	}
	
	return ret;
}

/* Entry point */
static int __init motiondev_init(void)
{
	int ret;
	dev_t dev_no;
	dev_t dev;

	/* Create the device structure */
	motiondev_cdev = cdev_alloc();
	motiondev_cdev->ops = &file_ops;
	motiondev_cdev->owner = THIS_MODULE;
	
	printk(KERN_INFO "module init\n");
	
	/* Allocate region */
	ret = alloc_chrdev_region(&dev_no, 0, 1, DEVICE_FILE_NAME);
	if(ret < 0) {
		printk(KERN_ERR "major number allocation failed\n");
		return ret;
	}
	
	/* Compute the module numbers */
	motiondev_major = MAJOR(dev_no);
	dev = MKDEV(motiondev_major, 0);
	
	printk(KERN_INFO "the major number for the module is %d\n", motiondev_major);
	
	/* Add the device */
	ret = cdev_add(motiondev_cdev, dev, 1);
	if(ret < 0) {
		printk(KERN_ERR "unable to allocate cdev\n");
		return ret;
	}
	
	/* Init the hardware */
	motiondev_lld_init();
	
	/* Ok */
	printk(KERN_INFO "%s initialization finished\n", DEVICE_FILE_NAME);
	return 0;
}

/* Exit point */
static void __exit motiondev_exit(void)
{
	/* Clean relevant stuff */
	printk(KERN_INFO "module exit\n");
	cdev_del(motiondev_cdev);
	unregister_chrdev_region(motiondev_major, 1);
}

/* Entry and exit functions */
module_init(motiondev_init);
module_exit(motiondev_exit);

/* Module info */
MODULE_AUTHOR("Pavel Ionut Catalin");
MODULE_DESCRIPTION("DDCSV1.1 FPGA Motion Control Driver");
MODULE_LICENSE("GPL");
