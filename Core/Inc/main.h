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
#define UART_SENS1_TX_Pin GPIO_PIN_0
#define UART_SENS1_TX_GPIO_Port GPIOA
#define UART_SENS1_RX_Pin GPIO_PIN_1
#define UART_SENS1_RX_GPIO_Port GPIOA
#define MODBUS1_EN_Pin GPIO_PIN_2
#define MODBUS1_EN_GPIO_Port GPIOA
#define FLASH_SPI_CS_Pin GPIO_PIN_4
#define FLASH_SPI_CS_GPIO_Port GPIOA
#define FLASH_SPI_CLK_Pin GPIO_PIN_5
#define FLASH_SPI_CLK_GPIO_Port GPIOA
#define FLASH_SPI_MISO_Pin GPIO_PIN_6
#define FLASH_SPI_MISO_GPIO_Port GPIOA
#define FALSH_SPI_MOSI_Pin GPIO_PIN_7
#define FALSH_SPI_MOSI_GPIO_Port GPIOA
#define UART_SENS2_TX_Pin GPIO_PIN_8
#define UART_SENS2_TX_GPIO_Port GPIOD
#define UART_SENS2_RX_Pin GPIO_PIN_9
#define UART_SENS2_RX_GPIO_Port GPIOD
#define MODBUS2_EN_Pin GPIO_PIN_10
#define MODBUS2_EN_GPIO_Port GPIOD
#define UART_CUP_TX_Pin GPIO_PIN_6
#define UART_CUP_TX_GPIO_Port GPIOC
#define UART_CUP_RX_Pin GPIO_PIN_7
#define UART_CUP_RX_GPIO_Port GPIOC
/* USER CODE BEGIN Private defines */
// General settings
#define DEVICE_VERSION      (0x01)
#define SENS_READ_PERIOD_MS (60000)
#define DATA_TRNS_PERIOD_MS (86400000)
#define DEFAULT_UART_DELAY  ((uint32_t)0x100)
// SD card
extern SPI_HandleTypeDef    hspi1;
#define FLASH_SPI           (hspi1)
// MODBUS
extern UART_HandleTypeDef   huart4;
#define LOW_MB_UART         (huart4)
#define LOW_MB_UART_INSTC   (huart4.Instance)
#define LOW_MB_SENS_COUNT   (127)

#define LOW_MB_ARR_SIZE     (LOW_MB_SENS_COUNT + 1)
// CUP slave
//extern UART_HandleTypeDef huart6;
//extern TIM_HandleTypeDef  htim1;
//#define CUP_UART          huart6
//#define CUP_UART_INSTANCE huart6.Instance
//#define CUP_TIM			  htim1
//#define CUP_TIM_INSTANCE  htim1.Instance
// BEDUG UART
extern UART_HandleTypeDef   huart1;
#define BEDUG_UART          (huart1)
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
