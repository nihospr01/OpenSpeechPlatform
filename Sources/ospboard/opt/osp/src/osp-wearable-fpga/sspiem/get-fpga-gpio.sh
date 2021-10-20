#!/bin/bash

date +"%T.%3N"
printf "INIT "
cat /sys/class/gpio/gpio29/value
printf "DONE "
cat /sys/class/gpio/gpio30/value
printf "PRGM "
cat /sys/class/gpio/gpio6/value
