/**
  ******************************************************************************
  * @file           : mx_hal_def.h
  * @brief          : Exporting peripherals initialization
  *                   Include entry for the target folder to the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef MX_HAL_DEF_H
#define MX_HAL_DEF_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "mx_def.h"
#include "stm32_hal.h"
#include "mx_cortex_mpu.h"
#include "mx_cortex_nvic.h"
#include "mx_gpio_default.h"
#include "mx_usb_drd_fs.h"
#include "mx_i2c1.h"
#include "mx_icache.h"
#include "mx_lptim1.h"
#include "mx_rcc.h"
#include "mx_tim1.h"
#include "mx_tim17.h"
#include "mx_tim12.h"
#include "mx_tim2.h"
#include "mx_tim7.h"
#include "mx_tim3.h"
#include "mx_tim4.h"
#include "mx_tim5.h"
#include "mx_tim8.h"
#include "mx_tim15.h"
#include "mx_usart2.h"
#include "mx_usart3.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* ########### Aliases to initialization functions ########### */

  /* *************************************************************
    Cortex_MPU: No software label has been defined for this peripheral instance
      in the STM32CubeMX2 configuration panel.
      As a result, no aliases are generated for mx_cortex_mpu_init
    ************************************************************* */

  /* *************************************************************
    Cortex_NVIC: No software label has been defined for this peripheral instance
      in the STM32CubeMX2 configuration panel.
      As a result, no aliases are generated for mx_cortex_nvic_init
    ************************************************************* */

  /* *************************************************************
    gpio_default: No software label has been defined for this peripheral instance
      in the STM32CubeMX2 configuration panel.
      As a result, no aliases are generated for mx_gpio_default_init
    ************************************************************* */

  /* *************************************************************
    USB_DRD_FS_HOST: No software label has been defined for this peripheral instance
      in the STM32CubeMX2 configuration panel.
      As a result, no aliases are generated for mx_usb_drd_fs_host_init
    ************************************************************* */

  /* *************************************************************
    I2C1: No software label has been defined for this peripheral instance
      in the STM32CubeMX2 configuration panel.
      As a result, no aliases are generated for mx_i2c1_i2c_init
    ************************************************************* */

  /* *************************************************************
    ICACHE: No software label has been defined for this peripheral instance
      in the STM32CubeMX2 configuration panel.
      As a result, no aliases are generated for mx_icache_init
    ************************************************************* */

  /* ***********************************************************
      LPTIM1: aliases for initialization functions
    *********************************************************** */

/**
  * @brief  Initialize the mx_lptim1 with HAL layer
  *         Name of the User label:
  *                   m8_encoder_timer
  * @retval hal_lptim_handle_t Pointer on the handle on the LPTIM1 instance
  */
#define m8_encoder_timer_init mx_lptim1_init

  /* *************************************************************
    RCC: No software label has been defined for this peripheral instance
      in the STM32CubeMX2 configuration panel.
      As a result, no aliases are generated for mx_rcc_init
    ************************************************************* */

  /* ***********************************************************
      TIM1: aliases for initialization functions
    *********************************************************** */

/**
  * @brief  Initialize the mx_tim1 with HAL layer
  *         Name of the User label:
  *                   m1_encoder_timer
  * @retval hal_tim_handle_t Pointer on the handle on the TIM1 instance
  */
#define m1_encoder_timer_init mx_tim1_init

  /* *************************************************************
    TIM17: No software label has been defined for this peripheral instance
      in the STM32CubeMX2 configuration panel.
      As a result, no aliases are generated for mx_tim17_init
    ************************************************************* */

  /* ***********************************************************
      TIM12: aliases for initialization functions
    *********************************************************** */

/**
  * @brief  Initialize the mx_tim12 with HAL layer
  *         Name of the User label:
  *                   step_timer
  * @retval hal_tim_handle_t Pointer on the handle on the TIM12 instance
  */
#define step_timer_init mx_tim12_init

  /* ***********************************************************
      TIM2: aliases for initialization functions
    *********************************************************** */

/**
  * @brief  Initialize the mx_tim2 with HAL layer
  *         Name of the User label:
  *                   m2_encoder_timer
  * @retval hal_tim_handle_t Pointer on the handle on the TIM2 instance
  */
#define m2_encoder_timer_init mx_tim2_init

  /* ***********************************************************
      TIM7: aliases for initialization functions
    *********************************************************** */

/**
  * @brief  Initialize the mx_tim7 with HAL layer
  *         Name of the User label:
  *                   mb_timer
  * @retval hal_tim_handle_t Pointer on the handle on the TIM7 instance
  */
#define mb_timer_init mx_tim7_init

  /* ***********************************************************
      TIM3: aliases for initialization functions
    *********************************************************** */

/**
  * @brief  Initialize the mx_tim3 with HAL layer
  *         Name of the User label:
  *                   m3_encoder_timer
  * @retval hal_tim_handle_t Pointer on the handle on the TIM3 instance
  */
#define m3_encoder_timer_init mx_tim3_init

  /* ***********************************************************
      TIM4: aliases for initialization functions
    *********************************************************** */

/**
  * @brief  Initialize the mx_tim4 with HAL layer
  *         Name of the User label:
  *                   m4_encoder_timer
  * @retval hal_tim_handle_t Pointer on the handle on the TIM4 instance
  */
#define m4_encoder_timer_init mx_tim4_init

  /* ***********************************************************
      TIM5: aliases for initialization functions
    *********************************************************** */

/**
  * @brief  Initialize the mx_tim5 with HAL layer
  *         Name of the User label:
  *                   m5_encoder_timer
  * @retval hal_tim_handle_t Pointer on the handle on the TIM5 instance
  */
#define m5_encoder_timer_init mx_tim5_init

  /* ***********************************************************
      TIM8: aliases for initialization functions
    *********************************************************** */

/**
  * @brief  Initialize the mx_tim8 with HAL layer
  *         Name of the User label:
  *                   m6_encoder_timer
  * @retval hal_tim_handle_t Pointer on the handle on the TIM8 instance
  */
#define m6_encoder_timer_init mx_tim8_init

  /* ***********************************************************
      TIM15: aliases for initialization functions
    *********************************************************** */

/**
  * @brief  Initialize the mx_tim15 with HAL layer
  *         Name of the User label:
  *                   m7_encoder_timer
  * @retval hal_tim_handle_t Pointer on the handle on the TIM15 instance
  */
#define m7_encoder_timer_init mx_tim15_init

  /* *************************************************************
    USART2: No software label has been defined for this peripheral instance
      in the STM32CubeMX2 configuration panel.
      As a result, no aliases are generated for mx_usart2_uart_init
    ************************************************************* */

  /* ***********************************************************
      USART3: aliases for initialization functions
    *********************************************************** */

/**
  * @brief  Initialize the mx_usart3_uart with HAL layer
  *         Name of the User label:
  *                   mb_slave
  * @retval hal_uart_handle_t Pointer on the handle on the USART3 instance
  */
#define mb_slave_init mx_usart3_uart_init

/* ########################################################### */

/* ########### Aliases to De-Initialization functions ########### */

  /* *************************************************************
    Cortex_MPU: No software label has been defined for this peripheral instance
      in the STM32CubeMX2 configuration panel.
      As a result, no aliases are generated for mx_cortex_mpu_deinit
    ************************************************************* */

  /* *************************************************************
    Cortex_NVIC: No software label has been defined for this peripheral instance
      in the STM32CubeMX2 configuration panel.
      As a result, no aliases are generated for mx_cortex_nvic_deinit
    ************************************************************* */

  /* *************************************************************
    gpio_default: No software label has been defined for this peripheral instance
      in the STM32CubeMX2 configuration panel.
      As a result, no aliases are generated for mx_gpio_default_deinit
    ************************************************************* */

  /* *************************************************************
    USB_DRD_FS_HOST: No software label has been defined for this peripheral instance
      in the STM32CubeMX2 configuration panel.
      As a result, no aliases are generated for mx_usb_drd_fs_host_deinit
    ************************************************************* */

  /* *************************************************************
    I2C1: No software label has been defined for this peripheral instance
      in the STM32CubeMX2 configuration panel.
      As a result, no aliases are generated for mx_i2c1_i2c_deinit
    ************************************************************* */

  /* *************************************************************
    ICACHE: No software label has been defined for this peripheral instance
      in the STM32CubeMX2 configuration panel.
      As a result, no aliases are generated for mx_icache_deinit
    ************************************************************* */

  /* ***********************************************************
      LPTIM1: aliases for De-Initialization functions
    *********************************************************** */

/**
  * @brief  De-Initialize the mx_lptim1 with HAL layer
  *         Name of the User label:
  *                   m8_encoder_timer
  */
#define m8_encoder_timer_deinit mx_lptim1_deinit

  /* ***********************************************************
      TIM1: aliases for De-Initialization functions
    *********************************************************** */

/**
  * @brief  De-Initialize the mx_tim1 with HAL layer
  *         Name of the User label:
  *                   m1_encoder_timer
  */
#define m1_encoder_timer_deinit mx_tim1_deinit

  /* *************************************************************
    TIM17: No software label has been defined for this peripheral instance
      in the STM32CubeMX2 configuration panel.
      As a result, no aliases are generated for mx_tim17_deinit
    ************************************************************* */

  /* ***********************************************************
      TIM12: aliases for De-Initialization functions
    *********************************************************** */

/**
  * @brief  De-Initialize the mx_tim12 with HAL layer
  *         Name of the User label:
  *                   step_timer
  */
#define step_timer_deinit mx_tim12_deinit

  /* ***********************************************************
      TIM2: aliases for De-Initialization functions
    *********************************************************** */

/**
  * @brief  De-Initialize the mx_tim2 with HAL layer
  *         Name of the User label:
  *                   m2_encoder_timer
  */
#define m2_encoder_timer_deinit mx_tim2_deinit

  /* ***********************************************************
      TIM7: aliases for De-Initialization functions
    *********************************************************** */

/**
  * @brief  De-Initialize the mx_tim7 with HAL layer
  *         Name of the User label:
  *                   mb_timer
  */
#define mb_timer_deinit mx_tim7_deinit

  /* ***********************************************************
      TIM3: aliases for De-Initialization functions
    *********************************************************** */

/**
  * @brief  De-Initialize the mx_tim3 with HAL layer
  *         Name of the User label:
  *                   m3_encoder_timer
  */
#define m3_encoder_timer_deinit mx_tim3_deinit

  /* ***********************************************************
      TIM4: aliases for De-Initialization functions
    *********************************************************** */

/**
  * @brief  De-Initialize the mx_tim4 with HAL layer
  *         Name of the User label:
  *                   m4_encoder_timer
  */
#define m4_encoder_timer_deinit mx_tim4_deinit

  /* ***********************************************************
      TIM5: aliases for De-Initialization functions
    *********************************************************** */

/**
  * @brief  De-Initialize the mx_tim5 with HAL layer
  *         Name of the User label:
  *                   m5_encoder_timer
  */
#define m5_encoder_timer_deinit mx_tim5_deinit

  /* ***********************************************************
      TIM8: aliases for De-Initialization functions
    *********************************************************** */

/**
  * @brief  De-Initialize the mx_tim8 with HAL layer
  *         Name of the User label:
  *                   m6_encoder_timer
  */
#define m6_encoder_timer_deinit mx_tim8_deinit

  /* ***********************************************************
      TIM15: aliases for De-Initialization functions
    *********************************************************** */

/**
  * @brief  De-Initialize the mx_tim15 with HAL layer
  *         Name of the User label:
  *                   m7_encoder_timer
  */
#define m7_encoder_timer_deinit mx_tim15_deinit

  /* *************************************************************
    USART2: No software label has been defined for this peripheral instance
      in the STM32CubeMX2 configuration panel.
      As a result, no aliases are generated for mx_usart2_uart_deinit
    ************************************************************* */

  /* ***********************************************************
      USART3: aliases for De-Initialization functions
    *********************************************************** */

/**
  * @brief  De-Initialize the mx_usart3_uart with HAL layer
  *         Name of the User label:
  *                   mb_slave
  */
#define mb_slave_deinit mx_usart3_uart_deinit

/* ########################################################### */

/* ########### Aliases to get HAL handle functions ########### */

  /* ***********************************************************
      gpio_default/EXTI: aliases for EXTI get handle functions
    *********************************************************** */

/**
  * @brief  Get the EXTI handle for EXTI6 used for gpio_default
  * @retval hal_exti_handle_t Pointer on the handle on the EXTI6 used by gpio_default instance
  */
#define m1_fault_exti_gethandle mx_gpio_default_exti6_gethandle

/**
  * @brief  Get the EXTI handle for EXTI14 used for gpio_default
  * @retval hal_exti_handle_t Pointer on the handle on the EXTI14 used by gpio_default instance
  */
#define m2_fault_exti_gethandle mx_gpio_default_exti14_gethandle

  /* *************************************************************
    USB_DRD_FS_HOST: No software label has been defined for this peripheral instance
      in the STM32CubeMX2 configuration panel.
      As a result, no aliases are generated for mx_usb_drd_fs_host_gethandle
    ************************************************************* */

  /* *************************************************************
    I2C1: No software label has been defined for this peripheral instance
      in the STM32CubeMX2 configuration panel.
      As a result, no aliases are generated for mx_i2c1_i2c_gethandle
    ************************************************************* */

  /* *************************************************************
    ICACHE: No software label has been defined for this peripheral instance
      in the STM32CubeMX2 configuration panel.
      As a result, no aliases are generated for mx_icache_gethandle
    ************************************************************* */

  /* ***********************************************************
      LPTIM1: aliases for get handle functions
    *********************************************************** *//**
  * @brief  Get the HAL handle for LPTIM1
  *         Name of the User label:
  *                   m8_encoder_timer
  * @retval hal_lptim_handle_t Pointer on the handle on the LPTIM1 instance
  */
#define m8_encoder_timer_gethandle mx_lptim1_gethandle

  /* ***********************************************************
      TIM1: aliases for get handle functions
    *********************************************************** *//**
  * @brief  Get the HAL handle for TIM1
  *         Name of the User label:
  *                   m1_encoder_timer
  * @retval hal_tim_handle_t Pointer on the handle on the TIM1 instance
  */
#define m1_encoder_timer_gethandle mx_tim1_gethandle

  /* *************************************************************
    TIM17: No software label has been defined for this peripheral instance
      in the STM32CubeMX2 configuration panel.
      As a result, no aliases are generated for mx_tim17_gethandle
    ************************************************************* */

  /* ***********************************************************
      TIM12: aliases for get handle functions
    *********************************************************** *//**
  * @brief  Get the HAL handle for TIM12
  *         Name of the User label:
  *                   step_timer
  * @retval hal_tim_handle_t Pointer on the handle on the TIM12 instance
  */
#define step_timer_gethandle mx_tim12_gethandle

  /* ***********************************************************
      TIM2: aliases for get handle functions
    *********************************************************** *//**
  * @brief  Get the HAL handle for TIM2
  *         Name of the User label:
  *                   m2_encoder_timer
  * @retval hal_tim_handle_t Pointer on the handle on the TIM2 instance
  */
#define m2_encoder_timer_gethandle mx_tim2_gethandle

  /* ***********************************************************
      TIM7: aliases for get handle functions
    *********************************************************** *//**
  * @brief  Get the HAL handle for TIM7
  *         Name of the User label:
  *                   mb_timer
  * @retval hal_tim_handle_t Pointer on the handle on the TIM7 instance
  */
#define mb_timer_gethandle mx_tim7_gethandle

  /* ***********************************************************
      TIM3: aliases for get handle functions
    *********************************************************** *//**
  * @brief  Get the HAL handle for TIM3
  *         Name of the User label:
  *                   m3_encoder_timer
  * @retval hal_tim_handle_t Pointer on the handle on the TIM3 instance
  */
#define m3_encoder_timer_gethandle mx_tim3_gethandle

  /* ***********************************************************
      TIM4: aliases for get handle functions
    *********************************************************** *//**
  * @brief  Get the HAL handle for TIM4
  *         Name of the User label:
  *                   m4_encoder_timer
  * @retval hal_tim_handle_t Pointer on the handle on the TIM4 instance
  */
#define m4_encoder_timer_gethandle mx_tim4_gethandle

  /* ***********************************************************
      TIM5: aliases for get handle functions
    *********************************************************** *//**
  * @brief  Get the HAL handle for TIM5
  *         Name of the User label:
  *                   m5_encoder_timer
  * @retval hal_tim_handle_t Pointer on the handle on the TIM5 instance
  */
#define m5_encoder_timer_gethandle mx_tim5_gethandle

  /* ***********************************************************
      TIM8: aliases for get handle functions
    *********************************************************** *//**
  * @brief  Get the HAL handle for TIM8
  *         Name of the User label:
  *                   m6_encoder_timer
  * @retval hal_tim_handle_t Pointer on the handle on the TIM8 instance
  */
#define m6_encoder_timer_gethandle mx_tim8_gethandle

  /* ***********************************************************
      TIM15: aliases for get handle functions
    *********************************************************** *//**
  * @brief  Get the HAL handle for TIM15
  *         Name of the User label:
  *                   m7_encoder_timer
  * @retval hal_tim_handle_t Pointer on the handle on the TIM15 instance
  */
#define m7_encoder_timer_gethandle mx_tim15_gethandle

  /* *************************************************************
    USART2: No software label has been defined for this peripheral instance
      in the STM32CubeMX2 configuration panel.
      As a result, no aliases are generated for mx_usart2_uart_gethandle
    ************************************************************* */

  /* ***********************************************************
      USART3: aliases for get handle functions
    *********************************************************** *//**
  * @brief  Get the HAL handle for USART3
  *         Name of the User label:
  *                   mb_slave
  * @retval hal_uart_handle_t Pointer on the handle on the USART3 instance
  */
#define mb_slave_gethandle mx_usart3_uart_gethandle

/* ########################################################### */

/* ########### Aliases to get IRQ Handlers functions ########### */
  /* ########################################################### */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MX_HAL_DEF_H */
