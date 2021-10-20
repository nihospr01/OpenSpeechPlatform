#include "osp_bmi160.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static int8_t set_tap_config(struct bmi160_dev *bmi160dev, uint8_t feature_enable) {
    int8_t rslt = BMI160_OK;
    struct bmi160_int_settg int_config;

    if (feature_enable > 0) {
        /* Select the Interrupt channel/pin */
        int_config.int_channel = BMI160_INT_CHANNEL_1; /* Interrupt channel/pin 1 */

        /* Select the interrupt channel/pin settings */
        int_config.int_pin_settg.output_en = BMI160_ENABLE;         /* Enabling interrupt pins to act as output pin */
        int_config.int_pin_settg.output_mode = BMI160_DISABLE;      /* Choosing push-pull mode for interrupt pin */
        int_config.int_pin_settg.output_type = BMI160_ENABLE;       /* Choosing active low output */
        int_config.int_pin_settg.edge_ctrl = BMI160_DISABLE;        /* Choosing edge triggered output */
        int_config.int_pin_settg.input_en = BMI160_DISABLE;         /* Disabling interrupt pin to act as input */
        int_config.int_pin_settg.latch_dur = BMI160_LATCH_DUR_NONE; /* non-latched output */

        /* Select the Interrupt type */
        int_config.int_type = BMI160_ACC_DOUBLE_TAP_INT; /* Choosing tap interrupt */

        /* Select the Any-motion interrupt parameters */
        int_config.int_type_cfg.acc_tap_int.tap_en = BMI160_ENABLE; /* 1- Enable tap, 0- disable tap */
        int_config.int_type_cfg.acc_tap_int.tap_thr = 2;            /* Set tap threshold */
        int_config.int_type_cfg.acc_tap_int.tap_dur = 2;            /* Set tap duration */
        int_config.int_type_cfg.acc_tap_int.tap_shock = 0;          /* Set tap shock value */
        int_config.int_type_cfg.acc_tap_int.tap_quiet = 0;          /* Set tap quiet duration */
        int_config.int_type_cfg.acc_tap_int.tap_data_src = 1;       /* data source 0 : filter or 1 : pre-filter */

        /* Set the Any-motion interrupt */
        rslt = bmi160_set_int_config(&int_config, bmi160dev); /* sensor is an instance of the structure bmi160_dev  */
        printf("bmi160_set_int_config(tap enable) status:%d\n", rslt);
    } else {
        /* Select the Interrupt channel/pin */
        int_config.int_channel = BMI160_INT_CHANNEL_1;
        int_config.int_pin_settg.output_en = BMI160_DISABLE; /* Disabling interrupt pins to act as output pin */
        int_config.int_pin_settg.edge_ctrl = BMI160_DISABLE; /* Choosing edge triggered output */

        /* Select the Interrupt type */
        int_config.int_type = BMI160_ACC_DOUBLE_TAP_INT;             /* Choosing Tap interrupt */
        int_config.int_type_cfg.acc_tap_int.tap_en = BMI160_DISABLE; /* 1- Enable tap, 0- disable tap */

        /* Set the Data ready interrupt */
        rslt = bmi160_set_int_config(&int_config, bmi160dev); /* sensor is an instance of the structure bmi160_dev */
        printf("bmi160_set_int_config(tap disable) status:%d\n", rslt);
    }

    return rslt;
}

struct timespec diff(struct timespec start, struct timespec end) {
    struct timespec temp;
    if ((end.tv_nsec - start.tv_nsec) < 0) {
        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return temp;
}

int main(int argc, char *argv[]) {
    struct bmi160_dev bmi160dev;
    init_bmi160(&bmi160dev, "/dev/spidev1.1");

    int8_t rslt = set_tap_config(&bmi160dev, BMI160_ENABLE);
    if (rslt == BMI160_OK) {
        union bmi160_int_status int_status;
        uint8_t loop = 0;
        struct timespec time1, time2;
        printf("Do Single or Double Tap the board\n");
        fflush(stdout);

        memset(int_status.data, 0x00, sizeof(int_status.data));

        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time1);

        while (loop < 10) {
            /* Read interrupt status */
            rslt = bmi160_get_int_status(BMI160_INT_STATUS_ALL, &int_status, &bmi160dev);
            if (rslt == BMI160_OK) {
                clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time2);

                /* Enters only if the obtained interrupt is single-tap */
                if (int_status.bit.s_tap) {
                    printf("Single tap, iter:%d, time:%f.2 ms, int_status:0x%x\n", loop++,
                           (float)diff(time1, time2).tv_nsec / 1e6, int_status.data[0]);
                }
                /* Enters only if the obtained interrupt is double-tap */
                else if (int_status.bit.d_tap) {
                    printf("Double tap, iter:%d, time:%f.2 ms, int_status:0x%x\n", loop++,
                           (float)diff(time1, time2).tv_nsec / 1e6, int_status.data[0]);
                }
                fflush(stdout);
            } else {
                break;
            }

            memset(int_status.data, 0x00, sizeof(int_status.data));
            time1 = time2;
        }

        /* Disable tap feature */
        printf("\nDisable tap test...\n");
        rslt = set_tap_config(&bmi160dev, BMI160_DISABLE);
        printf("bmi160_set_int_config(tap enable) status:%d\n", rslt);

        fflush(stdout);
    }

    // coines_close_comm_intf(COINES_COMM_INTF_USB);

    return EXIT_SUCCESS;
}
