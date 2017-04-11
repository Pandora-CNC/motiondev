#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>

#include "motiondev.h"
#include "motiondev_lld.h"

/* Private function prototypes */
static int motiondev_open(struct inode *inode, struct file *file);
static int motiondev_close(struct inode *inode, struct file *file);
static int motiondev_ioctl(struct inode *inode, struct file *file,	unsigned int ioctl_num, unsigned long ioctl_param);

/* Control structure */
static struct file_operations file_ops = {
	.ioctl = motiondev_ioctl,
	.open = motiondev_open,
	.release = motiondev_close
};

/* Open file */
static int motiondev_open(struct inode *inode, struct file *file)
{

}

/* Close file */
static int motiondev_close(struct inode *inode, struct file *file)
{

}

/* IO control access */
static int motiondev_ioctl(struct inode *inode, struct file *file,	unsigned int ioctl_num, unsigned long ioctl_param)
{

}

/* Entry point */
static void motiondev_init(void)
{
	int res;
	
	/* Register the device */
	res = register_chrdev(MAJOR_NUM, DEVICE_NAME, &file_ops);
	
	if (res < 0) {
		printk(KERN_ALERT "Failed with error code %d", res);
		return res;
	}
	
	/* Ok */
	return 0;
}

/* Exit point */
static void motiondev_exit(void)
{
	int res;

	/* Unregister the device */
	res = unregister_chrdev(MAJOR_NUM, DEVICE_NAME);

	if (res < 0) {
		printk(KERN_ALERT "Failed with error code %d", res);
	}
}

/* Entry and exit functions */
module_init(motiondev_init);
module_exit(motiondev_exit);

/* Module info */
MODULE_AUTHOR("Pavel Ionut - Catalin");
MODULE_DESCRIPTION("DDCSV1.1 FPGA Motion Control Driver");
MODULE_LICENSE("GPL");