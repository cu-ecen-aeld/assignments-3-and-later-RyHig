#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_PATCH_REPO=https://github.com/bwalle/ptxdist-vetero.git
KERNEL_PATCH_PATH="${OUTDIR}/ptxdist-vetero/patches/linux-5.0"
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
SYSROOT=`${CROSS_COMPILE}gcc -print-sysroot`
if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"


if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    if [ ! -d "${OUTDIR}/ptxdist-vetero" ]; then
        echo "PUTTING dtc-multiple-definition.patch in ${OUTDIR}/ptxdist-vetero"
        git clone ${KERNEL_PATCH_REPO} --depth 1 --single-branch --branch master
    fi
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}
    # TODO: Add your kernel build steps here
    echo "mrproper starting"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    #CROSS_COMPILE=${CROSS_COMPILE} arch=${ARCH} make menuconfig
    echo "defconfig"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    echo "creating image"
    make -j16 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} %.dtb || RESULT=$?
    if [ RESULT -eq 0 ]; then
        echo "Success no need to patch"
    else
        git apply ${KERNEL_PATCH_PATH}/dtc-multiple-definition.patch
    fi
    make -j16 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
fi

echo "Adding the Image in outdir"
cd ${OUTDIR}/linux-stable/arch/${ARCH}/boot
cp Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
if [ ! -d "$OUTDIR/rootfs" ]; then
    mkdir -p ${OUTDIR}/rootfs
    cd ${OUTDIR}/rootfs
    mkdir -p bin dev etc home lib lib64 proc sbin sys tmp 
    mkdir -p usr/bin usr/lib usr/sbin
    mkdir -p var/log
fi

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    make distclean
    make defconfig
else
    cd busybox
fi

# TODO: Make and install busybox
make arch=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make arch=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} CONFIG_PREFIX=../rootfs install

cd ${OUTDIR}/rootfs

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
cp -a $SYSROOT/lib/ld-linux-aarch64.so.1 lib
cp -a $SYSROOT/lib64/libm* lib64
cp -a $SYSROOT/lib64/libc* lib64
cp -a $SYSROOT/lib64/libresolv* lib64
# TODO: Make device nodes
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1
# TODO: Clean and build the writer utility
cd ${FINDER_APP_DIR}
make clean || echo "Attempting to clean..."
make CROSS_COMPILE=${CROSS_COMPILE}
# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp -r ../conf ${OUTDIR}/rootfs/
cp -a conf writer finder* autorun-qemu.sh ${OUTDIR}/rootfs/home
# TODO: Chown the root directory
sudo chown root:root ${OUTDIR}/rootfs
# TODO: Create initramfs.cpio.gz
cd "$OUTDIR/rootfs"
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd ${OUTDIR}
gzip -f initramfs.cpio
mkimage -A ${ARCH} -O linux -T ramdisk -d initramfs.cpio.gz uRamdisk