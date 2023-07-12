/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>

#include "SettingsManager.h"
#include "RecordManager.h"
#include "SensorManager.h"
#include "CUPSlaveManager.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
SettingsManager* stngs_m;
SensorManager* sens_m;
CUPSlaveManager* CUP_m;
RecordManager* rcrd_m;

uint8_t low_modbus_uart_val = 0;
uint8_t CUP_uart_val = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint16_t CUPSlaveManager::load_data_to_buffer_handler(uint8_t* buffer) {
	uint16_t counter = 0;
	switch (this->message.command) {
	case CUP_CMD_STTNGS:
		this->message.data_len = sizeof(SettingsManager::payload_settings_t);
		counter += serialize(&buffer[counter], this->message.data_len);

		counter += serialize(&buffer[counter], SettingsManager::sd_sttngs.v1.payload_settings.sens_record_period);
		counter += serialize(&buffer[counter], SettingsManager::sd_sttngs.v1.payload_settings.sens_transmit_period);
		for (uint16_t i = 0; i < (sizeof(SettingsManager::sd_sttngs.v1.payload_settings.low_sens_status) / sizeof(*SettingsManager::sd_sttngs.v1.payload_settings.low_sens_status)); i++) {
			counter += serialize(&buffer[counter], SettingsManager::sd_sttngs.v1.payload_settings.low_sens_status[i]);
		}
		for (uint16_t i = 0; i < (sizeof(SettingsManager::sd_sttngs.v1.payload_settings.low_sens_register) / sizeof(*SettingsManager::sd_sttngs.v1.payload_settings.low_sens_register)); i++) {
			counter += serialize(&buffer[counter], SettingsManager::sd_sttngs.v1.payload_settings.low_sens_register[i]);
		}
		break;
	case CUP_CMD_DATA:
		if (RecordManager::sens_record == NULL) {
			return 0;
		}

		this->message.data_len = sizeof(RecordManager::payload_record_t);
		counter += serialize(&buffer[counter], this->message.data_len);

		counter += serialize(&buffer[counter], RecordManager::sens_record->record_id);
		counter += serialize(&buffer[counter], RecordManager::sens_record->record_time);
		for (uint16_t i = 0; i < (sizeof(RecordManager::sens_record->sensors_statuses) / sizeof(*RecordManager::sens_record->sensors_statuses)); i++) {
			counter += serialize(&buffer[counter], RecordManager::sens_record->sensors_statuses[i]);
		}
		for (uint16_t i = 0; i < (sizeof(RecordManager::sens_record->sensors_values) / sizeof(*RecordManager::sens_record->sensors_values)); i++) {
			counter += serialize(&buffer[counter], RecordManager::sens_record->sensors_values[i]);
		}
		break;
	default:
		break;
	}
	return counter;
}

void CUPSlaveManager::load_settings_data_handler() {
	uint8_t* buffer = this->message.data;
	uint16_t counter = 0;

	SettingsManager::payload_settings_t tmpBuf = {0};
	counter += deserialize(&buffer[counter], &(tmpBuf.sens_record_period));
	counter += deserialize(&buffer[counter], &(tmpBuf.sens_transmit_period));
	for (uint16_t i = 0; i < (sizeof(tmpBuf.low_sens_status) / sizeof(*tmpBuf.low_sens_status)); i++) {
		counter += deserialize(&buffer[counter], &(tmpBuf.low_sens_status[i]));
	}
	for (uint16_t i = 0; i < (sizeof(tmpBuf.low_sens_register) / sizeof(tmpBuf.low_sens_register)); i++) {
		counter += deserialize(&buffer[counter], &(tmpBuf.low_sens_register[i]));
	}

	if (!tmpBuf.sens_record_period || !tmpBuf.sens_transmit_period) {
		LOG_BEDUG(MODULE_TAG, " unavailable settings\n");
		return;
	}

	LOG_BEDUG(MODULE_TAG, " save new settings\n");
	memcpy((uint8_t*)&(SettingsManager::sd_sttngs.v1.payload_settings), (uint8_t*)&tmpBuf, sizeof(SettingsManager::payload_settings_t));
	Debug_HexDump(MODULE_TAG, (uint8_t*)&(tmpBuf), sizeof(SettingsManager::payload_settings_t));
	SettingsManager::save();
}

void CUPSlaveManager::send_byte(uint8_t msg) {
	HAL_UART_Transmit(&CUP_UART, &msg, 1, CUP_WAIT_TIMEOUT);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == LOW_MB_UART_INSTC) {
		mb_rx_new_data((uint8_t)low_modbus_uart_val);
		HAL_UART_Receive_IT(&LOW_MB_UART, (uint8_t*)&low_modbus_uart_val, 1);
	} else if (huart->Instance == CUP_UART_INSTANCE) {
		CUP_m->char_data_handler(CUP_uart_val);
		CUP_TIM.Instance->CNT = 0x00;
		__HAL_TIM_CLEAR_FLAG(&CUP_TIM, TIM_SR_UIF);
		HAL_TIM_Base_Start_IT(&CUP_TIM);
		HAL_UART_Receive_IT(&CUP_UART, (uint8_t*)&CUP_uart_val, 1);
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == CUP_TIM_INSTANCE) {
		CUP_m->timeout();
		CUP_TIM.Instance->CNT = 0x00;
		__HAL_TIM_CLEAR_FLAG(&CUP_TIM, TIM_SR_UIF);
		HAL_TIM_Base_Stop_IT(&CUP_TIM);
	}
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_TIM1_Init();
  MX_USART6_UART_Init();
  MX_FATFS_Init();
  MX_UART4_Init();
  /* USER CODE BEGIN 2 */
  stngs_m = new SettingsManager();
  rcrd_m = new RecordManager();
  sens_m = new SensorManager();
  CUP_m = new CUPSlaveManager();

  HAL_GPIO_WritePin(BEDUG_LED_GPIO_Port, BEDUG_LED_Pin, GPIO_PIN_RESET);
  HAL_UART_Receive_IT(&LOW_MB_UART, (uint8_t*)&low_modbus_uart_val, 1);
  HAL_UART_Receive_IT(&CUP_UART, (uint8_t*)&CUP_uart_val, 1);
  /* USER CODE END 2 */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  printf("Hello world\n");
  while (1)
  {
    CUP_m->proccess();
	sens_m->proccess();
//	RecordManager::load(2);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 64;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
	Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
							  |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
	Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
#ifdef DEBUG
int _write(int file, uint8_t *ptr, int len) {
//	HAL_UART_Transmit(&CMD_UART, (uint8_t *) ptr, len, DEFAULT_UART_DELAY);
	for (int DataIdx = 0; DataIdx < len; DataIdx++) {
		ITM_SendChar(*ptr++);
	}
	return len;
}
#endif
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
