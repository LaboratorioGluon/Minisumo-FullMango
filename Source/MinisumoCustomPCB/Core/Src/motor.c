#include "motor.h"
#include "stm32h5xx_hal_gpio.h"
#include "stm32h5xx_hal_tim_ex.h"

void motor_init(Motor* dev)
{
    dev->reloadValue = __HAL_TIM_GET_AUTORELOAD(dev->init.timer);
}

void motor_setDuty(Motor* dev, MotorDirection dir, uint32_t duty)
{
    uint32_t localDuty = duty;

    if (dir == MOTOR_DIRECTION_FWD) {
        HAL_GPIO_WritePin(dev->init.directionPort, dev->init.directionPin,
                          GPIO_PIN_RESET);
    }
    else {
        localDuty = 100 - duty;
        HAL_GPIO_WritePin(dev->init.directionPort, dev->init.directionPin,
                          GPIO_PIN_SET);
    }

    if (localDuty <= MOTOR_DUTY_MAX_VALUE) {
        uint32_t calculatedPwmValue = dev->reloadValue / 100.0f * localDuty;
        __HAL_TIM_SET_COMPARE(dev->init.timer, dev->init.timerChannel,
                              calculatedPwmValue);
    }
}