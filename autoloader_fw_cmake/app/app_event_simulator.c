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
#define APP_SIM_MAX_PENDING 4U

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/
typedef struct {
  TimerHandle_t handle;
  app_event_id_enum event_id;
  volatile bool armed;
} app_sim_slot_t;

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/
static app_sim_slot_t slots[APP_SIM_MAX_PENDING];

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/
static void app_sim_timer_callback(TimerHandle_t handle);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

/**
 * @brief Schedule a simulated event after a delay.
 * @param delay_ms Delay in milliseconds before the event is triggered.
 * @param event_id ID of the event to simulate.
 * @return true if the event was successfully scheduled, false otherwise.
 */
bool app_simulate_event(uint32_t delay_ms, app_event_id_enum event_id) {
  bool scheduled = false;
  TickType_t period = pdMS_TO_TICKS(delay_ms);

  if (period == 0U) {
    period = 1U;
  }

  for (uint32_t i = 0U; (i < APP_SIM_MAX_PENDING) && (scheduled == false); i++) {
    app_sim_slot_t* slot = &slots[i];
    bool claimed;

    taskENTER_CRITICAL();
    claimed = (slot->armed == false);
    if (claimed) {
      slot->armed = true;
    }
    taskEXIT_CRITICAL();

    if (claimed) {
      slot->event_id = event_id;

      if (slot->handle == NULL) {
        slot->handle = xTimerCreate("AppSim", period, pdFALSE, slot, app_sim_timer_callback);
      }

      if ((slot->handle != NULL) && (xTimerChangePeriod(slot->handle, period, 0U) == pdPASS)) {
        scheduled = true;
      }
      else {
        taskENTER_CRITICAL();
        slot->armed = false;
        taskEXIT_CRITICAL();
      }
    }
  }

  return scheduled;
}

/**
 * @brief Cancel a pending simulated event before it expires.
 * @param event_id ID of the event to cancel.
 * @return true if a pending event was cancelled, false otherwise.
 */
bool app_cancel_simulated_event(app_event_id_enum event_id) {
  bool cancelled = false;

  for (uint32_t i = 0U; i < APP_SIM_MAX_PENDING; i++) {
    app_sim_slot_t* slot = &slots[i];
    bool mine;

    taskENTER_CRITICAL();
    mine = (slot->armed == true) && (slot->event_id == event_id);
    if (mine) {
      slot->armed = false;
    }
    taskEXIT_CRITICAL();

    if (mine) {
      (void)xTimerStop(slot->handle, 0U);
      cancelled = true;
    }
  }

  return cancelled;
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/

/**
 * @brief Simulated event timer callback.
 */
static void app_sim_timer_callback(TimerHandle_t handle) {
  app_sim_slot_t* slot = (app_sim_slot_t*)pvTimerGetTimerID(handle);
  bool fire;

  taskENTER_CRITICAL();
  fire = slot->armed;
  slot->armed = false;
  taskEXIT_CRITICAL();

  if (fire) {
    app_event_t event = {
        .id = slot->event_id,
        .slot = APP_NO_SLOT,
        .value = 0U,
    };

    bool queued = app_task_post(&event);
    configASSERT(queued == true);
  }
}
