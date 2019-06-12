ifneq ($(KERNELRELEASE),)
	obj-m := hello_zed.o
else
	KERNELDIR ?= /home/vivado2017-4-zxc123/Desktop/zynq/linux-xlnx

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

endif

help:

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean