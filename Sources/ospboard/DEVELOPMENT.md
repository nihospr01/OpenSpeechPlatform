# PCD Software Development and Debugging

First, you should read the [README](README.md).  To do testing and development, you
will want to set the mode to NetworkManager and configure ssh.

Once the device is flashed, you can use Windows, Mac, or Linux to connect to it.

## Using SSH

Once ssh is setup, you can open multiple terminals from your
laptop/workstation to the device.

## Using Visual Studio Code

[Visual Studio Code](https://code.visualstudio.com/) with the [Remote SSH Extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-ssh) is a great way to do development on the device.  You can open multiple terminals with it, as well as edit files on the device.

## Device Configuration


## Monitoring and Controlling the OSP Processes

By default boots up in HotSpot mode.  You can connect from a device using wifi by attaching to SSID “ospboard”, password “hearingaid”  Then connect to http://192.168.8.1:5000 and  http://192.168.8.1:8080

If you are using NetworkManager mode, you will need to use the ip address
assigned by your wifi router.

OSP has three process that are started automatically in /etc/rc.local
```
ionice -c 1 /opt/release/bin/osp -m > /var/log/osp.log &
/opt/release/bin/start-ews > /var/log/ews.log &
/opt/release/bin/start-ews-php > /var/log/ews-php.log &
```

You can cat or tail the log files.  You can also kill any of the processes you don't need.

For `osp` you can monitor CPU usage with `ps_osp`.  `osp` runs realtime threads
in CPUs 1-3.  
```
root@ospboard:~> ps_osp
CPUID CLS PRI %CPU   LWP COMMAND
    0  TS  19  0.0   558 OSP
    1  FF 130 22.3   566 OSP: Chan 0
    2  FF 130 21.8   567 OSP: Chan 1
    3  FF  41 13.0   607 OSP: AudioCB
```

If you wish to disable all the OSP processes, simply create a
file named `/root/.no_osp_startup` and reboot.  As long as that file exists,
OSP will not be run on startup.


root@ospboard:/etc> ls os*
os-release  osp_version  ospboard-rootfs
root@ospboard:/etc> cat os-release
PRETTY_NAME="Debian GNU/Linux 10 (buster)"
NAME="Debian GNU/Linux"
VERSION_ID="10"
VERSION="10 (buster)"
VERSION_CODENAME=buster
ID=debian
HOME_URL="https://www.debian.org/"
SUPPORT_URL="https://www.debian.org/support"
BUG_REPORT_URL="https://bugs.debian.org/"

root@ospboard:/etc> cat osp_version 
12Feb21
root@ospboard:/etc> cat ospboard-rootfs 
v1.2
uname -a
Linux ospboard 5.10.0-g9d98f36e53ad-dirty #1 SMP PREEMPT Mon Feb 8 23:27:43 UTC 2021 aarch64 GNU/Linux

/usr/local/bin:
total 92
-rwxr-xr-x 1 root root 14624 Feb 12 18:36 charging_monitor
-rwxr-xr-x 1 root root   573 Feb 12 18:36 init_trigger.sh
-rwxr-xr-x 1 root root  9456 Feb  8 20:10 lslver
lrwxrwxrwx 1 root root    20 Feb 12 18:36 osp -> /opt/release/bin/osp
lrwxrwxrwx 1 root root    24 Feb 12 18:36 osp_cli -> /opt/release/bin/osp_cli
lrwxrwxrwx 1 root root    24 Feb 12 18:36 pa_devs -> /opt/release/bin/pa_devs
-rwxr-xr-x 1 root root 60616 Feb 12 18:36 refimu

/usr/local/lib:
total 4624
-rw-r--r-- 1 root root  382840 Jan 25  2020 libOSP.a
lrwxrwxrwx 1 root root      16 Dec 18 23:33 liblsl.so -> liblsl.so.1.14.0
-rw-r--r-- 1 root root 4331320 Feb  8 20:10 liblsl.so.1.14.0

root@ospboard:/etc> ls /opt
bin  charging-monitor  libimu  liblsl  release	scripts
root@ospboard:/etc> ls /opt/charging-monitor/
CMakeLists.txt	main.c
root@ospboard:/etc> ls /opt/bin
charging_monitor  djb_amixer_setup.sh  enable_battery_charging.sh  fpga_configuration
root@ospboard:/etc> ls /opt/release/bin
latency_test  osp  osp_cli  pa_devs  run_osp  start-ews  start-ews-php


ls /opt/libimu
CMakeLists.txt	README.md  build  init_trigger.sh  libimu.cpp  libimu.h  misc  reference_imu.cpp

ls /opt/liblsl/
CHANGELOG	LICENSE    azure-pipelines.yml	cmake  docs	 include   pi.cmake  src			      testing	  update_lslboost.sh
CMakeLists.txt	README.md  build		conda  examples  lslboost  project   standalone_compilation_linux.sh  thirdparty




