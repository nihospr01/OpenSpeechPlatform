#include "../BMI160_driver/osp_bmi160.h"
#include "include/imu_sensor.hpp"
#include <chrono>
#include <ctime>
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

ImuSensor160::ImuSensor160(std::string imu_type, std::string filename, int rate, bool isPrint, bool isQuiet, bool isTap,
                           bool isSpin, bool isFlip)
    : ImuSensor(imu_type, filename, rate, isPrint, isQuiet, isTap, isSpin, isFlip) {

    printf("BMI160 Initialize\n");

    if (init_bmi160(&bmi160dev, "/dev/spidev1.1") != 0)
        throw int();
}

ImuSensor160::~ImuSensor160() {}

// Note the negative signs on some computations.  This is because the BMI160 in
// the V8 PCD is orientated differently that the BMI270 in the V9
void ImuSensor160::sample() {
    /* To read both Accel and Gyro data */
    bmi160_get_sensor_data((BMI160_ACCEL_SEL | BMI160_GYRO_SEL), &bmi160_accel, &bmi160_gyro, &bmi160dev);

    // Converting lsb to meter per second squared for 16 bit accelerometer at 2G range
    acc_x = lsb_to_mps2(bmi160_accel.x, 4);
    acc_y = -lsb_to_mps2(bmi160_accel.y, 4);
    acc_z = -lsb_to_mps2(bmi160_accel.z, 4);

    // Converting lsb to degree per second for 16 bit gyro at 2000dps range
    gyr_x = lsb_to_dps(bmi160_gyro.x, 2000);
    gyr_y = -lsb_to_dps(bmi160_gyro.y, 2000);
    gyr_z = -lsb_to_dps(bmi160_gyro.z, 2000);

    if (!isQuiet)
        fprintf(outfp, "%.2f, %.2f, %.2f, %.2f, %.2f, %.2f\n", acc_x, acc_y, acc_z, gyr_x, gyr_y, gyr_z);

    if (isTap)
        tap_detection.forward(acc_x, acc_y, acc_z);
    if (isSpin)
        spin_detection.forward(gyr_y);
    if (isFlip)
        flip_detection.forward(gyr_x);

    // compute necessary delay
    t_end = std::chrono::high_resolution_clock::now();
    float time_us = std::chrono::duration<float, std::micro>(t_end - t_start).count();
    int32_t delay_us = (int32_t)period_us - (int32_t)time_us;
    // printf("time_us=%f delay=%d\n", time_us, delay_us);
    if (delay_us > 0)
        bmi2_delay_us(delay_us);
    t_start = std::chrono::high_resolution_clock::now();
}
