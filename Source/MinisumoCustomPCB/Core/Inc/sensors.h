#ifndef SENSORS_H__
#define SENSORS_H__

#include <stdint.h>
#include "stm32h5xx_hal.h"
#include "stm32h5xx_hal_adc.h"

typedef enum {
    SENSOR_LINE_RIGHT = 0,
    SENSOR_SHARP_RIGHT,
    SENSOR_SHARP_LEFT,
    SENSOR_BATT,
    SENSOR_SHARP_CENTER,
    SENSOR_LINE_LEFT,
    SENSOR_MOTOR_RIGHT,
    SENSOR_MOTOR_LEFT,
    SENSOR_ENUM_LEN
} SensorResult;

void sensors_init(ADC_HandleTypeDef* adc);

uint8_t sensors_isDataReady();

uint16_t sensors_get(SensorResult sensor);

#endif  //SENSORS_H__