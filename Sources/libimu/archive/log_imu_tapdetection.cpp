/**
  * Copyright (c) 2020 Bosch Sensortec GmbH. All rights reserved.
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * Author:    Anshuman Dewangan <adewangan@ucsd.edu>
  * Created:   05/02/2021
  *
  **/

#include <iostream>
#include <string.h>
#include <fstream>
#include "../include/log_imu_helper.hpp"
#include "../include/tap_detection.hpp"




/////////////////////////////////
// Main Function
/////////////////////////////////

int main(void)
{
    // Status of api are returned to this variable.
    int8_t rslt;
    // Assign accel and gyro sensor to variable.
    uint8_t sensor_list[2] = { BMI2_ACCEL, BMI2_GYRO };
    // Create an instance of sensor data structure.
    struct bmi2_sensor_data sensor_data[2] = { { 0 } };

    // Initialize the interrupt status of accel and gyro.
    uint16_t int_status = 0;
    // Initialize accelerometer / gyroscope values
    float acc_x = 0, acc_y = 0, acc_z = 0, gyr_x = 0, gyr_y = 0, gyr_z = 0;

    // Initialize other variables
    std::chrono::high_resolution_clock::time_point t_start, t_end;
    double latency;
    std::string temp_str;

    // Initialize output file
    std::string out_file_name = "/opt/imu_data/" + get_time_string() + "_PCD_imu_data_100HZ.csv";
    std::ofstream out_file (out_file_name);
    // Write header of csv
    out_file << "latency,acc_x,acc_y,acc_z,gyr_x,gyr_y,gyr_z" << "\n";

    /* Sensor initialization configuration
     * 0: PCD IMU
     * 1: Left BTE-RIC IMU
     * 2: Right BTE-RIC IMU
     */
    struct bmi2_dev bmi2_dev;

    /* Interface reference is given as a parameter
     * For I2C : BMI2_I2C_INTF
     * For SPI : BMI2_SPI_INTF
     */
    printf("\n== Initializing PCD IMU ==\n");
    rslt = bmi2_interface_init(&bmi2_dev, "/dev/spidev1.1");
    bmi2_error_codes_print_result(rslt);
    if (rslt != BMI2_OK) {return exit_sequence(rslt, bmi2_dev);}

    // Initialize bmi270. */
    rslt = bmi270_init(&bmi2_dev);
    bmi2_error_codes_print_result(rslt);
    if (rslt != BMI2_OK) {return exit_sequence(rslt, bmi2_dev);}

    // disable Advanced Power Save, which slows down polling
    bmi2_dev.aps_status = 0;

    // Enable the accel and gyro sensor
    rslt = bmi270_sensor_enable(sensor_list, 2, &bmi2_dev);
    bmi2_error_codes_print_result(rslt);
    if (rslt != BMI2_OK) {return exit_sequence(rslt, bmi2_dev);}

    // Accel and gyro configuration settings
    rslt = set_accel_gyro_config(&bmi2_dev);
    bmi2_error_codes_print_result(rslt);
    if (rslt != BMI2_OK) {return exit_sequence(rslt, bmi2_dev);}

    // Assign accel and gyro sensor
    sensor_data[ACCEL].type = BMI2_ACCEL;
    sensor_data[GYRO].type = BMI2_GYRO;

    // Initialize time start for latency logging
    t_start = std::chrono::high_resolution_clock::now();

    // Initialize tap detection
    int fs = 50;
  	float acc_thr = 11;
  	float da__dt_thr = 100;
  	float t_tap_low = 200; // msec
  	float t_tap_high = 400; // msec
    std::vector<float> data;

    TapDetection tap_det(fs, acc_thr, da__dt_thr, t_tap_low, t_tap_high);

    printf("\n== Logging to %s...\n", out_file_name.c_str());

    while (true) {
      // To get the data ready interrupt status of accel and gyro
      rslt = bmi2_get_int_status(&int_status, &bmi2_dev);
      bmi2_error_codes_print_result(rslt);
      if (rslt != BMI2_OK) {return exit_sequence(rslt, bmi2_dev);}

      // To check the data ready interrupt status
      if ((int_status & BMI2_ACC_DRDY_INT_MASK) && (int_status & BMI2_GYR_DRDY_INT_MASK))
      {
          // Log latency
          t_end = std::chrono::high_resolution_clock::now();
          latency = std::chrono::duration<double, std::milli>(t_end-t_start).count();
          out_file << std::to_string(latency);
          t_start = t_end;

          // Get accel and gyro data for x, y and z axis
          rslt = bmi270_get_sensor_data(sensor_data, 2, &bmi2_dev);
          bmi2_error_codes_print_result(rslt);

          // Converting lsb to meter per second squared for 16 bit accelerometer at 2G range
          acc_x = lsb_to_mps2(sensor_data[ACCEL].sens_data.acc.x, 2, bmi2_dev.resolution);
          acc_y = lsb_to_mps2(sensor_data[ACCEL].sens_data.acc.y, 2, bmi2_dev.resolution);
          acc_z = lsb_to_mps2(sensor_data[ACCEL].sens_data.acc.z, 2, bmi2_dev.resolution);

          temp_str = std::string(",") + std::to_string(acc_x) + std::string(",") + std::to_string(acc_y) + std::string(",") + std::to_string(acc_z);
          out_file << temp_str;

          // Converting lsb to degree per second for 16 bit gyro at 2000dps range
          gyr_x = lsb_to_dps(sensor_data[GYRO].sens_data.gyr.x, 2000, bmi2_dev.resolution);
          gyr_y = lsb_to_dps(sensor_data[GYRO].sens_data.gyr.y, 2000, bmi2_dev.resolution);
          gyr_z = lsb_to_dps(sensor_data[GYRO].sens_data.gyr.z, 2000, bmi2_dev.resolution);

          temp_str = std::string(",") + std::to_string(gyr_x) + std::string(",") + std::to_string(gyr_y) + std::string(",") + std::to_string(gyr_z);
          out_file << temp_str << "\n";

          // Detect tapping
          data = {acc_x, acc_y, acc_z, gyr_x, gyr_y, gyr_z};
          tap_det.forward(data);
      }
    }

    return exit_sequence(rslt, bmi2_dev);
}
