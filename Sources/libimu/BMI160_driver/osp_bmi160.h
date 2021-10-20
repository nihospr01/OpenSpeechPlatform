#ifndef OSP_BMI160_H_
#define OSP_BMI160_H_

#include "bmi160.h"

#ifdef __cplusplus
extern "C" {
#endif

int8_t init_bmi160(struct bmi160_dev *bmi160dev, const char *name);

void osp_delay_usec(uint32_t delay_us);
void osp_delay_msec(uint32_t delay_ms);

#ifdef __cplusplus
}
#endif

#endif // OSP_BMI160_H_