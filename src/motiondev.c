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

/* Trace flags */
#define WR_FLAG	0x1
#define RD_FLAG 0x2

static void debug_write(unsigned short addr, unsigned short data);
static unsigned short debug_read(unsigned short addr, unsigned short data);
static void trace_write(unsigned short addr, unsigned short data, unsigned char flags);
static void trace_init(void);

/* Trace structure */
struct {
	/* Buffers */
    struct {
        signed long last;
        struct {
            unsigned short data;
            unsigned char addr;
            unsigned char flags;
        } buffer[TRACE_SIZE];
    } data;
    unsigned char reset;
	
	/* Registers */
	struct {
		unsigned short write;
		unsigned short read;
	} regs[NUM_OF_REGS];
	unsigned char bypass;
} motiondev_debug;

/* Private debug functions */

/* Init trace */
static void trace_init(void)
{
	/* Just assure that the index is 0 */
	/* Don't know exactly how the driver initializes statics */
	motiondev_debug.reset = 1;
	motiondev_debug.bypass = 0u;
}

/* Trace write */
static void trace_write(unsigned short addr, unsigned short data, unsigned char flags)
{
	/* Reset the trace */
	if(motiondev_debug.reset) {
        motiondev_debug.data.last = -1;
        motiondev_debug.reset = 0u;
    }
    
	/* Insert into trace */
    if(motiondev_debug.data.last < (TRACE_SIZE - 1)) {
        motiondev_debug.data.last++;
        motiondev_debug.data.buffer[motiondev_debug.data.last].data = data;
        motiondev_debug.data.buffer[motiondev_debug.data.last].addr = (unsigned char)addr;
        motiondev_debug.data.buffer[motiondev_debug.data.last].flags = flags;
    }
}

/* Write hook */
static void debug_write(unsigned short addr, unsigned short data)
{
	/* Make a write copy */
	if(addr < NUM_OF_REGS) {
		motiondev_debug.regs[addr].write = data;
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
		if(motiondev_debug.bypass != 0u) {
			val = motiondev_debug.regs[addr].read;
		} else {
			motiondev_debug.regs[addr].read = val;
		}
	}
	
	/* Write the trace */
	trace_write(addr, val, RD_FLAG);
	
	return val;
}
#endif

/* Private data */
static struct cdev motiondev_cdev;
static struct class *motiondev_class;
static dev_t motiondev_major;

/* Private function prototypes */
static int motiondev_open(struct inode *inode, struct file *file);
static int motiondev_release(struct inode *inode, struct file *file);
static int motiondev_ioctl(struct inode *inode, struct file *file, unsigned int ioctl_num, unsigned long ioctl_param);
static ssize_t motiondev_read(struct file *filp, char *buff, size_t count, loff_t *offp);
static ssize_t motiondev_write(struct file *filp, const char *buff, size_t count, loff_t *offp);

/* Control structure */
static struct file_operations file_ops = {
	.ioctl = motiondev_ioctl,
	.open = motiondev_open,
	.release = motiondev_release,
	.read = motiondev_read,
	.write = motiondev_write
};

/* Read data */
static ssize_t motiondev_read(struct file *filp, char *buff, size_t count, loff_t *offp)
{
	printk("call to read %d", count);
	return count;
}

/* Write data */
static ssize_t motiondev_write(struct file *filp, const char *buff, size_t count, loff_t *offp)
{
	printk("call to write %d", count);
	return count;
}

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
	int result;
	int devno;
	dev_t dev;
	
	
	result = alloc_chrdev_region(&dev, 0, MINOR_DEVICE_NUMBER, DEVICE_FILE_NAME);
	if(result < 0) {
		printk(KERN_ERR "error in allocating device.");
		return -1;
	}
	
	/* Generate number andd init */
	devno = MKDEV(motiondev_major, MINOR_DEVICE_NUMBER);
	cdev_init(&motiondev_cdev, &file_ops);
	motiondev_cdev.owner = THIS_MODULE;
	motiondev_cdev.ops = &file_ops;
	
	/* Add the module */
	result = cdev_add(&motiondev_cdev, devno, MINOR_DEVICE_NUMBER);
	if(result) {
		printk (KERN_NOTICE "couldn't add cdev.");
	}

	/* Create the class */
	motiondev_class = class_create(THIS_MODULE, DEVICE_FILE_NAME);
	if(motiondev_class == NULL) {
		unregister_chrdev_region(dev, 1);
		return -1;
	} 
	
	/* Create the device */
	if(device_create(motiondev_class, NULL, dev, NULL, DEVICE_FILE_NAME) == NULL) {
		class_destroy(motiondev_class);
		unregister_chrdev_region(dev, 1);
		return -1;
	}
	
	/* Print the major number */
	motiondev_major = MAJOR(dev);
	printk(KERN_INFO "kernel assigned major number is %d ..\r\n", motiondev_major);

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
	/** A reverse - destroy mechansim -- the way it was created **/
	printk(KERN_INFO "%s begin cleanup.",  DEVICE_FILE_NAME);
	
	/* Delete the character driver */
	cdev_del(&motiondev_cdev);
	
	/* Destroy the device */
	device_destroy(motiondev_class, MKDEV(motiondev_major, 0));
	
	/* Destroy the class */
	class_destroy(motiondev_class);
	
	/* Unregister the driver */
	unregister_chrdev(motiondev_major, MINOR_DEVICE_NUMBER);
}

/* Entry and exit functions */
module_init(motiondev_init);
module_exit(motiondev_exit);

/* Module info */
MODULE_AUTHOR("Pavel Ionut Catalin");
MODULE_DESCRIPTION("DDCSV1.1 FPGA Motion Control Driver");
MODULE_LICENSE("GPL");
