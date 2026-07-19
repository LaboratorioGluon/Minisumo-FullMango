#include "wsLed.h"

void wsled_init(WsLed *ws) {
  switch (ws->init.timChannel) {
  case TIM_CHANNEL_1:
    ws->dstDma = (uint32_t)&(ws->init.tim->Instance->CCR1);
    break;
  case TIM_CHANNEL_2:
    ws->dstDma = (uint32_t)&(ws->init.tim->Instance->CCR2);
    break;
  case TIM_CHANNEL_3:
    ws->dstDma = (uint32_t)&(ws->init.tim->Instance->CCR3);
    break;
  case TIM_CHANNEL_4:
    ws->dstDma = (uint32_t)&(ws->init.tim->Instance->CCR4);
    break;
  case TIM_CHANNEL_5:
    ws->dstDma = (uint32_t)&(ws->init.tim->Instance->CCR5);
    break;
  case TIM_CHANNEL_6:
    ws->dstDma = (uint32_t)&(ws->init.tim->Instance->CCR6);
    break;
  }

  ws->one.timeH = 200;
  ws->zero.timeH = 100;
}

void wsled_sendRgb(WsLed *ws, Rgb *rgb, uint32_t numLeds) {

  //__HAL_TIM_ENABLE_DMA(ws->init.tim, TIM_DMA_UPDATE);
  // ret = HAL_DMA_Start_IT(ws->init.dma, (uint32_t)pwmData,
  // (uint32_t)&TIM4->CCR3,12);
}

void wsled_sendBytes(WsLed *ws, uint16_t *wsData, uint32_t numBytes) {
  __HAL_TIM_ENABLE_DMA(ws->init.tim, TIM_DMA_UPDATE);
  HAL_DMA_Start_IT(ws->init.dma, (uint32_t)wsData, (uint32_t)ws->dstDma,
                   numBytes);
}

void wsled_genData(WsLed *ws, Rgb *rgb, uint16_t *wsData, uint32_t numLeds) {
  for (uint32_t led = 0; led < numLeds; led++) {
    Rgb color = rgb[led];
    uint32_t baseOffset = led * 24;
    for (uint32_t bit = 0; bit < 8; bit++) {
      wsData[baseOffset + (7 - bit)] =
          (color.g & (0x1 << bit)) ? ws->one.timeH : ws->zero.timeH;
      wsData[baseOffset + (7 - bit) + 8] =
          (color.r & (0x1 << bit)) ? ws->one.timeH : ws->zero.timeH;
      wsData[baseOffset + (7 - bit) + 16] =
          (color.b & (0x1 << bit)) ? ws->one.timeH : ws->zero.timeH;
    }
  }
  
}