/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h5xx_hal.h"

    /* Private includes ----------------------------------------------------------*/
    /* USER CODE BEGIN Includes */

    /* USER CODE END Includes */

    /* Exported types ------------------------------------------------------------*/
    /* USER CODE BEGIN ET */

    /* USER CODE END ET */

    /* Exported constants --------------------------------------------------------*/
    /* USER CODE BEGIN EC */

    /* USER CODE END EC */

    /* Exported macro ------------------------------------------------------------*/
    /* USER CODE BEGIN EM */

    /* USER CODE END EM */

    void HAL_TIM_MspPostInit(TIM_HandleTypeDef* htim);

    /* Exported functions prototypes ---------------------------------------------*/
    void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define V_BATT_SENSE_Pin GPIO_PIN_0
#define V_BATT_SENSE_GPIO_Port GPIOA
#define LINE_RIGHT_Pin GPIO_PIN_1
#define LINE_RIGHT_GPIO_Port GPIOA
#define LINE_LEFT_Pin GPIO_PIN_2
#define LINE_LEFT_GPIO_Port GPIOA
#define MOTOR_SENSE_R_Pin GPIO_PIN_3
#define MOTOR_SENSE_R_GPIO_Port GPIOA
#define MOTOR_SENSE_L_Pin GPIO_PIN_4
#define MOTOR_SENSE_L_GPIO_Port GPIOA
#define CONFIG_ADC_Pin GPIO_PIN_5
#define CONFIG_ADC_GPIO_Port GPIOA
#define SHARP_C_ANALOG_Pin GPIO_PIN_6
#define SHARP_C_ANALOG_GPIO_Port GPIOA
#define SHARP_L_ANALOG_Pin GPIO_PIN_7
#define SHARP_L_ANALOG_GPIO_Port GPIOA
#define SHARP_R_ANALOG_Pin GPIO_PIN_0
#define SHARP_R_ANALOG_GPIO_Port GPIOB
#define MOTOR_ENABLE_Pin GPIO_PIN_13
#define MOTOR_ENABLE_GPIO_Port GPIOB

    /* USER CODE BEGIN Private defines */

    /* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
