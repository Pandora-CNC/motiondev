#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/jiffies.h>
#include <asm/uaccess.h>
#include <asm/param.h>	

#include "motiondev.h"
#include "motiondev_lld.h"

/* Placeholder functions */
static void data_write(unsigned short addr, unsigned short data);
static unsigned data_read(unsigned short addr);

/* Febug framework */ 
#if (DEBUG_ENABLED != 0)

static void trace_write(unsigned short addr, unsigned short data, unsigned char flags);
static void trace_init(void);

/* Trace structure */
struct {
	/* Buffers */
    struct {
        unsigned long last;
		unsigned long cursor;
		unsigned long msstart;
        struct s_buffer {
			unsigned long msecs;
            unsigned short data;
            unsigned char addr;
			unsigned char flags;
        } buffer[TRACE_SIZE];
    } data;
    unsigned char reset;
	unsigned char enable;
	
	/* Registers */
	struct {
		unsigned short read;
		unsigned char ctrl;
	} regs[NUM_OF_REGS];
} motiondev_debug;

/* Private debug functions */

/* Init trace */
static void trace_init(void)
{
	int ind;
	
	/* Just assure correct init */
	motiondev_debug.reset = 1u;
	motiondev_debug.enable = 1u;
	
	/* Clear bypass flags */
	for(ind = 0u; ind < NUM_OF_REGS; ind++) {
		motiondev_debug.regs[ind].ctrl &= ~BYPASS_RD_FLAG;
		motiondev_debug.regs[ind].ctrl |= (TRACE_WR_FLAG | TRACE_RD_FLAG);
	}
}

/* Trace write */
static void trace_write(unsigned short addr, unsigned short data, unsigned char flags)
{
	/* Only if enabled */
	if(motiondev_debug.enable) {
		/* Reset the trace */
		if(motiondev_debug.reset) {
			motiondev_debug.data.cursor = 0u;
			motiondev_debug.data.last = 0u;
			motiondev_debug.reset = 0u;
			motiondev_debug.data.msstart = jiffies_to_msecs(jiffies);
		}
		
		/* Insert into trace only if address is correct and we have space */
		if((addr < NUM_OF_REGS) && (motiondev_debug.data.last < TRACE_SIZE)) {
			/* And the address is not excluded */
			if(motiondev_debug.regs[addr].ctrl & flags) {
				/* And if we have more space */
				motiondev_debug.data.buffer[motiondev_debug.data.last].data = data;
				motiondev_debug.data.buffer[motiondev_debug.data.last].addr = (unsigned char)addr;
				motiondev_debug.data.buffer[motiondev_debug.data.last].flags = flags;
				motiondev_debug.data.buffer[motiondev_debug.data.last].msecs = jiffies_to_msecs(jiffies) - motiondev_debug.data.msstart;
				motiondev_debug.data.last++;
			}
		}

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
	trace_write(addr, data, TRACE_WR_FLAG);
#endif
}

/* Data read */
static unsigned data_read(unsigned short addr)
{
	unsigned short val;
	
	/* Read data from chip */
	val = motiondev_lld_read_data(addr);
	
#if (DEBUG_ENABLED != 0)	
	/* Overwrite if flag requires it */
	if(addr < NUM_OF_REGS) {
		if(motiondev_debug.regs[addr].ctrl & BYPASS_RD_FLAG) {
			val = motiondev_debug.regs[addr].read;
		} else {
			motiondev_debug.regs[addr].read = val;
		}
	}
	
	/* Write the trace */
	trace_write(addr, val, TRACE_RD_FLAG);
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

#if (DEBUG_ENABLED != 0)
static ssize_t motiondev_read(struct file *filp, char *buff, size_t count, loff_t *offp);
static ssize_t motiondev_write(struct file *filp, const char *buff, size_t count, loff_t *offp);
#endif

/* Control structure */
static struct file_operations file_ops = {
#if (DEBUG_ENABLED != 0)	
	.read = motiondev_read,
	.write = motiondev_write,
#endif
	.ioctl = motiondev_ioctl,
	.open = motiondev_open,
	.release = motiondev_release
};

#if (DEBUG_ENABLED != 0)
/* Read data */
static ssize_t motiondev_read(struct file *filp, char *buff, size_t count, loff_t *offp)
{
    /* Return format [FADDTTTT] Flags, Address, Data, Time */
	unsigned char buf[8];
	int max;
	int sz;
	int ind;
	
	max = motiondev_debug.data.last - motiondev_debug.data.cursor;
	sz = (count < max) ? count : max;
	
	
	for(ind = 0; ind < sz; ind++) {
		/* Copy data */
		buf[0] = motiondev_debug.data.buffer[motiondev_debug.data.cursor].flags;
		buf[1] = motiondev_debug.data.buffer[motiondev_debug.data.cursor].addr;
		buf[2] = (motiondev_debug.data.buffer[motiondev_debug.data.cursor].data >> 8u) & 0xFFu;
		buf[3] = motiondev_debug.data.buffer[motiondev_debug.data.cursor].data & 0xFFu;
		
		/* Copy time */
		buf[4] = (motiondev_debug.data.buffer[motiondev_debug.data.cursor].msecs >> 24u) & 0xFFu;
		buf[5] = (motiondev_debug.data.buffer[motiondev_debug.data.cursor].msecs >> 16u) & 0xFFu;
		buf[6] = (motiondev_debug.data.buffer[motiondev_debug.data.cursor].msecs >> 8u) & 0xFFu;
		buf[7] = motiondev_debug.data.buffer[motiondev_debug.data.cursor].msecs & 0xFFu;
		
		/* Copy buffer */
		if(copy_to_user(&buff[ind * sizeof(buf)], buf, sizeof(buf)) != 0) {
			printk(KERN_ERR "could not read buffer\n");
			return -1;
		}
		
		/* Next element */
		motiondev_debug.data.cursor++;
	}

	return sz;
}

/* Write data */
static ssize_t motiondev_write(struct file *filp, const char *buff, size_t count, loff_t *offp)
{
    unsigned char buf[5]; /* [OALTS] largest char line */
    int sz, ind;
    unsigned short addr, data;
	unsigned char len, fltemp, state;
    
    /* Decide on size */
    sz = (count < sizeof(buf)) ? count : sizeof(buf);
    
    /* Write into the buffer */
    if(copy_from_user(buf, buff, sz) != 0u) {
        printk(KERN_ERR "could not write\n");
        return -1;
    }
    
    /* find case */
    switch(buf[0]) {
		
		/* Enable trace */
		case OP_ENABLE_TRACE: /* format [O] Op */
			motiondev_debug.enable = 1u;
            printk(KERN_INFO "trace enabled\n");
			break;
			
		/* Enable trace */
		case OP_DISABLE_TRACE: /* format [O] Op */
			motiondev_debug.enable = 0u;
            printk(KERN_INFO "trace disabled\n");
			break;	
		
        /* Reset trace */
        case OP_RESET_TRACE: /* format [O] Op */
            motiondev_debug.reset = 1u;
            printk(KERN_INFO "reset activated\n");
            break;
        
        /* Bypass set */
        case OP_SET_BYPASS: /* format [OADD] Op, Address, Data*/
            if (sz > 3) {
				/* Get address */
				addr = buf[1];
				
				/* Only if it clears */
				if(addr < NUM_OF_REGS) {
					/* Get data */
					data = ((unsigned short)buf[2] << 8u) | buf[3];
					
					/* Write */
					motiondev_debug.regs[addr].ctrl |= BYPASS_RD_FLAG;
					motiondev_debug.regs[addr].read = data;
					
					printk(KERN_INFO "bypass address %d with value %x\n", addr, data);
				} else {
					printk(KERN_NOTICE "invalid set bypass address %d\n", addr);
				}
            } else {
                printk(KERN_NOTICE "invalid set bypass format\n");
            }
            break;
		
		/* Bypass clear */
		case OP_CLEAR_BYPASS: /* format [OA] Op, Address */
			if(sz > 1) {
				/* Get address */
				addr = buf[1];
				
				/* Only if it clears */
				if(addr < NUM_OF_REGS) {
				
					/* Clear */
					motiondev_debug.regs[addr].ctrl &= ~BYPASS_RD_FLAG;
				} else {
					printk(KERN_NOTICE "invalid clear bypass address %d\n", addr);
				}
			} else {
				printk(KERN_NOTICE "invalid clear bypass format\n");
			}
			break;
			
		/* Filter trace write */	
		case OP_FILTER_WRITE: /* format [OALTS] Op, Address, Length, Type, State */
			if(sz > 4) {
				/* Get address */
				addr = buf[1];
				len = buf[2];
				fltemp = buf[3] & (TRACE_WR_FLAG | TRACE_RD_FLAG);
				state = buf[4];
				
				/* Only if it clears */
				if(addr < NUM_OF_REGS) {
					if((addr + len) <= NUM_OF_REGS) {
						for(ind = addr; ind < (addr + len); ind++) {
							if(state) {
								motiondev_debug.regs[ind].ctrl &= ~fltemp;
							} else {
								motiondev_debug.regs[ind].ctrl |= fltemp;
							}
						}
					} else {
						printk(KERN_NOTICE "invalid filter write length %d\n", len);
					}
				} else {
					printk(KERN_NOTICE "invalid filter write address %d\n", addr);
				}
			} else {
				printk(KERN_NOTICE "invalid filter write format\n");
			}
			break;

        /* Just pass */
        default:
            printk(KERN_NOTICE "no valid parameters\n");
            break;    
    }
     
	return sz;
}
#endif

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
				
				/* Clear others */
				buf[1] = 0u;
				buf[3] = 0u;
				
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
					for(n = 32u; n < 65u; n += 32u) {
						/* Write from 32/64 to 47/79 */
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
					
					/* Latch ?? */
					data_write(48u, 0u); 
				} while(data_read(18u) == 0u); /* Write check ?? */
				
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
	dev_t dev_no;
	
	/* Allocate driver region */
	result = alloc_chrdev_region(&dev_no, 0, MINOR_DEVICE_NUMBER, DEVICE_FILE_NAME);
	if(result < 0) {
		printk(KERN_ERR "error in allocating device\n");
		return -1;
	}
	
    /* Write the major number */
    motiondev_major = MAJOR(dev_no);
    dev_no = MKDEV(motiondev_major, MINOR_DEVICE_NUMBER);
    
	/* Init */
	cdev_init(&motiondev_cdev, &file_ops);
	
	/* Add the module */
	result = cdev_add(&motiondev_cdev, dev_no, 1);
	if(result) {
		printk (KERN_NOTICE "couldn't add cdev\n");
        return -1;
	}

	/* Create the class */
	motiondev_class = class_create(THIS_MODULE, DEVICE_FILE_NAME);
	if(motiondev_class == NULL) {
		unregister_chrdev_region(dev_no, 1);
		return -1;
	} 
	
	/* Create the device */
	if(device_create(motiondev_class, NULL, dev_no, NULL, DEVICE_FILE_NAME) == NULL) {
		class_destroy(motiondev_class);
		unregister_chrdev_region(dev_no, 1);
		return -1;
	}
	
	/* Print the major number */
	printk(KERN_INFO "kernel assigned major number is %d\n", motiondev_major);

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
	printk(KERN_INFO "%s begin cleanup\n",  DEVICE_FILE_NAME);
	
	/* Delete the character driver */
	cdev_del(&motiondev_cdev);
	
	/* Destroy the device */
	device_destroy(motiondev_class, MKDEV(motiondev_major, MINOR_DEVICE_NUMBER));
	
	/* Destroy the class */
	class_destroy(motiondev_class);
	
	/* Unregister the driver */
	unregister_chrdev(motiondev_major, DEVICE_FILE_NAME);
}

/* Entry and exit functions */
module_init(motiondev_init);
module_exit(motiondev_exit);

/* Module info */
MODULE_AUTHOR("Pavel Ionut Catalin");
MODULE_DESCRIPTION("DDCSV1.1 FPGA Motion Control Driver");
MODULE_LICENSE("GPL");
