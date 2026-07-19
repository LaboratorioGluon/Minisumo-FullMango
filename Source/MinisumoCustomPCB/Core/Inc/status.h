#ifndef STATUS_H__
#define STATUS_H__

#include <stdint.h>

#include <stm32h5xx_hal.h>
#include "wsLed.h"

typedef struct{
    DMA_HandleTypeDef *dma;
    TIM_HandleTypeDef *tim;
    uint32_t timChannel;
} StatusConfig;

typedef struct{
    uint8_t led1;
    uint8_t led2;
    Rgb led1rgb;
} StatusInfo;


void status_init(StatusConfig *config);

void status_update(StatusInfo *info);
void status_rawLeds(Rgb a, Rgb b);

#endif //STATUS_H__