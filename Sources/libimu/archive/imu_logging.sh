#!/bin/bash
/root/libimu/init_trigger.sh
echo "* Number of connected devices: $1"
mkdir -p /root/libimu/build && cd /root/libimu/build
cmake ..
make
#echo $1
#echo $2
nohup taskset --cpu-list 3 ./refimu $1 fs all $2 > /opt/recordings/imu_log.log &
