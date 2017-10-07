# export ARCH=arm && export CROSS_COMPILE=arm-linux-gnueabihf-

K_PATH = /home/juriaan/linux-xlnx/
B_PATH = $(PWD)/output

TARGET_IP = 192.168.5.10

NAME = btnled

obj-m := $(NAME).o


all: build install clean
	

build:
	make -C $(K_PATH) M=$(PWD) KBUILD_VERBOSE=0 modules

install:
	sshpass -p "root" scp $(NAME).ko root@$(TARGET_IP):$(NAME).ko

clean:
	rm *.ko
	rm *.mod.c
	rm *.mod.o
	rm *.o
	rm Module.symvers
	rm modules.order

