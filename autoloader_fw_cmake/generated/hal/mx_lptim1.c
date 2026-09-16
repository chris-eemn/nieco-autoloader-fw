/**
  ******************************************************************************
  * @file           : mx_lptim1.c
  * @brief          : LPTIM1 Peripheral initialization
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
#include "mx_lptim1.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions prototype------------------------------------------------*/

/* Exported variables by reference--------------------------------------------*/
static hal_lptim_handle_t hLPTIM1;
/* Exported function definition ----------------------------------------------*/
/******************************************************************************/
/* Exported functions for SW instance in HAL layer */
/******************************************************************************/

hal_lptim_handle_t *mx_lptim1_init(void)
{
  /* Init GPIO */
  /* ### LPTIM1 GPIO Configuration ########################### */
  /* GPIO Clocks activation */
  HAL_RCC_GPIOD_EnableClock();

  HAL_RCC_GPIOG_EnableClock();

  HAL_RCC_GPIOE_EnableClock();

  hal_gpio_config_t  gpio_config;

  /**
    [GPIO Pin] ------> [Signal Name] ------> [Labels]

       PD10    ------>   LPTIM1_CH2   ------>  PD10
    **/
  gpio_config.mode        = HAL_GPIO_MODE_ALTERNATE;
  gpio_config.output_type = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.pull        = HAL_GPIO_PULL_NO;
  gpio_config.speed       = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.alternate   = HAL_GPIO_AF_1;
  HAL_GPIO_Init(PD10_PORT, PD10_PIN, &gpio_config);

  /**
    [GPIO Pin] ------> [Signal Name] ------> [Labels]

       PG12    ------>   LPTIM1_IN1   ------>  PG12
       PG13    ------>   LPTIM1_CH1   ------>  PG13
    **/
  gpio_config.mode        = HAL_GPIO_MODE_ALTERNATE;
  gpio_config.output_type = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.pull        = HAL_GPIO_PULL_NO;
  gpio_config.speed       = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.alternate   = HAL_GPIO_AF_1;
  HAL_GPIO_Init(HAL_GPIOG, PG12_PIN | PG13_PIN, &gpio_config);

  /**
    [GPIO Pin] ------> [Signal Name] ------> [Labels]

       PE1     ------>   LPTIM1_IN2   ------>  PE1
    **/
  gpio_config.mode        = HAL_GPIO_MODE_ALTERNATE;
  gpio_config.output_type = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.pull        = HAL_GPIO_PULL_NO;
  gpio_config.speed       = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.alternate   = HAL_GPIO_AF_1;
  HAL_GPIO_Init(PE1_PORT, PE1_PIN, &gpio_config);

  hal_lptim_config_t  config;

  if (HAL_LPTIM_Init(&hLPTIM1, HAL_LPTIM1) != HAL_OK)
  {
    return NULL;
  }

  HAL_RCC_LPTIM1_EnableClock();

  if (HAL_RCC_LPTIM1_SetKernelClkSource(HAL_RCC_LPTIM1_CLK_SRC_PCLK3) != HAL_OK)
  {
    return NULL;
  }

  /* Timer configuration with external clock */
  config.mode               = HAL_LPTIM_CONTINUOUS;
  config.clock_source       = HAL_LPTIM_CLK_ENCODER_SUBMODE_3;
  config.period             = 0xFFFF;
  config.prescaler          = HAL_LPTIM_CLK_SRC_DIV1;
  config.repetition_counter = 0;

  if (HAL_LPTIM_SetConfig(&hLPTIM1, &config) != HAL_OK)
  {
    return NULL;
  }

  hal_lptim_ic_config_t ic_config1;
  ic_config1.source = HAL_LPTIM_INPUT_GPIO;
  ic_config1.polarity = HAL_LPTIM_IC_RISING;
  ic_config1.prescaler = HAL_LPTIM_IC_DIV1;
  ic_config1.filter = HAL_LPTIM_FDIV1;

  if (HAL_LPTIM_IC_SetConfigChannel(&hLPTIM1, HAL_LPTIM_CHANNEL_1, &ic_config1) != HAL_OK)
  {
    return NULL;
  }

  hal_lptim_ic_config_t ic_config2;
  ic_config2.source = HAL_LPTIM_INPUT_GPIO;
  ic_config2.polarity = HAL_LPTIM_IC_RISING;
  ic_config2.prescaler = HAL_LPTIM_IC_DIV1;
  ic_config2.filter = HAL_LPTIM_FDIV1;

  if (HAL_LPTIM_IC_SetConfigChannel(&hLPTIM1, HAL_LPTIM_CHANNEL_2, &ic_config2) != HAL_OK)
  {
    return NULL;
  }

  hal_lptim_encoder_config_t p_encoder;
  p_encoder.input1 = HAL_LPTIM_INPUT1_GPIO;
  p_encoder.input2 = HAL_LPTIM_INPUT2_GPIO;
  p_encoder.filter = HAL_LPTIM_FDIV1;
  if (HAL_LPTIM_SetConfigEncoder(&hLPTIM1, &p_encoder) != HAL_OK)
  {
    return NULL;
  }

  return &hLPTIM1;
}

void mx_lptim1_deinit(void)
{
  (void)HAL_LPTIM_DeInit(&hLPTIM1);

  HAL_RCC_LPTIM1_Reset();

  HAL_RCC_LPTIM1_DisableClock();

  /* De-initialize all GPIOD pins associated with LPTIM1 */
  HAL_GPIO_DeInit(PD10_PORT, PD10_PIN);

  /* De-initialize all GPIOG pins associated with LPTIM1 */
  HAL_GPIO_DeInit(HAL_GPIOG, PG12_PIN | PG13_PIN);

  /* De-initialize all GPIOE pins associated with LPTIM1 */
  HAL_GPIO_DeInit(PE1_PORT, PE1_PIN);
}

hal_lptim_handle_t *mx_lptim1_gethandle(void)
{
  return &hLPTIM1;
}
