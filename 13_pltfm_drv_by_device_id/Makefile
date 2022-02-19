
obj-m := pltfm_drv_by_device_id.o pltfm_dvc_by_device_id.o

KERNEL_DIR=/home/rkroshan/Downloads/ldd2/linux-4.14
ARCH=arm
CROSS_COMPILER=arm-linux-gnueabihf-

all: modules
install: modules_install

modules modules_install clean help:
	make -C $(KERNEL_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILER) M=$(PWD) $@

copy:
	cp *.ko ../tank/	
