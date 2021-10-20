#!/bin/bash
resize2fs /dev/mmcblk0p10
nmcli con up THELab
export DEBIAN_FRONTEND=noninteractive #Makes package installs not ask you questions
apt-mark hold linux-image-4.14.0-qcomlt-arm64
apt remove -y openjdk-10-jre-headless java-common
apt autoremove --purge -y && apt clean
apt update
apt upgrade -y
apt autoremove --purge -y && apt clean
apt autoclean
apt install -y alsa-tools alsa-utils autoconf automake cmake bash-completion screen
apt autoremove --purge -y && apt clean

