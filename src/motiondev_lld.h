#ifndef _MOTIONDEV_LLD_H_
#define _MOTIONDEV_LLD_H_

/* Exported functions */
void motiondev_lld_init(void);
void motiondev_lld_write_data(unsigned short addr, unsigned short data);
unsigned short motiondev_lld_read_data(unsigned short addr);
							
#endif
