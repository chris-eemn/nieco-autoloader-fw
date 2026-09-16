/**
 ******************************************************************************
 * file           : main.c
 * brief          : Main program body
 *                  main() initializes the system and creates the app tasks.
 ******************************************************************************
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

#include "FreeRTOS.h"
#include "app_bringup_task.h"
#include "app_console.h"
#include "app_task.h"
#include "mb_regs.h"
#include "stepper_ctrl.h"
#include "task.h"
#include "usb_loader.h"
#include "w25q.h"

/** @brief The application entry point. */
int main(void) {
  if (mx_system_init() != SYSTEM_OK) {
    app_console_print("[ERROR] System init failed.\r\n");
    while (1)
      ;
  }

  /* CubeMX has no NVIC-priority control for USB_DRD_FS -- mx_usb_drd_fs_host_init() (called from
   * mx_system_init() above) hardcodes preemption priority 0, which is numerically above
   * configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (5). USBX's FreeRTOS port layer calls
   * xSemaphoreGiveFromISR/portYIELD_FROM_ISR from this IRQ, which is undefined behavior at that
   * priority. Override it here since the generated file gets clobbered on every regen. */
  HAL_CORTEX_NVIC_SetPriority(USB_DRD_FS_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_5, HAL_CORTEX_NVIC_SUB_PRIORITY_0);

  /* Must be called before any FreeRTOS API, including task creation. Otherwise the SPI
   * interrupt priority prevents the initialization transfer from completing. */
  w25q_initialize();

#if defined(USE_TRACE) && USE_TRACE != 0
  mx_basic_stdio_init();
#endif

  app_console_init();
  app_console_commands_register();
  stepper_ctrl_init();
  mb_regs_init();
  usb_loader_start();

  if (app_task_start() == false) {
    while (1)
      ;
  }

  if (app_bringup_task_start() == false) {
    while (1)
      ;
  }

  vTaskStartScheduler();

  return 0;
}

void HardFault_Handler(void) {
  app_console_print("[ERROR] HardFault occurred.\r\n");
  while (1)
    ;
}
