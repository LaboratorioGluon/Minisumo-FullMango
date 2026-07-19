#include "sensors.h"
#include "stm32h5xx_hal_adc.h"
#include <stdint.h>
#include <string.h>



static ADC_HandleTypeDef *hAdc;
static uint8_t adcConversionCompleted;
static uint16_t adcDataBuffer[SENSOR_ENUM_LEN] = {0};
static uint16_t adcDataCopy[SENSOR_ENUM_LEN] = {0};


void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc){
  adcConversionCompleted = 1;
}


void sensors_init(ADC_HandleTypeDef *adc)
{
    hAdc = adc;
    adcConversionCompleted = 0;
    HAL_ADC_Start_DMA(hAdc, (uint32_t*)adcDataBuffer, SENSOR_ENUM_LEN);
}


uint8_t sensors_isDataReady()
{

    uint8_t ret = adcConversionCompleted;
    if (adcConversionCompleted == 1)
    {
        adcConversionCompleted = 0;
        memcpy(adcDataCopy, adcDataBuffer, sizeof(uint16_t) * SENSOR_ENUM_LEN);
        HAL_ADC_Start_DMA(hAdc, (uint32_t*)adcDataBuffer, SENSOR_ENUM_LEN);
    }

    return ret;
}


inline uint16_t sensors_get(SensorResult sensor)
{
    return adcDataCopy[sensor];
}