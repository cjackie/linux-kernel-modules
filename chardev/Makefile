CC := gcc
PWD := $(shell pwd)
KP := /lib/modules/$(shell uname -r)/build

obj-m := cj_cdev.o

top: 
	make -C $(KP) M=$(PWD) modules
