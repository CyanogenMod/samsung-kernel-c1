#!/bin/bash
set -x

# Set Default Path
TOP_DIR=$1
KERNEL_PATH=$PWD

# TODO: Set toolchain and root filesystem path
TOOLCHAIN="$TOP_DIR/prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/bin/arm-eabi-"
ROOTFS_PATH=$2

# Copy Kernel Configuration File
#cp -f $KERNEL_PATH/arch/arm/configs/c1_galaxys2_defconfig $KERNEL_PATH/.config

make -C $KERNEL_PATH oldconfig
make -C $KERNEL_PATH ARCH=arm CROSS_COMPILE=$TOOLCHAIN CONFIG_INITRAMFS_SOURCE="$ROOTFS_PATH"

# Copy kernel modules
cp drivers/bluetooth/bthid/bthid.ko $ROOTFS_PATH/lib/modules/
cp drivers/media/video/gspca/gspca_main.ko $ROOTFS_PATH/lib/modules/
cp drivers/net/wireless/bcm4330/dhd.ko $ROOTFS_PATH/lib/modules/
cp drivers/samsung/fm_si4709/Si4709_driver.ko $ROOTFS_PATH/lib/modules/
cp drivers/samsung/j4fs/j4fs.ko $ROOTFS_PATH/lib/modules/
cp drivers/samsung/vibetonz/vibrator.ko $ROOTFS_PATH/lib/modules/
cp drivers/scsi/scsi_wait_scan.ko $ROOTFS_PATH/lib/modules/

# Build again
make -C $KERNEL_PATH ARCH=arm CROSS_COMPILE=$TOOLCHAIN CONFIG_INITRAMFS_SOURCE="$ROOTFS_PATH"
