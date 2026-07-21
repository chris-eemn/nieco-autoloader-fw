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
static hal_exti_handle_t hEXTI6;
static hal_exti_handle_t hEXTI14;
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

  HAL_RCC_GPIOF_EnableClock();

  HAL_RCC_GPIOH_EnableClock();

  /*
    GPIO pin labels :
    PA0   ---------> PA0, SPI_FLASH_CS, SPI_FLASH_CS
    */
  /* Configure PA0 GPIO pin in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = PA0_INIT_STATE;
  if (HAL_GPIO_Init(PA0_PORT, PA0_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PA4   ---------> PA4, M4_STEP, M4_STEP
    PA10  ---------> PA10, M1_EN, M1_EN
    */
  /* Configure PA4, PA10 GPIO pins in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = HAL_GPIO_PIN_RESET;
  if (HAL_GPIO_Init(HAL_GPIOA, PA4_PIN | PA10_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PB0   ---------> PB0, M4_DIR, M4_DIR
    PB5   ---------> PB5, M1_DIR, M1_DIR
    PB8   ---------> PB8, M3_STEP, M3_STEP
    PB9   ---------> PB9, M4_EN, M4_EN
    PB12  ---------> PB12, M1_NSLP, M1_NSLP
    */
  /* Configure PB0, PB5, PB8, PB9, PB12 GPIO pins in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = HAL_GPIO_PIN_RESET;
  if (HAL_GPIO_Init(HAL_GPIOB, PB0_PIN | PB5_PIN | PB8_PIN | PB9_PIN | PB12_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PB1   ---------> PB1, M3_NFAULT, M3_NFAULT
    */
  /* Configure PB1 GPIO pin in input mode */
  gpio_config.mode            = HAL_GPIO_MODE_INPUT;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  if (HAL_GPIO_Init(PB1_PORT, PB1_PIN, &gpio_config) != HAL_OK)
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

  /*
    GPIO pin labels :
    PC7   ---------> PC7, M2_EN, M2_EN
    PC8   ---------> PC8, M2_DIR, M2_DIR
    PC9   ---------> PC9, M2_STEP, M2_STEP
    PC10  ---------> PC10, M3_EN, M3_EN
    PC11  ---------> PC11, M3_NSLP, M3_NSLP
    */
  /* Configure PC7, PC8, PC9, PC10, PC11 GPIO pins in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = HAL_GPIO_PIN_RESET;
  if (HAL_GPIO_Init(HAL_GPIOC, PC7_PIN | PC8_PIN | PC9_PIN | PC10_PIN | PC11_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PD0   ---------> PD0, M3_DIR, M3_DIR
    PD5   ---------> PD5, M1_STEP, M1_STEP
    */
  /* Configure PD0, PD5 GPIO pins in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = HAL_GPIO_PIN_RESET;
  if (HAL_GPIO_Init(HAL_GPIOD, PD0_PIN | PD5_PIN, &gpio_config) != HAL_OK)
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

  /* Initialize the EXTI for line 6 */
  HAL_EXTI_Init(&hEXTI6, HAL_EXTI_LINE_6);

  /* Set the trigger as FALLING for the GPIOD */
  exti_config.trigger   = HAL_EXTI_TRIGGER_FALLING;
  exti_config.gpio_port = HAL_EXTI_GPIOD;
  HAL_EXTI_SetConfig(&hEXTI6, &exti_config);

  /* Enable the INTERRUPT mode */
  HAL_EXTI_Enable(&hEXTI6, HAL_EXTI_MODE_INTERRUPT);

  /* Set line 6 Interrupt priority */
  HAL_CORTEX_NVIC_SetPriority(EXTI6_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_5, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_EnableIRQ(EXTI6_IRQn);

  /*
    GPIO pin labels :
    PF14  ---------> PF14, M2_NFAULT, M2_NFAULT
    */
  /* Configure PF14 GPIO pin in input mode */
  gpio_config.mode            = HAL_GPIO_MODE_INPUT;
  gpio_config.pull            = HAL_GPIO_PULL_UP;
  if (HAL_GPIO_Init(PF14_PORT, PF14_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /* Initialize the EXTI for line 14 */
  HAL_EXTI_Init(&hEXTI14, HAL_EXTI_LINE_14);

  /* Set the trigger as FALLING for the GPIOF */
  exti_config.trigger   = HAL_EXTI_TRIGGER_FALLING;
  exti_config.gpio_port = HAL_EXTI_GPIOF;
  HAL_EXTI_SetConfig(&hEXTI14, &exti_config);

  /* Set line 14 Interrupt priority */
  HAL_CORTEX_NVIC_SetPriority(EXTI14_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_EnableIRQ(EXTI14_IRQn);

  /*
    GPIO pin labels :
    PF15  ---------> PF15, M2_NSLP, M2_NSLP
    */
  /* Configure PF15 GPIO pin in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = PF15_INIT_STATE;
  if (HAL_GPIO_Init(PF15_PORT, PF15_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PH4   ---------> PH4, M4_NSLP, M4_NSLP
    */
  /* Configure PH4 GPIO pin in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = PH4_INIT_STATE;
  if (HAL_GPIO_Init(PH4_PORT, PH4_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PH5   ---------> PH5, M4_NFAULT, M4_NFAULT
    */
  /* Configure PH5 GPIO pin in input mode */
  gpio_config.mode            = HAL_GPIO_MODE_INPUT;
  gpio_config.pull            = HAL_GPIO_PULL_UP;
  if (HAL_GPIO_Init(PH5_PORT, PH5_PIN, &gpio_config) != HAL_OK)
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

  /* De-initialize the EXTI for GPIOD line6 */
  HAL_EXTI_DeInit(&hEXTI6);

  /* set line 6 Interrupt priority */
  HAL_CORTEX_NVIC_DisableIRQ(EXTI6_IRQn);

  /* De-initialize the EXTI for GPIOF line14 */
  HAL_EXTI_DeInit(&hEXTI14);

  /* set line 14 Interrupt priority */
  HAL_CORTEX_NVIC_DisableIRQ(EXTI14_IRQn);

  /* De-initialize the EXTI for GPIOH line5 */
  HAL_EXTI_DeInit(&hEXTI5);

  /* set line 5 Interrupt priority */
  HAL_CORTEX_NVIC_DisableIRQ(EXTI5_IRQn);

  /* De-initialize pins of GPIOA port */
  HAL_GPIO_DeInit(HAL_GPIOA, PA0_PIN | PA4_PIN | PA10_PIN);

  /* De-initialize pins of GPIOB port */
  HAL_GPIO_DeInit(HAL_GPIOB, PB0_PIN | PB1_PIN | PB5_PIN | PB8_PIN | PB9_PIN | PB12_PIN);

  /* De-initialize pins of GPIOC port */
  HAL_GPIO_DeInit(HAL_GPIOC, PC7_PIN | PC8_PIN | PC9_PIN | PC10_PIN | PC11_PIN);

  /* De-initialize pins of GPIOD port */
  HAL_GPIO_DeInit(HAL_GPIOD, PD0_PIN | PD5_PIN | PD6_PIN);

  /* De-initialize pins of GPIOF port */
  HAL_GPIO_DeInit(HAL_GPIOF, PF14_PIN | PF15_PIN);

  /* De-initialize pins of GPIOH port */
  HAL_GPIO_DeInit(HAL_GPIOH, PH4_PIN | PH5_PIN);

  return SYSTEM_OK;
}

hal_exti_handle_t *mx_gpio_default_exti1_gethandle(void)
{
  return &hEXTI1;
}

hal_exti_handle_t *mx_gpio_default_exti6_gethandle(void)
{
  return &hEXTI6;
}

hal_exti_handle_t *mx_gpio_default_exti14_gethandle(void)
{
  return &hEXTI14;
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
/*                            EXTI Line6 interrupt                            */
/******************************************************************************/
void EXTI6_IRQHandler(void)
{
  HAL_EXTI_IRQHandler(&hEXTI6);
}

/******************************************************************************/
/*                           EXTI Line14 interrupt                            */
/******************************************************************************/
void EXTI14_IRQHandler(void)
{
  HAL_EXTI_IRQHandler(&hEXTI14);
}

/******************************************************************************/
/*                            EXTI Line5 interrupt                            */
/******************************************************************************/
void EXTI5_IRQHandler(void)
{
  HAL_EXTI_IRQHandler(&hEXTI5);
}
