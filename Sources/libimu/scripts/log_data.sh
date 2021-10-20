#!/bin/bash
# Make directories
mkdir -p /opt/audio_data

# Log audio data
nohup taskset --cpu-list 2 arecord -D plughw:0,2 -r 48000 -f S16_LE -d 0  "/opt/audio_data/$(date +%Y-%m-%d_%H-%M-%S)_case_mics.wav" > /opt/recordings/$(date +%Y-%m-%d_%H-%M-%S)_audio_log.log &

# Log IMU data
log_imu -i PCD -c 1 &
log_imu -i LEFT -c 2 &
log_imu -i RIGHT -c 3 &