export ARCH:=arm
export CROSS_COMPILE:=arm-none-linux-gnueabi-

KDIR := /mcuzone/work/w55fa93bsp-2.6.35/linux-2.6.35.4
#KDIR := /lib/modules/$(shell uname -r)/build

PWD := $(shell pwd)

MY_CFLAGS += -g -DDEBUG
ccflags-y += ${MY_CFLAGS}
CC += ${MY_CFLAGS}

obj-m += motiondev.o
motiondev-objs := /src/motiondev.o /src/motiondev_lld.o

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	EXTRA_CFLAGS="$(MY_CFLAGS)"

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
