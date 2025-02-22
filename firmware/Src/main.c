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
#include "experiments.h"
#include "experiment_ccd.h"

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
I2C_HandleTypeDef hi2c2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C2_Init(void);
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
  /* USER CODE BEGIN 2 */

	Wdog_init(); //initialize watchdog

  // flash code on-off-on-off for "boot complete"
  for (uint8_t i = 0; i < 2; i++) {
      HAL_GPIO_WritePin(PIN_LED_SUCCESS_GPIO_Port, PIN_LED_SUCCESS_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(PIN_LED_FAILURE_GPIO_Port, PIN_LED_FAILURE_Pin, GPIO_PIN_RESET);
      delay_ms(250);
      HAL_GPIO_WritePin(PIN_LED_SUCCESS_GPIO_Port, PIN_LED_SUCCESS_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(PIN_LED_FAILURE_GPIO_Port, PIN_LED_FAILURE_Pin, GPIO_PIN_SET);
      delay_ms(250);
  }

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

  write_radfet_enable(0);
  set_resting_ccd_state(); // lowest power state

	// store reset reason at boot, because it can only be fetched once via `reset_cause_get()`
	this_boot_reset_cause = reset_cause_get();

  #if 0 // FIXME: just for testing
  init_ccd_adc();

  delay_ms(1000);

  //uint8_t fetched_data[CCD_DATA_LEN_BYTES];
  while (1) {
    //query_ccd_measurement(fetched_data, 1);

    fetch_ccd_measurement_and_log_it(2);

    // delay_ms(1000);
    // Wdog_reset();

    for (uint16_t i = 0; i < 1000; i++) { // 10,000 gives 7.5ms (1.3 MHz?)
      asm("NOP");
    }
  }

  #endif

  // These ADC inits are called directly when they're neede to save power on boot
  //init_ccd_adc();
  //init_adc_radfets();

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

	Loop_process_incoming_uart_commands(1, 1);

	Beacon_check(); //check beacons
  
  loop_service_ccd_experiment();

  // turn off success/failure LED, if it's been long enough
  if (get_system_uptime_ms() - uptime_at_last_success_failure_led_on > LED_SUCCESS_FAILURE_MIN_TIME_ON_MS) {
    HAL_GPIO_WritePin(PIN_LED_FAILURE_GPIO_Port, PIN_LED_FAILURE_Pin, GPIO_PIN_RESET);
	  HAL_GPIO_WritePin(PIN_LED_SUCCESS_GPIO_Port, PIN_LED_SUCCESS_Pin, GPIO_PIN_RESET);
  }

  if (get_system_uptime_ms() - timestamp_rx_led_turned_on_ms > LED_SUCCESS_FAILURE_MIN_TIME_ON_MS) {
    set_packet_received_led(0);
  }

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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
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
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, PIN_CCD_PHI_M_Pin|PIN_CCD_ICG_Pin|PIN_CCD_SH_Pin|PIN_LED_SUCCESS_Pin
                          |PIN_LED_FAILURE_Pin|PIN_LED_DCD_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, PIN_DRA_ENABLE_Pin|PIN_RFET_EN_OUT_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : PIN_CCD_PHI_M_Pin PIN_CCD_ICG_Pin PIN_CCD_SH_Pin PIN_LED_SUCCESS_Pin
                           PIN_LED_FAILURE_Pin PIN_LED_DCD_Pin */
  GPIO_InitStruct.Pin = PIN_CCD_PHI_M_Pin|PIN_CCD_ICG_Pin|PIN_CCD_SH_Pin|PIN_LED_SUCCESS_Pin
                          |PIN_LED_FAILURE_Pin|PIN_LED_DCD_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PIN_DRA_ENABLE_Pin PIN_RFET_EN_OUT_Pin */
  GPIO_InitStruct.Pin = PIN_DRA_ENABLE_Pin|PIN_RFET_EN_OUT_Pin;
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
