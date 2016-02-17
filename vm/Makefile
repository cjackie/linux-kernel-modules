1;3409;0cCC := gcc
PWD := $(shell pwd)
KP := /lib/modules/$(shell uname -r)/build

obj-m := cj_mem.o

top: main
	make -C $(KP) M=$(PWD) modules
clean: 
	rm main
	make -C $(KP) M=$(PWD) clean

main: main.c
	$(CC) -Wall -Werror main.c -o main
