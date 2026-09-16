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
static hal_exti_handle_t hEXTI1;
static hal_exti_handle_t hEXTI2;
static hal_exti_handle_t hEXTI3;
static hal_exti_handle_t hEXTI4;
static hal_exti_handle_t hEXTI6;
static hal_exti_handle_t hEXTI8;
static hal_exti_handle_t hEXTI13;
static hal_exti_handle_t hEXTI14;
static hal_exti_handle_t hEXTI0;
static hal_exti_handle_t hEXTI15;
static hal_exti_handle_t hEXTI5;

/******************************************************************************/
/* Exported functions for GPIO in HAL layer                                   */
/******************************************************************************/
system_status_t mx_gpio_default_init(void)
{
  hal_gpio_config_t  gpio_config;

  HAL_RCC_GPIOA_EnableClock();

  HAL_RCC_GPIOB_EnableClock();

  HAL_RCC_GPIOC_EnableClock();

  HAL_RCC_GPIOD_EnableClock();

  HAL_RCC_GPIOE_EnableClock();

  HAL_RCC_GPIOF_EnableClock();

  HAL_RCC_GPIOG_EnableClock();

  HAL_RCC_GPIOH_EnableClock();

  /*
    GPIO pin labels :
    PA1   ---------> MB_ADDR0
    */
  /* Configure PA1 GPIO pin in input mode */
  gpio_config.mode            = HAL_GPIO_MODE_INPUT;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  if (HAL_GPIO_Init(MB_ADDR0_PORT, MB_ADDR0_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PA4   ---------> USB_VBUS_EN
    PA5   ---------> M7_DIR
    PA10  ---------> M2_DIR
    */
  /* Configure PA4, PA5, PA10 GPIO pins in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = HAL_GPIO_PIN_RESET;
  if (HAL_GPIO_Init(HAL_GPIOA, USB_VBUS_EN_PIN | M7_DIR_PIN | M2_DIR_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PB0   ---------> CTG4_EN
    PB5   ---------> PANEL_LED3_Y, CTG3_LED_Y
    PB8   ---------> PANEL_LED2_G, CTG2_LED_G
    PB9   ---------> PANEL_LED2_Y, CTG2_LED_Y
    PB12  ---------> CTG4_NSLP
    PB14  ---------> SW3_24V, FUTURE_2
    */
  /* Configure PB0, PB5, PB8, PB9, PB12, PB14 GPIO pins in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = HAL_GPIO_PIN_RESET;
  if (HAL_GPIO_Init(HAL_GPIOB, CTG4_EN_PIN | PANEL_LED3_Y_PIN | PANEL_LED2_G_PIN | PANEL_LED2_Y_PIN | CTG4_NSLP_PIN | SW3_24V_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PB1   ---------> M3_NFAULT
    PB2   ---------> M5_NFAULT
    */
  /* Configure PB1, PB2 GPIO pins in input mode */
  gpio_config.mode            = HAL_GPIO_MODE_INPUT;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  if (HAL_GPIO_Init(HAL_GPIOB, M3_NFAULT_PIN | M5_NFAULT_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  hal_exti_config_t exti_config;

  /* Initialize the EXTI for line 1 */
  HAL_EXTI_Init(&hEXTI1, HAL_EXTI_LINE_1);

  /* Set the trigger as RISING for the GPIOB */
  exti_config.trigger   = HAL_EXTI_TRIGGER_RISING;
  exti_config.gpio_port = HAL_EXTI_GPIOB;
  HAL_EXTI_SetConfig(&hEXTI1, &exti_config);

  /* Set line 1 Interrupt priority */
  HAL_CORTEX_NVIC_SetPriority(EXTI1_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_EnableIRQ(EXTI1_IRQn);

  /* Initialize the EXTI for line 2 */
  HAL_EXTI_Init(&hEXTI2, HAL_EXTI_LINE_2);

  /* Set the trigger as RISING for the GPIOB */
  exti_config.trigger   = HAL_EXTI_TRIGGER_RISING;
  exti_config.gpio_port = HAL_EXTI_GPIOB;
  HAL_EXTI_SetConfig(&hEXTI2, &exti_config);

  /* Set line 2 Interrupt priority */
  HAL_CORTEX_NVIC_SetPriority(EXTI2_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_EnableIRQ(EXTI2_IRQn);

  /*
    GPIO pin labels :
    PC0   ---------> FUTURE_MB_DIR, MB_FUTURE_DIR
    PC1   ---------> M6_DIR
    PC4   ---------> M8_DIR
    PC5   ---------> M8_STEP
    PC7   ---------> CTG2_EN
    PC8   ---------> M3_DIR
    PC9   ---------> M3_STEP
    PC10  ---------> CTG1_EN
    PC11  ---------> M1_DIR
    PC13  ---------> PANEL_LED1_R, CTG1_LED_R
    */
  /* Configure PC0, PC1, PC4, PC5, PC7, PC8, PC9, PC10, PC11, PC13 GPIO pins in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = HAL_GPIO_PIN_RESET;
  if (HAL_GPIO_Init(HAL_GPIOC, FUTURE_MB_DIR_PIN | M6_DIR_PIN | M8_DIR_PIN | M8_STEP_PIN | CTG2_EN_PIN | M3_DIR_PIN | M3_STEP_PIN | CTG1_EN_PIN | M1_DIR_PIN | PANEL_LED1_R_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PC3   ---------> M6_NFAULT
    */
  /* Configure PC3 GPIO pin in input mode */
  gpio_config.mode            = HAL_GPIO_MODE_INPUT;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  if (HAL_GPIO_Init(M6_NFAULT_PORT, M6_NFAULT_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /* Initialize the EXTI for line 3 */
  HAL_EXTI_Init(&hEXTI3, HAL_EXTI_LINE_3);

  /* Set the trigger as RISING for the GPIOC */
  exti_config.trigger   = HAL_EXTI_TRIGGER_RISING;
  exti_config.gpio_port = HAL_EXTI_GPIOC;
  HAL_EXTI_SetConfig(&hEXTI3, &exti_config);

  /* Set line 3 Interrupt priority */
  HAL_CORTEX_NVIC_SetPriority(EXTI3_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_EnableIRQ(EXTI3_IRQn);

  /*
    GPIO pin labels :
    PD0   ---------> M1_STEP
    PD5   ---------> PANEL_LED5_G, EXTRA_LED_G
    PD7   ---------> PANEL_LED5_Y, EXTRA_LED_Y
    PD8   ---------> M6_STEP
    PD9   ---------> CTG3_EN
    PD10  ---------> CTG3_NSLP, CTG3_NSLEEP
    PD14  ---------> M5_DIR
    PD15  ---------> CTG2_NSLP
    */
  /* Configure PD0, PD5, PD7, PD8, PD9, PD10, PD14, PD15 GPIO pins in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = HAL_GPIO_PIN_RESET;
  if (HAL_GPIO_Init(HAL_GPIOD, M1_STEP_PIN | PANEL_LED5_G_PIN | PANEL_LED5_Y_PIN | M6_STEP_PIN | CTG3_EN_PIN | CTG3_NSLP_PIN | M5_DIR_PIN | CTG2_NSLP_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PD4   ---------> M2_NFAULT
    PD6   ---------> M1_NFAULT
    */
  /* Configure PD4, PD6 GPIO pins in input mode */
  gpio_config.mode            = HAL_GPIO_MODE_INPUT;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  if (HAL_GPIO_Init(HAL_GPIOD, M2_NFAULT_PIN | M1_NFAULT_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /* Initialize the EXTI for line 4 */
  HAL_EXTI_Init(&hEXTI4, HAL_EXTI_LINE_4);

  /* Set the trigger as RISING for the GPIOD */
  exti_config.trigger   = HAL_EXTI_TRIGGER_RISING;
  exti_config.gpio_port = HAL_EXTI_GPIOD;
  HAL_EXTI_SetConfig(&hEXTI4, &exti_config);

  /* Set line 4 Interrupt priority */
  HAL_CORTEX_NVIC_SetPriority(EXTI4_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_EnableIRQ(EXTI4_IRQn);

  /* Initialize the EXTI for line 6 */
  HAL_EXTI_Init(&hEXTI6, HAL_EXTI_LINE_6);

  /* Set the trigger as RISING for the GPIOD */
  exti_config.trigger   = HAL_EXTI_TRIGGER_RISING;
  exti_config.gpio_port = HAL_EXTI_GPIOD;
  HAL_EXTI_SetConfig(&hEXTI6, &exti_config);

  /* Set line 6 Interrupt priority */
  HAL_CORTEX_NVIC_SetPriority(EXTI6_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_EnableIRQ(EXTI6_IRQn);

  /*
    GPIO pin labels :
    PE2   ---------> PANEL_LED2_R, CTG2_LED_R
    PE5   ---------> PANEL_LED1_G, CTG1_LED_G
    PE9   ---------> SW1_24V, BUZZER
    PE10  ---------> STATUS_LED1
    PE12  ---------> STATUS_LED2
    PE13  ---------> SPI_FLASH_WP, EEPROM_CS
    PE15  ---------> SWITCH_12V, DOOR_LOCK_CTRL
    */
  /* Configure PE2, PE5, PE9, PE10, PE12, PE13, PE15 GPIO pins in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = HAL_GPIO_PIN_RESET;
  if (HAL_GPIO_Init(HAL_GPIOE, PANEL_LED2_R_PIN | PANEL_LED1_G_PIN | SW1_24V_PIN | STATUS_LED1_PIN | STATUS_LED2_PIN | SPI_FLASH_WP_PIN | SWITCH_12V_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PE3   ---------> DOOR_SW
    PE4   ---------> SHUTDOWN_SW
    PE8   ---------> M7_NFAULT
    */
  /* Configure PE3, PE4, PE8 GPIO pins in input mode */
  gpio_config.mode            = HAL_GPIO_MODE_INPUT;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  if (HAL_GPIO_Init(HAL_GPIOE, DOOR_SW_PIN | SHUTDOWN_SW_PIN | M7_NFAULT_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /* Initialize the EXTI for line 8 */
  HAL_EXTI_Init(&hEXTI8, HAL_EXTI_LINE_8);

  /* Set the trigger as RISING for the GPIOE */
  exti_config.trigger   = HAL_EXTI_TRIGGER_RISING;
  exti_config.gpio_port = HAL_EXTI_GPIOE;
  HAL_EXTI_SetConfig(&hEXTI8, &exti_config);

  /* Set line 8 Interrupt priority */
  HAL_CORTEX_NVIC_SetPriority(EXTI8_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_EnableIRQ(EXTI8_IRQn);

  /*
    GPIO pin labels :
    PE14  ---------> SPI_FLASH_RESET, SPI_FLASH_NRESET
    */
  /* Configure PE14 GPIO pin in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = SPI_FLASH_RESET_INIT_STATE;
  if (HAL_GPIO_Init(SPI_FLASH_RESET_PORT, SPI_FLASH_RESET_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PF0   ---------> RELOAD_SW
    PF1   ---------> DOOR_LOCK_DETECT_SW
    PF2   ---------> CARTRIDGE_SENSOR_1, CTG1_SENSE1
    PF3   ---------> CARTRIDGE_SENSOR_2, CTG1_SENSE2
    PF4   ---------> CARTRIDGE_SENSOR_3, CTG2_SENSE1
    PF5   ---------> CARTRIDGE_SENSOR_4, CTG2_SENSE2
    PF6   ---------> CARTRIDGE_SENSOR_5, CTG3_SENSE1
    PF7   ---------> CARTRIDGE_SENSOR_6, CTG3_SENSE2
    PF8   ---------> CARTRIDGE_SENSOR_7, CTG4_SENSE1
    PF9   ---------> CARTRIDGE_SENSOR_8, CTG4_SENSE2
    PF12  ---------> USB_VBUS_DETECT
    PF13  ---------> USB_VBUS_FAULT
    PF14  ---------> M8_NFAULT
    */
  /* Configure PF0, PF1, PF2, PF3, PF4, PF5, PF6, PF7, PF8, PF9, PF12, PF13, PF14 GPIO pins in input mode */
  gpio_config.mode            = HAL_GPIO_MODE_INPUT;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  if (HAL_GPIO_Init(HAL_GPIOF, RELOAD_SW_PIN | DOOR_LOCK_DETECT_SW_PIN | CARTRIDGE_SENSOR_1_PIN | CARTRIDGE_SENSOR_2_PIN | CARTRIDGE_SENSOR_3_PIN | CARTRIDGE_SENSOR_4_PIN | CARTRIDGE_SENSOR_5_PIN | CARTRIDGE_SENSOR_6_PIN | CARTRIDGE_SENSOR_7_PIN | CARTRIDGE_SENSOR_8_PIN | USB_VBUS_DETECT_PIN | USB_VBUS_FAULT_PIN | M8_NFAULT_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /* Initialize the EXTI for line 13 */
  HAL_EXTI_Init(&hEXTI13, HAL_EXTI_LINE_13);

  /* Set the trigger as RISING for the GPIOF */
  exti_config.trigger   = HAL_EXTI_TRIGGER_RISING;
  exti_config.gpio_port = HAL_EXTI_GPIOF;
  HAL_EXTI_SetConfig(&hEXTI13, &exti_config);

  /* Set line 13 Interrupt priority */
  HAL_CORTEX_NVIC_SetPriority(EXTI13_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_EnableIRQ(EXTI13_IRQn);

  /* Initialize the EXTI for line 14 */
  HAL_EXTI_Init(&hEXTI14, HAL_EXTI_LINE_14);

  /* Set the trigger as RISING for the GPIOF */
  exti_config.trigger   = HAL_EXTI_TRIGGER_RISING;
  exti_config.gpio_port = HAL_EXTI_GPIOF;
  HAL_EXTI_SetConfig(&hEXTI14, &exti_config);

  /* Set line 14 Interrupt priority */
  HAL_CORTEX_NVIC_SetPriority(EXTI14_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_EnableIRQ(EXTI14_IRQn);

  /*
    GPIO pin labels :
    PF10  ---------> UI_MB_DIR, MB_UI_DIR
    PF11  ---------> M5_STEP
    PF15  ---------> M7_STEP
    */
  /* Configure PF10, PF11, PF15 GPIO pins in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = HAL_GPIO_PIN_RESET;
  if (HAL_GPIO_Init(HAL_GPIOF, UI_MB_DIR_PIN | M5_STEP_PIN | M7_STEP_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PG0   ---------> ENC3_INDEX
    PG15  ---------> ENC1_INDEX
    */
  /* Configure PG0, PG15 GPIO pins in input mode */
  gpio_config.mode            = HAL_GPIO_MODE_INPUT;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  if (HAL_GPIO_Init(HAL_GPIOG, ENC3_INDEX_PIN | ENC1_INDEX_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /* Initialize the EXTI for line 0 */
  HAL_EXTI_Init(&hEXTI0, HAL_EXTI_LINE_0);

  /* Set the trigger as RISING for the GPIOG */
  exti_config.trigger   = HAL_EXTI_TRIGGER_RISING;
  exti_config.gpio_port = HAL_EXTI_GPIOG;
  HAL_EXTI_SetConfig(&hEXTI0, &exti_config);

  /* Set line 0 Interrupt priority */
  HAL_CORTEX_NVIC_SetPriority(EXTI0_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_EnableIRQ(EXTI0_IRQn);

  /* Initialize the EXTI for line 15 */
  HAL_EXTI_Init(&hEXTI15, HAL_EXTI_LINE_15);

  /* Set the trigger as RISING for the GPIOG */
  exti_config.trigger   = HAL_EXTI_TRIGGER_RISING;
  exti_config.gpio_port = HAL_EXTI_GPIOG;
  HAL_EXTI_SetConfig(&hEXTI15, &exti_config);

  /* Set line 15 Interrupt priority */
  HAL_CORTEX_NVIC_SetPriority(EXTI15_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_EnableIRQ(EXTI15_IRQn);

  /*
    GPIO pin labels :
    PG1   ---------> SW2_24V, FUTURE_1
    PG2   ---------> SPI_FLASH_CS
    PG3   ---------> M4_DIR
    PG6   ---------> M4_STEP
    PG7   ---------> CTG1_NSLP
    PG9   ---------> PANEL_LED5_R, EXTRA_LED_R
    PG10  ---------> PANEL_LED4_G, CTG4_LED_G
    PG11  ---------> PANEL_LED4_Y, CTG4_LED_Y
    PG13  ---------> PANEL_LED4_R, CTG4_LED_R
    PG14  ---------> PANEL_LED3_G, CTG3_LED_G
    */
  /* Configure PG1, PG2, PG3, PG6, PG7, PG9, PG10, PG11, PG13, PG14 GPIO pins in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = HAL_GPIO_PIN_RESET;
  if (HAL_GPIO_Init(HAL_GPIOG, SW2_24V_PIN | SPI_FLASH_CS_PIN | M4_DIR_PIN | M4_STEP_PIN | CTG1_NSLP_PIN | PANEL_LED5_R_PIN | PANEL_LED4_G_PIN | PANEL_LED4_Y_PIN | PANEL_LED4_R_PIN | PANEL_LED3_G_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PH2   ---------> PANEL_LED3_R, CTG3_LED_R
    PH3   ---------> M2_STEP
    PH15  ---------> PANEL_LED1_Y, CTG1_LED_Y
    */
  /* Configure PH2, PH3, PH15 GPIO pins in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = HAL_GPIO_PIN_RESET;
  if (HAL_GPIO_Init(HAL_GPIOH, PANEL_LED3_R_PIN | M2_STEP_PIN | PANEL_LED1_Y_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PH4   ---------> MB_ADDR1
    PH5   ---------> M4_NFAULT
    */
  /* Configure PH4, PH5 GPIO pins in input mode */
  gpio_config.mode            = HAL_GPIO_MODE_INPUT;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  if (HAL_GPIO_Init(HAL_GPIOH, MB_ADDR1_PIN | M4_NFAULT_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /* Initialize the EXTI for line 5 */
  HAL_EXTI_Init(&hEXTI5, HAL_EXTI_LINE_5);

  /* Set the trigger as RISING for the GPIOH */
  exti_config.trigger   = HAL_EXTI_TRIGGER_RISING;
  exti_config.gpio_port = HAL_EXTI_GPIOH;
  HAL_EXTI_SetConfig(&hEXTI5, &exti_config);

  /* Set line 5 Interrupt priority */
  HAL_CORTEX_NVIC_SetPriority(EXTI5_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_EnableIRQ(EXTI5_IRQn);

  return SYSTEM_OK;
}

system_status_t mx_gpio_default_deinit(void)
{
  /* De-initialize the EXTI for GPIOB line1 */
  HAL_EXTI_DeInit(&hEXTI1);

  /* set line 1 Interrupt priority */
  HAL_CORTEX_NVIC_DisableIRQ(EXTI1_IRQn);
  /* De-initialize the EXTI for GPIOB line2 */
  HAL_EXTI_DeInit(&hEXTI2);

  /* set line 2 Interrupt priority */
  HAL_CORTEX_NVIC_DisableIRQ(EXTI2_IRQn);

  /* De-initialize the EXTI for GPIOC line3 */
  HAL_EXTI_DeInit(&hEXTI3);

  /* set line 3 Interrupt priority */
  HAL_CORTEX_NVIC_DisableIRQ(EXTI3_IRQn);

  /* De-initialize the EXTI for GPIOD line4 */
  HAL_EXTI_DeInit(&hEXTI4);

  /* set line 4 Interrupt priority */
  HAL_CORTEX_NVIC_DisableIRQ(EXTI4_IRQn);
  /* De-initialize the EXTI for GPIOD line6 */
  HAL_EXTI_DeInit(&hEXTI6);

  /* set line 6 Interrupt priority */
  HAL_CORTEX_NVIC_DisableIRQ(EXTI6_IRQn);

  /* De-initialize the EXTI for GPIOE line8 */
  HAL_EXTI_DeInit(&hEXTI8);

  /* set line 8 Interrupt priority */
  HAL_CORTEX_NVIC_DisableIRQ(EXTI8_IRQn);

  /* De-initialize the EXTI for GPIOF line13 */
  HAL_EXTI_DeInit(&hEXTI13);

  /* set line 13 Interrupt priority */
  HAL_CORTEX_NVIC_DisableIRQ(EXTI13_IRQn);
  /* De-initialize the EXTI for GPIOF line14 */
  HAL_EXTI_DeInit(&hEXTI14);

  /* set line 14 Interrupt priority */
  HAL_CORTEX_NVIC_DisableIRQ(EXTI14_IRQn);

  /* De-initialize the EXTI for GPIOG line0 */
  HAL_EXTI_DeInit(&hEXTI0);

  /* set line 0 Interrupt priority */
  HAL_CORTEX_NVIC_DisableIRQ(EXTI0_IRQn);
  /* De-initialize the EXTI for GPIOG line15 */
  HAL_EXTI_DeInit(&hEXTI15);

  /* set line 15 Interrupt priority */
  HAL_CORTEX_NVIC_DisableIRQ(EXTI15_IRQn);

  /* De-initialize the EXTI for GPIOH line5 */
  HAL_EXTI_DeInit(&hEXTI5);

  /* set line 5 Interrupt priority */
  HAL_CORTEX_NVIC_DisableIRQ(EXTI5_IRQn);

  /* De-initialize pins of GPIOA port */
  HAL_GPIO_DeInit(HAL_GPIOA, MB_ADDR0_PIN | USB_VBUS_EN_PIN | M7_DIR_PIN | M2_DIR_PIN);

  /* De-initialize pins of GPIOB port */
  HAL_GPIO_DeInit(HAL_GPIOB, CTG4_EN_PIN | M3_NFAULT_PIN | M5_NFAULT_PIN | PANEL_LED3_Y_PIN | PANEL_LED2_G_PIN | PANEL_LED2_Y_PIN | CTG4_NSLP_PIN | SW3_24V_PIN);

  /* De-initialize pins of GPIOC port */
  HAL_GPIO_DeInit(HAL_GPIOC, FUTURE_MB_DIR_PIN | M6_DIR_PIN | M6_NFAULT_PIN | M8_DIR_PIN | M8_STEP_PIN | CTG2_EN_PIN | M3_DIR_PIN | M3_STEP_PIN | CTG1_EN_PIN | M1_DIR_PIN | PANEL_LED1_R_PIN);

  /* De-initialize pins of GPIOD port */
  HAL_GPIO_DeInit(HAL_GPIOD, M1_STEP_PIN | M2_NFAULT_PIN | PANEL_LED5_G_PIN | M1_NFAULT_PIN | PANEL_LED5_Y_PIN | M6_STEP_PIN | CTG3_EN_PIN | CTG3_NSLP_PIN | M5_DIR_PIN | CTG2_NSLP_PIN);

  /* De-initialize pins of GPIOE port */
  HAL_GPIO_DeInit(HAL_GPIOE, PANEL_LED2_R_PIN | DOOR_SW_PIN | SHUTDOWN_SW_PIN | PANEL_LED1_G_PIN | M7_NFAULT_PIN | SW1_24V_PIN | STATUS_LED1_PIN | STATUS_LED2_PIN | SPI_FLASH_WP_PIN | SPI_FLASH_RESET_PIN | SWITCH_12V_PIN);

  /* De-initialize pins of GPIOF port */
  HAL_GPIO_DeInit(HAL_GPIOF, RELOAD_SW_PIN | DOOR_LOCK_DETECT_SW_PIN | CARTRIDGE_SENSOR_1_PIN | CARTRIDGE_SENSOR_2_PIN | CARTRIDGE_SENSOR_3_PIN | CARTRIDGE_SENSOR_4_PIN | CARTRIDGE_SENSOR_5_PIN | CARTRIDGE_SENSOR_6_PIN | CARTRIDGE_SENSOR_7_PIN | CARTRIDGE_SENSOR_8_PIN | UI_MB_DIR_PIN | M5_STEP_PIN | USB_VBUS_DETECT_PIN | USB_VBUS_FAULT_PIN | M8_NFAULT_PIN | M7_STEP_PIN);

  /* De-initialize pins of GPIOG port */
  HAL_GPIO_DeInit(HAL_GPIOG, ENC3_INDEX_PIN | SW2_24V_PIN | SPI_FLASH_CS_PIN | M4_DIR_PIN | M4_STEP_PIN | CTG1_NSLP_PIN | PANEL_LED5_R_PIN | PANEL_LED4_G_PIN | PANEL_LED4_Y_PIN | PANEL_LED4_R_PIN | PANEL_LED3_G_PIN | ENC1_INDEX_PIN);

  /* De-initialize pins of GPIOH port */
  HAL_GPIO_DeInit(HAL_GPIOH, PANEL_LED3_R_PIN | M2_STEP_PIN | MB_ADDR1_PIN | M4_NFAULT_PIN | PANEL_LED1_Y_PIN);

  return SYSTEM_OK;
}

hal_exti_handle_t *mx_gpio_default_exti1_gethandle(void)
{
  return &hEXTI1;
}

hal_exti_handle_t *mx_gpio_default_exti2_gethandle(void)
{
  return &hEXTI2;
}

hal_exti_handle_t *mx_gpio_default_exti3_gethandle(void)
{
  return &hEXTI3;
}

hal_exti_handle_t *mx_gpio_default_exti4_gethandle(void)
{
  return &hEXTI4;
}

hal_exti_handle_t *mx_gpio_default_exti6_gethandle(void)
{
  return &hEXTI6;
}

hal_exti_handle_t *mx_gpio_default_exti8_gethandle(void)
{
  return &hEXTI8;
}

hal_exti_handle_t *mx_gpio_default_exti13_gethandle(void)
{
  return &hEXTI13;
}

hal_exti_handle_t *mx_gpio_default_exti14_gethandle(void)
{
  return &hEXTI14;
}

hal_exti_handle_t *mx_gpio_default_exti0_gethandle(void)
{
  return &hEXTI0;
}

hal_exti_handle_t *mx_gpio_default_exti15_gethandle(void)
{
  return &hEXTI15;
}

hal_exti_handle_t *mx_gpio_default_exti5_gethandle(void)
{
  return &hEXTI5;
}

/******************************************************************************/
/*                            EXTI Line1 interrupt                            */
/******************************************************************************/
void EXTI1_IRQHandler(void)
{
  HAL_EXTI_IRQHandler(&hEXTI1);
}

/******************************************************************************/
/*                            EXTI Line2 interrupt                            */
/******************************************************************************/
void EXTI2_IRQHandler(void)
{
  HAL_EXTI_IRQHandler(&hEXTI2);
}

/******************************************************************************/
/*                            EXTI Line3 interrupt                            */
/******************************************************************************/
void EXTI3_IRQHandler(void)
{
  HAL_EXTI_IRQHandler(&hEXTI3);
}

/******************************************************************************/
/*                            EXTI Line4 interrupt                            */
/******************************************************************************/
void EXTI4_IRQHandler(void)
{
  HAL_EXTI_IRQHandler(&hEXTI4);
}

/******************************************************************************/
/*                            EXTI Line6 interrupt                            */
/******************************************************************************/
void EXTI6_IRQHandler(void)
{
  HAL_EXTI_IRQHandler(&hEXTI6);
}

/******************************************************************************/
/*                            EXTI Line8 interrupt                            */
/******************************************************************************/
void EXTI8_IRQHandler(void)
{
  HAL_EXTI_IRQHandler(&hEXTI8);
}

/******************************************************************************/
/*                           EXTI Line13 interrupt                            */
/******************************************************************************/
void EXTI13_IRQHandler(void)
{
  HAL_EXTI_IRQHandler(&hEXTI13);
}

/******************************************************************************/
/*                           EXTI Line14 interrupt                            */
/******************************************************************************/
void EXTI14_IRQHandler(void)
{
  HAL_EXTI_IRQHandler(&hEXTI14);
}

/******************************************************************************/
/*                            EXTI Line0 interrupt                            */
/******************************************************************************/
void EXTI0_IRQHandler(void)
{
  HAL_EXTI_IRQHandler(&hEXTI0);
}

/******************************************************************************/
/*                           EXTI Line15 interrupt                            */
/******************************************************************************/
void EXTI15_IRQHandler(void)
{
  HAL_EXTI_IRQHandler(&hEXTI15);
}

/******************************************************************************/
/*                            EXTI Line5 interrupt                            */
/******************************************************************************/
void EXTI5_IRQHandler(void)
{
  HAL_EXTI_IRQHandler(&hEXTI5);
}
