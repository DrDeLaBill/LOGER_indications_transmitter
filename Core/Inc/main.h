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
#define MODBUS1_TX_Pin GPIO_PIN_6
#define MODBUS1_TX_GPIO_Port GPIOC
#define MODBUS1_RX_Pin GPIO_PIN_12
#define MODBUS1_RX_GPIO_Port GPIOA
#define SPI3_SD_NSS_Pin GPIO_PIN_2
#define SPI3_SD_NSS_GPIO_Port GPIOD
#define DATA_TX_Pin GPIO_PIN_6
#define DATA_TX_GPIO_Port GPIOB
#define DATA_RX_Pin GPIO_PIN_7
#define DATA_RX_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */
//SD card
extern SPI_HandleTypeDef hspi3;
#define SD_HSPI         hspi3
#define SD_CS_GPIO_Port SPI3_SD_NSS_GPIO_Port
#define SD_CS_Pin       SPI3_SD_NSS_Pin
// MODBUS
extern UART_HandleTypeDef huart6;
#define LOW_MB_UART       huart6
#define LOW_MB_SENS_COUNT 127
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
