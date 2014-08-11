USER	:= $(shell whoami)
DEBUG   := y
UNAME	:= $(shell uname -m)
obj-m	:= gspca.o kinect.o  
KERNELDIR ?= /lib/modules/$(shell uname -r)/build

ifeq ($(DEBUG),y)
  DEBFLAGS = -O -g
else
  DEBFLAGS = -O2
endif
EXTRA_CFLAGS += $(DEBFLAGS)


ifeq ($(UNAME),armv6l)
	AMRKDIR	:= /usr/src/linux
else
	ARCH	:= arm
	CROSS	:= arm-linux-gnueabi-
	ARMKDIR	:= /usr/src/arm/linux
endif

default:
	make pc

arm:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS) -C $(ARMKDIR) M=$(PWD) modules


pc:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

unload_drivers:
	sudo sh unload_drivers.sh

reload_video_drivers:
	make unload_drivers
	sudo modprobe gspca_main
	sudo modprobe gspca_kinect

load:	
	make unload_drivers
	sudo modprobe videodev
	sudo insmod gspca.ko
	sudo insmod kinect.ko
	sudo chown -f -R $(USER):$(USER) /dev/video* 
	sudo chmod -f 755 /dev/video*

fi:
	make fix-rights
	make clean
	make
	make unload_drivers
	make install
	make fix-rights

fix-rights:
	sudo chown -f -R $(USER):$(USER) $(PWD)

clean:
	rm -f *~ *.o *.ko
	rm -f .built_in.o.cmd built_in.o
	rm -f .*.cmd *.ko *.mod.c
	rm -f *.symvers *.order
	rm -rf .tmp_versions
	rm -f *.log
	rm -rf modules.order *.mod.c
	rm -rf Module.symvers
