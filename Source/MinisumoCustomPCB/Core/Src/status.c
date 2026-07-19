#include "status.h"
#include "wsLed.h"

#include "main.h"
#include <string.h>

#define STATUS_NUM_LEDS (2)
#define BITS_PER_LED (24)

#define PWM_NUM_TAIL_BIT (1)

static WsLed leds;
static uint16_t pwmData[STATUS_NUM_LEDS*BITS_PER_LED + PWM_NUM_TAIL_BIT];

Rgb colors[2] = {
{5, 0, 0},
{5, 0, 0}
};

void status_init(StatusConfig *config) {

  leds.init.dma = config->dma;
  leds.init.tim = config->tim;
  leds.init.timChannel = config->timChannel;
  
  wsled_init(&leds);

  pwmData[STATUS_NUM_LEDS*BITS_PER_LED] = 0;

  wsled_genData(&leds, colors, pwmData, 2);
  
  HAL_TIM_PWM_Start(config->tim, config->timChannel);
}

void status_rawLeds(Rgb a, Rgb b)
{
    memcpy(&colors[0], (uint8_t*)&a, sizeof(Rgb));
    memcpy(&colors[1], (uint8_t*)&b, sizeof(Rgb));
    wsled_genData(&leds, colors, pwmData, 2);
    wsled_sendBytes(&leds, pwmData, sizeof(pwmData));

}

void status_update(StatusInfo *info) {

    if (info->led1 == 1)
    {
        colors[0].r = 5;
        colors[0].g = 0;
        colors[0].b = 0;
    }
    else if (info->led1 == 2)
    {
        colors[0].r = 0;
        colors[0].g = 0;
        colors[0].b = 5;
    }
    else
    {
        colors[0].r = 0;
        colors[0].g = 5;
        colors[0].b = 0;
    }

    if (info->led2 == 1)
    {
        colors[1].r = 5;
        colors[1].g = 0;
        colors[1].b = 0;
    }
    else
    {
        colors[1].r = 0;
        colors[1].g = 5;
        colors[1].b = 0;
    }

    memcpy(&colors[1], (uint8_t*)&info->led1rgb, sizeof(Rgb));
    wsled_genData(&leds, colors, pwmData, 2);
    wsled_sendBytes(&leds, pwmData, sizeof(pwmData));
}
