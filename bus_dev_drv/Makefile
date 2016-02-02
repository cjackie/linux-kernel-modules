CC := gcc
PWD := $(shell pwd)
KP := /lib/modules/$(shell uname -r)/build

obj-m += cj_bus.o cj_dev.o cj_drv.o

build: 
	make -C $(KP) M=$(PWD) modules

clean:
	make -C $(KP) M=$(PWD) clean
