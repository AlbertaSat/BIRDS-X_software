/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/*
This file is part of VP-Digi.

VP-Digi is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

VP-Digi is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with VP-Digi.  If not, see <http://www.gnu.org/licenses/>.
*/
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "drivers/modem.h"
#include "ax25.h"
#include "drivers/uart.h"
#include "drivers/systick.h"
#include "stm32f1xx.h"
#include "digipeater.h"
#include "common.h"
#include "drivers/watchdog.h"
#include "beacon.h"
#include "terminal.h"
#include "config.h"
#include "default_settings.h"
#include "common.h"
#include "dra_system.h"
#include "mboss_handler.h"
#include "sys_reboot_reason.h"
#include "frame_handler.h"

#include <stdint.h>
#include <string.h>

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
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c2;

/* USER CODE BEGIN PV */
uint32_t value_adc_radfet_1;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C2_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


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

  SysTick_init();


  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C2_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */



	Wdog_init(); //initialize watchdog

  // flash code on-off-on-off for "boot complete"
  HAL_GPIO_WritePin(PIN_LED_D304_GPIO_Port, PIN_LED_D304_Pin, GPIO_PIN_SET);
  delay_ms(250);
  HAL_GPIO_WritePin(PIN_LED_D304_GPIO_Port, PIN_LED_D304_Pin, GPIO_PIN_RESET);
  delay_ms(250);
  HAL_GPIO_WritePin(PIN_LED_D304_GPIO_Port, PIN_LED_D304_Pin, GPIO_PIN_SET);
  delay_ms(250);
  HAL_GPIO_WritePin(PIN_LED_D304_GPIO_Port, PIN_LED_D304_Pin, GPIO_PIN_RESET);



	//set some initial values in case there is no configuration saved in memory
	uart1.baudrate = DEFAULT_UART1_BAUD_RATE;
	uart2.baudrate = DEFAULT_UART2_BAUD_RATE;
	afskCfg.usePWM = 0;
	afskCfg.flatAudioIn = 0;
	ax25Cfg.quietTime = 300;
	ax25Cfg.txDelayLength = 300;
	ax25Cfg.txTailLength = 30;
	digi.dupeTime = 30;

	Config_read();

	Ax25_init();

	uart_init(&uart1, USART1, uart1.baudrate);
	uart_init(&uart2, USART2, uart2.baudrate);

	uart_config(&uart1, 1);
	uart_config(&uart2, 1);

	Afsk_init();
	Beacon_init();

	// store reset reason at boot, because it can only be fetched once via `reset_cause_get()`
	this_boot_reset_cause = reset_cause_get();

	// set DRA enable pin
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET);

	// DRA init handled during mode setting now
	/*
	set_dra_awake_mode(1);
	delay_ms(100);
	send_dra_init_commands();
	*/

	// ADC Setup Stuff (https://wiki.st.com/stm32mcu/wiki/Getting_started_with_ADC)
	//HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
	//HAL_ADC_Start_DMA(&hadc1,(uint32_t*)&value_adc,1);

	// ADC Setup Stuff: https://controllerstech.com/stm32-adc-single-channel/
	HAL_ADC_Start(&hadc1); // start the adc

  init_ccd_adc();

	send_str_to_mboss("INFO: boot complete");
  
	autoResetTimer = autoReset * 360000;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	Wdog_reset();

	if(ax25.frameReceived) {
		handleFrame();
	}

	Digi_viscousRefresh(); //refresh viscous-delay buffers


	Ax25_transmitBuffer(); //transmit buffer (will return if nothing to be transmitted)

	Ax25_transmitCheck(); //check for pending transmission request

	if(uart1.rxflag != DATA_NOTHING)
	{
		term_parse(uart1.bufrx, uart1.bufrxidx, TERM_UART1, uart1.rxflag, uart1.mode);
		uart1.rxflag = DATA_NOTHING;
		uart1.bufrxidx = 0;
		memset(uart1.bufrx, 0, UARTBUFLEN);
	}
	if(uart2.rxflag != DATA_NOTHING)
	{
		term_parse(uart2.bufrx, uart2.bufrxidx, TERM_UART2, uart2.rxflag, uart2.mode);
		uart2.rxflag = DATA_NOTHING;
		uart2.bufrxidx = 0;
		memset(uart2.bufrx, 0, UARTBUFLEN);
	}

	Beacon_check(); //check beacons


	if(((autoResetTimer != 0) && (ticks > autoResetTimer)) || (ticks > 4294960000)) {
		// auto-reset timer expired, or ticks overflowed
		send_str_to_mboss("INFO: auto-reset timer expired. Resetting.");
		NVIC_SystemReset();
	}

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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL2;
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
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 10000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(PIN_LED_D303_GPIO_Port, PIN_LED_D303_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, PIN_CCD_PHI_M_Pin|PIN_CCD_ICG_Pin|PIN_CCD_SH_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, PIN_LED_D304_Pin|GPIO_PIN_11|RFET_EN_OUT_PIN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : PIN_LED_D303_Pin */
  GPIO_InitStruct.Pin = PIN_LED_D303_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(PIN_LED_D303_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PIN_CCD_PHI_M_Pin PIN_CCD_ICG_Pin PIN_CCD_SH_Pin */
  GPIO_InitStruct.Pin = PIN_CCD_PHI_M_Pin|PIN_CCD_ICG_Pin|PIN_CCD_SH_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PIN_LED_D304_Pin PA11 RFET_EN_OUT_PIN_Pin */
  GPIO_InitStruct.Pin = PIN_LED_D304_Pin|GPIO_PIN_11|RFET_EN_OUT_PIN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
