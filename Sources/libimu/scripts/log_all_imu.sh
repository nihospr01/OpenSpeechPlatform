#!/bin/bash

log_imu -i PCD -c 1 &
log_imu -i LEFT -c 2 &
log_imu -i RIGHT -c 3 &

# to monitor CPU usage:
# > ps_imu
# CPUID CLS PRI %CPU   LWP COMMAND
# 1  FF  50  0.2 18357 IMU
# 2  FF  50  0.2 18609 IMU
# 3  FF  50  0.2 18748 IMU