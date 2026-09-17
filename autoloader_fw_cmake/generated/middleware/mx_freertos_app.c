/**
  ******************************************************************************
  * @file           : mx_freertos_app.c
  * @brief          : FreeRTOS initialization
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the mx_freertos_license.md file
  * in the same directory as the generated code.
  * If no mx_freertos_license.md file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "mx_freertos_app.h"

/* Private define ------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions prototype -----------------------------------------------*/
/**
  * @brief Initializes FreeRTOS kernel objects.
  * @param None
  * @retval int32_t Returns 0 on success, -1 on failure.
  */
int32_t app_synctasks_init (void)
{
  return 0;
}

/**
  * @brief Function for the application Stack Overflow Hook.
  * @param xTask: Handle of the task that overflowed its stack.
  * @param pcTaskName: Name of the task that overflowed its stack.
  * @retval None
  */
void vApplicationStackOverflowHook (TaskHandle_t xTask, char * pcTaskName)
{
  /* If configCHECK_FOR_STACK_OVERFLOW is set to either 1 or 2 then this
  function will automatically get called if a task overflows its stack. */
  ( void ) xTask;
  ( void ) pcTaskName;

  for( ;; );
}

