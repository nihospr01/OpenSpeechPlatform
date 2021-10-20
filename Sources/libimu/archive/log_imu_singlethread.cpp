/**
  * Copyright (c) 2020 Bosch Sensortec GmbH. All rights reserved.
  * SPDX-License-Identifier: BSD-3-Clause
  *
  * Author:    Anshuman Dewangan <adewangan@ucsd.edu>
  * Created:   05/02/2021
  *
  **/

#include <stdio.h>
#include "BMI270-Sensor-API/bmi270.h"
#include "BMI270-Sensor-API/examples/bmi270/common/common.h"
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <fstream>
#include <ctime>
#include <chrono>


#define GRAVITY_EARTH  (9.80665f)
#define ACCEL          UINT8_C(0x00)
#define GYRO           UINT8_C(0x01)
#define NUM_IMU        3


/////////////////////////////////
// Helper Functions
/////////////////////////////////

/*
 *  @brief This function converts lsb to meter per second squared for 16 bit accelerometer at
 *  range 2G, 4G, 8G or 16G.
 *
 *  @param[in] val       : LSB from each axis.
 *  @param[in] g_range   : Gravity range.
 *  @param[in] bit_width : Resolution for accel.
 *
 *  @return Gravity.
 */
 static float lsb_to_mps2(int16_t val, float g_range, uint8_t bit_width)
 {
     float half_scale = ((float)(1 << bit_width) / 2.0f);

     return (GRAVITY_EARTH * val * g_range) / half_scale;
 }

/*
 *  @brief This function converts lsb to degree per second for 16 bit gyro at
 *  range 125, 250, 500, 1000 or 2000dps.
 *
 *  @param[in] val       : LSB from each axis.
 *  @param[in] dps       : Degree per second.
 *  @param[in] bit_width : Resolution for gyro.
 *
 *  @return Degree per second.
 */
 static float lsb_to_dps(int16_t val, float dps, uint8_t bit_width)
 {
     float half_scale = ((float)(1 << bit_width) / 2.0f);

     return (dps / ((half_scale) + BMI2_GYR_RANGE_2000)) * (val);
 }

std::string get_time_string() {
  /* Purpose: returns properly formatted string of current time
   *
   * Returns:
   *   - time_str (string): current time formatted as %Y-%m-%d_%H-%M-%S
  */
  time_t rawtime;
  struct tm * timeinfo;
  char buffer[80];
  time (&rawtime);

  timeinfo = localtime(&rawtime);
  strftime(buffer,sizeof(buffer),"%Y-%m-%d_%H-%M-%S",timeinfo);
  std::string time_str(buffer);

  return time_str;
}

/////////////////////////////////
// Configuration Functions
/////////////////////////////////

/*
 *  @brief This internal API is used to set configurations for accel.
 *
 *  @param[in] dev       : Structure instance of bmi2_dev.
 *
 *  @return Status of execution.
 */
 static int8_t set_accel_gyro_config(struct bmi2_dev *bmi2_dev)
 {
     // Status of api are returned to this variable. */
     int8_t rslt;

     // Structure to define accelerometer and gyro configuration. */
     struct bmi2_sens_config config[2];

     // Configure the type of feature. */
     config[ACCEL].type = BMI2_ACCEL;
     config[GYRO].type = BMI2_GYRO;

     // Get default configurations for the type of feature selected. */
     rslt = bmi270_get_sensor_config(config, 2, bmi2_dev);
     bmi2_error_codes_print_result(rslt);

     // Map data ready interrupt to interrupt pin. */
     rslt = bmi2_map_data_int(BMI2_DRDY_INT, BMI2_INT1, bmi2_dev);
     bmi2_error_codes_print_result(rslt);

     if (rslt == BMI2_OK)
     {
         // NOTE: The user can change the following configuration parameters according to their requirement. */
         // Set Output Data Rate */
         config[ACCEL].cfg.acc.odr = BMI2_ACC_ODR_50HZ;

         // Gravity range of the sensor (+/- 2G, 4G, 8G, 16G). */
         config[ACCEL].cfg.acc.range = BMI2_ACC_RANGE_2G;

         /* The bandwidth parameter is used to configure the number of sensor samples that are averaged
          * if it is set to 2, then 2^(bandwidth parameter) samples
          * are averaged, resulting in 4 averaged samples.
          * Note1 : For more information, refer the datasheet.
          * Note2 : A higher number of averaged samples will result in a lower noise level of the signal, but
          * this has an adverse effect on the power consumed.
          */
         config[ACCEL].cfg.acc.bwp = BMI2_ACC_NORMAL_AVG4;

         /* Enable the filter performance mode where averaging of samples
          * will be done based on above set bandwidth and ODR.
          * There are two modes
          *  0 -> Ultra low power mode
          *  1 -> High performance mode(Default)
          * For more info refer datasheet.
          */
         config[ACCEL].cfg.acc.filter_perf = BMI2_PERF_OPT_MODE;

         // The user can change the following configuration parameters according to their requirement. */
         // Set Output Data Rate */
         config[GYRO].cfg.gyr.odr = BMI2_GYR_ODR_50HZ;

         // Gyroscope Angular Rate Measurement Range.By default the range is 2000dps. */
         config[GYRO].cfg.gyr.range = BMI2_GYR_RANGE_2000;

         // Gyroscope bandwidth parameters. By default the gyro bandwidth is in normal mode. */
         config[GYRO].cfg.gyr.bwp = BMI2_GYR_NORMAL_MODE;

         /* Enable/Disable the noise performance mode for precision yaw rate sensing
          * There are two modes
          *  0 -> Ultra low power mode(Default)
          *  1 -> High performance mode
          */
         config[GYRO].cfg.gyr.noise_perf = BMI2_POWER_OPT_MODE;

         /* Enable/Disable the filter performance mode where averaging of samples
          * will be done based on above set bandwidth and ODR.
          * There are two modes
          *  0 -> Ultra low power mode
          *  1 -> High performance mode(Default)
          */
         config[GYRO].cfg.gyr.filter_perf = BMI2_PERF_OPT_MODE;

         // Set the accel and gyro configurations. */
         rslt = bmi270_set_sensor_config(config, 2, bmi2_dev);
         bmi2_error_codes_print_result(rslt);
     }

     return rslt;
 }

int8_t exit_sequence(int8_t rslt, struct bmi2_dev bmi2_dev[NUM_IMU]) {
  printf("Exit sequence initiated");

  for (int i=0; i < NUM_IMU; i++){
    bmi2_deinit(&bmi2_dev[i]);
  }

  return rslt;
}

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
    float x = 0, y = 0, z = 0;

    // Initialize other variables
    std::chrono::high_resolution_clock::time_point t_start[NUM_IMU], t_end[NUM_IMU];
    double latency[NUM_IMU];
    std::string temp_str;

    // Initialize output file
    std::string out_file_name = "/opt/imu_data/" + get_time_string() + "_imu_data_50HZ.csv";
    std::ofstream out_file (out_file_name);
    // Write header of csv
    out_file << "pcd_latency,pcd_acc_x,pcd_acc_y,pcd_acc_z,pcd_gyr_x,pcd_gyr_y,pcd_gyr_z,left_latency,left_acc_x,left_acc_y,left_acc_z,left_gyr_x,left_gyr_y,left_gyr_z,right_latency,right_acc_x,right_acc_y,right_acc_z,right_gyr_x,right_gyr_y,right_gyr_z" << "\n";

    /* Sensor initialization configuration
     * 0: PCD IMU
     * 1: Left BTE-RIC IMU
     * 2: Right BTE-RIC IMU
     */
    struct bmi2_dev bmi2_dev[NUM_IMU];
    const char *imu_names[NUM_IMU] = {"/dev/spidev1.1", "/dev/spidev0.1", "/dev/spidev1.2"};

    for (int i=0; i < NUM_IMU; i++){
      /* Interface reference is given as a parameter
       * For I2C : BMI2_I2C_INTF
       * For SPI : BMI2_SPI_INTF
       */
      printf("\n== Initializing IMU #%i ==\n",i);
      rslt = bmi2_interface_init(&bmi2_dev[i], imu_names[i]); // PCD IMU
      bmi2_error_codes_print_result(rslt);
      if (rslt != BMI2_OK) {return exit_sequence(rslt, bmi2_dev);}

      // Initialize bmi270. */
      rslt = bmi270_init(&bmi2_dev[i]);
      bmi2_error_codes_print_result(rslt);
      if (rslt != BMI2_OK) {return exit_sequence(rslt, bmi2_dev);}

      // disable Advanced Power Save, which slows down polling
      bmi2_dev[i].aps_status = 0;

      // Enable the accel and gyro sensor
      rslt = bmi270_sensor_enable(sensor_list, 2, &bmi2_dev[i]);
      bmi2_error_codes_print_result(rslt);
      if (rslt != BMI2_OK) {return exit_sequence(rslt, bmi2_dev);}

      // Accel and gyro configuration settings
      rslt = set_accel_gyro_config(&bmi2_dev[i]);
      bmi2_error_codes_print_result(rslt);
      if (rslt != BMI2_OK) {return exit_sequence(rslt, bmi2_dev);}

      // Assign accel and gyro sensor
      sensor_data[ACCEL].type = BMI2_ACCEL;
      sensor_data[GYRO].type = BMI2_GYRO;

      // Initialize time start for latency logging
      t_start[i] = std::chrono::high_resolution_clock::now();
    }

    printf("\n== Logging to %s...\n", out_file_name.c_str());

    while (true) {
      for (int i=0; i < NUM_IMU; i++){
        // To get the data ready interrupt status of accel and gyro
        rslt = bmi2_get_int_status(&int_status, &bmi2_dev[i]);
        bmi2_error_codes_print_result(rslt);
        if (rslt != BMI2_OK) {return exit_sequence(rslt, bmi2_dev);}

        // To check the data ready interrupt status
        if ((int_status & BMI2_ACC_DRDY_INT_MASK) && (int_status & BMI2_GYR_DRDY_INT_MASK))
        {
            // Log latency
            t_end[i] = std::chrono::high_resolution_clock::now();
            latency[i] = std::chrono::duration<double, std::milli>(t_end[i]-t_start[i]).count();
            out_file << std::to_string(latency[i]);
            t_start[i] = t_end[i];

            // Get accel and gyro data for x, y and z axis
            rslt = bmi270_get_sensor_data(sensor_data, 2, &bmi2_dev[i]);
            bmi2_error_codes_print_result(rslt);

            // Converting lsb to meter per second squared for 16 bit accelerometer at 2G range
            x = lsb_to_mps2(sensor_data[ACCEL].sens_data.acc.x, 2, bmi2_dev[i].resolution);
            y = lsb_to_mps2(sensor_data[ACCEL].sens_data.acc.y, 2, bmi2_dev[i].resolution);
            z = lsb_to_mps2(sensor_data[ACCEL].sens_data.acc.z, 2, bmi2_dev[i].resolution);

            temp_str = std::string(",") + std::to_string(x) + std::string(",") + std::to_string(y) + std::string(",") + std::to_string(z);
            out_file << temp_str;

            // Converting lsb to degree per second for 16 bit gyro at 2000dps range
            x = lsb_to_dps(sensor_data[GYRO].sens_data.gyr.x, 2000, bmi2_dev[i].resolution);
            y = lsb_to_dps(sensor_data[GYRO].sens_data.gyr.y, 2000, bmi2_dev[i].resolution);
            z = lsb_to_dps(sensor_data[GYRO].sens_data.gyr.z, 2000, bmi2_dev[i].resolution);

            temp_str = std::string(",") + std::to_string(x) + std::string(",") + std::to_string(y) + std::string(",") + std::to_string(z);
            out_file << temp_str;

            if (i == NUM_IMU-1) {
              out_file << "\n";
            } else {
              out_file << ",";
            }
        }
      }
    }

    return exit_sequence(rslt, bmi2_dev);
}
