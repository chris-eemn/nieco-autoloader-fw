/**
  ******************************************************************************
  * @file           : mx_spi2.h
  * @brief          : Header for mx_spi2.c file.
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
#ifndef MX_SPI2_H
#define MX_SPI2_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "stm32_hal.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/** Primary aliases for SPI2_SCK pin */
#define PA9_PORT                              HAL_GPIOA
#define PA9_PIN                               HAL_GPIO_PIN_9

/** Secondary aliases for SPI2_SCK pin */
#define SPI_FLASH_SCK_PORT                    HAL_GPIOA
#define SPI_FLASH_SCK_PIN                     HAL_GPIO_PIN_9

/** Primary aliases for SPI2_MISO pin */
#define PC2_PORT                              HAL_GPIOC
#define PC2_PIN                               HAL_GPIO_PIN_2

/** Secondary aliases for SPI2_MISO pin */
#define SPI_FLASH_MISO_PORT                   HAL_GPIOC
#define SPI_FLASH_MISO_PIN                    HAL_GPIO_PIN_2

/** Primary aliases for SPI2_MOSI pin */
#define PC3_PORT                              HAL_GPIOC
#define PC3_PIN                               HAL_GPIO_PIN_3

/** Secondary aliases for SPI2_MOSI pin */
#define SPI_FLASH_MOSI_PORT                   HAL_GPIOC
#define SPI_FLASH_MOSI_PIN                    HAL_GPIO_PIN_3

/* Exported macros -----------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/******************************************************************************/
/* Exported functions for SPI in HAL layer */
/******************************************************************************/
/**
  * @brief mx_spi2 init function
  * This function configures the hardware resources used in this example
  * @retval pointer to handle or NULL in case of failure
  */
hal_spi_handle_t *mx_spi2_init(void);

/**
  * @brief  De-initialize spi2 instance and return it.
  */
void mx_spi2_deinit(void);

/**
  * @brief  Get the SPI2 object.
  * @retval Pointer on the SPI2 Handle
  */
hal_spi_handle_t *mx_spi2_gethandle(void);

/******************************************************************************/
/*                           SPI2 global interrupt                            */
/******************************************************************************/
void SPI2_IRQHandler(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MX_SPI2_H */
