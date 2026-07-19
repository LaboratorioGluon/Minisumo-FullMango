#ifndef MOTOR_H__
#define MOTOR_H__

#include <stm32h5xx_hal.h>
#include <stm32h5xx_hal_tim.h>
#include <stm32h5xx_hal_gpio.h>

#define MOTOR_DUTY_MAX_VALUE 100U

typedef struct{
    TIM_HandleTypeDef *timer;
    uint32_t timerChannel;
    GPIO_TypeDef *directionPort;
    uint16_t directionPin;
} MotorConfig;

typedef enum {
    MOTOR_DIRECTION_FWD = 0,
    MOTOR_DIRECTION_BCK
} MotorDirection;


typedef struct{
    MotorConfig init;
    MotorDirection direction;
    uint32_t duty; // 0..100
    uint32_t reloadValue;
} Motor;

void motor_init(Motor *dev);

void motor_setDuty(Motor *dev, MotorDirection dir, uint32_t duty);

#endif // MOTOR_H__