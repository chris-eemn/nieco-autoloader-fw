/**
 * @file app_task.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief FreeRTOS Control task for the auto-loader application state machine.
 * @version 0.1
 * @date 2026-08-07
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC. All Rights Reserved.
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include "app_console.h"
#include "app_task.h"
#include "axis.h"
#include "stepper_system.h"

#include <stddef.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

#define APP_TASK_EVENT_QUEUE_DEPTH (16U)
#define APP_TASK_STACK_DEPTH (512U)
#define APP_TASK_PRIORITY (tskIDLE_PRIORITY + 2U)

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

static QueueHandle_t s_event_queue = NULL;
static app_sm_t s_app_sm;

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

static void app_task_run(void* parameters);
static void on_axis_event(axis_t* axis, axis_event_enum event, void* ctx);
/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

bool app_task_start(void) {
  bool started = false;
  BaseType_t task_result;

  s_event_queue = xQueueCreate(APP_TASK_EVENT_QUEUE_DEPTH, sizeof(app_event_t));
  if (s_event_queue != NULL) {
    task_result = xTaskCreate(app_task_run, "Control", APP_TASK_STACK_DEPTH, NULL, APP_TASK_PRIORITY, NULL);
    if (task_result == pdPASS) {
      started = true;
    }
    else {
      vQueueDelete(s_event_queue);
      s_event_queue = NULL;
    }
  }

  return started;
}

bool app_task_begin(void) {
  const app_event_t start_event = {
      .id = APP_EV_START,
      .slot = APP_NO_SLOT,
      .value = 0U,
  };

  return app_task_post(&start_event);
}

bool app_task_post(const app_event_t* event) {
  bool queued = false;

  if ((event != NULL) && (s_event_queue != NULL)) {
    if (xQueueSend(s_event_queue, event, 0U) == pdPASS) {
      queued = true;
    }
  }

  return queued;
}

bool app_task_post_from_isr(const app_event_t* event) {
  bool queued = false;
  BaseType_t higher_priority_woken = pdFALSE;

  if ((event != NULL) && (s_event_queue != NULL)) {
    if (xQueueSendFromISR(s_event_queue, event, &higher_priority_woken) == pdPASS) {
      queued = true;
    }
    portYIELD_FROM_ISR(higher_priority_woken);
  }

  return queued;
}

void app_task_register_axis_event_cb(void) {
  stepper_system_register_axis_event_cb(on_axis_event, NULL);
}
/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/

/**
 * @brief Receive and dispatch application events forever.
 * @param parameters Unused FreeRTOS task parameter.
 */
static void app_task_run(void* parameters) {
  app_event_t event;

  (void)parameters;
  app_sm_init(&s_app_sm);

  for (;;) {
    if (xQueueReceive(s_event_queue, &event, portMAX_DELAY) == pdPASS) {
      app_sm_dispatch(&s_app_sm, &event);
    }
  }
}

static void on_axis_event(axis_t* axis, axis_event_enum event, void* ctx) {
  (void)ctx;

  app_event_t app_event = {.id = APP_EV_FAULT, .slot = APP_NO_SLOT, .value = 0U, .axis_num = axis->num};

  app_console_print("[Axis Event] Axis %d event %s\r\n", axis->num, axis_event_name(event));

  if (event == AXIS_EVENT_MOVE_FAILED) {
    app_event.id = APP_EV_FAULT; /* TODO: Replace with a more specific fault code. */
  }
  else if (event == AXIS_EVENT_HOME_DONE) {
    app_event.id = APP_EV_MOTION_DONE;
  }
  else if (event == AXIS_EVENT_HOME_ABORTED) {
    app_event.id = APP_EV_FAULT; /* TODO: Replace with a more specific fault code. */
  }
  else if (event == AXIS_EVENT_HOME_FAILED) {
    // todo: temporary since we are faking homing
    axis_clear_fault(axis);
    app_event.id = APP_EV_MOTION_DONE;
  }
  else if (event == AXIS_EVENT_IDLE_FAULT) {
    // todo: figure out why axis faults why idle.
    // todo: clear fault just so we dont lock up
    axis_clear_fault(axis); /* Clear the fault to allow further motion without immediately re-faulting the axis. */
  }
  else {
    app_console_print("[Axis Event] Unhandled axis event %s\r\n", axis_event_name(event));
  }
  (void)app_task_post(&app_event);
}
