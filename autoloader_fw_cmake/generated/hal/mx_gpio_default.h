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

/* Primary aliases for GPIO PA1 pin */
#define MB_ADDR0_PORT                                   HAL_GPIOA
#define MB_ADDR0_PIN                                    HAL_GPIO_PIN_1

/* Primary aliases for GPIO PA4 pin */
#define USB_VBUS_EN_PORT                                HAL_GPIOA
#define USB_VBUS_EN_PIN                                 HAL_GPIO_PIN_4
#define USB_VBUS_EN_INIT_STATE                          HAL_GPIO_PIN_RESET
#define USB_VBUS_EN_ACTIVE_STATE                        HAL_GPIO_PIN_SET
#define USB_VBUS_EN_INACTIVE_STATE                      HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PA5 pin */
#define M7_DIR_PORT                                     HAL_GPIOA
#define M7_DIR_PIN                                      HAL_GPIO_PIN_5
#define M7_DIR_INIT_STATE                               HAL_GPIO_PIN_RESET
#define M7_DIR_ACTIVE_STATE                             HAL_GPIO_PIN_SET
#define M7_DIR_INACTIVE_STATE                           HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PA10 pin */
#define M2_DIR_PORT                                     HAL_GPIOA
#define M2_DIR_PIN                                      HAL_GPIO_PIN_10
#define M2_DIR_INIT_STATE                               HAL_GPIO_PIN_RESET
#define M2_DIR_ACTIVE_STATE                             HAL_GPIO_PIN_SET
#define M2_DIR_INACTIVE_STATE                           HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PB0 pin */
#define CTG4_EN_PORT                                    HAL_GPIOB
#define CTG4_EN_PIN                                     HAL_GPIO_PIN_0
#define CTG4_EN_INIT_STATE                              HAL_GPIO_PIN_RESET
#define CTG4_EN_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define CTG4_EN_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PB1 pin */
#define M3_NFAULT_PORT                                  HAL_GPIOB
#define M3_NFAULT_PIN                                   HAL_GPIO_PIN_1

/* EXTI aliases */
#define M3_FAULT_EXTI_EXTI_LINE                         HAL_EXTI_LINE_1
#define M3_FAULT_EXTI_EXTI_TRIGGER                      HAL_EXTI_TRIGGER_RISING

/* Primary aliases for GPIO PB2 pin */
#define M5_NFAULT_PORT                                  HAL_GPIOB
#define M5_NFAULT_PIN                                   HAL_GPIO_PIN_2

/* EXTI aliases */
#define M5_FAULT_EXTI_EXTI_LINE                         HAL_EXTI_LINE_2
#define M5_FAULT_EXTI_EXTI_TRIGGER                      HAL_EXTI_TRIGGER_RISING

/* Primary aliases for GPIO PB5 pin */
#define PANEL_LED3_Y_PORT                               HAL_GPIOB
#define PANEL_LED3_Y_PIN                                HAL_GPIO_PIN_5
#define PANEL_LED3_Y_INIT_STATE                         HAL_GPIO_PIN_RESET
#define PANEL_LED3_Y_ACTIVE_STATE                       HAL_GPIO_PIN_SET
#define PANEL_LED3_Y_INACTIVE_STATE                     HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PB5 pin */
#define CTG3_LED_Y_PORT                                 HAL_GPIOB
#define CTG3_LED_Y_PIN                                  HAL_GPIO_PIN_5
#define CTG3_LED_Y_INIT_STATE                           HAL_GPIO_PIN_RESET
#define CTG3_LED_Y_ACTIVE_STATE                         HAL_GPIO_PIN_SET
#define CTG3_LED_Y_INACTIVE_STATE                       HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PB8 pin */
#define PANEL_LED2_G_PORT                               HAL_GPIOB
#define PANEL_LED2_G_PIN                                HAL_GPIO_PIN_8
#define PANEL_LED2_G_INIT_STATE                         HAL_GPIO_PIN_RESET
#define PANEL_LED2_G_ACTIVE_STATE                       HAL_GPIO_PIN_SET
#define PANEL_LED2_G_INACTIVE_STATE                     HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PB8 pin */
#define CTG2_LED_G_PORT                                 HAL_GPIOB
#define CTG2_LED_G_PIN                                  HAL_GPIO_PIN_8
#define CTG2_LED_G_INIT_STATE                           HAL_GPIO_PIN_RESET
#define CTG2_LED_G_ACTIVE_STATE                         HAL_GPIO_PIN_SET
#define CTG2_LED_G_INACTIVE_STATE                       HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PB9 pin */
#define PANEL_LED2_Y_PORT                               HAL_GPIOB
#define PANEL_LED2_Y_PIN                                HAL_GPIO_PIN_9
#define PANEL_LED2_Y_INIT_STATE                         HAL_GPIO_PIN_RESET
#define PANEL_LED2_Y_ACTIVE_STATE                       HAL_GPIO_PIN_SET
#define PANEL_LED2_Y_INACTIVE_STATE                     HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PB9 pin */
#define CTG2_LED_Y_PORT                                 HAL_GPIOB
#define CTG2_LED_Y_PIN                                  HAL_GPIO_PIN_9
#define CTG2_LED_Y_INIT_STATE                           HAL_GPIO_PIN_RESET
#define CTG2_LED_Y_ACTIVE_STATE                         HAL_GPIO_PIN_SET
#define CTG2_LED_Y_INACTIVE_STATE                       HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PB12 pin */
#define CTG4_NSLP_PORT                                  HAL_GPIOB
#define CTG4_NSLP_PIN                                   HAL_GPIO_PIN_12
#define CTG4_NSLP_INIT_STATE                            HAL_GPIO_PIN_RESET
#define CTG4_NSLP_ACTIVE_STATE                          HAL_GPIO_PIN_SET
#define CTG4_NSLP_INACTIVE_STATE                        HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PB14 pin */
#define SW3_24V_PORT                                    HAL_GPIOB
#define SW3_24V_PIN                                     HAL_GPIO_PIN_14
#define SW3_24V_INIT_STATE                              HAL_GPIO_PIN_RESET
#define SW3_24V_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define SW3_24V_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PB14 pin */
#define FUTURE_2_PORT                                   HAL_GPIOB
#define FUTURE_2_PIN                                    HAL_GPIO_PIN_14
#define FUTURE_2_INIT_STATE                             HAL_GPIO_PIN_RESET
#define FUTURE_2_ACTIVE_STATE                           HAL_GPIO_PIN_SET
#define FUTURE_2_INACTIVE_STATE                         HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PC0 pin */
#define FUTURE_MB_DIR_PORT                              HAL_GPIOC
#define FUTURE_MB_DIR_PIN                               HAL_GPIO_PIN_0
#define FUTURE_MB_DIR_INIT_STATE                        HAL_GPIO_PIN_RESET
#define FUTURE_MB_DIR_ACTIVE_STATE                      HAL_GPIO_PIN_SET
#define FUTURE_MB_DIR_INACTIVE_STATE                    HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PC0 pin */
#define MB_FUTURE_DIR_PORT                              HAL_GPIOC
#define MB_FUTURE_DIR_PIN                               HAL_GPIO_PIN_0
#define MB_FUTURE_DIR_INIT_STATE                        HAL_GPIO_PIN_RESET
#define MB_FUTURE_DIR_ACTIVE_STATE                      HAL_GPIO_PIN_SET
#define MB_FUTURE_DIR_INACTIVE_STATE                    HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PC1 pin */
#define M6_DIR_PORT                                     HAL_GPIOC
#define M6_DIR_PIN                                      HAL_GPIO_PIN_1
#define M6_DIR_INIT_STATE                               HAL_GPIO_PIN_RESET
#define M6_DIR_ACTIVE_STATE                             HAL_GPIO_PIN_SET
#define M6_DIR_INACTIVE_STATE                           HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PC3 pin */
#define M6_NFAULT_PORT                                  HAL_GPIOC
#define M6_NFAULT_PIN                                   HAL_GPIO_PIN_3

/* EXTI aliases */
#define M6_FAULT_EXTI_EXTI_LINE                         HAL_EXTI_LINE_3
#define M6_FAULT_EXTI_EXTI_TRIGGER                      HAL_EXTI_TRIGGER_RISING

/* Primary aliases for GPIO PC4 pin */
#define M8_DIR_PORT                                     HAL_GPIOC
#define M8_DIR_PIN                                      HAL_GPIO_PIN_4
#define M8_DIR_INIT_STATE                               HAL_GPIO_PIN_RESET
#define M8_DIR_ACTIVE_STATE                             HAL_GPIO_PIN_SET
#define M8_DIR_INACTIVE_STATE                           HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PC5 pin */
#define M8_STEP_PORT                                    HAL_GPIOC
#define M8_STEP_PIN                                     HAL_GPIO_PIN_5
#define M8_STEP_INIT_STATE                              HAL_GPIO_PIN_RESET
#define M8_STEP_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define M8_STEP_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PC7 pin */
#define CTG2_EN_PORT                                    HAL_GPIOC
#define CTG2_EN_PIN                                     HAL_GPIO_PIN_7
#define CTG2_EN_INIT_STATE                              HAL_GPIO_PIN_RESET
#define CTG2_EN_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define CTG2_EN_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PC8 pin */
#define M3_DIR_PORT                                     HAL_GPIOC
#define M3_DIR_PIN                                      HAL_GPIO_PIN_8
#define M3_DIR_INIT_STATE                               HAL_GPIO_PIN_RESET
#define M3_DIR_ACTIVE_STATE                             HAL_GPIO_PIN_SET
#define M3_DIR_INACTIVE_STATE                           HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PC9 pin */
#define M3_STEP_PORT                                    HAL_GPIOC
#define M3_STEP_PIN                                     HAL_GPIO_PIN_9
#define M3_STEP_INIT_STATE                              HAL_GPIO_PIN_RESET
#define M3_STEP_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define M3_STEP_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PC10 pin */
#define CTG1_EN_PORT                                    HAL_GPIOC
#define CTG1_EN_PIN                                     HAL_GPIO_PIN_10
#define CTG1_EN_INIT_STATE                              HAL_GPIO_PIN_RESET
#define CTG1_EN_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define CTG1_EN_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PC11 pin */
#define M1_DIR_PORT                                     HAL_GPIOC
#define M1_DIR_PIN                                      HAL_GPIO_PIN_11
#define M1_DIR_INIT_STATE                               HAL_GPIO_PIN_RESET
#define M1_DIR_ACTIVE_STATE                             HAL_GPIO_PIN_SET
#define M1_DIR_INACTIVE_STATE                           HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PC13 pin */
#define PANEL_LED1_R_PORT                               HAL_GPIOC
#define PANEL_LED1_R_PIN                                HAL_GPIO_PIN_13
#define PANEL_LED1_R_INIT_STATE                         HAL_GPIO_PIN_RESET
#define PANEL_LED1_R_ACTIVE_STATE                       HAL_GPIO_PIN_SET
#define PANEL_LED1_R_INACTIVE_STATE                     HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PC13 pin */
#define CTG1_LED_R_PORT                                 HAL_GPIOC
#define CTG1_LED_R_PIN                                  HAL_GPIO_PIN_13
#define CTG1_LED_R_INIT_STATE                           HAL_GPIO_PIN_RESET
#define CTG1_LED_R_ACTIVE_STATE                         HAL_GPIO_PIN_SET
#define CTG1_LED_R_INACTIVE_STATE                       HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PD0 pin */
#define M1_STEP_PORT                                    HAL_GPIOD
#define M1_STEP_PIN                                     HAL_GPIO_PIN_0
#define M1_STEP_INIT_STATE                              HAL_GPIO_PIN_RESET
#define M1_STEP_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define M1_STEP_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PD4 pin */
#define M2_NFAULT_PORT                                  HAL_GPIOD
#define M2_NFAULT_PIN                                   HAL_GPIO_PIN_4

/* EXTI aliases */
#define M2_FAULT_EXTI_EXTI_LINE                         HAL_EXTI_LINE_4
#define M2_FAULT_EXTI_EXTI_TRIGGER                      HAL_EXTI_TRIGGER_RISING

/* Primary aliases for GPIO PD5 pin */
#define PANEL_LED5_G_PORT                               HAL_GPIOD
#define PANEL_LED5_G_PIN                                HAL_GPIO_PIN_5
#define PANEL_LED5_G_INIT_STATE                         HAL_GPIO_PIN_RESET
#define PANEL_LED5_G_ACTIVE_STATE                       HAL_GPIO_PIN_SET
#define PANEL_LED5_G_INACTIVE_STATE                     HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PD5 pin */
#define EXTRA_LED_G_PORT                                HAL_GPIOD
#define EXTRA_LED_G_PIN                                 HAL_GPIO_PIN_5
#define EXTRA_LED_G_INIT_STATE                          HAL_GPIO_PIN_RESET
#define EXTRA_LED_G_ACTIVE_STATE                        HAL_GPIO_PIN_SET
#define EXTRA_LED_G_INACTIVE_STATE                      HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PD6 pin */
#define M1_NFAULT_PORT                                  HAL_GPIOD
#define M1_NFAULT_PIN                                   HAL_GPIO_PIN_6

/* EXTI aliases */
#define M1_FAULT_EXTI_EXTI_LINE                         HAL_EXTI_LINE_6
#define M1_FAULT_EXTI_EXTI_TRIGGER                      HAL_EXTI_TRIGGER_RISING

/* Primary aliases for GPIO PD7 pin */
#define PANEL_LED5_Y_PORT                               HAL_GPIOD
#define PANEL_LED5_Y_PIN                                HAL_GPIO_PIN_7
#define PANEL_LED5_Y_INIT_STATE                         HAL_GPIO_PIN_RESET
#define PANEL_LED5_Y_ACTIVE_STATE                       HAL_GPIO_PIN_SET
#define PANEL_LED5_Y_INACTIVE_STATE                     HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PD7 pin */
#define EXTRA_LED_Y_PORT                                HAL_GPIOD
#define EXTRA_LED_Y_PIN                                 HAL_GPIO_PIN_7
#define EXTRA_LED_Y_INIT_STATE                          HAL_GPIO_PIN_RESET
#define EXTRA_LED_Y_ACTIVE_STATE                        HAL_GPIO_PIN_SET
#define EXTRA_LED_Y_INACTIVE_STATE                      HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PD8 pin */
#define M6_STEP_PORT                                    HAL_GPIOD
#define M6_STEP_PIN                                     HAL_GPIO_PIN_8
#define M6_STEP_INIT_STATE                              HAL_GPIO_PIN_RESET
#define M6_STEP_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define M6_STEP_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PD9 pin */
#define CTG3_EN_PORT                                    HAL_GPIOD
#define CTG3_EN_PIN                                     HAL_GPIO_PIN_9
#define CTG3_EN_INIT_STATE                              HAL_GPIO_PIN_RESET
#define CTG3_EN_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define CTG3_EN_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PD10 pin */
#define CTG3_NSLP_PORT                                  HAL_GPIOD
#define CTG3_NSLP_PIN                                   HAL_GPIO_PIN_10
#define CTG3_NSLP_INIT_STATE                            HAL_GPIO_PIN_RESET
#define CTG3_NSLP_ACTIVE_STATE                          HAL_GPIO_PIN_SET
#define CTG3_NSLP_INACTIVE_STATE                        HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PD10 pin */
#define CTG3_NSLEEP_PORT                                HAL_GPIOD
#define CTG3_NSLEEP_PIN                                 HAL_GPIO_PIN_10
#define CTG3_NSLEEP_INIT_STATE                          HAL_GPIO_PIN_RESET
#define CTG3_NSLEEP_ACTIVE_STATE                        HAL_GPIO_PIN_SET
#define CTG3_NSLEEP_INACTIVE_STATE                      HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PD14 pin */
#define M5_DIR_PORT                                     HAL_GPIOD
#define M5_DIR_PIN                                      HAL_GPIO_PIN_14
#define M5_DIR_INIT_STATE                               HAL_GPIO_PIN_RESET
#define M5_DIR_ACTIVE_STATE                             HAL_GPIO_PIN_SET
#define M5_DIR_INACTIVE_STATE                           HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PD15 pin */
#define CTG2_NSLP_PORT                                  HAL_GPIOD
#define CTG2_NSLP_PIN                                   HAL_GPIO_PIN_15
#define CTG2_NSLP_INIT_STATE                            HAL_GPIO_PIN_RESET
#define CTG2_NSLP_ACTIVE_STATE                          HAL_GPIO_PIN_SET
#define CTG2_NSLP_INACTIVE_STATE                        HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PE2 pin */
#define PANEL_LED2_R_PORT                               HAL_GPIOE
#define PANEL_LED2_R_PIN                                HAL_GPIO_PIN_2
#define PANEL_LED2_R_INIT_STATE                         HAL_GPIO_PIN_RESET
#define PANEL_LED2_R_ACTIVE_STATE                       HAL_GPIO_PIN_SET
#define PANEL_LED2_R_INACTIVE_STATE                     HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PE2 pin */
#define CTG2_LED_R_PORT                                 HAL_GPIOE
#define CTG2_LED_R_PIN                                  HAL_GPIO_PIN_2
#define CTG2_LED_R_INIT_STATE                           HAL_GPIO_PIN_RESET
#define CTG2_LED_R_ACTIVE_STATE                         HAL_GPIO_PIN_SET
#define CTG2_LED_R_INACTIVE_STATE                       HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PE3 pin */
#define DOOR_SW_PORT                                    HAL_GPIOE
#define DOOR_SW_PIN                                     HAL_GPIO_PIN_3

/* Primary aliases for GPIO PE4 pin */
#define SHUTDOWN_SW_PORT                                HAL_GPIOE
#define SHUTDOWN_SW_PIN                                 HAL_GPIO_PIN_4

/* Primary aliases for GPIO PE5 pin */
#define PANEL_LED1_G_PORT                               HAL_GPIOE
#define PANEL_LED1_G_PIN                                HAL_GPIO_PIN_5
#define PANEL_LED1_G_INIT_STATE                         HAL_GPIO_PIN_RESET
#define PANEL_LED1_G_ACTIVE_STATE                       HAL_GPIO_PIN_SET
#define PANEL_LED1_G_INACTIVE_STATE                     HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PE5 pin */
#define CTG1_LED_G_PORT                                 HAL_GPIOE
#define CTG1_LED_G_PIN                                  HAL_GPIO_PIN_5
#define CTG1_LED_G_INIT_STATE                           HAL_GPIO_PIN_RESET
#define CTG1_LED_G_ACTIVE_STATE                         HAL_GPIO_PIN_SET
#define CTG1_LED_G_INACTIVE_STATE                       HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PE8 pin */
#define M7_NFAULT_PORT                                  HAL_GPIOE
#define M7_NFAULT_PIN                                   HAL_GPIO_PIN_8

/* EXTI aliases */
#define M7_FAULT_EXTI_EXTI_LINE                         HAL_EXTI_LINE_8
#define M7_FAULT_EXTI_EXTI_TRIGGER                      HAL_EXTI_TRIGGER_RISING

/* Primary aliases for GPIO PE9 pin */
#define SW1_24V_PORT                                    HAL_GPIOE
#define SW1_24V_PIN                                     HAL_GPIO_PIN_9
#define SW1_24V_INIT_STATE                              HAL_GPIO_PIN_RESET
#define SW1_24V_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define SW1_24V_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PE9 pin */
#define BUZZER_PORT                                     HAL_GPIOE
#define BUZZER_PIN                                      HAL_GPIO_PIN_9
#define BUZZER_INIT_STATE                               HAL_GPIO_PIN_RESET
#define BUZZER_ACTIVE_STATE                             HAL_GPIO_PIN_SET
#define BUZZER_INACTIVE_STATE                           HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PE10 pin */
#define STATUS_LED1_PORT                                HAL_GPIOE
#define STATUS_LED1_PIN                                 HAL_GPIO_PIN_10
#define STATUS_LED1_INIT_STATE                          HAL_GPIO_PIN_RESET
#define STATUS_LED1_ACTIVE_STATE                        HAL_GPIO_PIN_SET
#define STATUS_LED1_INACTIVE_STATE                      HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PE12 pin */
#define STATUS_LED2_PORT                                HAL_GPIOE
#define STATUS_LED2_PIN                                 HAL_GPIO_PIN_12
#define STATUS_LED2_INIT_STATE                          HAL_GPIO_PIN_RESET
#define STATUS_LED2_ACTIVE_STATE                        HAL_GPIO_PIN_SET
#define STATUS_LED2_INACTIVE_STATE                      HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PE13 pin */
#define SPI_FLASH_WP_PORT                               HAL_GPIOE
#define SPI_FLASH_WP_PIN                                HAL_GPIO_PIN_13
#define SPI_FLASH_WP_INIT_STATE                         HAL_GPIO_PIN_RESET
#define SPI_FLASH_WP_ACTIVE_STATE                       HAL_GPIO_PIN_SET
#define SPI_FLASH_WP_INACTIVE_STATE                     HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PE13 pin */
#define EEPROM_CS_PORT                                  HAL_GPIOE
#define EEPROM_CS_PIN                                   HAL_GPIO_PIN_13
#define EEPROM_CS_INIT_STATE                            HAL_GPIO_PIN_RESET
#define EEPROM_CS_ACTIVE_STATE                          HAL_GPIO_PIN_SET
#define EEPROM_CS_INACTIVE_STATE                        HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PE14 pin */
#define SPI_FLASH_RESET_PORT                            HAL_GPIOE
#define SPI_FLASH_RESET_PIN                             HAL_GPIO_PIN_14
#define SPI_FLASH_RESET_INIT_STATE                      HAL_GPIO_PIN_SET
#define SPI_FLASH_RESET_ACTIVE_STATE                    HAL_GPIO_PIN_SET
#define SPI_FLASH_RESET_INACTIVE_STATE                  HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PE14 pin */
#define SPI_FLASH_NRESET_PORT                           HAL_GPIOE
#define SPI_FLASH_NRESET_PIN                            HAL_GPIO_PIN_14
#define SPI_FLASH_NRESET_INIT_STATE                     HAL_GPIO_PIN_SET
#define SPI_FLASH_NRESET_ACTIVE_STATE                   HAL_GPIO_PIN_SET
#define SPI_FLASH_NRESET_INACTIVE_STATE                 HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PE15 pin */
#define SWITCH_12V_PORT                                 HAL_GPIOE
#define SWITCH_12V_PIN                                  HAL_GPIO_PIN_15
#define SWITCH_12V_INIT_STATE                           HAL_GPIO_PIN_RESET
#define SWITCH_12V_ACTIVE_STATE                         HAL_GPIO_PIN_SET
#define SWITCH_12V_INACTIVE_STATE                       HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PE15 pin */
#define DOOR_LOCK_CTRL_PORT                             HAL_GPIOE
#define DOOR_LOCK_CTRL_PIN                              HAL_GPIO_PIN_15
#define DOOR_LOCK_CTRL_INIT_STATE                       HAL_GPIO_PIN_RESET
#define DOOR_LOCK_CTRL_ACTIVE_STATE                     HAL_GPIO_PIN_SET
#define DOOR_LOCK_CTRL_INACTIVE_STATE                   HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PF0 pin */
#define RELOAD_SW_PORT                                  HAL_GPIOF
#define RELOAD_SW_PIN                                   HAL_GPIO_PIN_0

/* Primary aliases for GPIO PF1 pin */
#define DOOR_LOCK_DETECT_SW_PORT                        HAL_GPIOF
#define DOOR_LOCK_DETECT_SW_PIN                         HAL_GPIO_PIN_1

/* Primary aliases for GPIO PF2 pin */
#define CARTRIDGE_SENSOR_1_PORT                         HAL_GPIOF
#define CARTRIDGE_SENSOR_1_PIN                          HAL_GPIO_PIN_2

/* Secondary aliases for GPIO PF2 pin */
#define CTG1_SENSE1_PORT                                HAL_GPIOF
#define CTG1_SENSE1_PIN                                 HAL_GPIO_PIN_2

/* Primary aliases for GPIO PF3 pin */
#define CARTRIDGE_SENSOR_2_PORT                         HAL_GPIOF
#define CARTRIDGE_SENSOR_2_PIN                          HAL_GPIO_PIN_3

/* Secondary aliases for GPIO PF3 pin */
#define CTG1_SENSE2_PORT                                HAL_GPIOF
#define CTG1_SENSE2_PIN                                 HAL_GPIO_PIN_3

/* Primary aliases for GPIO PF4 pin */
#define CARTRIDGE_SENSOR_3_PORT                         HAL_GPIOF
#define CARTRIDGE_SENSOR_3_PIN                          HAL_GPIO_PIN_4

/* Secondary aliases for GPIO PF4 pin */
#define CTG2_SENSE1_PORT                                HAL_GPIOF
#define CTG2_SENSE1_PIN                                 HAL_GPIO_PIN_4

/* Primary aliases for GPIO PF5 pin */
#define CARTRIDGE_SENSOR_4_PORT                         HAL_GPIOF
#define CARTRIDGE_SENSOR_4_PIN                          HAL_GPIO_PIN_5

/* Secondary aliases for GPIO PF5 pin */
#define CTG2_SENSE2_PORT                                HAL_GPIOF
#define CTG2_SENSE2_PIN                                 HAL_GPIO_PIN_5

/* Primary aliases for GPIO PF6 pin */
#define CARTRIDGE_SENSOR_5_PORT                         HAL_GPIOF
#define CARTRIDGE_SENSOR_5_PIN                          HAL_GPIO_PIN_6

/* Secondary aliases for GPIO PF6 pin */
#define CTG3_SENSE1_PORT                                HAL_GPIOF
#define CTG3_SENSE1_PIN                                 HAL_GPIO_PIN_6

/* Primary aliases for GPIO PF7 pin */
#define CARTRIDGE_SENSOR_6_PORT                         HAL_GPIOF
#define CARTRIDGE_SENSOR_6_PIN                          HAL_GPIO_PIN_7

/* Secondary aliases for GPIO PF7 pin */
#define CTG3_SENSE2_PORT                                HAL_GPIOF
#define CTG3_SENSE2_PIN                                 HAL_GPIO_PIN_7

/* Primary aliases for GPIO PF8 pin */
#define CARTRIDGE_SENSOR_7_PORT                         HAL_GPIOF
#define CARTRIDGE_SENSOR_7_PIN                          HAL_GPIO_PIN_8

/* Secondary aliases for GPIO PF8 pin */
#define CTG4_SENSE1_PORT                                HAL_GPIOF
#define CTG4_SENSE1_PIN                                 HAL_GPIO_PIN_8

/* Primary aliases for GPIO PF9 pin */
#define CARTRIDGE_SENSOR_8_PORT                         HAL_GPIOF
#define CARTRIDGE_SENSOR_8_PIN                          HAL_GPIO_PIN_9

/* Secondary aliases for GPIO PF9 pin */
#define CTG4_SENSE2_PORT                                HAL_GPIOF
#define CTG4_SENSE2_PIN                                 HAL_GPIO_PIN_9

/* Primary aliases for GPIO PF10 pin */
#define UI_MB_DIR_PORT                                  HAL_GPIOF
#define UI_MB_DIR_PIN                                   HAL_GPIO_PIN_10
#define UI_MB_DIR_INIT_STATE                            HAL_GPIO_PIN_RESET
#define UI_MB_DIR_ACTIVE_STATE                          HAL_GPIO_PIN_SET
#define UI_MB_DIR_INACTIVE_STATE                        HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PF10 pin */
#define MB_UI_DIR_PORT                                  HAL_GPIOF
#define MB_UI_DIR_PIN                                   HAL_GPIO_PIN_10
#define MB_UI_DIR_INIT_STATE                            HAL_GPIO_PIN_RESET
#define MB_UI_DIR_ACTIVE_STATE                          HAL_GPIO_PIN_SET
#define MB_UI_DIR_INACTIVE_STATE                        HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PF11 pin */
#define M5_STEP_PORT                                    HAL_GPIOF
#define M5_STEP_PIN                                     HAL_GPIO_PIN_11
#define M5_STEP_INIT_STATE                              HAL_GPIO_PIN_RESET
#define M5_STEP_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define M5_STEP_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PF12 pin */
#define USB_VBUS_DETECT_PORT                            HAL_GPIOF
#define USB_VBUS_DETECT_PIN                             HAL_GPIO_PIN_12

/* Primary aliases for GPIO PF13 pin */
#define USB_VBUS_FAULT_PORT                             HAL_GPIOF
#define USB_VBUS_FAULT_PIN                              HAL_GPIO_PIN_13

/* Primary aliases for GPIO PF14 pin */
#define M8_NFAULT_PORT                                  HAL_GPIOF
#define M8_NFAULT_PIN                                   HAL_GPIO_PIN_14

/* EXTI aliases */
#define M8_FAULT_EXTI_EXTI_LINE                         HAL_EXTI_LINE_14
#define M8_FAULT_EXTI_EXTI_TRIGGER                      HAL_EXTI_TRIGGER_RISING

/* Primary aliases for GPIO PF15 pin */
#define M7_STEP_PORT                                    HAL_GPIOF
#define M7_STEP_PIN                                     HAL_GPIO_PIN_15
#define M7_STEP_INIT_STATE                              HAL_GPIO_PIN_RESET
#define M7_STEP_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define M7_STEP_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PG0 pin */
#define ENC3_INDEX_PORT                                 HAL_GPIOG
#define ENC3_INDEX_PIN                                  HAL_GPIO_PIN_0

/* EXTI aliases */
#define ENC3_INDEX_EXTI_EXTI_LINE                       HAL_EXTI_LINE_0
#define ENC3_INDEX_EXTI_EXTI_TRIGGER                    HAL_EXTI_TRIGGER_RISING

/* Primary aliases for GPIO PG1 pin */
#define SW2_24V_PORT                                    HAL_GPIOG
#define SW2_24V_PIN                                     HAL_GPIO_PIN_1
#define SW2_24V_INIT_STATE                              HAL_GPIO_PIN_RESET
#define SW2_24V_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define SW2_24V_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PG1 pin */
#define FUTURE_1_PORT                                   HAL_GPIOG
#define FUTURE_1_PIN                                    HAL_GPIO_PIN_1
#define FUTURE_1_INIT_STATE                             HAL_GPIO_PIN_RESET
#define FUTURE_1_ACTIVE_STATE                           HAL_GPIO_PIN_SET
#define FUTURE_1_INACTIVE_STATE                         HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PG2 pin */
#define SPI_FLASH_CS_PORT                               HAL_GPIOG
#define SPI_FLASH_CS_PIN                                HAL_GPIO_PIN_2
#define SPI_FLASH_CS_INIT_STATE                         HAL_GPIO_PIN_RESET
#define SPI_FLASH_CS_ACTIVE_STATE                       HAL_GPIO_PIN_SET
#define SPI_FLASH_CS_INACTIVE_STATE                     HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PG3 pin */
#define M4_DIR_PORT                                     HAL_GPIOG
#define M4_DIR_PIN                                      HAL_GPIO_PIN_3
#define M4_DIR_INIT_STATE                               HAL_GPIO_PIN_RESET
#define M4_DIR_ACTIVE_STATE                             HAL_GPIO_PIN_SET
#define M4_DIR_INACTIVE_STATE                           HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PG6 pin */
#define M4_STEP_PORT                                    HAL_GPIOG
#define M4_STEP_PIN                                     HAL_GPIO_PIN_6
#define M4_STEP_INIT_STATE                              HAL_GPIO_PIN_RESET
#define M4_STEP_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define M4_STEP_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PG7 pin */
#define CTG1_NSLP_PORT                                  HAL_GPIOG
#define CTG1_NSLP_PIN                                   HAL_GPIO_PIN_7
#define CTG1_NSLP_INIT_STATE                            HAL_GPIO_PIN_RESET
#define CTG1_NSLP_ACTIVE_STATE                          HAL_GPIO_PIN_SET
#define CTG1_NSLP_INACTIVE_STATE                        HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PG9 pin */
#define PANEL_LED5_R_PORT                               HAL_GPIOG
#define PANEL_LED5_R_PIN                                HAL_GPIO_PIN_9
#define PANEL_LED5_R_INIT_STATE                         HAL_GPIO_PIN_RESET
#define PANEL_LED5_R_ACTIVE_STATE                       HAL_GPIO_PIN_SET
#define PANEL_LED5_R_INACTIVE_STATE                     HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PG9 pin */
#define EXTRA_LED_R_PORT                                HAL_GPIOG
#define EXTRA_LED_R_PIN                                 HAL_GPIO_PIN_9
#define EXTRA_LED_R_INIT_STATE                          HAL_GPIO_PIN_RESET
#define EXTRA_LED_R_ACTIVE_STATE                        HAL_GPIO_PIN_SET
#define EXTRA_LED_R_INACTIVE_STATE                      HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PG10 pin */
#define PANEL_LED4_G_PORT                               HAL_GPIOG
#define PANEL_LED4_G_PIN                                HAL_GPIO_PIN_10
#define PANEL_LED4_G_INIT_STATE                         HAL_GPIO_PIN_RESET
#define PANEL_LED4_G_ACTIVE_STATE                       HAL_GPIO_PIN_SET
#define PANEL_LED4_G_INACTIVE_STATE                     HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PG10 pin */
#define CTG4_LED_G_PORT                                 HAL_GPIOG
#define CTG4_LED_G_PIN                                  HAL_GPIO_PIN_10
#define CTG4_LED_G_INIT_STATE                           HAL_GPIO_PIN_RESET
#define CTG4_LED_G_ACTIVE_STATE                         HAL_GPIO_PIN_SET
#define CTG4_LED_G_INACTIVE_STATE                       HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PG11 pin */
#define PANEL_LED4_Y_PORT                               HAL_GPIOG
#define PANEL_LED4_Y_PIN                                HAL_GPIO_PIN_11
#define PANEL_LED4_Y_INIT_STATE                         HAL_GPIO_PIN_RESET
#define PANEL_LED4_Y_ACTIVE_STATE                       HAL_GPIO_PIN_SET
#define PANEL_LED4_Y_INACTIVE_STATE                     HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PG11 pin */
#define CTG4_LED_Y_PORT                                 HAL_GPIOG
#define CTG4_LED_Y_PIN                                  HAL_GPIO_PIN_11
#define CTG4_LED_Y_INIT_STATE                           HAL_GPIO_PIN_RESET
#define CTG4_LED_Y_ACTIVE_STATE                         HAL_GPIO_PIN_SET
#define CTG4_LED_Y_INACTIVE_STATE                       HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PG13 pin */
#define PANEL_LED4_R_PORT                               HAL_GPIOG
#define PANEL_LED4_R_PIN                                HAL_GPIO_PIN_13
#define PANEL_LED4_R_INIT_STATE                         HAL_GPIO_PIN_RESET
#define PANEL_LED4_R_ACTIVE_STATE                       HAL_GPIO_PIN_SET
#define PANEL_LED4_R_INACTIVE_STATE                     HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PG13 pin */
#define CTG4_LED_R_PORT                                 HAL_GPIOG
#define CTG4_LED_R_PIN                                  HAL_GPIO_PIN_13
#define CTG4_LED_R_INIT_STATE                           HAL_GPIO_PIN_RESET
#define CTG4_LED_R_ACTIVE_STATE                         HAL_GPIO_PIN_SET
#define CTG4_LED_R_INACTIVE_STATE                       HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PG14 pin */
#define PANEL_LED3_G_PORT                               HAL_GPIOG
#define PANEL_LED3_G_PIN                                HAL_GPIO_PIN_14
#define PANEL_LED3_G_INIT_STATE                         HAL_GPIO_PIN_RESET
#define PANEL_LED3_G_ACTIVE_STATE                       HAL_GPIO_PIN_SET
#define PANEL_LED3_G_INACTIVE_STATE                     HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PG14 pin */
#define CTG3_LED_G_PORT                                 HAL_GPIOG
#define CTG3_LED_G_PIN                                  HAL_GPIO_PIN_14
#define CTG3_LED_G_INIT_STATE                           HAL_GPIO_PIN_RESET
#define CTG3_LED_G_ACTIVE_STATE                         HAL_GPIO_PIN_SET
#define CTG3_LED_G_INACTIVE_STATE                       HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PG15 pin */
#define ENC1_INDEX_PORT                                 HAL_GPIOG
#define ENC1_INDEX_PIN                                  HAL_GPIO_PIN_15

/* EXTI aliases */
#define ENC1_INDEX_EXTI_EXTI_LINE                       HAL_EXTI_LINE_15
#define ENC1_INDEX_EXTI_EXTI_TRIGGER                    HAL_EXTI_TRIGGER_RISING

/* Primary aliases for GPIO PH2-BOOT0 pin */
#define PANEL_LED3_R_PORT                               HAL_GPIOH
#define PANEL_LED3_R_PIN                                HAL_GPIO_PIN_2
#define PANEL_LED3_R_INIT_STATE                         HAL_GPIO_PIN_RESET
#define PANEL_LED3_R_ACTIVE_STATE                       HAL_GPIO_PIN_SET
#define PANEL_LED3_R_INACTIVE_STATE                     HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PH2-BOOT0 pin */
#define CTG3_LED_R_PORT                                 HAL_GPIOH
#define CTG3_LED_R_PIN                                  HAL_GPIO_PIN_2
#define CTG3_LED_R_INIT_STATE                           HAL_GPIO_PIN_RESET
#define CTG3_LED_R_ACTIVE_STATE                         HAL_GPIO_PIN_SET
#define CTG3_LED_R_INACTIVE_STATE                       HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PH3 pin */
#define M2_STEP_PORT                                    HAL_GPIOH
#define M2_STEP_PIN                                     HAL_GPIO_PIN_3
#define M2_STEP_INIT_STATE                              HAL_GPIO_PIN_RESET
#define M2_STEP_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define M2_STEP_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PH4 pin */
#define MB_ADDR1_PORT                                   HAL_GPIOH
#define MB_ADDR1_PIN                                    HAL_GPIO_PIN_4

/* Primary aliases for GPIO PH5 pin */
#define M4_NFAULT_PORT                                  HAL_GPIOH
#define M4_NFAULT_PIN                                   HAL_GPIO_PIN_5

/* EXTI aliases */
#define M4_FAULT_EXTI_EXTI_LINE                         HAL_EXTI_LINE_5
#define M4_FAULT_EXTI_EXTI_TRIGGER                      HAL_EXTI_TRIGGER_RISING

/* Primary aliases for GPIO PH15 pin */
#define PANEL_LED1_Y_PORT                               HAL_GPIOH
#define PANEL_LED1_Y_PIN                                HAL_GPIO_PIN_15
#define PANEL_LED1_Y_INIT_STATE                         HAL_GPIO_PIN_RESET
#define PANEL_LED1_Y_ACTIVE_STATE                       HAL_GPIO_PIN_SET
#define PANEL_LED1_Y_INACTIVE_STATE                     HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PH15 pin */
#define CTG1_LED_Y_PORT                                 HAL_GPIOH
#define CTG1_LED_Y_PIN                                  HAL_GPIO_PIN_15
#define CTG1_LED_Y_INIT_STATE                           HAL_GPIO_PIN_RESET
#define CTG1_LED_Y_ACTIVE_STATE                         HAL_GPIO_PIN_SET
#define CTG1_LED_Y_INACTIVE_STATE                       HAL_GPIO_PIN_RESET

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
  * @brief  Get the EXTI2 object.
  * @retval Pointer on the EXTI2 Handle
  */
hal_exti_handle_t *mx_gpio_default_exti2_gethandle(void);

/**
  * @brief  Get the EXTI3 object.
  * @retval Pointer on the EXTI3 Handle
  */
hal_exti_handle_t *mx_gpio_default_exti3_gethandle(void);

/**
  * @brief  Get the EXTI4 object.
  * @retval Pointer on the EXTI4 Handle
  */
hal_exti_handle_t *mx_gpio_default_exti4_gethandle(void);

/**
  * @brief  Get the EXTI6 object.
  * @retval Pointer on the EXTI6 Handle
  */
hal_exti_handle_t *mx_gpio_default_exti6_gethandle(void);

/**
  * @brief  Get the EXTI8 object.
  * @retval Pointer on the EXTI8 Handle
  */
hal_exti_handle_t *mx_gpio_default_exti8_gethandle(void);

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
  * @brief  Get the EXTI0 object.
  * @retval Pointer on the EXTI0 Handle
  */
hal_exti_handle_t *mx_gpio_default_exti0_gethandle(void);

/**
  * @brief  Get the EXTI15 object.
  * @retval Pointer on the EXTI15 Handle
  */
hal_exti_handle_t *mx_gpio_default_exti15_gethandle(void);

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
/*                            EXTI Line2 interrupt                            */
/******************************************************************************/
void EXTI2_IRQHandler(void);

/******************************************************************************/
/*                            EXTI Line3 interrupt                            */
/******************************************************************************/
void EXTI3_IRQHandler(void);

/******************************************************************************/
/*                            EXTI Line4 interrupt                            */
/******************************************************************************/
void EXTI4_IRQHandler(void);

/******************************************************************************/
/*                            EXTI Line6 interrupt                            */
/******************************************************************************/
void EXTI6_IRQHandler(void);

/******************************************************************************/
/*                            EXTI Line8 interrupt                            */
/******************************************************************************/
void EXTI8_IRQHandler(void);

/******************************************************************************/
/*                           EXTI Line13 interrupt                            */
/******************************************************************************/
void EXTI13_IRQHandler(void);

/******************************************************************************/
/*                           EXTI Line14 interrupt                            */
/******************************************************************************/
void EXTI14_IRQHandler(void);

/******************************************************************************/
/*                            EXTI Line0 interrupt                            */
/******************************************************************************/
void EXTI0_IRQHandler(void);

/******************************************************************************/
/*                           EXTI Line15 interrupt                            */
/******************************************************************************/
void EXTI15_IRQHandler(void);

/******************************************************************************/
/*                            EXTI Line5 interrupt                            */
/******************************************************************************/
void EXTI5_IRQHandler(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MX_GPIO_DEFAULT_H */
