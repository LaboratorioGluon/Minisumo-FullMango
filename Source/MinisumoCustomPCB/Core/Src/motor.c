#include "motor.h"
#include "stm32h5xx_hal_gpio.h"
#include "stm32h5xx_hal_tim_ex.h"

void motor_init(Motor* dev)
{
    dev->reloadValue = __HAL_TIM_GET_AUTORELOAD(dev->init.timer);
}

void motor_setDuty(Motor* dev, MotorDirection dir, uint32_t duty)
{
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
        HAL_GPIO_WritePin(dev->init.directionPort, dev->init.directionPin,
                          pinStateFwd);
    }
    else {
        localDuty = localDutyBck;
        HAL_GPIO_WritePin(dev->init.directionPort, dev->init.directionPin,
                          pinStateBck);
    }

    if (localDuty <= MOTOR_DUTY_MAX_VALUE) {
        uint32_t calculatedPwmValue = dev->reloadValue / 100.0f * localDuty;
        __HAL_TIM_SET_COMPARE(dev->init.timer, dev->init.timerChannel,
                              calculatedPwmValue);
    }
}