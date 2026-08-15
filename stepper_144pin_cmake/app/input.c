/**
 * @file input.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief FreeRTOS input task for door debounce and periodic input polling.
 * @version 0.1
 * @date 2026-08-08
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC. All Rights Reserved.
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include "input.h"
#include "app_task.h"
#include "autoloader_types.h"

#include <stddef.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "stm32_hal.h"
#include "mx_hal_def.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

#define DOOR_DEBOUNCE_MS (50U)

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/* Binary semaphore: the door EXTI ISR gives this to wake the input task. */
static SemaphoreHandle_t s_door_exti_sem = NULL;

/* Door pin state — updated by the ISR, read by the task thread. */
static volatile uint8_t s_door_last_raw = 0U;
static volatile uint32_t s_door_last_change_tick = 0U;

/* Lock pin state — read by the task thread on every poll. */
static volatile uint8_t s_lock_last_raw = 0U;
static volatile uint32_t s_lock_last_change_tick = 0U;

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

static void input_door_debounce(void);
static void input_poll_lock_pin(void);
static void input_poll_inputs(void);
static void input_door_exti_cb(hal_exti_handle_t* hexti, hal_exti_trigger_t trigger);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

/**
 * @brief Initialise the input module.
 *
 *        Creates the binary semaphore used by the door EXTI ISR to wake the
 *        input task.
 */
void input_init(void) {
  s_door_exti_sem = xSemaphoreCreateBinary();
  configASSERT(s_door_exti_sem != NULL);
  xTaskCreate(input_task_run, "Input", INPUT_TASK_STACK_DEPTH, NULL, INPUT_TASK_PRIORITY, NULL);
}

/**
 * @brief Register the door EXTI callback and initialise debounce state.
 */
void input_register_exti_cb(void) {
  hal_exti_handle_t* door_exti = door_exti_gethandle();

  if (door_exti == NULL) {
    return;
  }

  /* Initialise debounce state by sampling the pin. */
  s_door_last_raw = HAL_GPIO_ReadPin(DOOR_SW_PORT, DOOR_SW_PIN);
  s_door_last_change_tick = 0U;

  HAL_EXTI_SetUserData(door_exti, (const void*)1); /* non-NULL = registered */

  configASSERT(HAL_EXTI_RegisterTriggerCallback(door_exti, input_door_exti_cb) == HAL_OK);
}

/**
 * @brief FreeRTOS input task entry point.
 * @param parameters Unused FreeRTOS task parameter.
 */
void input_task_run(void* parameters) {
  (void)parameters;

  for (;;) {
    /* Wait for door EXTI notification or periodic poll timeout. */
    (void)xSemaphoreTake(s_door_exti_sem, pdMS_TO_TICKS(INPUT_POLL_PERIOD_MS));

    /* Service door debounce regardless of wake source. */
    input_door_debounce();

    /* Poll additional inputs (placeholder for future expansion). */
    input_poll_inputs();
  }
}

/**
 * @brief EXTI callback registered with the HAL.
 *
 *        Thin wrapper that forwards to input_door_exti_callback_from_isr().
 * @param hexti EXTI handle (unused).
 * @param trigger Trigger edge (unused — both edges enabled).
 */
static void input_door_exti_cb(hal_exti_handle_t* hexti, hal_exti_trigger_t trigger) {
  (void)hexti;
  (void)trigger;

  input_door_exti_callback_from_isr();
}

/**
 * @brief ISR-safe entry point called from the door EXTI handler.
 *
 *        Reads the door pin, records the tick of the last edge, and gives
 *        the binary semaphore to wake the input task.
 */
void input_door_exti_callback_from_isr(void) {
  BaseType_t higher_priority_woken = pdFALSE;

  s_door_last_raw = HAL_GPIO_ReadPin(DOOR_SW_PORT, DOOR_SW_PIN);
  s_door_last_change_tick = xTaskGetTickCountFromISR();
  xSemaphoreGiveFromISR(s_door_exti_sem, &higher_priority_woken);
  portYIELD_FROM_ISR(higher_priority_woken);
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/

/**
 * @brief Debounce the door pin and post events to the app queue.
 *
 *        If DOOR_DEBOUNCE_MS have elapsed since the last EXTI edge and the
 *        pin state has changed, an APP_EV_DOOR_OPENED or APP_EV_DOOR_CLOSED
 *        event is posted to the application event queue.
 */
static void input_door_debounce(void) {
  TickType_t elapsed;
  uint8_t raw;
  app_event_t door_event;

  elapsed = xTaskGetTickCount() - s_door_last_change_tick;

  if (elapsed < pdMS_TO_TICKS(DOOR_DEBOUNCE_MS)) {
    return; /* Still debouncing */
  }

  raw = HAL_GPIO_ReadPin(DOOR_SW_PORT, DOOR_SW_PIN);

  if (raw == s_door_last_raw) {
    return; /* No state change */
  }

  s_door_last_raw = raw;

  door_event.id = (raw != 0U) ? APP_EV_DOOR_OPENED : APP_EV_DOOR_CLOSED;
  door_event.slot = APP_NO_SLOT;
  door_event.value = 0U;
  door_event.axis_num = 0U;

  app_task_post(&door_event);
}

/**
 * @brief Placeholder for periodic polling of additional inputs.
 *
 *        Called on every wake of the input task.  No inputs are implemented
 *        yet — future additions go here.
 */
static void input_poll_lock_pin(void) {
  TickType_t elapsed;
  uint8_t raw;
  app_event_t lock_event;

  elapsed = xTaskGetTickCount() - s_lock_last_change_tick;

  if (elapsed < pdMS_TO_TICKS(DOOR_DEBOUNCE_MS)) {
    return; /* Still debouncing */
  }

  raw = HAL_GPIO_ReadPin(DOOR_LOCK_DETECT_SW_PORT, DOOR_LOCK_DETECT_SW_PIN);

  if (raw == s_lock_last_raw) {
    return; /* No state change */
  }

  s_lock_last_raw = raw;

  lock_event.id = (raw != 0U) ? APP_EV_LOCK_CONFIRMED : APP_EV_LOCK_RELEASED;
  lock_event.slot = APP_NO_SLOT;
  lock_event.value = 0U;
  lock_event.axis_num = 0U;

  app_task_post(&lock_event);
}

static void input_poll_inputs(void) {
  input_poll_lock_pin();
}
