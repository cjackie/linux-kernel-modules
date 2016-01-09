CC := gcc
PWD := $(shell pwd)
KP := /lib/modules/$(shell uname -r)/build

obj-m := cj_cdev_pipe.o

top: 
	make -C $(KP) M=$(PWD) modules
