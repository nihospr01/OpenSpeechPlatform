#!/bin/bash

algo=OWF_car_field_v7_fmexg_impl1_algo.sea
data=OWF_car_field_v7_fmexg_impl1_data.sed

cd /opt/bin/fpga_configuration

printf "configuring fpga...\n"

./export-fpga-gpio.sh

#printf "set program pin\n"
echo 1 > /sys/class/gpio/gpio6/value

sleep 0.2

#printf "release program pin\n"
echo 0 > /sys/class/gpio/gpio6/value

sleep 0.5

#printf "launch sspiem\n"
./sspiem /dev/spidev3.0 $algo $data > log.txt

printf "done configuring fpga\n"
