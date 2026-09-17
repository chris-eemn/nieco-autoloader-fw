/**
  ******************************************************************************
  * @file           : mx_system.c
  * @brief          : STM32 system program body
  *                   Applicative target system level initialization
  *                   (system clock, cache, TZ...) and system level peripherals
  *                   initialization. mx_system_init() service is called by the main
  *                   before jumping the example entry point.
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
#include "mx_system.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions prototype------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/

system_status_t mx_system_init(void)
{
  if (pre_system_init_hook() != SYSTEM_OK)
  {
    return SYSTEM_PRESYSTEM_ERROR;
  }

  /*
    CORTEX MPU initialization in case of isolation is not activated
  */
  if (mx_cortex_mpu_init() != SYSTEM_OK)
  {
    return SYSTEM_RESOURCES_ISOLATION_ERROR;
  }

  /*
    startup system section
  */
  if (HAL_Init() != HAL_OK)
  {
    return SYSTEM_STARTUP_ERROR;
  }

  /*
    Interruptions section
  */
  if (mx_cortex_nvic_init() != SYSTEM_OK)
  {
    return SYSTEM_INTERRUPTS_ERROR;
  }

  /*
    ICACHE section
  */
  if (mx_icache_init() == NULL)
  {
    return SYSTEM_STARTUP_ERROR;
  }

  /* ICACHE automatically started at startup */
  if (HAL_ICACHE_Start(mx_icache_gethandle(), HAL_ICACHE_IT_NONE) != HAL_OK)
  {
    return SYSTEM_STARTUP_ERROR;
  }

  /*
    Peripheral init section
  */

  /** gpio_default */
  if (mx_gpio_default_init() != SYSTEM_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /** mx_tim17_init() has been generated,
    * but TIM17 is used as timebase
    * then it is initialized in stm32_hal_timebase_tim.c.
    */

  /** LPTIM1 */
  if (mx_lptim1_init() == NULL)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /** TIM2 */
  if (mx_tim2_init() == NULL)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /** TIM15 */
  if (mx_tim15_init() == NULL)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /** TIM4 */
  if (mx_tim4_init() == NULL)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /** TIM5 */
  if (mx_tim5_init() == NULL)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /** TIM8 */
  if (mx_tim8_init() == NULL)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /** TIM3 */
  if (mx_tim3_init() == NULL)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /** TIM1 */
  if (mx_tim1_init() == NULL)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /** USART3 */
  if (mx_usart3_uart_init() == NULL)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /** UART4 */
  if (mx_uart4_uart_init() == NULL)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /** USART2 */
  if (mx_usart2_uart_init() == NULL)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /** I2C1 */
  if (mx_i2c1_i2c_init() == NULL)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /** SPI2 */
  if (mx_spi2_init() == NULL)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /** USB_DRD_FS_HOST */
  if (mx_usb_drd_fs_host_init() == NULL)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /** TIM12 */
  if (mx_tim12_init() == NULL)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /** TIM7 */
  if (mx_tim7_init() == NULL)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  if (post_system_init_hook() != SYSTEM_OK)
  {
    return SYSTEM_POSTSYSTEM_ERROR;
  }

  return SYSTEM_OK;
}

/**
  * @brief  User hook function called before the HAL_Init() function
  * @retval system_status_t System status
  */
__WEAK system_status_t pre_system_init_hook(void)
{
  /* NOTE : This function must not be modified. When the callback is needed,
            the pre_system_init_hook can be implemented in the user file
   */
  return SYSTEM_OK;
}

/**
  * @brief  User hook function called after the HAL_Init() and Peripheral init functions
  * @retval system_status_t System status
  */
__WEAK system_status_t post_system_init_hook(void)
{
  /* NOTE : This function must not be modified. When the callback is needed,
            the post_system_init_hook can be implemented in the user file
   */
  return SYSTEM_OK;
}
