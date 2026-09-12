#include "minisumo.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "motor.h"
#include "rc5.h"
#include "sensors.h"
#include "startStopModule.h"
#include "status.h"
#include "stm32h523xx.h"
#include "stm32h5xx_hal.h"
#include "stm32h5xx_hal_adc.h"
#include "stm32h5xx_hal_uart.h"

static StatusInfo info;

static ADC_HandleTypeDef* adc;
static Rc5                rc5;
static Motor              motorLeft, motorRight;
static uint8_t            isFighting = 0;

typedef enum {
    MOVE_STOP = 0,
    MOVE_FWD,
    MOVE_BCK,
    MOVE_LEFT,
    MOVE_RIGHT
} MoveDirection;

struct {
    MoveDirection move;    // Current direction
    int32_t       timeMs;  // -1: move forever.
} CurrentMove;

void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == rc5.init.pin) {
        rc5_handleRising(&rc5);
    }
}

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == rc5.init.pin) {
        rc5_handleFalling(&rc5);
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
    if (htim == rc5.init.tim) {
        rc5_handleTimEvent(&rc5);
    }
}

void minisumo_setup(MinisumoConfig* config)
{
    startstop_init();

    rc5.init.pin  = GPIO_PIN_1;
    rc5.init.port = GPIOB;
    rc5.init.irq  = EXTI1_IRQn;
    rc5.init.tim  = config->rc5Timer;
    rc5_init(&rc5);

    // config_init();
    status_init(&config->statusConfig);

    motorLeft.init = config->motorLeft;
    motor_init(&motorLeft);
    motorRight.init = config->motorRight;
    motor_init(&motorRight);

    adc = config->adc;

    // rc5_init();
    // sensors_init();

    info.led1 = 0;
    info.led2 = 0;
}

extern UART_HandleTypeDef huart5;
#define MOTOR_CURRENT_SAMPLES 1
uint32_t start;
void     minisumo_loop()
{

    uint8_t           uartBuf[150] = "";
    volatile uint16_t sharp[3];

    if (sensors_isDataReady() == 1) {
        start = HAL_GetTick();

        volatile uint16_t lines[2];
        lines[0] = sensors_get(SENSOR_LINE_LEFT);
        lines[1] = sensors_get(SENSOR_LINE_RIGHT);

        sharp[0] = sensors_get(SENSOR_SHARP_LEFT);
        sharp[1] = sensors_get(SENSOR_SHARP_CENTER);
        sharp[2] = sensors_get(SENSOR_SHARP_RIGHT);

        /*snprintf(uartBuf, 150, "$%d,%d,%d;\r\n", sharp[0], sharp[1], sharp[2]);
HAL_UART_Transmit(&huart5, uartBuf, strlen(uartBuf), 1000);*/
        snprintf((char*)uartBuf, 150, "$%d,%d;\r\n", lines[0], lines[1]);
        HAL_UART_Transmit(&huart5, uartBuf, strlen((char*)uartBuf), 1000);
    }

    Rc5Packet pkt;
    Rc5Ret    ret = rc5_getPkt(&rc5, &pkt);
    if (ret == RC5_OK) {
        if (startstop_run(&pkt) == STARTSTOP_STOP) {
            isFighting = 0;
        }
    }

    if (isFighting) {
        // TODO: Check for stop;

        /*
if (sharp[1] > 1500)
{
  motor_setDuty(&motorLeft, MOTOR_DIRECTION_FWD, 70);
  motor_setDuty(&motorRight, MOTOR_DIRECTION_FWD, 70);
}
else if (sharp[0] > 1500)
{
  motor_setDuty(&motorLeft, MOTOR_DIRECTION_BCK, 50);
  motor_setDuty(&motorRight, MOTOR_DIRECTION_FWD, 50);
}
else if (sharp[2] > 1500)
{
  motor_setDuty(&motorLeft, MOTOR_DIRECTION_FWD, 50);
  motor_setDuty(&motorRight, MOTOR_DIRECTION_BCK, 50);
}
else
{
  // motor_setDuty(&motorLeft, MOTOR_DIRECTION_FWD, 0);
  // motor_setDuty(&motorRight, MOTOR_DIRECTION_FWD, 0);
}
  */
    }
    else {
        // Not fighting
        motor_setDuty(&motorLeft, MOTOR_DIRECTION_FWD, 0);
        motor_setDuty(&motorRight, MOTOR_DIRECTION_FWD, 0);

        Rc5Packet pkt;
        rc5_getBlocking(&rc5, &pkt);
        snprintf(uartBuf, 150, "RC5: 0x%02X 0x%02X\r\n", pkt.address,
                 pkt.command);

        StartStopRet starstop = startstop_run(&pkt);

        switch (starstop) {
            case STARTSTOP_OK:
                status_rawLeds((Rgb){20, 0, 0}, (Rgb){20, 0, 0});
                break;
            case STARTSTOP_RUN:
                isFighting = 1;
                break;  // Start fighting
            case STARTSTOP_STOP:
                isFighting = 0;
                break;
            case STARTSTOP_DOHYOERR:
                status_rawLeds((Rgb){0, 0, 0}, (Rgb){0, 20, 0});
                break;
            case STARTSTOP_ADDRERR:
                status_rawLeds((Rgb){0, 20, 0}, (Rgb){0, 0, 0});
                break;
            default:
                break;
        }
    }
#if 0
  Rc5Packet pkt;
  rc5_getBlocking(&rc5, &pkt);
  snprintf(uartBuf, 150, "RC5: 0x%02X 0x%02X\r\n", pkt.address , pkt.command);

  StartStopRet starstop = startstop_run(&pkt);


  HAL_UART_Transmit(&huart5, uartBuf, strlen(uartBuf), 1000);

  if (pkt.command == 0x01)
  {
    status_rawLeds((Rgb){0,0,20}, (Rgb){20,0,0});
  }
  if (pkt.command == 0x02)
  {
    status_rawLeds((Rgb){0,20,0}, (Rgb){0,20,0});
  }
  if (pkt.command == 0x03)
  {
    for( uint8_t i = 0; i <10; i++){
      status_rawLeds((Rgb){20,20,0}, (Rgb){0,20,0});
      HAL_Delay(300);
      status_rawLeds((Rgb){0,20,0}, (Rgb){20,20,0});
      HAL_Delay(300);

    }

  }
  // 0 | 0 0 1 0 1 | 0 1 0 1 1 X | 1
  if (pkt.command == 0x16)
  {
    info.led1 = 1;
  }
  else if (pkt.command == 0x17)
  {
    info.led1 = 2;
  }
  else
  {
    info.led1 = 0;
  }
#endif

    // status_update(&info);

    // sensors_update();

    /** logic **/
}