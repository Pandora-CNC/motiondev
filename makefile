export ARCH:=arm
export CROSS_COMPILE:=arm-none-linux-gnueabi-

KDIR := /mcuzone/work/w55fa93bsp-2.6.35/linux-2.6.35.4

PWD := $(shell pwd)

obj-m += motiondev.o
motiondev-objs := /src/motiondev.o /src/motiondev_lld.o

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
