#!/bin/bash
mkdir -p /opt/audio_data
mkdir -p /opt/imu_data

uid=`hostid`
time=`date +%s`
num_dev=`grep bmi160  /sys/bus/iio/devices/iio\:device*/name | wc -l`
name_prefix="${uid}_${time}"
echo $name_prefix
/root/libimu/scripts/both_audio.sh $name_prefix
/root/libimu/scripts/imu_logging.sh $num_dev $name_prefix
