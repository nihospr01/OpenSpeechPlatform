/**
 * Copyright (c) 2020 Bosch Sensortec GmbH. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Author:    Anshuman Dewangan <adewangan@ucsd.edu>
 * Created:   05/02/2021
 *
 **/

#include "include/imu_sensor.hpp"
#include "include/tclap/CmdLine.h"
#include <iostream>
#include <signal.h>
#include <string.h>
#include <thread>

ImuSensor *imuSensor = NULL;

// cleans up on exit
void handle_signal(int sig) {
    if (imuSensor)
        delete imuSensor;
    exit(0);
}

/**
 * @brief Set thread name, affinity and priority
 *
 * Only works for Linux.
 *
 * @param thread_name If not NULL, will set the name of the current thread.
 * @param cpu_num If > 0, will set the cpu affinity. Should be in range of 0 to 3.
 * @param policy If > 0 will set the scheduling policy
 * @param prio priority
 *
 * @note If there are at least 6 cpus, the affinity will be
 * set to 2*cpu_num, to avoid scheduling processes in the same logical core.
 */

void set_cpu_pri(const char *thread_name, int cpu_num, int policy, int prio) {
#ifdef __linux__
    int rc;

    if (thread_name != NULL) {
        rc = pthread_setname_np(pthread_self(), thread_name);
        if (rc != 0)
            std::cerr << "pthread_setname_np failed" << std::endl;
    }

    if (cpu_num >= 0) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);

        unsigned num_cpus = std::thread::hardware_concurrency();
        if (num_cpus < 6)
            CPU_SET(cpu_num, &cpuset);
        else
            CPU_SET(2 * cpu_num, &cpuset); // skip odd because hyperthreading

        rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
        if (rc != 0) {
            std::cerr << "Error calling pthread_setaffinity_np: " << rc << std::endl;
        }
    }

    if (policy >= 0) {
        struct sched_param param;

        if (policy == SCHED_OTHER && prio != 0)
            std::cerr << "SCHED_OTHER only supports priority = 0" << std::endl;

        param.sched_priority = prio;
        if (pthread_setschedparam(pthread_self(), policy, &param) != 0)
            std::cerr << __func__ << " : pthread_setschedparam failed" << std::endl;
    }
#endif // __linux__
}

using namespace TCLAP;

int main(int argc, char *argv[]) {

    CmdLine cmd("Log IMU activity", ' ', "0.1");

    // set typeArg
    std::vector<std::string> imu_types = {"PCD", "LEFT", "RIGHT"};
    ValuesConstraint<std::string> allowedTypes(imu_types);
    ValueArg<std::string> typeArg("i", "imu", "IMU", true, "PCD", &allowedTypes);
    cmd.add(typeArg);

    ValueArg<std::string> fileArg("f", "filename", "Filename for output", false, "", "filename");
    cmd.add(fileArg);

    SwitchArg printSwitch("p", "print", "Print IMU data on command line", false);
    cmd.add(printSwitch);

    SwitchArg tapSwitch("T", "Tap", "Enables tap detection for toggling beamforming", false);
    cmd.add(tapSwitch);
    SwitchArg spinSwitch("S", "Spin", "Enables spin detection for toggling beamforming", false);
    cmd.add(spinSwitch);
    SwitchArg flipSwitch("F", "Flip", "Enables flip detection for toggling beamforming", false);
    cmd.add(flipSwitch);

    ValueArg<int> rateArg("r", "rate", "Sampling rate in Hz [1-125] Default 100.", false, 100, "int");
    cmd.add(rateArg);

    ValueArg<int> cpuArg("c", "cpu", "Which CPU to use [0-3] Default 1.", false, 1, "int");
    cmd.add(cpuArg);

    SwitchArg quietSwitch("q", "quiet", "Don't log or print IMU data.", false);
    cmd.add(quietSwitch);

    cmd.parse(argc, argv);

    if (rateArg.getValue() < 1 || rateArg.getValue() > 125) {
        std::cerr << "Error: rate must be between 1 and 125" << std::endl;
        exit(1);
    }

    if (cpuArg.getValue() < 0 || cpuArg.getValue() > 3) {
        std::cerr << "Error: CPU must be between 0 and 3" << std::endl;
        exit(1);
    }

    if (quietSwitch.getValue() && !tapSwitch.getValue() && !spinSwitch.getValue() && !flipSwitch.getValue()) {
        std::cerr << "Quiet flag is enabled, but there are no activity flags enabled (Spin, Tap, Flip)." << std::endl;
        exit(1);
    }
    set_cpu_pri("IMU", cpuArg.getValue(), SCHED_FIFO, 10);

    // set up signal handler
    struct sigaction a;
    a.sa_handler = handle_signal;
    a.sa_flags = 0;
    sigemptyset(&a.sa_mask);
    sigaction(SIGINT, &a, NULL);
    sigaction(SIGHUP, &a, NULL);

    // Create ImuSensor object and start processing IMU data
    ImuSensor *imu =
        find_imu(typeArg.getValue(), fileArg.getValue(), rateArg.getValue(), printSwitch.getValue(),
                 quietSwitch.getValue(), tapSwitch.getValue(), spinSwitch.getValue(), flipSwitch.getValue());

    if (imu == NULL) {
        fprintf(stderr, "\nFailed to open any IMU device\n");
        return 1;
    }
    imu->start();
    return 0;
}

/**
log_imu -i PCD -r 5 -c 1 &
log_imu -i LEFT -r 5 -c 2 &
log_imu -i RIGHT -r 5 -c 3 &
> ps_imu
CPUID CLS PRI %CPU   LWP COMMAND
1  FF  50  0.2 18357 IMU
2  FF  50  0.2 18609 IMU
3  FF  50  0.2 18748 IMU
**/