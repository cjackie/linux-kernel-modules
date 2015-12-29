CC := gcc
PWD := $(shell pwd)
KP := /lib/modules/$(shell uname -r)/build

obj-m := my_cdev.o

top: 
	make -C $(KP) M=$(PWD) modules

install:
	$(shell install)

uninstall:
	rmmod my_cdev.ko
	rm /dev/cj
