/**
  ******************************************************************************
  * @file           : mx_tim8.c
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
#include "mx_tim8.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions prototype------------------------------------------------*/
/* Exported variables by reference--------------------------------------------*/
static hal_tim_handle_t hTIM8;

/* Exported function definition ----------------------------------------------*/
/******************************************************************************/
/* Exported functions for TIM8 in HAL layer */
/******************************************************************************/
hal_tim_handle_t *mx_tim8_init(void)
{
  if (HAL_TIM_Init(&hTIM8, HAL_TIM8) != HAL_OK)
  {
    return NULL;
  }

  HAL_RCC_TIM8_EnableClock();

  /* Timer configuration with external clock */
  hal_tim_config_t config;
  config.prescaler              = 0;
  config.counter_mode           = HAL_TIM_COUNTER_UP;
  config.period                 = 0xFFFF;
  config.repetition_counter     = 0;
  config.clock_sel.clock_source = HAL_TIM_CLK_ENCODER_X4_TI12;
  if (HAL_TIM_SetConfig(&hTIM8, &config) != HAL_OK)
  {
    return NULL;
  }

  /* Sampling Clock */
  if (HAL_TIM_SetDTSPrescaler(&hTIM8, HAL_TIM_DTS_DIV1) != HAL_OK)
  {
    return NULL;
  }
  if (HAL_TIM_SetDTS2Prescaler(&hTIM8, HAL_TIM_DTS2_DIV1) != HAL_OK)
  {
    return NULL;
  }

  hal_tim_ic_channel_config_t ic_config;

  ic_config.source    = HAL_TIM_INPUT_TIM8_TI1_GPIO;
  ic_config.polarity  = HAL_TIM_IC_RISING;
  ic_config.filter    = HAL_TIM_FDIV1;
  if (HAL_TIM_IC_SetConfigChannel(&hTIM8, HAL_TIM_CHANNEL_1, &ic_config) != HAL_OK)
  {
    return NULL;
  }

  hal_tim_ic_capture_unit_config_t ic_capture_unit_config;

  ic_capture_unit_config.source     = HAL_TIM_IC_DIRECT;
  ic_capture_unit_config.prescaler  = HAL_TIM_IC_DIV1;
  if (HAL_TIM_IC_SetConfigCaptureUnit(&hTIM8, HAL_TIM_IC_CAPTURE_UNIT_1, &ic_capture_unit_config) != HAL_OK)
  {
    return NULL;
  }

  ic_config.source    = HAL_TIM_INPUT_TIM8_TI2_GPIO;
  ic_config.polarity  = HAL_TIM_IC_RISING;
  ic_config.filter    = HAL_TIM_FDIV1;
  if (HAL_TIM_IC_SetConfigChannel(&hTIM8, HAL_TIM_CHANNEL_2, &ic_config) != HAL_OK)
  {
    return NULL;
  }

  ic_capture_unit_config.source     = HAL_TIM_IC_DIRECT;
  ic_capture_unit_config.prescaler  = HAL_TIM_IC_DIV1;
  if (HAL_TIM_IC_SetConfigCaptureUnit(&hTIM8, HAL_TIM_IC_CAPTURE_UNIT_2, &ic_capture_unit_config) != HAL_OK)
  {
    return NULL;
  }

  /* Update Event Management */
  if (HAL_TIM_SetUpdateSource(&hTIM8, HAL_TIM_UPDATE_REGULAR) != HAL_OK)
  {
    return NULL;
  }
  if (HAL_TIM_EnableUpdateGeneration(&hTIM8) != HAL_OK)
  {
    return NULL;
  }
  /* External Trigger Configuration */
  hal_tim_ext_trig_config_t ext_trig;
  ext_trig.source     = HAL_TIM_EXT_TRIG_TIM8_GPIO;
  ext_trig.polarity   = HAL_TIM_EXT_TRIG_NONINVERTED;
  ext_trig.filter     = HAL_TIM_FDIV1;
  ext_trig.prescaler  = HAL_TIM_EXT_TRIG_DIV1;
  ext_trig.sync_prescaler = HAL_TIM_EXT_TRIG_SYNC_DIV1;
  if (HAL_TIM_SetExternalTriggerInput(&hTIM8, &ext_trig) != HAL_OK)
  {
    return NULL;
  }

  /* Encoder Index Configuration */
  hal_tim_encoder_index_config_t encoder_index;
  encoder_index.dir       = HAL_TIM_ENCODER_INDEX_UP_DOWN;
  encoder_index.pos       = HAL_TIM_ENCODER_INDEX_POS_DOWN_DOWN;
  encoder_index.blanking  = HAL_TIM_ENCODER_INDEX_BLANK_ALWAYS;
  encoder_index.idx       = HAL_TIM_ENCODER_INDEX_ALL;
  if (HAL_TIM_SetConfigEncoderIndex(&hTIM8, &encoder_index) != HAL_OK)
  {
    return NULL;
  }

  if (HAL_TIM_EnableEncoderIndex(&hTIM8) != HAL_OK)
  {
    return NULL;
  }

  /* ### TIM8 GPIO Configuration ########################### */
  /* GPIO Clocks activation */
  HAL_RCC_GPIOB_EnableClock();

  HAL_RCC_GPIOC_EnableClock();

  HAL_RCC_GPIOG_EnableClock();

  hal_gpio_config_t  gpio_config;

  /**
    [GPIO Pin] ------> [Signal Name] ------> [Labels]

       PB13    ------>   TIM8_CH2   ------>  PB13
    **/
  gpio_config.mode        = HAL_GPIO_MODE_ALTERNATE;
  gpio_config.output_type = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.pull        = HAL_GPIO_PULL_NO;
  gpio_config.speed       = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.alternate   = HAL_GPIO_AF_2;
  HAL_GPIO_Init(PB13_PORT, PB13_PIN, &gpio_config);

  /**
    [GPIO Pin] ------> [Signal Name] ------> [Labels]

       PC6     ------>   TIM8_CH1   ------>  PC6
    **/
  gpio_config.mode        = HAL_GPIO_MODE_ALTERNATE;
  gpio_config.output_type = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.pull        = HAL_GPIO_PULL_NO;
  gpio_config.speed       = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.alternate   = HAL_GPIO_AF_3;
  HAL_GPIO_Init(PC6_PORT, PC6_PIN, &gpio_config);

  /**
    [GPIO Pin] ------> [Signal Name] ------> [Labels]

       PG8     ------>   TIM8_ETR   ------>  PG8
    **/
  gpio_config.mode        = HAL_GPIO_MODE_ALTERNATE;
  gpio_config.output_type = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.pull        = HAL_GPIO_PULL_NO;
  gpio_config.speed       = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.alternate   = HAL_GPIO_AF_3;
  HAL_GPIO_Init(PG8_PORT, PG8_PIN, &gpio_config);

  return &hTIM8;
}

void mx_tim8_deinit(void)
{
  (void)HAL_TIM_DeInit(&hTIM8);

  HAL_RCC_TIM8_DisableClock();

  HAL_RCC_TIM8_Reset();

  /* De-initialize all GPIOB pins associated with TIM8 */
  HAL_GPIO_DeInit(PB13_PORT, PB13_PIN);

  /* De-initialize all GPIOC pins associated with TIM8 */
  HAL_GPIO_DeInit(PC6_PORT, PC6_PIN);

  /* De-initialize all GPIOG pins associated with TIM8 */
  HAL_GPIO_DeInit(PG8_PORT, PG8_PIN);
}

hal_tim_handle_t *mx_tim8_gethandle(void)
{
  return &hTIM8;
}
