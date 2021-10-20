// Modified for OSP PCD using SPI interface

#include "osp_bmi160.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    int times_to_read = 0;
   
    init_bmi160(&bmi160dev, "/dev/spidev1.1");

    while (times_to_read++ < 100) {
        /* To read both Accel and Gyro data */
        bmi160_get_sensor_data((BMI160_ACCEL_SEL | BMI160_GYRO_SEL), &bmi160_accel, &bmi160_gyro, &bmi160dev);

        printf("ax:%d\tay:%d\taz:%d\n", bmi160_accel.x, bmi160_accel.y, bmi160_accel.z);
        printf("gx:%d\tgy:%d\tgz:%d\n", bmi160_gyro.x, bmi160_gyro.y, bmi160_gyro.z);
        fflush(stdout);

        osp_delay_msec(10);
    }

    return EXIT_SUCCESS;
}
