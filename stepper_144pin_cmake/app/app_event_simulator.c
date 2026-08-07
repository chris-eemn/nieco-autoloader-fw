/**
 * @file app_event_simulator.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief
 * @version 0.1
 * @date 2026-08-07
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include "app_event_simulator.h"
#include "app_task.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

/**
 * @brief Simulated event timer callback.
 */
void app_sim_timer_callback(TimerHandle_t timer) {
  app_event_t event = {
      .id = (app_event_id_enum)(uintptr_t)pvTimerGetTimerID(timer),
      .slot = APP_NO_SLOT,
      .value = 0U,
  };

  bool queued = app_task_post(&event);
  configASSERT(queued == true);

  BaseType_t deleted = xTimerDelete(timer, 0U);
  configASSERT(deleted == pdPASS);
}

/**
 * @brief Schedule a simulated event after a delay.
 * @param delay_ms Delay in milliseconds before the event is triggered.
 * @param event_id ID of the event to simulate.
 * @return true if the event was successfully scheduled, false otherwise.
 */
bool app_simulate_event(uint32_t delay_ms, app_event_id_enum event_id) {
  TimerHandle_t timer;
  bool scheduled = false;

  timer = xTimerCreate("AppSim", pdMS_TO_TICKS(delay_ms), pdFALSE, (void*)(uintptr_t)event_id, app_sim_timer_callback);

  if (timer != NULL) {
    if (xTimerStart(timer, 0U) == pdPASS) {
      scheduled = true;
    }
    else {
      BaseType_t deleted = xTimerDelete(timer, 0U);
      configASSERT(deleted == pdPASS);
    }
  }

  return scheduled;
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/
