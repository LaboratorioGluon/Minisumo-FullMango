#include "rc5.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "cQueue.h"
#include "stm32h5xx_hal.h"
#include "stm32h5xx_hal_def.h"
#include "stm32h5xx_hal_gpio.h"
#include "stm32h5xx_hal_tim.h"

#define ADDR_BIT_SIZE 5
#define ADDR_BIT_OFFSET 1
#define CMD_BIT_SIZE 6
#define CMD_BIT_OFFSET 6

#define RC5_WATCHDOG_MS 100

extern TIM_HandleTypeDef htim6;

void logDebug(Rc5* dev, uint8_t type)
{
    dev->debugEvents[dev->debugCnt].type    = type;
    dev->debugEvents[dev->debugCnt].haltime = dev->syncValue;
    dev->debugEvents[dev->debugCnt].timtime =
        __HAL_TIM_GET_COUNTER(dev->init.tim);
    dev->debugEvents[dev->debugCnt].othertim = __HAL_TIM_GET_COUNTER(&htim6);

    dev->debugCnt++;
}

void rc5_init(Rc5* dev)
{
    dev->pkt.toggleBit = 0;
    dev->pkt.address   = 0;
    dev->pkt.command   = 0;
    dev->bitCounter    = 0;
    dev->irqStage      = RC5_IRQ_WAIT_S1;
    dev->timStage      = RC5_TIM_STOP;
    __HAL_TIM_SET_COUNTER(dev->init.tim, 0);
    __HAL_TIM_SET_AUTORELOAD(dev->init.tim, 65000);

    q_init(&dev->pktQueue, sizeof(Rc5Packet), 10, FIFO, false);
}

void rc5_startReceiving(Rc5* dev)
{
    // @TODO: Implement the start receiving.
}

void rc5_getBlocking(Rc5* dev, Rc5Packet* pkt)
{
    memset(pkt, 0, sizeof(Rc5Packet));

    while (dev->timStage == RC5_TIM_STOP)
        ;

    // Wait until timer process finished.
    while (dev->timStage != RC5_TIM_STOP)
        ;

    memcpy(pkt, &dev->pkt, sizeof(Rc5Packet));
    dev->debugCnt = 0;
}

Rc5Ret rc5_getPkt(Rc5* dev, Rc5Packet* pkt)
{
    bool ret = q_pop(&dev->pktQueue, pkt);
    if (ret == false) {
        return RC5_QEMPTY;
    }
    else {
        return RC5_OK;
    }
}

void rc5_handleRising(Rc5* dev)
{
    if (dev->irqStage == RC5_IRQ_WAIT_S1_RISE) {
        // Store the HalfBit and FullBit duration
        dev->halfBitDuration = __HAL_TIM_GET_COUNTER(dev->init.tim);

        logDebug(dev, 1);
        // Stop the timer and measure half bit duration.
        HAL_TIM_Base_Stop(dev->init.tim);
        dev->fullBitDuration = dev->halfBitDuration * 2;

        __HAL_TIM_SET_COUNTER(dev->init.tim, 0);
        __HAL_TIM_SET_AUTORELOAD(dev->init.tim,
                                 (int32_t)(dev->halfBitDuration * 3.0f / 2.0f));

        // Set to wait for S2.
        dev->irqStage = RC5_IRQ_WAIT_S2;
    }
    else if (dev->irqStage == RC5_IRQ_SYNC) {
        dev->syncValue = __HAL_TIM_GET_COUNTER(dev->init.tim);
    }
}

void rc5_handleFalling(Rc5* dev)
{

    if (dev->irqStage == RC5_IRQ_WAIT_S1 ||
        ((HAL_GetTick() - dev->watchdog) > RC5_WATCHDOG_MS)) {

        __HAL_TIM_SET_COUNTER(dev->init.tim, 0);
        __HAL_TIM_SET_AUTORELOAD(dev->init.tim, 65000);

        HAL_TIM_Base_Start(dev->init.tim);
        dev->watchdog = HAL_GetTick();
        __HAL_TIM_SET_COUNTER(&htim6, 0);
        dev->irqStage = RC5_IRQ_WAIT_S1_RISE;
    }
    else if (dev->irqStage == RC5_IRQ_WAIT_S2) {

        //NVIC_DisableIRQ(dev->init.irq);

        HAL_TIM_Base_Start_IT(dev->init.tim);

        dev->timStage = RC5_TIM_WAIT_POST_S2;

        memset(&dev->pkt, 0, sizeof(Rc5Packet));
        memset(dev->bits, 0, sizeof(uint8_t) * RC5_READ_BITS);
        // Start the measuring timer.

        dev->irqStage   = RC5_IRQ_SYNC;
        dev->bitCounter = 0;
        dev->syncValue  = 0;
        logDebug(dev, 2);
    }
    else if (dev->irqStage == RC5_IRQ_SYNC) {
        dev->syncValue = __HAL_TIM_GET_COUNTER(dev->init.tim);
    }
}

void rc5_handleTimEvent(Rc5* dev)
{
    logDebug(dev, 3);
    uint8_t bit = HAL_GPIO_ReadPin(dev->init.port, dev->init.pin);
    dev->bits[dev->bitCounter] = bit;

    int32_t deltaSync =
        __HAL_TIM_GET_AUTORELOAD(dev->init.tim) - dev->syncValue;
    if (abs((int)deltaSync) < dev->halfBitDuration) {
        __HAL_TIM_SET_AUTORELOAD(
            dev->init.tim,
            dev->fullBitDuration + (dev->halfBitDuration / 2.0f - deltaSync));
    }
    else {
        __HAL_TIM_SET_AUTORELOAD(
            dev->init.tim,
            dev->fullBitDuration +
                (dev->halfBitDuration * 3.0f / 2.0f - deltaSync));
    }

    // Store each bit in the corresponding field.
    if (dev->bitCounter == 0) {

        // After first bit, the timer shall be set to the full bit duration.
        //__HAL_TIM_SET_AUTORELOAD(dev->init.tim, dev->fullBitDuration);
        dev->timStage      = RC5_TIM_READING_BITS;
        dev->pkt.toggleBit = bit;
    }
    else if (dev->bitCounter <= 5) {
        dev->pkt.address |= (bit & 0x1)
                            << (ADDR_BIT_SIZE -
                                (dev->bitCounter - ADDR_BIT_OFFSET) - 1);
    }
    else if (dev->bitCounter <= 11) {
        dev->pkt.command |= (bit & 0x1)
                            << (CMD_BIT_SIZE -
                                (dev->bitCounter - CMD_BIT_OFFSET) - 1);
    }
    else if (dev->bitCounter == RC5_READ_BITS - 1) {

        // Stop the timer to avoid retriggering this interrupt.
        HAL_TIM_Base_Stop_IT(dev->init.tim);

        q_push(&dev->pktQueue, &dev->pkt);

        __HAL_TIM_SET_COUNTER(dev->init.tim, 0);
        __HAL_TIM_SET_AUTORELOAD(dev->init.tim, 65000);

        dev->timStage = RC5_TIM_STOP;
        dev->irqStage = RC5_IRQ_WAIT_S1;

        // Clear preivous IRQs.
        __HAL_GPIO_EXTI_CLEAR_IT(dev->init.pin);

        // Restart GPIO interrupts to receive next IR packet.
        NVIC_EnableIRQ(dev->init.irq);
    }
    dev->bitCounter++;
}