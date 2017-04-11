KERNEL_DIR=F:\Projects\ddcsv_custom\motiondev\kernel

KERNEL_INC=$(KERNEL_DIR)/include THIS_DIR=F:\Projects\ddcsv_custom\motiondev\

obj-m += src/motiondev.o

default:
	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- -C $(KERNEL_DIR) SUBDIRS=$(THIS_DIR) modules