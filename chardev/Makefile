CC := gcc
PWD := $(shell pwd)
KP := /lib/modules/$(shell uname -r)/build

obj-m := cj_cdev.o

top: cj_info_out
	make -C $(KP) M=$(PWD) modules

clean:
	rm cj_info_out
	make -C $(KP) M=$(PWD) clean



cj_info_out: cj_info_out.c
	$(CC) -o cj_info_out cj_info_out.c 
