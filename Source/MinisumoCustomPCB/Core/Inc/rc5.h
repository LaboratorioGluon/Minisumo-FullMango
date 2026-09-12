#ifndef RC_5_H__
#define RC_5_H__

#include <stm32h5xx_hal.h>
#include "cQueue.h"

#define RC5_NUM_BITS 12
#define RC5_READ_BITS (RC5_NUM_BITS + 1)

typedef enum {
  RC5_TIM_STOP = 0,
  RC5_TIM_WAIT_POST_S2,
  RC5_TIM_READING_BITS,
} Rc5TimStage;

typedef enum {
  RC5_IRQ_WAIT_S1 = 0,
  RC5_IRQ_WAIT_S1_RISE,
  RC5_IRQ_WAIT_S2,
  RC5_IRQ_SYNC
} Rc5IrqStage;

typedef struct {
  uint8_t toggleBit;
  uint8_t address;
  uint8_t command;
} Rc5Packet;

typedef struct {
  struct {
    TIM_HandleTypeDef *tim;
    IRQn_Type irq;
    GPIO_TypeDef *port;
    uint32_t pin;
  } init;
  Rc5TimStage timStage;
  Rc5IrqStage irqStage;
  uint8_t bitCounter;
  uint8_t bits[RC5_READ_BITS];
  Rc5Packet pkt;
  Queue_t pktQueue;
  uint32_t halfBitDuration;
  uint32_t fullBitDuration;
  uint32_t watchdog;
  uint32_t syncValue;
  struct{
    uint8_t type;
    uint32_t haltime;
    uint32_t timtime;
    uint32_t othertim;
  } debugEvents[200];
  uint32_t debugCnt;
} Rc5;

typedef enum{
  RC5_QEMPTY = 0,
  RC5_OK,
  RC5_ERROR
} Rc5Ret;

/** @brief Initialize the RC5 device.
 *      The 'dev.init' field shall be completelly filled.
 *
 *  @param dev Rc5 device pointer.
 */
void rc5_init(Rc5 *dev);

void rc5_startReceiving(Rc5 *dev);

void rc5_getBlocking(Rc5 *dev, Rc5Packet *pkt);

Rc5Ret rc5_getPkt(Rc5 *dev, Rc5Packet *pkt);

/** @brief Functions that shall be called from the
 *         IRQ handler from the EXTI_IRQ and TIM_IRQ
 *  @param dev Rc5 device pointer.
 */
void rc5_handleRising(Rc5 *dev);
void rc5_handleFalling(Rc5 *dev);
void rc5_handleTimEvent(Rc5 *dev);
#endif // RC_5_H__