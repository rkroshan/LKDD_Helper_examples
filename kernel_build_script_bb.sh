#!/bin/sh -ev

KERNEL_DIR=/home/rkroshan/Downloads/ldd2/linux-4.14
ARCH=arm
CROSS_COMPILER=arm-linux-gnueabihf-
DEF_CONFIG=bb.org_defconfig
LOADADDR=0x80008000
DTBS=dtbs
IMAGE_TYPE=uImage

#go to the kernel source directory
cd ${KERNEL_DIR}
#clean the repo
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILER} distclean
#setup default .config if any for the board
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILER} ${DEF_CONFIG}
#build the kernel image
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILER} ${IMAGE_TYPE} ${DTBS} LOADADDR=${LOADADDR} -j`nproc`
