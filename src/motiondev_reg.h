#ifndef _MOTIONDEV_REG_H_
#define _MOTIONDEV_REG_H_

/*These are all VIRTUAL addresses */

/* GPIO registers base */
#define GPIO_BASE 0xF8001000u

/* Used GPIO registers */
#define GPIOA_OMD *(volatile unsigned long *)(GPIO_BASE + 0x00u)
#define GPIOA_PUEN *(volatile unsigned long *)(GPIO_BASE + 0x04u)
#define GPIOA_DOUT *(volatile unsigned long *)(GPIO_BASE + 0x08u)
#define GPIOA_PIN *(volatile unsigned long *)(GPIO_BASE + 0x0Cu)
#define GPIOB_OMD *(volatile unsigned long *)(GPIO_BASE + 0x10u)
#define GPIOB_PUEN *(volatile unsigned long *)(GPIO_BASE + 0x14u)
#define GPIOB_DOUT *(volatile unsigned long *)(GPIO_BASE + 0x18u)
#define GPIOB_PIN *(volatile unsigned long *)(GPIO_BASE + 0x1Cu)
#define GPIOD_OMD *(volatile unsigned long *)(GPIO_BASE + 0x30u)
#define GPIOD_PUEN *(volatile unsigned long *)(GPIO_BASE + 0x34u)
#define GPIOD_DOUT *(volatile unsigned long *)(GPIO_BASE + 0x38u)
#define GPIOD_PIN *(volatile unsigned long *)(GPIO_BASE + 0x3Cu)
#define GPIOE_OMD *(volatile unsigned long *)(GPIO_BASE + 0x40u)
#define GPIOE_PUEN *(volatile unsigned long *)(GPIO_BASE + 0x44u)
#define GPIOE_DOUT *(volatile unsigned long *)(GPIO_BASE + 0x48u)
#define GPIOE_PIN *(volatile unsigned long *)(GPIO_BASE + 0x4Cu)

/* GCR registers base */
#define GCR_BASE 0xF8000000u

/* Used GCR registers */
#define GPAFUN *(volatile unsigned long *)(GCR_BASE + 0x80u)
#define GPBFUN *(volatile unsigned long *)(GCR_BASE + 0x84u)
#define GPDFUN *(volatile unsigned long *)(GCR_BASE + 0x8Cu)
#define GPEFUN *(volatile unsigned long *)(GCR_BASE + 0x90u)

#endif