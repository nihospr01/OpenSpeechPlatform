#!/bin/bash

algo=OWF_car_field_v7_fmexg_impl1_algo.sea
data=OWF_car_field_v7_fmexg_impl1_data.sed

uname -a | grep -q "4.14"
if [ $? -eq 0 ]; then
	linux414=true;
else
	linux414=false;
fi;

cd /opt/bin/fpga_configuration

printf "configuring fpga...\n"

if [ "$linux414" = true ]; then
	./export-fpga-gpio.sh
fi

#printf "set program pin\n"
if [ "$linux414" = true ]; then
	echo 1 > /sys/class/gpio/gpio6/value
else
	gpioset 0 6=1
fi

sleep 0.2

#printf "release program pin\n"
if [ "$linux414" = true ]; then
	echo 0 > /sys/class/gpio/gpio6/value
else
	gpioset 0 6=0
fi

sleep 0.5

#printf "launch sspiem\n"
if [ "$linux414" = true ]; then
	./sspiem /dev/spidev3.0 $algo $data > log.txt
else
	./sspiem /dev/spidev2.0 $algo $data > log.txt
fi

printf "done configuring fpga\n"
