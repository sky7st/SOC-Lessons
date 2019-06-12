
obj-m := ip.o
KERNELDIR ?= /home/vivado2017-4-zxc123/Desktop/zynq/linux-xlnx

default:
	ARCH=arm
	CROSS_COMPILE=$(CROSS_COMPILE)
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

help:

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
