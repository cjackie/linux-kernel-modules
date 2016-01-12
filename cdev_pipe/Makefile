CC := gcc
PWD := $(shell pwd)
KP := /lib/modules/$(shell uname -r)/build

obj-m := cj_cdev_pipe.o

top: main_fork
	make -C $(KP) M=$(PWD) modules

main_fork: main_fork.c
	gcc -o main_fork main_fork.c
