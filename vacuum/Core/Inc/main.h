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
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal.h"
#include "stm32g0xx_ll_crc.h"
#include "stm32g0xx_ll_rcc.h"
#include "stm32g0xx_ll_bus.h"
#include "stm32g0xx_ll_system.h"
#include "stm32g0xx_ll_exti.h"
#include "stm32g0xx_ll_cortex.h"
#include "stm32g0xx_ll_utils.h"
#include "stm32g0xx_ll_pwr.h"
#include "stm32g0xx_ll_dma.h"
#include "stm32g0xx_ll_gpio.h"

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

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Vi1sns_Pin LL_GPIO_PIN_0
#define Vi1sns_GPIO_Port GPIOA
#define Vi2sns_Pin LL_GPIO_PIN_1
#define Vi2sns_GPIO_Port GPIOA
#define Psns2_Pin LL_GPIO_PIN_2
#define Psns2_GPIO_Port GPIOA
#define id2_Pin LL_GPIO_PIN_3
#define id2_GPIO_Port GPIOA
#define Psns1_Pin LL_GPIO_PIN_6
#define Psns1_GPIO_Port GPIOA
#define id1_Pin LL_GPIO_PIN_7
#define id1_GPIO_Port GPIOA
#define ADDR0_Pin LL_GPIO_PIN_0
#define ADDR0_GPIO_Port GPIOD
#define ADDR1_Pin LL_GPIO_PIN_1
#define ADDR1_GPIO_Port GPIOD
#define ADDR2_Pin LL_GPIO_PIN_2
#define ADDR2_GPIO_Port GPIOD
#define ADDR3_Pin LL_GPIO_PIN_3
#define ADDR3_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */
extern ADC_HandleTypeDef hadc1;
extern I2C_HandleTypeDef hi2c1;
extern LPTIM_HandleTypeDef hlptim1;
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern UART_HandleTypeDef huart1;

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
