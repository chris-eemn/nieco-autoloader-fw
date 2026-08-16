/**
  ******************************************************************************
  * @file           : mx_gpio_default.h
  * @brief          : Header for mx_gpio_default.c file.
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
#ifndef MX_GPIO_DEFAULT_H
#define MX_GPIO_DEFAULT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "stm32_hal.h"
#include "mx_def.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/******************************************************************************/
/* Exported defines for gpio_default in HAL layer                             */
/******************************************************************************/

/* Primary aliases for GPIO PA0 pin */
#define PA0_PORT                                        HAL_GPIOA
#define PA0_PIN                                         HAL_GPIO_PIN_0
#define PA0_INIT_STATE                                  HAL_GPIO_PIN_SET
#define PA0_ACTIVE_STATE                                HAL_GPIO_PIN_RESET
#define PA0_INACTIVE_STATE                              HAL_GPIO_PIN_SET

/* Secondary aliases for GPIO PA0 pin */
#define SPI_FLASH_CS_PORT                               HAL_GPIOA
#define SPI_FLASH_CS_PIN                                HAL_GPIO_PIN_0
#define SPI_FLASH_CS_INIT_STATE                         HAL_GPIO_PIN_SET
#define SPI_FLASH_CS_ACTIVE_STATE                       HAL_GPIO_PIN_RESET
#define SPI_FLASH_CS_INACTIVE_STATE                     HAL_GPIO_PIN_SET

#define SPI_FLASH_CS_PORT                               HAL_GPIOA
#define SPI_FLASH_CS_PIN                                HAL_GPIO_PIN_0
#define SPI_FLASH_CS_INIT_STATE                         HAL_GPIO_PIN_SET
#define SPI_FLASH_CS_ACTIVE_STATE                       HAL_GPIO_PIN_RESET
#define SPI_FLASH_CS_INACTIVE_STATE                     HAL_GPIO_PIN_SET

/* Primary aliases for GPIO PA4 pin */
#define PA4_PORT                                        HAL_GPIOA
#define PA4_PIN                                         HAL_GPIO_PIN_4
#define PA4_INIT_STATE                                  HAL_GPIO_PIN_RESET
#define PA4_ACTIVE_STATE                                HAL_GPIO_PIN_SET
#define PA4_INACTIVE_STATE                              HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PA4 pin */
#define M4_STEP_PORT                                    HAL_GPIOA
#define M4_STEP_PIN                                     HAL_GPIO_PIN_4
#define M4_STEP_INIT_STATE                              HAL_GPIO_PIN_RESET
#define M4_STEP_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define M4_STEP_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

#define M4_STEP_PORT                                    HAL_GPIOA
#define M4_STEP_PIN                                     HAL_GPIO_PIN_4
#define M4_STEP_INIT_STATE                              HAL_GPIO_PIN_RESET
#define M4_STEP_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define M4_STEP_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PA10 pin */
#define PA10_PORT                                       HAL_GPIOA
#define PA10_PIN                                        HAL_GPIO_PIN_10
#define PA10_INIT_STATE                                 HAL_GPIO_PIN_RESET
#define PA10_ACTIVE_STATE                               HAL_GPIO_PIN_SET
#define PA10_INACTIVE_STATE                             HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PA10 pin */
#define M1_EN_PORT                                      HAL_GPIOA
#define M1_EN_PIN                                       HAL_GPIO_PIN_10
#define M1_EN_INIT_STATE                                HAL_GPIO_PIN_RESET
#define M1_EN_ACTIVE_STATE                              HAL_GPIO_PIN_SET
#define M1_EN_INACTIVE_STATE                            HAL_GPIO_PIN_RESET

#define M1_EN_PORT                                      HAL_GPIOA
#define M1_EN_PIN                                       HAL_GPIO_PIN_10
#define M1_EN_INIT_STATE                                HAL_GPIO_PIN_RESET
#define M1_EN_ACTIVE_STATE                              HAL_GPIO_PIN_SET
#define M1_EN_INACTIVE_STATE                            HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PB0 pin */
#define PB0_PORT                                        HAL_GPIOB
#define PB0_PIN                                         HAL_GPIO_PIN_0
#define PB0_INIT_STATE                                  HAL_GPIO_PIN_RESET
#define PB0_ACTIVE_STATE                                HAL_GPIO_PIN_SET
#define PB0_INACTIVE_STATE                              HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PB0 pin */
#define M4_DIR_PORT                                     HAL_GPIOB
#define M4_DIR_PIN                                      HAL_GPIO_PIN_0
#define M4_DIR_INIT_STATE                               HAL_GPIO_PIN_RESET
#define M4_DIR_ACTIVE_STATE                             HAL_GPIO_PIN_SET
#define M4_DIR_INACTIVE_STATE                           HAL_GPIO_PIN_RESET

#define M4_DIR_PORT                                     HAL_GPIOB
#define M4_DIR_PIN                                      HAL_GPIO_PIN_0
#define M4_DIR_INIT_STATE                               HAL_GPIO_PIN_RESET
#define M4_DIR_ACTIVE_STATE                             HAL_GPIO_PIN_SET
#define M4_DIR_INACTIVE_STATE                           HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PB1 pin */
#define PB1_PORT                                        HAL_GPIOB
#define PB1_PIN                                         HAL_GPIO_PIN_1

/* Secondary aliases for GPIO PB1 pin */
#define M3_NFAULT_PORT                                  HAL_GPIOB
#define M3_NFAULT_PIN                                   HAL_GPIO_PIN_1

#define M3_NFAULT_PORT                                  HAL_GPIOB
#define M3_NFAULT_PIN                                   HAL_GPIO_PIN_1

/* EXTI aliases */
#define M3_FAULT_EXTI_EXTI_LINE                         HAL_EXTI_LINE_1
#define M3_FAULT_EXTI_EXTI_TRIGGER                      HAL_EXTI_TRIGGER_RISING

/* Primary aliases for GPIO PB5 pin */
#define PB5_PORT                                        HAL_GPIOB
#define PB5_PIN                                         HAL_GPIO_PIN_5
#define PB5_INIT_STATE                                  HAL_GPIO_PIN_RESET
#define PB5_ACTIVE_STATE                                HAL_GPIO_PIN_SET
#define PB5_INACTIVE_STATE                              HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PB5 pin */
#define M1_DIR_PORT                                     HAL_GPIOB
#define M1_DIR_PIN                                      HAL_GPIO_PIN_5
#define M1_DIR_INIT_STATE                               HAL_GPIO_PIN_RESET
#define M1_DIR_ACTIVE_STATE                             HAL_GPIO_PIN_SET
#define M1_DIR_INACTIVE_STATE                           HAL_GPIO_PIN_RESET

#define M1_DIR_PORT                                     HAL_GPIOB
#define M1_DIR_PIN                                      HAL_GPIO_PIN_5
#define M1_DIR_INIT_STATE                               HAL_GPIO_PIN_RESET
#define M1_DIR_ACTIVE_STATE                             HAL_GPIO_PIN_SET
#define M1_DIR_INACTIVE_STATE                           HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PB8 pin */
#define PB8_PORT                                        HAL_GPIOB
#define PB8_PIN                                         HAL_GPIO_PIN_8
#define PB8_INIT_STATE                                  HAL_GPIO_PIN_RESET
#define PB8_ACTIVE_STATE                                HAL_GPIO_PIN_SET
#define PB8_INACTIVE_STATE                              HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PB8 pin */
#define M3_STEP_PORT                                    HAL_GPIOB
#define M3_STEP_PIN                                     HAL_GPIO_PIN_8
#define M3_STEP_INIT_STATE                              HAL_GPIO_PIN_RESET
#define M3_STEP_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define M3_STEP_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

#define M3_STEP_PORT                                    HAL_GPIOB
#define M3_STEP_PIN                                     HAL_GPIO_PIN_8
#define M3_STEP_INIT_STATE                              HAL_GPIO_PIN_RESET
#define M3_STEP_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define M3_STEP_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PB9 pin */
#define PB9_PORT                                        HAL_GPIOB
#define PB9_PIN                                         HAL_GPIO_PIN_9
#define PB9_INIT_STATE                                  HAL_GPIO_PIN_RESET
#define PB9_ACTIVE_STATE                                HAL_GPIO_PIN_SET
#define PB9_INACTIVE_STATE                              HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PB9 pin */
#define M4_EN_PORT                                      HAL_GPIOB
#define M4_EN_PIN                                       HAL_GPIO_PIN_9
#define M4_EN_INIT_STATE                                HAL_GPIO_PIN_RESET
#define M4_EN_ACTIVE_STATE                              HAL_GPIO_PIN_SET
#define M4_EN_INACTIVE_STATE                            HAL_GPIO_PIN_RESET

#define M4_EN_PORT                                      HAL_GPIOB
#define M4_EN_PIN                                       HAL_GPIO_PIN_9
#define M4_EN_INIT_STATE                                HAL_GPIO_PIN_RESET
#define M4_EN_ACTIVE_STATE                              HAL_GPIO_PIN_SET
#define M4_EN_INACTIVE_STATE                            HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PB12 pin */
#define PB12_PORT                                       HAL_GPIOB
#define PB12_PIN                                        HAL_GPIO_PIN_12
#define PB12_INIT_STATE                                 HAL_GPIO_PIN_RESET
#define PB12_ACTIVE_STATE                               HAL_GPIO_PIN_SET
#define PB12_INACTIVE_STATE                             HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PB12 pin */
#define M1_NSLP_PORT                                    HAL_GPIOB
#define M1_NSLP_PIN                                     HAL_GPIO_PIN_12
#define M1_NSLP_INIT_STATE                              HAL_GPIO_PIN_RESET
#define M1_NSLP_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define M1_NSLP_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

#define M1_NSLP_PORT                                    HAL_GPIOB
#define M1_NSLP_PIN                                     HAL_GPIO_PIN_12
#define M1_NSLP_INIT_STATE                              HAL_GPIO_PIN_RESET
#define M1_NSLP_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define M1_NSLP_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PB14 pin */
#define PB14_PORT                                       HAL_GPIOB
#define PB14_PIN                                        HAL_GPIO_PIN_14

/* Secondary aliases for GPIO PB14 pin */
#define CARTRIDGE_SENSOR_2_PORT                         HAL_GPIOB
#define CARTRIDGE_SENSOR_2_PIN                          HAL_GPIO_PIN_14

#define CART1_SIZE_B_PORT                               HAL_GPIOB
#define CART1_SIZE_B_PIN                                HAL_GPIO_PIN_14

/* Primary aliases for GPIO PB15 pin */
#define PB15_PORT                                       HAL_GPIOB
#define PB15_PIN                                        HAL_GPIO_PIN_15

/* Secondary aliases for GPIO PB15 pin */
#define CARTRIDGE_SENSOR_1_PORT                         HAL_GPIOB
#define CARTRIDGE_SENSOR_1_PIN                          HAL_GPIO_PIN_15

#define CART1_SIZE_A_PORT                               HAL_GPIOB
#define CART1_SIZE_A_PIN                                HAL_GPIO_PIN_15

/* Primary aliases for GPIO PC7 pin */
#define PC7_PORT                                        HAL_GPIOC
#define PC7_PIN                                         HAL_GPIO_PIN_7
#define PC7_INIT_STATE                                  HAL_GPIO_PIN_RESET
#define PC7_ACTIVE_STATE                                HAL_GPIO_PIN_SET
#define PC7_INACTIVE_STATE                              HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PC7 pin */
#define M2_EN_PORT                                      HAL_GPIOC
#define M2_EN_PIN                                       HAL_GPIO_PIN_7
#define M2_EN_INIT_STATE                                HAL_GPIO_PIN_RESET
#define M2_EN_ACTIVE_STATE                              HAL_GPIO_PIN_SET
#define M2_EN_INACTIVE_STATE                            HAL_GPIO_PIN_RESET

#define M2_EN_PORT                                      HAL_GPIOC
#define M2_EN_PIN                                       HAL_GPIO_PIN_7
#define M2_EN_INIT_STATE                                HAL_GPIO_PIN_RESET
#define M2_EN_ACTIVE_STATE                              HAL_GPIO_PIN_SET
#define M2_EN_INACTIVE_STATE                            HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PC8 pin */
#define PC8_PORT                                        HAL_GPIOC
#define PC8_PIN                                         HAL_GPIO_PIN_8
#define PC8_INIT_STATE                                  HAL_GPIO_PIN_RESET
#define PC8_ACTIVE_STATE                                HAL_GPIO_PIN_SET
#define PC8_INACTIVE_STATE                              HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PC8 pin */
#define M2_DIR_PORT                                     HAL_GPIOC
#define M2_DIR_PIN                                      HAL_GPIO_PIN_8
#define M2_DIR_INIT_STATE                               HAL_GPIO_PIN_RESET
#define M2_DIR_ACTIVE_STATE                             HAL_GPIO_PIN_SET
#define M2_DIR_INACTIVE_STATE                           HAL_GPIO_PIN_RESET

#define M2_DIR_PORT                                     HAL_GPIOC
#define M2_DIR_PIN                                      HAL_GPIO_PIN_8
#define M2_DIR_INIT_STATE                               HAL_GPIO_PIN_RESET
#define M2_DIR_ACTIVE_STATE                             HAL_GPIO_PIN_SET
#define M2_DIR_INACTIVE_STATE                           HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PC9 pin */
#define PC9_PORT                                        HAL_GPIOC
#define PC9_PIN                                         HAL_GPIO_PIN_9
#define PC9_INIT_STATE                                  HAL_GPIO_PIN_RESET
#define PC9_ACTIVE_STATE                                HAL_GPIO_PIN_SET
#define PC9_INACTIVE_STATE                              HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PC9 pin */
#define M2_STEP_PORT                                    HAL_GPIOC
#define M2_STEP_PIN                                     HAL_GPIO_PIN_9
#define M2_STEP_INIT_STATE                              HAL_GPIO_PIN_RESET
#define M2_STEP_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define M2_STEP_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

#define M2_STEP_PORT                                    HAL_GPIOC
#define M2_STEP_PIN                                     HAL_GPIO_PIN_9
#define M2_STEP_INIT_STATE                              HAL_GPIO_PIN_RESET
#define M2_STEP_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define M2_STEP_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PC10 pin */
#define PC10_PORT                                       HAL_GPIOC
#define PC10_PIN                                        HAL_GPIO_PIN_10
#define PC10_INIT_STATE                                 HAL_GPIO_PIN_RESET
#define PC10_ACTIVE_STATE                               HAL_GPIO_PIN_SET
#define PC10_INACTIVE_STATE                             HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PC10 pin */
#define M3_EN_PORT                                      HAL_GPIOC
#define M3_EN_PIN                                       HAL_GPIO_PIN_10
#define M3_EN_INIT_STATE                                HAL_GPIO_PIN_RESET
#define M3_EN_ACTIVE_STATE                              HAL_GPIO_PIN_SET
#define M3_EN_INACTIVE_STATE                            HAL_GPIO_PIN_RESET

#define M3_EN_PORT                                      HAL_GPIOC
#define M3_EN_PIN                                       HAL_GPIO_PIN_10
#define M3_EN_INIT_STATE                                HAL_GPIO_PIN_RESET
#define M3_EN_ACTIVE_STATE                              HAL_GPIO_PIN_SET
#define M3_EN_INACTIVE_STATE                            HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PC11 pin */
#define PC11_PORT                                       HAL_GPIOC
#define PC11_PIN                                        HAL_GPIO_PIN_11
#define PC11_INIT_STATE                                 HAL_GPIO_PIN_RESET
#define PC11_ACTIVE_STATE                               HAL_GPIO_PIN_SET
#define PC11_INACTIVE_STATE                             HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PC11 pin */
#define M3_NSLP_PORT                                    HAL_GPIOC
#define M3_NSLP_PIN                                     HAL_GPIO_PIN_11
#define M3_NSLP_INIT_STATE                              HAL_GPIO_PIN_RESET
#define M3_NSLP_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define M3_NSLP_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

#define M3_NSLP_PORT                                    HAL_GPIOC
#define M3_NSLP_PIN                                     HAL_GPIO_PIN_11
#define M3_NSLP_INIT_STATE                              HAL_GPIO_PIN_RESET
#define M3_NSLP_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define M3_NSLP_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PD0 pin */
#define PD0_PORT                                        HAL_GPIOD
#define PD0_PIN                                         HAL_GPIO_PIN_0
#define PD0_INIT_STATE                                  HAL_GPIO_PIN_RESET
#define PD0_ACTIVE_STATE                                HAL_GPIO_PIN_SET
#define PD0_INACTIVE_STATE                              HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PD0 pin */
#define M3_DIR_PORT                                     HAL_GPIOD
#define M3_DIR_PIN                                      HAL_GPIO_PIN_0
#define M3_DIR_INIT_STATE                               HAL_GPIO_PIN_RESET
#define M3_DIR_ACTIVE_STATE                             HAL_GPIO_PIN_SET
#define M3_DIR_INACTIVE_STATE                           HAL_GPIO_PIN_RESET

#define M3_DIR_PORT                                     HAL_GPIOD
#define M3_DIR_PIN                                      HAL_GPIO_PIN_0
#define M3_DIR_INIT_STATE                               HAL_GPIO_PIN_RESET
#define M3_DIR_ACTIVE_STATE                             HAL_GPIO_PIN_SET
#define M3_DIR_INACTIVE_STATE                           HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PD3 pin */
#define PD3_PORT                                        HAL_GPIOD
#define PD3_PIN                                         HAL_GPIO_PIN_3
#define PD3_INIT_STATE                                  HAL_GPIO_PIN_RESET
#define PD3_ACTIVE_STATE                                HAL_GPIO_PIN_SET
#define PD3_INACTIVE_STATE                              HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PD3 pin */
#define CART1_EMPTY_LED_PORT                            HAL_GPIOD
#define CART1_EMPTY_LED_PIN                             HAL_GPIO_PIN_3
#define CART1_EMPTY_LED_INIT_STATE                      HAL_GPIO_PIN_RESET
#define CART1_EMPTY_LED_ACTIVE_STATE                    HAL_GPIO_PIN_SET
#define CART1_EMPTY_LED_INACTIVE_STATE                  HAL_GPIO_PIN_RESET

#define CART1_EMPTY_LED_PORT                            HAL_GPIOD
#define CART1_EMPTY_LED_PIN                             HAL_GPIO_PIN_3
#define CART1_EMPTY_LED_INIT_STATE                      HAL_GPIO_PIN_RESET
#define CART1_EMPTY_LED_ACTIVE_STATE                    HAL_GPIO_PIN_SET
#define CART1_EMPTY_LED_INACTIVE_STATE                  HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PD4 pin */
#define PD4_PORT                                        HAL_GPIOD
#define PD4_PIN                                         HAL_GPIO_PIN_4

/* Secondary aliases for GPIO PD4 pin */
#define SHUTDOWN_SW_PORT                                HAL_GPIOD
#define SHUTDOWN_SW_PIN                                 HAL_GPIO_PIN_4

#define SHUTDOWN_SW_PORT                                HAL_GPIOD
#define SHUTDOWN_SW_PIN                                 HAL_GPIO_PIN_4

/* Primary aliases for GPIO PD5 pin */
#define PD5_PORT                                        HAL_GPIOD
#define PD5_PIN                                         HAL_GPIO_PIN_5
#define PD5_INIT_STATE                                  HAL_GPIO_PIN_RESET
#define PD5_ACTIVE_STATE                                HAL_GPIO_PIN_SET
#define PD5_INACTIVE_STATE                              HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PD5 pin */
#define M1_STEP_PORT                                    HAL_GPIOD
#define M1_STEP_PIN                                     HAL_GPIO_PIN_5
#define M1_STEP_INIT_STATE                              HAL_GPIO_PIN_RESET
#define M1_STEP_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define M1_STEP_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

#define M1_STEP_PORT                                    HAL_GPIOD
#define M1_STEP_PIN                                     HAL_GPIO_PIN_5
#define M1_STEP_INIT_STATE                              HAL_GPIO_PIN_RESET
#define M1_STEP_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define M1_STEP_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PD6 pin */
#define PD6_PORT                                        HAL_GPIOD
#define PD6_PIN                                         HAL_GPIO_PIN_6

/* Secondary aliases for GPIO PD6 pin */
#define M1_NFAULT_PORT                                  HAL_GPIOD
#define M1_NFAULT_PIN                                   HAL_GPIO_PIN_6

#define M1_NFAULT_PORT                                  HAL_GPIOD
#define M1_NFAULT_PIN                                   HAL_GPIO_PIN_6

/* EXTI aliases */
#define M1_FAULT_EXTI_EXTI_LINE                         HAL_EXTI_LINE_6
#define M1_FAULT_EXTI_EXTI_TRIGGER                      HAL_EXTI_TRIGGER_FALLING

/* Primary aliases for GPIO PE14 pin */
#define PE14_PORT                                       HAL_GPIOE
#define PE14_PIN                                        HAL_GPIO_PIN_14

/* Secondary aliases for GPIO PE14 pin */
#define RELOAD_SW_PORT                                  HAL_GPIOE
#define RELOAD_SW_PIN                                   HAL_GPIO_PIN_14

#define RELOAD_SW_PORT                                  HAL_GPIOE
#define RELOAD_SW_PIN                                   HAL_GPIO_PIN_14

/* Primary aliases for GPIO PF12 pin */
#define PF12_PORT                                       HAL_GPIOF
#define PF12_PIN                                        HAL_GPIO_PIN_12

/* Secondary aliases for GPIO PF12 pin */
#define DOOR_LOCK_DETECT_SW_PORT                        HAL_GPIOF
#define DOOR_LOCK_DETECT_SW_PIN                         HAL_GPIO_PIN_12

#define DOOR_LOCK_DETECT_SW_PORT                        HAL_GPIOF
#define DOOR_LOCK_DETECT_SW_PIN                         HAL_GPIO_PIN_12

/* Primary aliases for GPIO PF13 pin */
#define PF13_PORT                                       HAL_GPIOF
#define PF13_PIN                                        HAL_GPIO_PIN_13

/* Secondary aliases for GPIO PF13 pin */
#define DOOR_SW_PORT                                    HAL_GPIOF
#define DOOR_SW_PIN                                     HAL_GPIO_PIN_13

#define DOOR_SW_PORT                                    HAL_GPIOF
#define DOOR_SW_PIN                                     HAL_GPIO_PIN_13

/* EXTI aliases */
#define DOOR_EXTI_EXTI_LINE                             HAL_EXTI_LINE_13
#define DOOR_EXTI_EXTI_TRIGGER                          HAL_EXTI_TRIGGER_RISING_FALLING

/* Primary aliases for GPIO PF14 pin */
#define PF14_PORT                                       HAL_GPIOF
#define PF14_PIN                                        HAL_GPIO_PIN_14

/* Secondary aliases for GPIO PF14 pin */
#define M2_NFAULT_PORT                                  HAL_GPIOF
#define M2_NFAULT_PIN                                   HAL_GPIO_PIN_14

#define M2_NFAULT_PORT                                  HAL_GPIOF
#define M2_NFAULT_PIN                                   HAL_GPIO_PIN_14

/* EXTI aliases */
#define M2_FAULT_EXTI_EXTI_LINE                         HAL_EXTI_LINE_14
#define M2_FAULT_EXTI_EXTI_TRIGGER                      HAL_EXTI_TRIGGER_FALLING

/* Primary aliases for GPIO PF15 pin */
#define PF15_PORT                                       HAL_GPIOF
#define PF15_PIN                                        HAL_GPIO_PIN_15
#define PF15_INIT_STATE                                 HAL_GPIO_PIN_RESET
#define PF15_ACTIVE_STATE                               HAL_GPIO_PIN_SET
#define PF15_INACTIVE_STATE                             HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PF15 pin */
#define M2_NSLP_PORT                                    HAL_GPIOF
#define M2_NSLP_PIN                                     HAL_GPIO_PIN_15
#define M2_NSLP_INIT_STATE                              HAL_GPIO_PIN_RESET
#define M2_NSLP_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define M2_NSLP_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

#define M2_NSLP_PORT                                    HAL_GPIOF
#define M2_NSLP_PIN                                     HAL_GPIO_PIN_15
#define M2_NSLP_INIT_STATE                              HAL_GPIO_PIN_RESET
#define M2_NSLP_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define M2_NSLP_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PH4 pin */
#define PH4_PORT                                        HAL_GPIOH
#define PH4_PIN                                         HAL_GPIO_PIN_4
#define PH4_INIT_STATE                                  HAL_GPIO_PIN_RESET
#define PH4_ACTIVE_STATE                                HAL_GPIO_PIN_SET
#define PH4_INACTIVE_STATE                              HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PH4 pin */
#define M4_NSLP_PORT                                    HAL_GPIOH
#define M4_NSLP_PIN                                     HAL_GPIO_PIN_4
#define M4_NSLP_INIT_STATE                              HAL_GPIO_PIN_RESET
#define M4_NSLP_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define M4_NSLP_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

#define M4_NSLP_PORT                                    HAL_GPIOH
#define M4_NSLP_PIN                                     HAL_GPIO_PIN_4
#define M4_NSLP_INIT_STATE                              HAL_GPIO_PIN_RESET
#define M4_NSLP_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define M4_NSLP_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PH5 pin */
#define PH5_PORT                                        HAL_GPIOH
#define PH5_PIN                                         HAL_GPIO_PIN_5

/* Secondary aliases for GPIO PH5 pin */
#define M4_NFAULT_PORT                                  HAL_GPIOH
#define M4_NFAULT_PIN                                   HAL_GPIO_PIN_5

#define M4_NFAULT_PORT                                  HAL_GPIOH
#define M4_NFAULT_PIN                                   HAL_GPIO_PIN_5

/* EXTI aliases */
#define M4_FAULT_EXTI_EXTI_LINE                         HAL_EXTI_LINE_5
#define M4_FAULT_EXTI_EXTI_TRIGGER                      HAL_EXTI_TRIGGER_RISING

/* Exported macros -----------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/******************************************************************************/
/* Exported functions for gpio_default in HAL layer                           */
/******************************************************************************/
/**
  * @brief mx_gpio_default init function
  * This function configures the hardware resources used in this example
  * @retval 0  GPIO group correctly initialized
  * @retval -1 Issue detected during GPIO group initialization
  */
system_status_t mx_gpio_default_init(void);

/**
  * @brief  De-initialize gpio_default instance.
  */
system_status_t mx_gpio_default_deinit(void);

/**
  * @brief  Get the EXTI1 object.
  * @retval Pointer on the EXTI1 Handle
  */
hal_exti_handle_t *mx_gpio_default_exti1_gethandle(void);

/**
  * @brief  Get the EXTI6 object.
  * @retval Pointer on the EXTI6 Handle
  */
hal_exti_handle_t *mx_gpio_default_exti6_gethandle(void);

/**
  * @brief  Get the EXTI13 object.
  * @retval Pointer on the EXTI13 Handle
  */
hal_exti_handle_t *mx_gpio_default_exti13_gethandle(void);

/**
  * @brief  Get the EXTI14 object.
  * @retval Pointer on the EXTI14 Handle
  */
hal_exti_handle_t *mx_gpio_default_exti14_gethandle(void);

/**
  * @brief  Get the EXTI5 object.
  * @retval Pointer on the EXTI5 Handle
  */
hal_exti_handle_t *mx_gpio_default_exti5_gethandle(void);

/******************************************************************************/
/*                            EXTI Line1 interrupt                            */
/******************************************************************************/
void EXTI1_IRQHandler(void);

/******************************************************************************/
/*                            EXTI Line6 interrupt                            */
/******************************************************************************/
void EXTI6_IRQHandler(void);

/******************************************************************************/
/*                           EXTI Line13 interrupt                            */
/******************************************************************************/
void EXTI13_IRQHandler(void);

/******************************************************************************/
/*                           EXTI Line14 interrupt                            */
/******************************************************************************/
void EXTI14_IRQHandler(void);

/******************************************************************************/
/*                            EXTI Line5 interrupt                            */
/******************************************************************************/
void EXTI5_IRQHandler(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MX_GPIO_DEFAULT_H */
