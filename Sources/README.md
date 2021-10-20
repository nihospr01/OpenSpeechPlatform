# Open Speech Platform

OSP works on Linux and Mac operating systems.  It also works quite well on
Chromebooks that support Linux.

## Prerequisites

All platforms need python3 with the 'rich' package installed.

```
pip3 install rich
```

### Mac

- xcode

Install xcode from from "Software Update".

- brew

```
ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
```

- packages from brew

```
brew install coreutils cmake portaudio pkg-config composer php@7.4 node@14 googletest libsndfile jack
```


### Linux (Debian)

node.js must be v14 or earlier.  

```
sudo apt install alsa-base alsa-utils libasound2-dev portaudio19-dev \
    cmake sqlite sqlite3 php php-mbstring libgtest-dev \
    php-xml composer zip unzip php-sqlite3 nodejs yarn

sudo apt install libnode-dev node-gyp libssl-dev npm libgtest-dev googletest
```

### Linux (Red Hat)

node.js must be v14 or earlier.  

```
sudo yum install alsa-base alsa-utils libasound2-dev portaudio19-dev \
    cmake sqlite sqlite3 php php-mbstring \
    php-xml composer zip unzip php-sqlite3 libnode-dev node-gyp\
    libssl-dev npm git libgtest-dev
```

## Checking out Everything Else

Run `scripts/checkout` to check some openMHA and liblsl.

```
/OpenSpeechPlatform-new/Sources ❯❯❯ ./scripts/checkout
Cloning into 'openMHA'...
remote: Enumerating objects: 17062, done.
remote: Counting objects: 100% (1458/1458), done.
remote: Compressing objects: 100% (945/945), done.
remote: Total 17062 (delta 722), reused 1039 (delta 486), pack-reused 15604
Receiving objects: 100% (17062/17062), 252.14 MiB | 36.32 MiB/s, done.
Resolving deltas: 100% (9652/9652), done.
~/OpenSpeechPlatform-new/Sources/openMHA ~/OpenSpeechPlatform-new/Sources
Branch 'development' set up to track remote branch 'development' from 'origin'.
Switched to a new branch 'development'
```

## Building Everything

`scripts/install` will build everything and install it in a directory called "release". 

```
~/OpenSpeechPlatform-new/Sources ❯❯❯ ./scripts/install
Building and Installing libosp
~/OpenSpeechPlatform-new/Sources/libosp ~/OpenSpeechPlatform-new/Sources
make -C docs clean
make[1]: Entering directory '/home/mmh/OpenSpeechPlatform-new/Sources/libosp/docs'
rm -f -r html
make[1]: Leaving directory '/home/mmh/OpenSpeechPlatform-new/Sources/libosp/docs'
rm -f -rf release debug
# python
rm -f -r build dist
rm -f -r .pytest_cache
find . -name __pycache__ -exec rm -r {} +
find rtmha -name \*.cpp -exec rm -r {} +
find rtmha -name \*.so -exec rm -r {} +
mkdir -p release
cd release && cmake -DCMAKE_INSTALL_PREFIX=/home/mmh/OpenSpeechPlatform-new/Sources/release ..
-- The C compiler identification is GNU 9.3.0
-- The CXX compiler identification is GNU 9.3.0
-- Check for working C compiler: /usr/bin/cc
...
make[2]: Entering directory '/home/mmh/OpenSpeechPlatform-new/Sources/openMHA/mha/plugins/example6'
make[2]: Nothing to be done for 'all'.
make[2]: Leaving directory '/home/mmh/OpenSpeechPlatform-new/Sources/openMHA/mha/plugins/example6'
make[1]: Leaving directory '/home/mmh/OpenSpeechPlatform-new/Sources/openMHA/mha/plugins'
```

```
~/OpenSpeechPlatform-new/Sources ❯❯❯ ls -l release/bin
total 12104
-rwxrwxr-x 1 mmh mmh 1699808 Sep 16 12:36 analysemhaplugin
-rwxrwxr-x 1 mmh mmh  550040 Sep 16 12:36 browsemhaplugins
-rwxrwxr-x 1 mmh mmh 4064936 Sep 16 12:36 generatemhaplugindoc
-rwxrwxr-x 1 mmh mmh 5468936 Sep 16 12:36 mha
-rwxr-xr-x 1 mmh mmh  510272 Sep 16 12:31 osp
-rwxr-xr-x 1 mmh mmh   22066 Sep  9 21:32 osp_cli
-rwxr-xr-x 1 mmh mmh    1666 Sep  9 21:32 osp_convert
-rwxr-xr-x 1 mmh mmh   19232 Sep 16 12:31 pa_devs
-rwxrwxr-x 1 mmh mmh     288 Sep  9 21:32 run_mha
-rwxrwxr-x 1 mmh mmh    2717 Sep  9 21:32 run_osp
-rwxrwxr-x 1 mmh mmh      64 Sep  9 21:32 start-ews
-rwxrwxr-x 1 mmh mmh      88 Sep  9 21:32 start-ews-php
-rwxrwxr-x 1 mmh mmh   22000 Sep 16 12:36 testalsadevice
```

- osp - The audio processing server, formerly RT-MHA  
- osp_cli - The command line interface for osp
- osp_convert - Converts OSP recordings to wav format.
- pa_devs - Linux only.  Print all the audio devices.  
- run_osp - Runs osp, osp_cli, ews, and ews-php in tabs in a terminal.
- run_mha - A script to set up the environment and run openMHA (mha)  
- start-ews - Starts the nodejs Embedded Web Server
- start-ews-php - Starts the PHP Embedded Web Server


## Building PCD Images

**Building images for the PCD device is not a simple task.  If you have a PCD device, we recommend you use the official builds.**

First you must edit `scripts/checkout` and uncomment the section near the bottom.  Then run `scripts/checkout` to check out the necessary software.

The `ospboard` directory contains the sources.  Take a look at README.md and BUILDING.md for directions.

# openMHA

For our builds, OpenMHA is run with a script, `run_mha` which sets up the environment and calls `mha`.

On PCD, you must disable OSP processing by either creating a
file named `/root/.no_osp_startup` and rebooting OR
killing the osp process like this:

```bash
root@ospboard:/opt/openMHA/examples/00-gain> ps_osp
CPUID CLS PRI %CPU   LWP COMMAND
0  TS  19  0.0   534 OSP
1  FF 130 20.2   544 OSP: Chan 0
2  FF 130 19.5   545 OSP: Chan 1
3  FF  41 14.1   579 OSP: AudioCB
root@ospboard:/opt/openMHA/examples/00-gain> kill 534
```

To use openMHA with the PCD, your config files must set the following to
get live input from the mics:
```
srate = 48000
iolib=MHAIOPortAudio
io.device_index_in=1
io.device_index_out=4
```

For other devices, check the input and output device using `pa_devs`.
You can find the default devices like this:
```bash
root@ospboard:~> pa_devs | grep "\[ Default" -B 1
--------------------------------------- device #1
[ Default Input ]
--
--------------------------------------- device #4
[ Default Output ]
```

The following examples in /opt/openMHA/examples have been tested and should work as is on PCD, and with proper input/output numbers on other.

- 00-gain/gain_live_getting_started.cfg
- 00-gain/gain_live.cfg
- 00-gain/gain_live_double.cfg
- 01-dynamic-compression/dynamiccompression_live.cfg
- 17-PHL-generic-hearing-aid/generic-hearing-aid/index.cfg
