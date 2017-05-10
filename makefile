export ARCH:=arm
export CROSS_COMPILE:=arm-none-linux-gnueabi-

KDIR := /mcuzone/work/w55fa93bsp-2.6.35/linux-2.6.35.4
#KDIR := /lib/modules/$(shell uname -r)/build

PWD := $(shell pwd)

MY_CFLAGS +=
ccflags-y += ${MY_CFLAGS}
CC += ${MY_CFLAGS}

obj-m += motiondev.o
motiondev-objs := /src/motiondev.o /src/motiondev_lld.o

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	$(CROSS_COMPILE)gcc -Wall --static -O2 -marm -march=armv5 src/control.c -o out/control
	cp -f motiondev.ko out/motiondev.ko
	

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
