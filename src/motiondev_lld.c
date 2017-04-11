#include "motiondev_lld.h"
#include "motiondev_reg.h"

/************************************************************************
<Info>
Looks like a standard 16bit address/data multiplexed scheme mapped on GPIO

<Data lines>
16 bit bus Xn = GPIOXn
MSB                                           LSB
[B6|B5|B4|B3|B2|E7|E6|E5|E4|E3|E2|A7|A6|A5|A4|A3]

<Control lines>
GPIOB_14 = A0 (0 for address write, 1 for data access)
GPIOB_13 = RD (Active high)
GPIOA_0 = CS (Active high)
GPIOA_1 = WR (Active high)
************************************************************************/

/* Control pin location */
#define A0	(14u)
#define RD	(13u)
#define CS	(0u)
#define WR	(1u)

/* Data nibbles masks */
#define DATA_MASK_GPIOA	(0xF8u)
#define DATA_MASK_GPIOE	(0xFCu)
#define DATA_MASK_GPIOB	(0x7Cu)

/* Macros for control signals */
#define A0_HIGH() 	GPIOB_DOUT |= (1u << A0);
#define A0_LOW() 	GPIOB_DOUT &= ~(1u << A0);
#define RD_HIGH() 	GPIOB_DOUT |= (1u << RD);
#define RD_LOW() 	GPIOB_DOUT &= ~(1u << RD);
#define CS_HIGH() 	GPIOA_DOUT |= (1u << CS);
#define CS_LOW() 	GPIOA_DOUT &= ~(1u << CS);
#define WR_HIGH() 	GPIOA_DOUT |= (1u << WR);
#define WR_LOW() 	GPIOA_DOUT &= ~(1u << WR);

/* Address / data macro */
#define ACC_ADDR	(0u)
#define ACC_DATA	(1u)

/* Data access macro */
#define OUT_DATA(dat) do{GPIOA_DOUT = (GPIOA_DOUT & ~DATA_MASK_GPIOA) | ((dat & 0x1Fu) << 3u)); \
						GPIOE_DOUT = (GPIOE_DOUT & ~DATA_MASK_GPIOE) | ((dat & 0x7E0u) >> 3u)); \
						GPIOB_DOUT = (GPIOB_DOUT & ~DATA_MASK_GPIOB) | ((dat & 0xF800u) >> 9u));} while(0u)

#define IN_DATA() (((GPIOA_PIN & DATA_MASK_GPIOA) >> 3u) \
					| ((GPIOE_PIN & DATA_MASK_GPIOE) << 3u) \
					| ((GPIOB_PIN & DATA_MASK_GPIOB) << 9u))					

/* Port direction macros */
#define DATA_DIR_OUT() do{GPIOA_OMD |= DATA_MASK_GPIOA; \
						GPIOE_OMD |= DATA_MASK_GPIOE; \
						GPIOB_OMD |= DATA_MASK_GPIOB;} while(0u)

#define DATA_DIR_IN() do{GPIOA_OMD &= ~DATA_MASK_GPIOA; \
						GPIOE_OMD &= ~DATA_MASK_GPIOE; \
						GPIOB_OMD &= ~DATA_MASK_GPIOB;} while(0u)

/* Init all the GPIO registers */
void motiondev_lld_init(void)
{
	/* Pin modes setup (GPIO mode) */
	GPAFUN &= ~(0xFFCFu); /* Outputs only on CS, WR and DATA */
	GPBFUN &= ~(0x3C003FF0); /* Outputs only on A0, RD and DATA */
	GPDFUN &= ~(0xFF00003F); /* GPIOD usage unknown at the moment */
	GPEFUN &= ~(0xFFF0u); /* Only data pins */
	
	/* Clear CS and WR */
	GPIOA_DOUT &= ~((1u << CS) | (1u << WR));
	
	/* Clear RD */
	GPIOB_DOUT &= ~(1u << RD);
	
	/* Pull - ups */
	GPIOA_PUEN |= DATA_MASK_GPIOA;
	GPIOB_PUEN |= DATA_MASK_GPIOB;
	GPIOE_PUEN |= DATA_MASK_GPIOE;

	/* Output mode */
	GPIOA_OMD |= (DATA_MASK_GPIOA | (1u << CS) | (1u << WR)); 
	GPIOB_OMD |= (DATA_MASK_GPIOB | (1u << A0) |  (1u << RD));
	GPIOE_OMD |= DATA_MASK_GPIOE;
	
	/* Don't know exactly what for */
	GPIOD_PUEN |= (1u << 15u); /* Pull up */
	GPIOD_OMD &= ~(1u << 15u); /* Input */
	GPIOD_OMD |= (1u << 14u) | (1u << 13u) | (1u << 12u) | (1u << 2u) | (1u << 1u) | (1u << 0u);
}

/* Write data */
void motiondev_lld_write_data(unsigned short addr, unsigned short data)
{
	A0_LOW(); /* Address access */
	OUT_DATA(addr); /* Put address */
	CS_HIGH(); /* Activate chip select */
	WR_HIGH(); /* Perform a write cycle*/
	WR_LOW();
	A0_HIGH(); /* Data access */
	OUT_DATA(data);	/* Put data */ 
	WR_HIGH(); /* Perform a write cycle*/
	WR_LOW();
	CS_LOW(); /* Deactivate the chip */
}

/* Read data */
unsigned short motiondev_lld_read_data(unsigned short addr)
{
	unsigned short data;

	A0_LOW(); /* Address access */
	OUT_DATA(addr); /* Put address */
	CS_HIGH(); /* Activate chip select */
	WR_HIGH(); /* Perform a write cycle*/
	WR_LOW();
	DATA_DIR_IN(); /* Configure Pins as inputs, Pull-ups should be enabled at init */	
	A0_HIGH(); /* Data access */
	RD_HIGH(); /* Read access */	
	data = IN_DATA(); /* Read data */	
	RD_LOW(); /* Disable read access */	
	CS_LOW(); /* Deactivate chip select */
	DATA_DIR_OUT(); /* Configure Pins as outputs */
	
	return data;
}