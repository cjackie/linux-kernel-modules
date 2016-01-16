CC := gcc
CFLAG := -g -Wall -Werror
PWD := $(shell pwd)
KP := /lib/modules/$(shell uname -r)/build

obj-m := cj_cdev_pipe.o

top: main_fork main_pthread
	make -C $(KP) M=$(PWD) modules

clean:                                                                          
	rm main_fork main_pthread
	make -C $(KP) M=$(PWD) clean                                        

main_fork: main_fork.c
	gcc $(CFLAG) -o main_fork main_fork.c

main_pthread: main_pthread.c
	gcc $(CFLAG) -pthread -o main_pthread main_pthread.c
