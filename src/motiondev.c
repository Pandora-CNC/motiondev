#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#include "motiondev.h"
#include "motiondev_lld.h"

/* Placeholder functions */
static void data_write(unsigned short addr, unsigned short data);
static unsigned data_read(unsigned short addr);

/* Febug framework */ 
#if (DEBUG_ENABLED != 0)

/* Max write/read index (+ 1) */
#define NUM_OF_REGS 167

/* Trace flags */
#define WR_FLAG	0x1
#define RD_FLAG 0x2

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
	/* Just assure correct init */
	motiondev_debug.reset = 1u;
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
#endif

/* Data write */
static void data_write(unsigned short addr, unsigned short data)
{
	/* Write the data */
	motiondev_lld_write_data(addr, data);
	
#if (DEBUG_ENABLED != 0)
	/* Write the trace */
	trace_write(addr, data, WR_FLAG);

	/* Make a write copy */
	if(addr < NUM_OF_REGS) {
		motiondev_debug.regs[addr].write = data;
	}
#endif
}

/* Data read */
static unsigned data_read(unsigned short addr)
{
	unsigned short val;
	
	/* Read data from chip */
	val = motiondev_lld_read_data(addr);
	
#if (DEBUG_ENABLED != 0)	
	/* Write the trace */
	trace_write(addr, val, RD_FLAG);
	
	/* Overwrite if flag requires it */
	if(addr < NUM_OF_REGS) {
		if(motiondev_debug.bypass != 0u) {
			val = motiondev_debug.regs[addr].read;
		} else {
			motiondev_debug.regs[addr].read = val;
		}
	}
#endif

	return val;
}

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
	int ind;
	unsigned char n;

	/* Temp buffer */
	unsigned short buf[512];
	
	/* Temp pointer */
	unsigned short *pbuf;
	
	/* Requests */
	switch(ioctl_num) {
		/* Write data */
		case 0x4620u:
			ret = copy_from_user(buf, (unsigned long *)ioctl_param, 8);
			if(ret == 0) {
				/* Write the data */
				data_write(buf[0], buf[2]);
			}
			break;
		
		/* Read data */
		case 0x4621u:
			ret = copy_from_user(buf, (unsigned long *)ioctl_param, 8);
			if(ret == 0) {
				/* Just read the data and write the buffer back */
				buf[2] = data_read(buf[0]);
				ret = copy_to_user((unsigned long *)ioctl_param, buf, 8);
			}
			break;
			
		/* Write motion param */	
		case 0x4622:
			ret = copy_from_user(buf, (unsigned long *)ioctl_param, 1024);
			for(ind = 0; ind < 32; ind++) {
				/* Get the pointer */
				pbuf = &buf[ind * 16];
				
				/* Multiple tries ?? */
				do {
					for(n = 32u; n < 65u; n <<= 2) {
						/* Write from 32 to 47*/
						data_write(n, pbuf[0]);
						data_write(n + 1u, pbuf[1]);
						data_write(n + 2u, pbuf[4]);
						data_write(n + 3u, pbuf[5]);
						data_write(n + 4u, pbuf[6]);
						data_write(n + 5u, pbuf[7]);
						data_write(n + 6u, pbuf[8]);
						data_write(n + 7u, pbuf[9]);
						data_write(n + 8u, pbuf[2]);
						data_write(n + 9u, pbuf[3]);
						data_write(n + 10u, pbuf[10]);
						data_write(n + 11u, pbuf[11]);
						data_write(n + 12u, pbuf[12]);
						data_write(n + 13u, pbuf[13]);
						data_write(n + 14u, pbuf[14]);
						data_write(n + 15u, pbuf[15]);
					    }
				} while(data_read(18u)); /* Write check ?? */
				
				/* Latch ?? */
				data_write(48u, 1u);
				data_write(48u, 0u);
			}
			break;
			
		default:
			/* Case 0x4622 not used ?? */
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
	dev_t dev_no;
	
	/* Allocate driver region */
	result = alloc_chrdev_region(&dev_no, 0, MINOR_DEVICE_NUMBER, DEVICE_FILE_NAME);
	if(result < 0) {
		printk(KERN_ERR "error in allocating device.");
		return -1;
	}
	
    /* Write the major number */
    motiondev_major = MAJOR(dev_no);
    dev_no = MKDEV(motiondev_major, MINOR_DEVICE_NUMBER)
    
	/* Init */
	cdev_init(&motiondev_cdev, &file_ops);
	
	/* Add the module */
	result = cdev_add(&motiondev_cdev, devno, MINOR_DEVICE_NUMBER);
	if(result) {
		printk (KERN_NOTICE "couldn't add cdev.");
        return -1;
	}

	/* Create the class */
	motiondev_class = class_create(THIS_MODULE, "chardrv");
	if(motiondev_class == NULL) {
		unregister_chrdev_region(dev, 1);
		return -1;
	} 
	
	/* Create the device */
	if(device_create(motiondev_class, NULL, motiondev_cdev, NULL, DEVICE_FILE_NAME) == NULL) {
		class_destroy(motiondev_class);
		unregister_chrdev_region(dev, 1);
		return -1;
	}
	
	/* Print the major number */
	printk(KERN_INFO "kernel assigned major number is %d ..\r\n", motiondev_major);

#if (DEBUG_ENABLED != 0)
	/* Just to be sure */
	trace_init();
#endif	

	/* Init the hardware */
	motiondev_lld_init();
	
	/* Ok */
	printk(KERN_INFO "%s initialization finished\n", DEVICE_FILE_NAME);
	return 0;
}

/* Exit point */
static void __exit motiondev_exit(void)
{
	/* Clean all*/
	printk(KERN_INFO "%s begin cleanup.",  DEVICE_FILE_NAME);
	
	/* Delete the character driver */
	cdev_del(&motiondev_cdev);
	
	/* Destroy the device */
	device_destroy(motiondev_class, MKDEV(motiondev_major, MINOR_DEVICE_NUMBER));
	
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
