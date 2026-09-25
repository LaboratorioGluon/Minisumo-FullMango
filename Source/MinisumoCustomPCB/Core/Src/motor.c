#include "motor.h"
#include <stdio.h>
#include <string.h>
#include "stm32h5xx_hal.h"
#include "stm32h5xx_hal_gpio.h"
#include "stm32h5xx_hal_tim_ex.h"

void motor_init(Motor* dev)
{
    dev->reloadValue = __HAL_TIM_GET_AUTORELOAD(dev->init.timer);
}

void motor_setDuty(Motor* dev, MotorDirection dir, uint32_t duty)
{
    dev->currentSpeed          = duty;
    uint32_t      localDutyFwd = duty;
    uint32_t      localDutyBck = duty;
    uint32_t      localDuty;
    GPIO_PinState pinStateFwd;
    GPIO_PinState pinStateBck;

    if (dev->init.isReversed == 1) {
        pinStateFwd  = GPIO_PIN_RESET;
        pinStateBck  = GPIO_PIN_SET;
        localDutyFwd = duty;
        localDutyBck = 100 - duty;
    }
    else {
        pinStateFwd  = GPIO_PIN_SET;
        pinStateBck  = GPIO_PIN_RESET;
        localDutyFwd = 100 - duty;
        localDutyBck = duty;
    }

    if (dir == MOTOR_DIRECTION_FWD) {
        // If RESET/SET is swapped between directions
        // Move the localDuty adjustment
        localDuty = localDutyFwd;
        HAL_GPIO_WritePin(dev->init.directionPort, dev->init.directionPin, pinStateFwd);
    }
    else {
        localDuty = localDutyBck;
        HAL_GPIO_WritePin(dev->init.directionPort, dev->init.directionPin, pinStateBck);
    }

    if (localDuty <= MOTOR_DUTY_MAX_VALUE) {
        uint32_t calculatedPwmValue = dev->reloadValue / 100.0f * localDuty;
        __HAL_TIM_SET_COMPARE(dev->init.timer, dev->init.timerChannel, calculatedPwmValue);
    }
}

void motor_setTarget(Motor* dev, MotorDirection dir, uint32_t dutyTarget)
{
    dev->targetSpeed = dutyTarget;
    if (dir != dev->targetDirection) {
        dev->currentSpeed = 0;
    }
    dev->targetDirection = dir;
    dev->lastMs          = HAL_GetTick();
    dev->startMs         = HAL_GetTick();
    dev->startSpeed      = dev->currentSpeed;

    return;
}

extern UART_HandleTypeDef huart5;
static uint8_t            uartBuf[200];

void motor_update(Motor* dev)
{
    uint32_t currentMs = HAL_GetTick();
    uint32_t deltaTms  = currentMs - dev->startMs;
    if (dev->currentSpeed == dev->targetSpeed) {
        return;  // Do nothing, motor already at desired speed.
    }
    uint32_t rate     = (dev->currentSpeed > 25) ? 5 * dev->maxRate : dev->maxRate;
    dev->currentSpeed = rate * deltaTms / 10 + dev->startSpeed;
    if (dev->currentSpeed >= dev->targetSpeed) {
        dev->currentSpeed = dev->targetSpeed;
    }
    motor_setDuty(dev, dev->targetDirection, dev->currentSpeed);
    /*snprintf((char*)uartBuf, 150, "$%lu,%lu;%lu;\r\n", currentMs, dev->lastMs, dev->currentSpeed);

    HAL_UART_Transmit(&huart5, uartBuf, strlen((char*)uartBuf), 1000);*/
    dev->lastMs = currentMs;

    return;
}