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

/* Max write/read index (+ 1) */
#define NUM_OF_REGS 167

/* Flags */
#define WR_FLAG 0
#define RD_FLAG 1
#define NEXT_FLAG 2
#define FIRST_FLAG 3

static void debug_write(unsigned short addr, unsigned short data);
static unsigned short debug_read(unsigned short addr, unsigned short data);
static void trace_write(unsigned short addr, unsigned short data, unsigned char flags);
static void trace_init(void);

struct mdev_trace {
	unsigned short data;
	unsigned char addr;
	unsigned char flags;
};

struct mdev_shadow {
	unsigned short write;
	unsigned short read;
};

static struct mdev_debug {
	/* Trace data */
	struct mdev_trace trace[DEBUG_TRACE_LENGTH];
	
	/* Shadow registers */
	struct mdev_shadow shadow[NUM_OF_REGS];
	
	/* Current trace index */
	unsigned long index;
	
	/* Trace control */
	volatile char cont_sample;
	
	/* Enable the read bypass from the debugger */
	volatile char bypass_read;
} motiondev_debug;

/* Private debug functions */

/* Init trace */
static void trace_init(void)
{
	/* Just assure that the index is 0 */
	/* Don't know exactly how the driver initializes statics */
	motiondev_debug.index = 0;
}

/* Trace write */
static void trace_write(unsigned short addr, unsigned short data, unsigned char flags)
{
	struct mdev_trace *loc;
	
	/* TODO add single shot mode ?? */
	/* Only on continous sample or not first sample*/
	if(motiondev_debug.cont_sample) {
		/* Get current index */
		loc = &motiondev_debug.trace[motiondev_debug.index];
		
		/* Clear the next flag */
		loc->flags &= ~(1u << NEXT_FLAG);
		
		/* Write a sample */
		loc->data = data;
		loc->addr = (unsigned char)(addr & 0xFFu);
		loc->flags |= (1u << flags);

		/* Loop the counter */
		if(motiondev_debug.index < DEBUG_TRACE_LENGTH) {
			motiondev_debug.index++;
		} else {
			motiondev_debug.index = 0u;
		}
		
		/* Get next index */
		loc = &motiondev_debug.trace[motiondev_debug.index];
		
		/* Write the next flag */
		loc->flags |= (1u << NEXT_FLAG);
	}
}

/* Write hook */
static void debug_write(unsigned short addr, unsigned short data)
{
	/* Make a write copy */
	if(addr < NUM_OF_REGS) {
		motiondev_debug.shadow[addr].write = data;
	}
	
	/* Write the trace */
	trace_write(addr, data, WR_FLAG);
}

/* Read hook */
static unsigned short debug_read(unsigned short addr, unsigned short data)
{
	unsigned short val = data;
	
	/* Overwrite if flag requires it */
	if(addr < NUM_OF_REGS) {
		if(motiondev_debug.bypass_read != 0) {
			val = motiondev_debug.shadow[addr].read;
		} else {
			motiondev_debug.shadow[addr].read = val;
		}
	}
	
	/* Write the trace */
	trace_write(addr, val, RD_FLAG);
	
	return val;
}

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
	int ret;
	unsigned short val, addr;
	
	/* Temp buffer */
	unsigned long buf[33];
	
	/* Requests */
	switch(ioctl_num) {
		/* Write data */
		case 0x4620u:
			ret = copy_from_user(buf, (unsigned long *)ioctl_param, 8);
			if(ret == 0) {
				addr = (unsigned short)(buf[0] & 0xFFFFu);
				val = (unsigned short)(buf[1] & 0xFFFFu);
#if (DEBUG_ENABLED != 0)
				debug_write(addr, val);
#endif
				motiondev_lld_write_data(addr, val);	
			}
			break;
		
		/* Read data */
		case 0x4621u:
			ret = copy_from_user(buf, (unsigned long *)ioctl_param, 8);
			if(ret == 0) {
				addr = (unsigned short)(buf[0] & 0xFFFFu);
				val = motiondev_lld_read_data(addr);
#if (DEBUG_ENABLED != 0)
				val = debug_read(addr, val);
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
			ret = -1;
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
	
#if (DEBUG_ENABLED != 0)
	/* Just to be sure */
	trace_init();
#endif	
	
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
