#include "include/imu_sensor.hpp"
#include <chrono>
#include <ctime>
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define GRAVITY_EARTH (9.80665f)

// IMU Common Functions for log_imu

/*
 *  @brief This function converts lsb to meter per second squared for 16 bit accelerometer at
 *  range 2G, 4G, 8G or 16G.
 *
 *  @param[in] val       : LSB from each axis.
 *  @param[in] g_range   : Gravity range.
 *
 *  @return Gravity.
 */
float lsb_to_mps2(int16_t val, float g_range) { return (GRAVITY_EARTH * val * g_range) / 32768.0f; }

/*
 *  @brief This function converts lsb to degree per second for 16 bit gyro at
 *  range 125, 250, 500, 1000 or 2000dps.
 *
 *  @param[in] val       : LSB from each axis.
 *  @param[in] dps       : Degree per second.
 *
 *  @return Degree per second.
 */
float lsb_to_dps(int16_t val, float dps) { return (dps / (32768.0f + BMI2_GYR_RANGE_2000)) * val; }

std::string get_time_string() {
    /* Purpose: returns properly formatted string of current time
     *
     * Returns:
     *   - time_str (string): current time formatted as %Y-%m-%d_%H-%M-%S
     */
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[80];
    time(&rawtime);

    timeinfo = localtime(&rawtime);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H-%M-%S", timeinfo);
    std::string time_str(buffer);

    return time_str;
}

// IMUSensor Base Class

ImuSensor::ImuSensor(std::string imu_type, std::string filename, int rate, bool isPrint, bool isQuiet, bool isTap,
                     bool isSpin, bool isFlip)
    : imu_type(imu_type), filename(filename), rate(rate), isPrint(isPrint), isQuiet(isQuiet), isTap(isTap),
      isSpin(isSpin), isFlip(isFlip) {

    period_us = (int)1000000 / rate;

    if (isTap)
        printf("\n Tap detection enabled.\n");
    if (isSpin)
        printf("\n Spin detection enabled.\n");
    if (isFlip)
        printf("\n Flip detection enabled.\n");
}

ImuSensor::~ImuSensor() {}

void ImuSensor::initializeLogFile(std::string filename) {
    char buf[256];
    /* Purpose: Initializes the CSV file to log data to */
    if (filename == "")
        filename =
            "/opt/imu_data/" + get_time_string() + "_" + imu_type + "_imu_data_" + std::to_string(rate) + "HZ.csv";

    char *dname = strdup(filename.c_str());
    sprintf(buf, "mkdir -p %s", dirname(dname));
    int res = system(buf);
    free(dname);
    if (res)
        exit(res);

    outfp = fopen(filename.c_str(), "w");
    if (outfp == NULL) {
        perror(filename.c_str());
        fprintf(stderr, "Could not open file \"%s\"\n", filename.c_str());
        exit(errno);
    }
    printf("\n== Logging to %s\n", filename.c_str());
}

void ImuSensor::start() {
    if (isPrint)
        outfp = stdout;
    else if (!isQuiet)
        initializeLogFile(filename);

    fflush(stdout);

    if (!isQuiet)
        fprintf(outfp, "acc_x, acc_y, acc_z, gyr_x, gyr_y, gyr_z\n");

    /* Purpose: Loops through the forward step indefinitely */
    t_start = std::chrono::high_resolution_clock::now();
    t_print_start = t_start;
    while (true) {
        sample();
    }
}

ImuSensor *find_imu(std::string imu_type, std::string filename, int rate, bool isPrint, bool isQuiet, bool isTap,
                    bool isSpin, bool isFlip) {
    try {
        return new ImuSensor270(imu_type, filename, rate, isPrint, isQuiet, isTap, isSpin, isFlip);
    } catch (int e) {
        fprintf(stderr, "Failed to Open BMI270 IMU\n");
    }
    try {
        return new ImuSensor160(imu_type, filename, rate, isPrint, isQuiet, isTap, isSpin, isFlip);
    } catch (int e) {
        fprintf(stderr, "Failed to Open BMI160 IMU\n");
    }
    return NULL;
}