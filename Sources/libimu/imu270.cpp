#include "include/imu_sensor.hpp"
#include <chrono>
#include <ctime>
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define GRAVITY_EARTH (9.80665f)
#define ACCEL UINT8_C(0x00)
#define GYRO UINT8_C(0x01)

/*
 *  @brief This internal API is used to set configurations for accel.
 *
 *  @param[in] dev       : Structure instance of bmi2_dev.
 *
 *  @return Status of execution.
 */
static int8_t set_accel_gyro_config(struct bmi2_dev *bmi2_dev) {
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
    //  rslt = bmi2_map_data_int(BMI2_DRDY_INT, BMI2_INT1, bmi2_dev);
    //  bmi2_error_codes_print_result(rslt);

    if (rslt == BMI2_OK) {
        // NOTE: The user can change the following configuration parameters according to their requirement. */
        // Set Output Data Rate */
        config[ACCEL].cfg.acc.odr = BMI2_ACC_ODR_200HZ;

        // Gravity range of the sensor (+/- 2G, 4G, 8G, 16G). */
        config[ACCEL].cfg.acc.range = BMI2_ACC_RANGE_4G;

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
        config[GYRO].cfg.gyr.odr = BMI2_GYR_ODR_200HZ;

        // Gyroscope Angular Rate Measurement Range.By default the range is 2000dps. */
        // #define BMI2_GYR_RANGE_2000                       UINT8_C(0x00)
        // #define BMI2_GYR_RANGE_1000                       UINT8_C(0x01)
        // #define BMI2_GYR_RANGE_500                        UINT8_C(0x02)
        // #define BMI2_GYR_RANGE_250                        UINT8_C(0x03)
        // #define BMI2_GYR_RANGE_125                        UINT8_C(0x04)
        config[GYRO].cfg.gyr.range = BMI2_GYR_RANGE_2000;

        // Gyroscope bandwidth parameters. By default the gyro bandwidth is in normal mode. */
        config[GYRO].cfg.gyr.bwp = BMI2_GYR_NORMAL_MODE;

        /* Enable/Disable the noise performance mode for precision yaw rate sensing
         * There are two modes
         *  0 -> Ultra low power mode(Default)
         *  1 -> High performance mode
         */
        config[GYRO].cfg.gyr.noise_perf = BMI2_PERF_OPT_MODE;

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

int8_t ImuSensor270::initialize() {
    printf("\n== Initializing %s IMU ==\n", imu_type.c_str());
    int8_t rslt;
    uint8_t sensor_list[2] = {BMI2_ACCEL, BMI2_GYRO}; // Assign accel and gyro sensor to variable.

    if (imu_type == "PCD")
        rslt = bmi2_interface_init(&bmi2_dev, "/dev/spidev1.1");
    else if (imu_type == "LEFT")
        rslt = bmi2_interface_init(&bmi2_dev, "/dev/spidev0.1");
    else if (imu_type == "RIGHT")
        rslt = bmi2_interface_init(&bmi2_dev, "/dev/spidev1.2");
    else {
        fprintf(stderr, "Unable to parse imu_type\n");
        return 1;
    }

    if (rslt != BMI2_OK) {
        // if (rslt == (int8_t)0xD1)
        //     printf("BMI160 detected\n");
        return rslt;
    }

    // Initialize bmi270. */
    rslt = bmi270_init(&bmi2_dev);
    if (rslt != BMI2_OK) {
        bmi2_error_codes_print_result(rslt);
        return rslt;
    }

    // disable Advanced Power Save, which slows down polling
    bmi2_dev.aps_status = 0;

    // Enable the accel and gyro sensor
    rslt = bmi270_sensor_enable(sensor_list, 2, &bmi2_dev);
    if (rslt != BMI2_OK) {
        bmi2_error_codes_print_result(rslt);
        return rslt;
    }

    // Accel and gyro configuration settings
    rslt = set_accel_gyro_config(&bmi2_dev);
    if (rslt != BMI2_OK) {
        bmi2_error_codes_print_result(rslt);
        return rslt;
    }

    return rslt;
}

ImuSensor270::ImuSensor270(std::string imu_type, std::string filename, int rate, bool isPrint, bool isQuiet, bool isTap,
                           bool isSpin, bool isFlip)
    : ImuSensor(imu_type, filename, rate, isPrint, isQuiet, isTap, isSpin, isFlip) {

    printf("BMI270 Initialize\n");

    if (initialize() != 0) {
        throw int();
    }

    // Assign accel and gyro sensor
    sensor_data[ACCEL].type = BMI2_ACCEL;
    sensor_data[GYRO].type = BMI2_GYRO;

    if (isPrint == 0)
        initializeLogFile(filename);
}

ImuSensor270::~ImuSensor270() { bmi2_deinit(&bmi2_dev); }

void ImuSensor270::sample() {
    // Get accel and gyro data for x, y and z axis
    bmi270_get_sensor_data(sensor_data, 2, &bmi2_dev);

    // Converting lsb to meter per second squared for 16 bit accelerometer at 2G range
    acc_x = lsb_to_mps2(sensor_data[ACCEL].sens_data.acc.x, 4);
    acc_y = lsb_to_mps2(sensor_data[ACCEL].sens_data.acc.y, 4);
    acc_z = lsb_to_mps2(sensor_data[ACCEL].sens_data.acc.z, 4);

    // Converting lsb to degree per second for 16 bit gyro at 2000dps range
    gyr_x = lsb_to_dps(sensor_data[GYRO].sens_data.gyr.x, 2000);
    gyr_y = lsb_to_dps(sensor_data[GYRO].sens_data.gyr.y, 2000);
    gyr_z = lsb_to_dps(sensor_data[GYRO].sens_data.gyr.z, 2000);

    // t_print_end = std::chrono::high_resolution_clock::now();
    // float latency = std::chrono::duration<float, std::milli>(t_print_end - t_print_start).count();
    // t_print_start = t_print_end;
    // fprintf(outfp, "%.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f\n", latency, acc_x, acc_y, acc_z, gyr_x, gyr_y, gyr_z);
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
