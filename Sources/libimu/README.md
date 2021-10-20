# LibIMU
A library to efficiently work with multiple IMU devices.
Currently only supports BMI160 and BMI270 connected via SPI.

## Application Overview
LibIMU is a C++ library that ease the acquisition of data from multiple IMU devices at high speed. It supports multiple IMU devices and buffering of data from IMU. In our case, there are three devices two ear-level and one on PCD.

## Installation

```bash
$ git clone https://user_name@bitbucket.org/openspeechplatform/libimu.git
cd libimu
git checkout develop

make
```
The executable will be in the 'release' directory.

## Usage

```
> ./release/log_imu -h

USAGE:

log_imu  [-FhpqST] [--version] [-c <int>] [-f <filename>] [-r <int>] -i
<PCD|LEFT|RIGHT>

Where:

-i <PCD|LEFT|RIGHT>,  --imu <PCD|LEFT|RIGHT>
(required) IMU

-f <filename>,  --filename <filename>
Filename for output

-p,  --print
Print IMU data on command line

-T,  --Tap
Enables tap detection for toggling beamforming

-S,  --Spin
Enables spin detection for toggling beamforming

-F,  --Flip
Enables flip detection for toggling beamforming

-r <int>,  --rate <int>
Sampling rate in Hz [1-125] Default 100.

-c <int>,  --cpu <int>
Which CPU to use [0-3] Default 1.

-q,  --quiet
Don't log or print IMU data.

--,  --ignore_rest
Ignores the rest of the labeled arguments following this flag.

--version
Displays version information and exits.

-h,  --help
Displays usage information and exits.

Log IMU activity
```
