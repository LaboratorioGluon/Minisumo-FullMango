#ifndef MINISUMO_H__
#define MINISUMO_H__

#include "status.h"
#include "stm32h523xx.h"
#include "stm32h5xx_hal_tim.h"
#include "motor.h"

typedef struct{
    StatusConfig statusConfig;
    ADC_HandleTypeDef *adc;
    TIM_HandleTypeDef *rc5Timer;
    MotorConfig motorLeft;
    MotorConfig motorRight;

} MinisumoConfig;

void minisumo_setup(MinisumoConfig *config);

void minisumo_loop();

#endif //MINISUMO_H__