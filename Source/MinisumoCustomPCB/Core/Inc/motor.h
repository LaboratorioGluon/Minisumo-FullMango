#ifndef MOTOR_H__
#define MOTOR_H__

#include <stdint.h>
#include <stm32h5xx_hal.h>
#include <stm32h5xx_hal_gpio.h>
#include <stm32h5xx_hal_tim.h>

#define MOTOR_DUTY_MAX_VALUE 100U

typedef struct {
    TIM_HandleTypeDef* timer;
    uint32_t           timerChannel;
    GPIO_TypeDef*      directionPort;
    uint16_t           directionPin;
    uint8_t            isReversed;
} MotorConfig;

typedef enum { MOTOR_DIRECTION_FWD = 0, MOTOR_DIRECTION_BCK } MotorDirection;

typedef struct {
    MotorConfig    init;
    MotorDirection direction;
    uint32_t       duty;  // 0..100
    uint32_t       reloadValue;

    MotorDirection targetDirection;
    uint32_t       currentSpeed;  // [0-100]
    uint32_t       targetSpeed;   // [0-100]
    uint32_t       maxRate;       // duty/10ms
    uint32_t       lastMs;        // [milliseconds]
    uint32_t       startMs;       // [milliseconds]
    uint32_t       startSpeed;    // [0-100]
} Motor;

void motor_init(Motor* dev);

void motor_setDuty(Motor* dev, MotorDirection dir, uint32_t duty);
void motor_setTarget(Motor* dev, MotorDirection dir, uint32_t dutyTarget);

void motor_update(Motor* dev);

#endif  // MOTOR_H__