/**
  ******************************************************************************
  * @file           : mx_gpio_default.c
  * @brief          : gpio_default Peripheral initialization
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
#include "mx_gpio_default.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/* Exported variables by reference -------------------------------------------*/

/******************************************************************************/
/* Exported functions for GPIO in HAL layer                                   */
/******************************************************************************/
system_status_t mx_gpio_default_init(void)
{
  hal_gpio_config_t  gpio_config;

  HAL_RCC_GPIOA_EnableClock();

  HAL_RCC_GPIOC_EnableClock();

  HAL_RCC_GPIOD_EnableClock();

  /*
    GPIO pin labels :
    PA0   ---------> PA0, M1_NSLP, M1_NSLP
    PA10  ---------> PA10, M1_EN, M1_EN
    */
  /* Configure PA0, PA10 GPIO pins in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = HAL_GPIO_PIN_RESET;
  if (HAL_GPIO_Init(HAL_GPIOA, PA0_PIN | PA10_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PC6   ---------> PC6, M1_DIR, M1_DIR
    */
  /* Configure PC6 GPIO pin in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = PC6_INIT_STATE;
  if (HAL_GPIO_Init(PC6_PORT, PC6_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PD5   ---------> PD5, M1_STEP, M1_STEP
    */
  /* Configure PD5 GPIO pin in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = PD5_INIT_STATE;
  if (HAL_GPIO_Init(PD5_PORT, PD5_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PD6   ---------> PD6, M1_NFAULT, M1_NFAULT
    */
  /* Configure PD6 GPIO pin in input mode */
  gpio_config.mode            = HAL_GPIO_MODE_INPUT;
  gpio_config.pull            = HAL_GPIO_PULL_UP;
  if (HAL_GPIO_Init(PD6_PORT, PD6_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  return SYSTEM_OK;
}

system_status_t mx_gpio_default_deinit(void)
{
  /* De-initialize pins of GPIOA port */
  HAL_GPIO_DeInit(HAL_GPIOA, PA0_PIN | PA10_PIN);

  /* De-initialize pins of GPIOC port */
  HAL_GPIO_DeInit(PC6_PORT, PC6_PIN);

  /* De-initialize pins of GPIOD port */
  HAL_GPIO_DeInit(HAL_GPIOD, PD5_PIN | PD6_PIN);

  return SYSTEM_OK;
}
