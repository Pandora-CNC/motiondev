#include "motiondev_lld.h"
//vermagic=2.6.35.4 preempt mod_unload ARMv5

/* IO Control */
static int motion_ioctl(void)
{

}

/* Open device */
static int motion_open(void)
{
	return 0;
}

/* Close device */
static int motion_close(void)
{
	return 0;
}

/* Entry point */
static void  motion_init(void)
{
	//Create a char device and do a init
}

/* Exit point */
static void  motion_exit(void)
{
	//Perform a cleanup
}

/* Entry and exit functions */
//module_init(motion_init);
//module_exit(motion_exit);

/* Module info */
//MODULE_AUTHOR("Pavel Ionut - Catalin");
//MODULE_DESCRIPTION("DDCSV1.1 FPGA Motion Control Driver");
//MODULE_LICENSE("GPL");