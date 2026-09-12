#ifndef WSLED_H__
#define WSLED_H__

#include <stdint.h>
#include <stm32h5xx_hal.h>

typedef struct {
    /**! Initialization struct, provided before calling wsled_init(); */
    struct {
        DMA_HandleTypeDef* dma;
        TIM_HandleTypeDef* tim;
        uint32_t           timChannel;
    } init;

    struct {
        uint32_t timeH;
        uint32_t timeL;
    } zero;

    struct {
        uint32_t timeH;
        uint32_t timeL;
    } one;
    uint32_t dstDma;
} WsLed;

typedef enum {
    WS2812B = 0,
} WsType;

/*  */
typedef struct {
    uint8_t g;
    uint8_t r;
    uint8_t b;
} Rgb;

void wsled_init(WsLed* ws);

void wsled_sendRgb(WsLed* ws, Rgb* rgb, uint32_t numLeds);
void wsled_sendBytes(WsLed* ws, uint16_t* wsData, uint32_t numBytes);

void wsled_genData(WsLed* ws, Rgb* rgb, uint16_t* wsData, uint32_t numLeds);

#endif  // WSLED_H__