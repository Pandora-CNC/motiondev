#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "motiondev.h"

int main(int argc, char *argv[])
{
	int fp;
	char buf[8];
	struct {
		unsigned int address;
		unsigned int data;
	} req_data;
	
	unsigned int msecs;
	
	unsigned int flag, addr, data, len;

	/* open file */
	fp = open("/dev/motion", O_RDWR);
	if(fp == -1) {
		printf("file open error\n"); 
		return 1;
	}
	
	/* arguments */
	if(argc > 1) {
		switch (argv[1][0]) {
			case 'h': /* Help */
				printf("e - enable trace\n");
				printf("d - disable trace\n");
				printf("r - reset trace\n");
				printf("saddr_data - set read bypass at address with data\n");
				printf("caddr - clear read bypass at address\n");
				printf("g - print trace data\n");
				printf("Raddr - read and print value at address\n");
				printf("Waddr_data - write data at address and print\n");
				break;
		
			case 'e': /* Enable trace */
				buf[0] = OP_ENABLE_TRACE;
				write(fp, buf, 1);
				break;
				
			case 'd': /* Disable trace */
				buf[0] = OP_DISABLE_TRACE;
				write(fp, buf, 1);
				break;
			
			case 'r': /* Reset trace */
				buf[0] = OP_RESET_TRACE;
				write(fp, buf, 1);
				break;
			
			case 's': /* Set bypass */
				sscanf(&argv[1][1], "%d_%x", &addr, &data); 
				buf[0] = OP_SET_BYPASS;
				buf[1] = addr;
				buf[2] = (data >> 8u) & 0xFFu;
				buf[3] = data & 0xFFu;
				write(fp, buf, 4);
				break;
				
			case 'c': /* Clear bypass */
			    sscanf(&argv[1][1], "%d", &addr); 
				buf[0] = OP_CLEAR_BYPASS;
				buf[1] = (unsigned char)addr;
				write(fp, buf, 2);
				break;
				
			case 'f': /* Filter trace */
				sscanf(&argv[1][2], "%d_%d", &addr, &len);
				buf[0] = OP_FILTER_WRITE;
				buf[1] = (unsigned char)addr;
				buf[2] = (unsigned char)len;
				buf[4] = 1;
				if(argv[1][1] == 'w') {
					buf[3] = TRACE_WR_FLAG;
				} else if (argv[1][1] == 'r') {
					buf[3] = TRACE_RD_FLAG;
				} else {
					printf("Unknown operation\n");
					break;
				}
				write(fp, buf, 5);
				break;
				
			case 'p': /* Filter trace */
				sscanf(&argv[1][2], "%d_%d", &addr, &len);
				buf[0] = OP_FILTER_WRITE;
				buf[1] = (unsigned char)addr;
				buf[2] = (unsigned char)len;
				buf[4] = 0;
				if(argv[1][1] == 'w') {
					buf[3] = TRACE_WR_FLAG;
				} else if (argv[1][1] == 'r') {
					buf[3] = TRACE_RD_FLAG;
				} else {
					printf("Unknown operation\n");
					break;
				}
				write(fp, buf, 5);
				break;

			case 'g': /* Get */
				while(read(fp, buf, 1)) {
					/* Get data */
					flag = buf[0] & (TRACE_WR_FLAG | TRACE_RD_FLAG);
					addr = buf[1] & 0xFFu;
					data = (((unsigned int)buf[2] & 0xFFu) << 8) | (buf[3] & 0xFFu);
					
					msecs = buf[4];
					msecs <<= 8u;
					msecs |= buf[5];
					msecs <<= 8u; 
					msecs |= buf[6];
					msecs <<= 8u; 
					msecs |= buf[7]; 
					
					if(flag & TRACE_WR_FLAG) {
						printf("WR(%03d, %04x) +%08dms\n", addr, data, msecs);
					} else if (flag & TRACE_RD_FLAG) {
						printf("RD(%03d, %04x) +%08dms\n", addr, data, msecs);
					} else {
						printf("Unknown flag!\n");
					}
				};
				break;
				
			case 'R': /* ioctl Read */
				sscanf(&argv[1][1], "%d", &addr);
				req_data.address = addr & 0xFFu; 
				if(ioctl(fp, 0x4621u, &req_data) != -1) {
					data = req_data.data & 0xFFFFu;
					printf("RD(%03d, %04x)\n", addr, data);
				} else {
					printf("could not read address %x\n", addr);
				}
				break;
				
			case 'W': /* ioctl Write */
				sscanf(&argv[1][1], "%d_%x", &addr, &data);
				req_data.address = addr & 0xFFu; 
				req_data.data = data & 0xFFFFu; 
				if(ioctl(fp, 0x4620u, &req_data) != -1) {
					printf("WR(%03d, %04x)\n", addr, data);
				} else {
					printf("could not write address %x\n", addr);
				}
				break;	
				
			default:
				printf("argument not found: %c\n", argv[1][0]);
				break;
		}
	} else {
		printf("not enough arguments\n");
	}
	
	/* Close the file */
	close(fp);
	
	return 0;
}

