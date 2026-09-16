/**
  ******************************************************************************
  * @file           : mx_filex_app.c
  * @brief          : FileX applicative file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025-2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the mx_filex_license.md file
  * in the same directory as the generated code.
  * If no mx_filex_license.md file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */


/* Includes ------------------------------------------------------------------*/
#include "mx_filex_app.h"

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  Application FileX Initialization.
  * @param  None
  * @retval UINT: FX_SUCCESS on success, error code otherwise
  */
UINT app_filex_init(VOID)
{
  fx_system_initialize();
  return FX_SUCCESS;
}

