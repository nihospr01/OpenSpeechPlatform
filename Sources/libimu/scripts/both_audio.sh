#!/bin/bash
# echo $1
#nohup taskset --cpu-list 1 arecord -D plughw:0,1 -r 48000 -f S16_LE -d 0 -c 2 /opt/audio_data/$1_twochan_mics.wav > /opt/recordings/audio_log2.log &
nohup taskset --cpu-list 2 arecord -D plughw:0,2 -r 48000 -f S16_LE -d 0  "/opt/audio_data/$(date +%Y-%m-%d_%H-%M-%S)_case_mics.wav" > /opt/recordings/$(date +%Y-%m-%d_%H-%M-%S)_audio_log.log &
