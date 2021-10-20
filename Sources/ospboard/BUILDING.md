# Building OSPboard Images

This document is for developers.

## Install Required Packages

Packages for OSP compilation

```
sudo apt install alsa-base alsa-utils libasound2-dev portaudio19-dev \
    cmake sqlite sqlite3 php php-mbstring \
    php-xml composer zip unzip php-sqlite3 nodejs yarn
sudo apt install libnode-dev node-gyp libssl-dev npm libgtest-dev googletest
```

Packages for OSPBoard
```
sudo apt install qemu-user-static docker.io
sudo usermod -a -G docker $USER
```

**qemu-user-static 4.2.3 on Ubuntu focal ran with many warnings and errors.  I had to upgrade
to 5.0 to get things working.**

## Get Repositories

This directory must be checked out along with the rest of OSP.
See https://bitbucket.org/openspeechplatform/osp/src/develop/README.md

```
> git clone git@bitbucket.org:openspeechplatform/osp.git
> cd osp
> scripts/checkout -a
```

The kernel submodule is checked out into `ospboard/opt/osp/src/kernel`

## Build Docker Container

The docker container is used to cross-compile the kernel.  Building it only needs to be done once.

```bash
cd ospboard
./build_docker
```

## Build Kernel

Building the kernel is simple:

```
./build_kernel
```

## Uncompress Rootfs Images

If you built the kernel, this was already done and you can skip this step.

Checked into git is `opt/osp/share/rootfs.img.7z`.  This must be
uncompressed into a system image (simg) before anything can be done.  The system image must be converted to a raw image (img) before it can be mounted.  This step will do both those steps if necessary.  If the img or simg files exist, they will not be replaced.

```
./uncompress_rootfs
```

```
/opt/osp/ospboard ❯❯❯ ls -lh ./opt/osp/share/rootfs*
-rw-r--r-- 1 root root 3.8G Dec  3 18:04 ./opt/osp/share/rootfs.img
-rw-r--r-- 1 mmh  mmh  3.8G Dec  3 18:04 ./opt/osp/share/rootfs.simg
-rw-r--r-- 1 root root 982M Dec  3 18:06 ./opt/osp/share/rootfs.simg.7z
```


## Build OSP Sources

The `build_osp` script takes a version string as the only parameter.

It copies OSP cources and configuratrion files into the mounted
rootfs and builds OSP.

```
opt/osp/ospboard ❯❯❯ ./build_osp 08Feb21
Building OSP
dbdir=/opt/release/database
Building and Installing libosp
/opt/libosp /opt
/usr/bin/make -C docs clean
make[1]: Entering directory '/opt/libosp/docs'
rm -f -r html
make[1]: Leaving directory '/opt/libosp/docs'
rm -f -rf release debug
mkdir -p release
cd release && cmake -DCMAKE_INSTALL_PREFIX=/opt/release ..
-- The C compiler identification is GNU 8.3.0
-- The CXX compiler identification is GNU 8.3.0
...
```

## Convert Filesystem Image

To flash, we need to convert the raw image to a sparse image (simg).  

```
/opt/osp/ospboard ❯❯❯ ./make_img
unmounted
opt/osp/share/rootfs.img: 129780/491520 files (2.0% non-contiguous), 898501/2097152 blocks
resize2fs 1.45.5 (07-Jan-2020)
Resizing the filesystem on opt/osp/share/rootfs.img to 1175046 (4k) blocks.
The filesystem on opt/osp/share/rootfs.img is now 1175046 (4k) blocks long.

opt/osp/share/rootfs.img: 129780/276480 files (2.2% non-contiguous), 884842/1175046 blocks
Updating Rootfs.......................... DONE
```

You can now flash the image using `pcdtool`

## Make a Distribution File

To build a zip file containing everything necessary to flash and configure a device.
You should use the same name as with `build_osp`.
```
~/osp/ospboard ❯❯❯ ./make_dist 11Feb21
Making distribution file "pcd-11Feb21.zip"
~/osp/ospboard/dist/pcd-11Feb21 ~/osp/ospboard
  adding: pcd-11Feb21/ (stored 0%)
  adding: pcd-11Feb21/README.md (deflated 57%)
  adding: pcd-11Feb21/README-11Feb21.pdf (deflated 2%)
  adding: pcd-11Feb21/ospboard_id (deflated 25%)
  adding: pcd-11Feb21/pcd (deflated 66%)
  adding: pcd-11Feb21/mac/ (stored 0%)
  adding: pcd-11Feb21/mac/fastboot (deflated 60%)
  adding: pcd-11Feb21/opt/ (stored 0%)
  adding: pcd-11Feb21/opt/osp/ (stored 0%)
  adding: pcd-11Feb21/opt/osp/var/ (stored 0%)
  adding: pcd-11Feb21/opt/osp/var/build/ (stored 0%)
  adding: pcd-11Feb21/opt/osp/var/build/boot-v9.img (deflated 1%)
  adding: pcd-11Feb21/opt/osp/var/build/boot-v7.img (deflated 1%)
  adding: pcd-11Feb21/opt/osp/share/ (stored 0%)
  adding: pcd-11Feb21/opt/osp/share/dragonboard-410c-bootloader-emmc-linux-150/ (stored 0%)
  adding: pcd-11Feb21/opt/osp/share/dragonboard-410c-bootloader-emmc-linux-150/fs_image_linux.tar.gz.mbn.img (deflated 99%)
  adding: pcd-11Feb21/opt/osp/share/dragonboard-410c-bootloader-emmc-linux-150/tz.mbn (deflated 56%)
  adding: pcd-11Feb21/opt/osp/share/dragonboard-410c-bootloader-emmc-linux-150/MD5SUMS.txt (deflated 37%)
  adding: pcd-11Feb21/opt/osp/share/dragonboard-410c-bootloader-emmc-linux-150/flashall (deflated 56%)
  adding: pcd-11Feb21/opt/osp/share/dragonboard-410c-bootloader-emmc-linux-150/sbl1.mbn (deflated 38%)
  adding: pcd-11Feb21/opt/osp/share/dragonboard-410c-bootloader-emmc-linux-150/zeros_1sector.bin (deflated 98%)
  adding: pcd-11Feb21/opt/osp/share/dragonboard-410c-bootloader-emmc-linux-150/LICENSE (deflated 63%)
  adding: pcd-11Feb21/opt/osp/share/dragonboard-410c-bootloader-emmc-linux-150/patch0.xml (deflated 88%)
  adding: pcd-11Feb21/opt/osp/share/dragonboard-410c-bootloader-emmc-linux-150/NON-HLOS.bin (deflated 52%)
  adding: pcd-11Feb21/opt/osp/share/dragonboard-410c-bootloader-emmc-linux-150/emmc_appsboot.mbn (deflated 61%)
  adding: pcd-11Feb21/opt/osp/share/dragonboard-410c-bootloader-emmc-linux-150/hyp.mbn (deflated 81%)
  adding: pcd-11Feb21/opt/osp/share/dragonboard-410c-bootloader-emmc-linux-150/prog_emmc_firehose_8916.mbn (deflated 41%)
  adding: pcd-11Feb21/opt/osp/share/dragonboard-410c-bootloader-emmc-linux-150/rpm.mbn (deflated 45%)
  adding: pcd-11Feb21/opt/osp/share/dragonboard-410c-bootloader-emmc-linux-150/zeros_33sectors.bin (deflated 100%)
  adding: pcd-11Feb21/opt/osp/share/dragonboard-410c-bootloader-emmc-linux-150/.gitignore (stored 0%)
  adding: pcd-11Feb21/opt/osp/share/dragonboard-410c-bootloader-emmc-linux-150/gpt_both0.bin (deflated 97%)
  adding: pcd-11Feb21/opt/osp/share/dragonboard-410c-bootloader-emmc-linux-150/boot-erase.img (deflated 100%)
  adding: pcd-11Feb21/opt/osp/share/dragonboard-410c-bootloader-emmc-linux-150/sbc_1.0_8016.bin (deflated 68%)
  adding: pcd-11Feb21/opt/osp/share/dragonboard-410c-bootloader-emmc-linux-150/gpt_main0.bin (deflated 95%)
  adding: pcd-11Feb21/opt/osp/share/dragonboard-410c-bootloader-emmc-linux-150/gpt_backup0.bin (deflated 95%)
  adding: pcd-11Feb21/opt/osp/share/dragonboard-410c-bootloader-emmc-linux-150/rawprogram0.xml (deflated 85%)
  adding: pcd-11Feb21/opt/osp/share/rootfs.simg (deflated 62%)
  adding: pcd-11Feb21/images/ (stored 0%)
  adding: pcd-11Feb21/images/old_pcd.jpg (deflated 0%)
  adding: pcd-11Feb21/images/usb.jpg (deflated 8%)

```

## Building pctool for Windows

`pyinstaller.exe --clean -wF pcdtool` builds a 7MB executable in dist/pcdtool.exe
