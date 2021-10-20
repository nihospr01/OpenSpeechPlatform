#ifndef IMU_SENSOR_H_
#define IMU_SENSOR_H_

#include "../BMI160_driver/osp_bmi160.h"
#include "../BMI270-Sensor-API/bmi270.h"
#include "../BMI270-Sensor-API/examples/bmi270/common/common.h"
#include "gesture_detection.hpp"
#include <chrono>
#include <ctime>
#include <stdio.h>
#include <string>
#include <unistd.h>

/////////////////////////////////
// ImuSensor Classes
/////////////////////////////////

class ImuSensor {
  protected:
    std::string imu_type; // "PCD", "LEFT", "RIGHT"
    std::string filename; // log filename
    int rate;             // sampling frequency
    int period_us;        // sampling period in microseconds
    bool isPrint;         // Print to terminal?
    bool isQuiet;         // Don't print or log
    bool isTap;           // Should tap detection be enabled?
    bool isSpin;          // Should spin detection be enabled?
    bool isFlip;          // Should flip detection be enabled?

    float acc_x = 0, acc_y = 0, acc_z = 0, gyr_x = 0, gyr_y = 0, gyr_z = 0;
    std::chrono::high_resolution_clock::time_point t_start, t_end, t_print_start, t_print_end;
    FILE *outfp;

    // Gesture Detection
    TapDetection tap_detection;
    SpinDetection spin_detection;
    FlipDetection flip_detection;

    virtual void sample(void){};
    void initializeLogFile(std::string filename);

  public:
    ImuSensor(std::string imu_type, std::string filename, int rate, bool isPrint, bool isQuiet, bool isTap, bool isSpin,
              bool isFlip);
    virtual ~ImuSensor();
    void start();
};

class ImuSensor270 : public ImuSensor {
  private:
    /* BMI270 initialization */
    struct bmi2_sensor_data sensor_data[2] = {{0}}; // Create an instance of sensor data structure.
    struct bmi2_dev bmi2_dev;

    void sample(void);
    int8_t initialize(void);

  public:
    ImuSensor270(std::string imu_type, std::string filename, int rate, bool isPrint, bool isQuiet, bool isTap,
                 bool isSpin, bool isFlip);
    ~ImuSensor270();
};

class ImuSensor160 : public ImuSensor {
  private:
    /* BMI160 initialization */
    struct bmi160_dev bmi160dev;
    struct bmi160_sensor_data bmi160_accel;
    struct bmi160_sensor_data bmi160_gyro;

    void sample(void);
    void initializeLogFile(std::string filename);
    int8_t initialize(void);

  public:
    ImuSensor160(std::string imu_type, std::string filename, int rate, bool isPrint, bool isQuiet, bool isTap,
                 bool isSpin, bool isFlip);
    ~ImuSensor160();
};

// factory func to return the correct IMU object
ImuSensor *find_imu(std::string imu_type, std::string filename, int rate, bool isPrint, bool isQuiet, bool isTap,
                    bool isSpin, bool isFlip);

std::string get_time_string(void);
float lsb_to_dps(int16_t val, float dps);
float lsb_to_mps2(int16_t val, float g_range);

#endif // IMU_SENSOR_H_