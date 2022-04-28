
obj-m := ioctl_example_prog.o

KERNEL_DIR=/home/rkroshan/Downloads/ldd2/linux-4.14
ARCH=arm
CROSS_COMPILER=arm-linux-gnueabihf-

all: modules application_program
install: modules_install

application_program:
	$(CROSS_COMPILER)gcc ioctl_userspace_application.c -o ioctl_demo

modules modules_install clean help:
	make -C $(KERNEL_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILER) M=$(PWD) $@

copy:
	cp *.ko ../tank/	
