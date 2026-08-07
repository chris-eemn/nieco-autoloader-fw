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

#include "app_task.h"

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
