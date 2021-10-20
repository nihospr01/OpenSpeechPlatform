#!/usr/bin/bash

# setup development environment for osp
OSP_ROOT=/opt/osp
OSP_BUILD=$OSP_ROOT/var/build
KERNEL_MAKE_V=2
OSP_PROCESS=$OSP_ROOT/src/osp_process
ROOTFS_DIR=$OSP_ROOT/rootfs
REDIRECT=/dev/stderr

mkdir -p $OSP_BUILD
mkdir -p $ROOTFS_DIR

NTHREADS=$(expr $(grep -c ^processor /proc/cpuinfo) + 1)
export NTHREADS

PATH=/opt/osp/bin:$PATH
export PATH

ARCH=arm64
export ARCH

CROSS_COMPILE="ccache aarch64-linux-gnu-"
export CROSS_COMPILE

updateRootfs() {
  printf "$1Updating Rootfs.......................... "
  pushd /opt/osp/share &>> /dev/null
  umount /opt/osp/rootfs &>> /dev/null
  img2simg rootfs.img rootfs.simg &>> /dev/null
  mount -t ext4 -o rw,loop,auto /opt/osp/share/rootfs.img /opt/osp/rootfs &>> /dev/null
  popd &>> /dev/null
  printf "DONE\n"
}

compressRootfs() {
  printf "$1Compressing Rootfs.......................... "
  pushd /opt/osp/share &>> /dev/null
  rm -f rootfs.simg.7z &>> /dev/null
  7z a -bd rootfs.simg.7z rootfs.simg  &>> /dev/null
  popd &>> /dev/null
  printf "DONE\n"
}

buildKernel5() {
  KERNEL_DIR=$OSP_ROOT/src/kernel
  unCompress
  mount -t ext4 -o rw,loop,auto /opt/osp/share/rootfs.img /opt/osp/rootfs
  echo "Linux Kernel Build Tasks:"
  pushd $KERNEL_DIR &>> /dev/null
  if [ ! -f arch/arm64/configs/ospboard_defconfig ]; then
    pushd arch/arm64/configs
    ln -s defconfig ospboard_defconfig
    popd
  fi
  make clean
  rm -f .config
  make ospboard_defconfig V=$KERNEL_MAKE_V &>> $REDIRECT
  printf "  Building Image.gz......................... "
  make -j$NTHREADS Image.gz V=$KERNEL_MAKE_V &>> $REDIRECT
  printf "DONE\n"
  printf "  Building modules.......................... "
  make -j$NTHREADS modules V=$KERNEL_MAKE_V &>> $REDIRECT
  printf "DONE\n"
  printf "  Building dtbs............................. "
  make -j$NTHREADS dtbs V=$KERNEL_MAKE_V &>> $REDIRECT
  printf "DONE\n"
  printf "  Installing modules....................... "
  make modules_install INSTALL_MOD_STRIP=1 INSTALL_MOD_PATH=$ROOTFS_DIR V=$KERNEL_MAKE_V &>> $REDIRECT
  printf "DONE\n"
  printf "  Building boot images..................... "
  cat arch/arm64/boot/Image.gz arch/arm64/boot/dts/qcom/apq8016-osp-v9.dtb > $OSP_BUILD/Image.gz+dtb
  mkbootimg --kernel $OSP_BUILD/Image.gz+dtb \
            --ramdisk $OSP_ROOT/share/initrd.img \
            --output $OSP_BUILD/boot-v9.img \
            --pagesize 2048 \
            --base 0x80000000 \
            --cmdline "root=/dev/mmcblk0p14 rw rootwait console=ttyMSM0,115200n8 isolcpus=1,2,3"
  rm -f $OSP_BUILD/Image.gz+dtb &>> /dev/null
  printf "DONE\n"
  popd &>> /dev/null
}

buildKernel4() {
  KERNEL_DIR=$OSP_ROOT/src/kernel4
  unCompress
  mount -t ext4 -o rw,loop,auto /opt/osp/share/rootfs.img /opt/osp/rootfs
  echo "Linux Kernel Build Tasks:"
  pushd $KERNEL_DIR &>> /dev/null
  if [ ! -f arch/arm64/configs/ospboard_defconfig ]; then
    pushd arch/arm64/configs
    ln -s defconfig ospboard_defconfig
    popd
  fi
  make clean
  rm -f .config
  make ospboard_defconfig V=$KERNEL_MAKE_V &>> $REDIRECT
  printf "  Building Image.gz......................... "
  make -j$NTHREADS Image.gz V=$KERNEL_MAKE_V &>> $REDIRECT
  printf "DONE\n"
  printf "  Building modules.......................... "
  make -j$NTHREADS modules V=$KERNEL_MAKE_V &>> $REDIRECT
  printf "DONE\n"
  printf "  Building dtbs............................. "
  make -j$NTHREADS dtbs V=$KERNEL_MAKE_V &>> $REDIRECT
  printf "DONE\n"
  printf "  Installing modules....................... "
  make modules_install INSTALL_MOD_STRIP=1 INSTALL_MOD_PATH=$ROOTFS_DIR V=$KERNEL_MAKE_V &>> $REDIRECT
  printf "DONE\n"
  printf "  Building boot images..................... "
  cat arch/arm64/boot/Image.gz arch/arm64/boot/dts/qcom/ospboardv7.dtb > $OSP_BUILD/Image.gz+dtb
  mkbootimg --kernel $OSP_BUILD/Image.gz+dtb \
            --ramdisk $OSP_ROOT/share/initrd.img \
            --output $OSP_BUILD/boot-v9.img \
            --pagesize 2048 \
            --base 0x80000000 \
            --cmdline "root=/dev/mmcblk0p14 rw rootwait console=ttyMSM0,115200n8 isolcpus=1,2,3"
  rm -f $OSP_BUILD/Image.gz+dtb &>> /dev/null
  printf "DONE\n"
  popd &>> /dev/null
}

unCompress() {
  if [ ! -f /opt/osp/share/rootfs.simg ]; then
    echo "*** Decompressing rootfs, this may take a minute but occurs only on first launch ***"
    pushd /opt/osp/share &>> /dev/null
    7z e -bd rootfs.simg.7z &>> /dev/null
    popd &>> /dev/null
  fi

  if [ ! -f /opt/osp/share/rootfs.img ]; then
	  echo "*** Extracting rootfs, this may take a minute but occurs only on first launch ***"
	  pushd /opt/osp/share &>> /dev/null
    simg2img rootfs.simg rootfs.img &>> /dev/null
    popd &>> /dev/null
  fi
}

setOutput() {
  if [ "$1" = "verbose" ]; then
    REDIRECT=/dev/stderr
		KERNEL_MAKE_V=2
  elif [ "$1" = "quiet" ]; then
    REDIRECT=/dev/null
		KERNEL_MAKE_V=0
  elif [ "$1" = "log" ]; then
    REDIRECT=$OSP_BUILD/output.log
		KERNEL_MAKE_V=1
  fi
}

buildHelp() {
    echo 'Available Commands:'
    echo '  updateRootfs   -- updates rootfs.simg with changes to "/opt/osp/rootfs"'
    echo '  compressRootfs -- updates rootfs.simg.gz with latest rootfs.simg'
    echo '  buildKernel    -- builds the kernel and creates image'
    echo '  unCompress     -- uncompress rootfs.simg.7z to rootfs.img, if necessary'
    echo '  setOutput      -- sets the output to "quiet, verbose (default), or log"'
    echo '  buildHelp      -- print this help summary'
}

if ! [ -f /usr/local/bash-git-prompt/gitprompt.sh ]; then
	pushd /usr/local &> /dev/null
	git clone --branch=2.7.1 https://github.com/magicmonty/bash-git-prompt.git &> /dev/null
	popd &> /dev/null
fi
source /usr/local/bash-git-prompt/gitprompt.sh

complete -W "quiet verbose log" setOutput
$OSP_FUNC

# vim: set filetype=sh tabstop=2 shiftwidth=2 softtabstop=2 expandtab :
