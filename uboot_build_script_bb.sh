#!/bin/sh -ev

KERNEL_DIR=/home/rkroshan/Downloads/ldd2/u-boot-2019.01
ARCH=arm
CROSS_COMPILER=arm-linux-gnueabihf-
DEF_CONFIG=am335x_boneblack_defconfig

#go to the kernel source directory
cd ${KERNEL_DIR}
#clean the repo
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILER} distclean
#setup default .config if any for the board
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILER} ${DEF_CONFIG}
#build the kernel image
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILER} -j`nproc`
