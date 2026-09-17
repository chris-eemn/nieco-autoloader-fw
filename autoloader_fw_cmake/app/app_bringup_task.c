/**
 * @file app_bringup_task.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief One-time application and hardware bring-up task.
 * @version 0.1
 * @date 2026-08-07
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC. All Rights Reserved.
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include "app_bringup_task.h"

#include "FreeRTOS.h"
#include "app_console.h"
#include "app_event_simulator.h"
#include "app_task.h"
#include "app_version_git.h"
#include "cal_data.h"
#include "ext_adc_i2c_service.h"
#include "input.h"
#include "stepper_auto_task.h"
#include "stepper_system.h"
#include "task.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

#define APP_BRINGUP_TASK_STACK_SIZE (512U)
#define APP_BRINGUP_TASK_PRIORITY (tskIDLE_PRIORITY + 1U)

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

static void app_bringup_task_run(void* parameters);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

bool app_bringup_task_start(void) {
  BaseType_t result = xTaskCreate(app_bringup_task_run, "Bringup", APP_BRINGUP_TASK_STACK_SIZE, NULL, APP_BRINGUP_TASK_PRIORITY, NULL);

  return (result == pdPASS);
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/

/**
 * @brief Initialise persistent data and motion hardware, then start runtime behavior.
 * @param parameters Unused FreeRTOS task parameter.
 */
static void app_bringup_task_run(void* parameters) {
  bool initialized = true;

  (void)parameters;

  app_console_print("autoloader_fw %s (%s) starting\r\n", APP_VERSION_GIT_DESCRIBE, APP_VERSION_GIT_COMMIT);

  if (cal_data_init() == false) {
    app_console_print("[INFO] Cal data blank or from an older layout -- factory defaults written.\r\n");
  }
  vTaskDelay(pdMS_TO_TICKS(10U));  // give the console time to flush before starting the stepper system

  if (stepper_system_init() == false) {
    app_console_print("[ERROR] Stepper system init failed.\r\n");
    initialized = false;
  }
  else if (stepper_auto_task_start() == false) {
    app_console_print("[ERROR] Stepper auto-test task start failed.\r\n");
    initialized = false;
  }
  else {
    // TODO: enable once we have the actual board with I2C adc
    // ext_adc_i2c_init();
  }
  vTaskDelay(pdMS_TO_TICKS(10U));  // give the console time to flush before starting the stepper system

  app_task_register_axis_event_cb();
  input_init();

  if (app_task_begin() == false) {
    app_console_print("[ERROR] Control task start event was not queued.\r\n");
    initialized = false;
  }
  vTaskDelay(pdMS_TO_TICKS(10U));  // give the console time to flush before starting the stepper system

  // app_simulate_event(2000U / portTICK_PERIOD_MS, APP_EV_DOOR_CLOSED);
  // app_simulate_event(2500U / portTICK_PERIOD_MS, APP_EV_LOCK_CONFIRMED);

  if (initialized != false) {
    vTaskDelete(NULL);
  }

  for (;;) {
    vTaskDelay(portMAX_DELAY);
  }
}
