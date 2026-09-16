/**
  ******************************************************************************
  * @file           : mx_tim15.c
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
#include "mx_tim15.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions prototype------------------------------------------------*/
/* Exported variables by reference--------------------------------------------*/
static hal_tim_handle_t hTIM15;

/* Exported function definition ----------------------------------------------*/
/******************************************************************************/
/* Exported functions for TIM15 in HAL layer */
/******************************************************************************/
hal_tim_handle_t *mx_tim15_init(void)
{
  if (HAL_TIM_Init(&hTIM15, HAL_TIM15) != HAL_OK)
  {
    return NULL;
  }

  HAL_RCC_TIM15_EnableClock();

  /* Timer configuration with external clock */
  hal_tim_config_t config;
  config.prescaler              = 0;
  config.counter_mode           = HAL_TIM_COUNTER_UP;
  config.period                 = 0xFFFF;
  config.repetition_counter     = 0;
  config.clock_sel.clock_source = HAL_TIM_CLK_ENCODER_X4_TI12;
  if (HAL_TIM_SetConfig(&hTIM15, &config) != HAL_OK)
  {
    return NULL;
  }

  /* Sampling Clock */
  if (HAL_TIM_SetDTSPrescaler(&hTIM15, HAL_TIM_DTS_DIV1) != HAL_OK)
  {
    return NULL;
  }
  if (HAL_TIM_SetDTS2Prescaler(&hTIM15, HAL_TIM_DTS2_DIV1) != HAL_OK)
  {
    return NULL;
  }

  hal_tim_ic_channel_config_t ic_config;

  ic_config.source    = HAL_TIM_INPUT_TIM15_TI1_GPIO;
  ic_config.polarity  = HAL_TIM_IC_RISING;
  ic_config.filter    = HAL_TIM_FDIV1;
  if (HAL_TIM_IC_SetConfigChannel(&hTIM15, HAL_TIM_CHANNEL_1, &ic_config) != HAL_OK)
  {
    return NULL;
  }

  hal_tim_ic_capture_unit_config_t ic_capture_unit_config;

  ic_capture_unit_config.source     = HAL_TIM_IC_DIRECT;
  ic_capture_unit_config.prescaler  = HAL_TIM_IC_DIV1;
  if (HAL_TIM_IC_SetConfigCaptureUnit(&hTIM15, HAL_TIM_IC_CAPTURE_UNIT_1, &ic_capture_unit_config) != HAL_OK)
  {
    return NULL;
  }

  ic_config.source    = HAL_TIM_INPUT_TIM15_TI2_GPIO;
  ic_config.polarity  = HAL_TIM_IC_RISING;
  ic_config.filter    = HAL_TIM_FDIV1;
  if (HAL_TIM_IC_SetConfigChannel(&hTIM15, HAL_TIM_CHANNEL_2, &ic_config) != HAL_OK)
  {
    return NULL;
  }

  ic_capture_unit_config.source     = HAL_TIM_IC_DIRECT;
  ic_capture_unit_config.prescaler  = HAL_TIM_IC_DIV1;
  if (HAL_TIM_IC_SetConfigCaptureUnit(&hTIM15, HAL_TIM_IC_CAPTURE_UNIT_2, &ic_capture_unit_config) != HAL_OK)
  {
    return NULL;
  }

  /* Update Event Management */
  if (HAL_TIM_SetUpdateSource(&hTIM15, HAL_TIM_UPDATE_REGULAR) != HAL_OK)
  {
    return NULL;
  }
  if (HAL_TIM_EnableUpdateGeneration(&hTIM15) != HAL_OK)
  {
    return NULL;
  }
  /* Master Mode Configuration */
  /* ### TIM15 GPIO Configuration ########################### */
  /* GPIO Clocks activation */
  HAL_RCC_GPIOC_EnableClock();

  HAL_RCC_GPIOE_EnableClock();

  hal_gpio_config_t  gpio_config;

  /**
    [GPIO Pin] ------> [Signal Name] ------> [Labels]

       PC12    ------>   TIM15_CH1   ------>  PC12
    **/
  gpio_config.mode        = HAL_GPIO_MODE_ALTERNATE;
  gpio_config.output_type = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.pull        = HAL_GPIO_PULL_NO;
  gpio_config.speed       = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.alternate   = HAL_GPIO_AF_2;
  HAL_GPIO_Init(PC12_PORT, PC12_PIN, &gpio_config);

  /**
    [GPIO Pin] ------> [Signal Name] ------> [Labels]

       PE6     ------>   TIM15_CH2   ------>  TRACE_D3
    **/
  gpio_config.mode        = HAL_GPIO_MODE_ALTERNATE;
  gpio_config.output_type = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.pull        = HAL_GPIO_PULL_NO;
  gpio_config.speed       = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.alternate   = HAL_GPIO_AF_4;
  HAL_GPIO_Init(TRACE_D3_PORT, TRACE_D3_PIN, &gpio_config);

  return &hTIM15;
}

void mx_tim15_deinit(void)
{
  (void)HAL_TIM_DeInit(&hTIM15);

  HAL_RCC_TIM15_DisableClock();

  HAL_RCC_TIM15_Reset();

  /* De-initialize all GPIOC pins associated with TIM15 */
  HAL_GPIO_DeInit(PC12_PORT, PC12_PIN);

  /* De-initialize all GPIOE pins associated with TIM15 */
  HAL_GPIO_DeInit(TRACE_D3_PORT, TRACE_D3_PIN);
}

hal_tim_handle_t *mx_tim15_gethandle(void)
{
  return &hTIM15;
}
