#include "osp_bmi160.h"
#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <time.h>

int8_t osp_read_spi(uint8_t fd, uint8_t reg_addr, uint8_t *reg_data, uint16_t len) {
    struct spi_ioc_transfer xfer[2];
    memset(xfer, 0, sizeof xfer);
    reg_addr |= 0x80;

    xfer[0].tx_buf = (unsigned long)&reg_addr;
    xfer[0].len = 1;
    // xfer[0].cs_change = 0;
    xfer[1].rx_buf = (unsigned long)reg_data;
    xfer[1].len = len;
    // xfer[1].cs_change = 1;

    if (ioctl(fd, SPI_IOC_MESSAGE(2), xfer) < 1) {
        printf("spi read failed\n");
        return 1;
    }
    return 0;
}

int8_t osp_write_spi(uint8_t fd, uint8_t reg_addr, uint8_t *reg_data, uint16_t len) {
    struct spi_ioc_transfer xfer[2];
    memset(xfer, 0, sizeof xfer);
    reg_addr &= 0x7f;

    xfer[0].tx_buf = (unsigned long)&reg_addr;
    xfer[0].len = 1;
    // xfer[0].cs_change = 0;
    xfer[1].tx_buf = (unsigned long)reg_data;
    xfer[1].len = len;
    // xfer[1].cs_change = 1;

    if (ioctl(fd, SPI_IOC_MESSAGE(2), xfer) < 1) {
        printf("spi write failed\n");
        return 1;
    }
    return 0;
}

/*!
 *  @brief This API is used for introducing a delay in microseconds
 */
void osp_delay_usec(uint32_t delay_us) {
#if _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = delay_us * 1000;
    nanosleep(&ts, NULL);
#else
    usleep(period);
#endif
}

/*!
 *
 *  @brief This API is used for introducing a delay in milliseconds
 */
void osp_delay_msec(uint32_t delay_ms) { osp_delay_usec(delay_ms * 1000); }

int8_t init_bmi160(struct bmi160_dev *bmi160dev, const char *name) {
    int8_t rslt;

    int fd = open(name, O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    bmi160dev->write = osp_write_spi;
    bmi160dev->read = osp_read_spi;
    bmi160dev->delay_ms = osp_delay_msec;
    bmi160dev->id = fd;
    bmi160dev->intf = BMI160_SPI_INTF;

    rslt = bmi160_init(bmi160dev);

    if (rslt == BMI160_OK) {
        printf("BMI160 initialization success !\n");
        // printf("Chip ID 0x%X\n", bmi160dev->chip_id);
    } else {
        printf("BMI160 initialization failure !\n");
        exit(1);
    }

    /* Select the Output data rate, range of accelerometer sensor */
    bmi160dev->accel_cfg.odr = BMI160_ACCEL_ODR_100HZ;
    bmi160dev->accel_cfg.range = BMI160_ACCEL_RANGE_4G;
    bmi160dev->accel_cfg.bw = BMI160_ACCEL_BW_NORMAL_AVG4;

    /* Select the power mode of accelerometer sensor */
    bmi160dev->accel_cfg.power = BMI160_ACCEL_NORMAL_MODE;

    /* Select the Output data rate, range of Gyroscope sensor */
    bmi160dev->gyro_cfg.odr = BMI160_GYRO_ODR_100HZ;
    bmi160dev->gyro_cfg.range = BMI160_GYRO_RANGE_2000_DPS;
    bmi160dev->gyro_cfg.bw = BMI160_GYRO_BW_NORMAL_MODE;

    /* Select the power mode of Gyroscope sensor */
    bmi160dev->gyro_cfg.power = BMI160_GYRO_NORMAL_MODE;

    /* Set the sensor configuration */
    rslt = bmi160_set_sens_conf(bmi160dev);
    return 0;
}
