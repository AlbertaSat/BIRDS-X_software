/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
extern I2C_HandleTypeDef hi2c2;
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
#define PIN_CCD_PHI_M_Pin GPIO_PIN_13
#define PIN_CCD_PHI_M_GPIO_Port GPIOB
#define PIN_CCD_ICG_Pin GPIO_PIN_14
#define PIN_CCD_ICG_GPIO_Port GPIOB
#define PIN_CCD_SH_Pin GPIO_PIN_15
#define PIN_CCD_SH_GPIO_Port GPIOB
#define PIN_DRA_ENABLE_Pin GPIO_PIN_11
#define PIN_DRA_ENABLE_GPIO_Port GPIOA
#define PIN_RFET_EN_OUT_Pin GPIO_PIN_12
#define PIN_RFET_EN_OUT_GPIO_Port GPIOA
#define PIN_LED_SUCCESS_Pin GPIO_PIN_3
#define PIN_LED_SUCCESS_GPIO_Port GPIOB
#define PIN_LED_FAILURE_Pin GPIO_PIN_4
#define PIN_LED_FAILURE_GPIO_Port GPIOB
#define PIN_LED_DCD_Pin GPIO_PIN_5
#define PIN_LED_DCD_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
