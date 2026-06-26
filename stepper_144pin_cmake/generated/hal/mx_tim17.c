/**
  ******************************************************************************
  * @file           : mx_tim17.c
  * @brief          : Peripheral initialization
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the mx_stm32c5xx_hal_drivers_license.md file
  * in the same directory as the generated code.
  * If no mx_stm32c5xx_hal_drivers_license.md file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "mx_tim17.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions prototype------------------------------------------------*/
/* Exported variables by reference--------------------------------------------*/
static hal_tim_handle_t hTIM17;

/* Exported function definition ----------------------------------------------*/
/******************************************************************************/
/* Exported functions for TIM17 in HAL layer */
/******************************************************************************/
hal_tim_handle_t *mx_tim17_init(void)
{
  if (HAL_TIM_Init(&hTIM17, HAL_TIM17) != HAL_OK)
  {
    return NULL;
  }

  HAL_RCC_TIM17_EnableClock();

  /* No GPIO configuration required for TIM17 */
  /* Enable the Timer global interrupt */
  HAL_CORTEX_NVIC_SetPriority(TIM17_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_EnableIRQ(TIM17_IRQn);

  return &hTIM17;
}

void mx_tim17_deinit(void)
{
  (void)HAL_TIM_DeInit(&hTIM17);

  HAL_RCC_TIM17_DisableClock();

  HAL_RCC_TIM17_Reset();

  /* No GPIO de-initialization required for TIM17 */
  /* Disable Timer global interrupt */
  HAL_CORTEX_NVIC_DisableIRQ(TIM17_IRQn);
}

hal_tim_handle_t *mx_tim17_gethandle(void)
{
  return &hTIM17;
}

/******************************************************************************/
/*                           TIM17 global interrupt                           */
/******************************************************************************/
void TIM17_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&hTIM17);
}
