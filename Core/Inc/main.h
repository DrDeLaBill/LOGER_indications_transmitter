/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SD_NSS_Pin GPIO_PIN_4
#define SD_NSS_GPIO_Port GPIOA
#define FAST_MB_TX_Pin GPIO_PIN_8
#define FAST_MB_TX_GPIO_Port GPIOD
#define FAST_MB_RX_Pin GPIO_PIN_9
#define FAST_MB_RX_GPIO_Port GPIOD
#define BEDUG_LED_Pin GPIO_PIN_11
#define BEDUG_LED_GPIO_Port GPIOA
/* USER CODE BEGIN Private defines */
// General settings
#define DEVICE_VERSION      0x01
#define SENS_READ_PERIOD_MS 60000
#define DATA_TRNS_PERIOD_MS 86400000
// SD card
extern SPI_HandleTypeDef  hspi1;
#define SD_HSPI           hspi1
#define SD_CS_GPIO_Port   SD_NSS_GPIO_Port
#define SD_CS_Pin         SD_NSS_Pin
// MODBUS
extern UART_HandleTypeDef huart4;
#define LOW_MB_UART       huart4
#define LOW_MB_UART_INSTC huart4.Instance
#define LOW_MB_SENS_COUNT 127

#define LOW_MB_ARR_SIZE   LOW_MB_SENS_COUNT + 1
// CUP slave
extern UART_HandleTypeDef huart6;
extern TIM_HandleTypeDef  htim1;
#define CUP_UART          huart6
#define CUP_UART_INSTANCE huart6.Instance
#define CUP_TIM			  htim1
#define CUP_TIM_INSTANCE  htim1.Instance
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
