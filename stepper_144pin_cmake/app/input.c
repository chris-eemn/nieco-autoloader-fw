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
#include "cal_data.h"
#include "app_console.h"

#include <stddef.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "stm32_hal.h"
#include "mx_hal_def.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/* Debounce window in milliseconds, latched from cal_data in input_init() before the
 * input task is created -- the zero init is never read. Requires cal_data_init() to
 * have run first (the bringup task orders them); a reordered boot would latch a zero
 * debounce window. Debounce timing is poll-sensitive, so the value is sampled at
 * boot: a runtime change to cal_data.door_debounce_ms takes effect on the next boot.
 * cal_data is the single source of truth -- cal_data_init() provisions the factory
 * defaults whenever flash holds nothing usable, and repairs a zero door_debounce_ms
 * to its default. */
static uint32_t s_door_debounce_ms;

/* Binary semaphore: the door EXTI ISR gives this to wake the input task. */
static SemaphoreHandle_t s_door_exti_sem = NULL;

/* Door pin state — updated by the ISR, read by the task thread. */
static volatile uint8_t s_door_last_raw = 0U;
static volatile uint32_t s_door_last_change_tick = 0U;

/* Lock pin state — read by the task thread on every poll. */
static volatile uint8_t s_lock_last_raw = 0U;
static volatile uint32_t s_lock_last_change_tick = 0U;

/* Reload-switch state — read by the task thread on every poll. */
static volatile uint8_t s_reload_last_raw = 1U;
static volatile uint32_t s_reload_last_change_tick = 0U;

/* Shutdown-switch state — read by the task thread on every poll. */
static volatile uint8_t s_shutdown_last_raw = 1U;
static volatile uint32_t s_shutdown_last_change_tick = 0U;

/* Lock-control output state — last commanded lock state (false = unlocked). */
static volatile bool s_lock_cmd_locked = false;

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

static void input_door_debounce(void);
static void input_poll_lock_pin(void);
static void input_poll_reload_pin(void);
static void input_poll_shutdown_pin(void);
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

  /* Latch the debounce window from cal_data once, at boot. cal_data_init()
   * runs before input_init() in the bringup task, so the factory defaults are
   * already provisioned. Debounce timing is poll-sensitive, so a runtime
   * change to cal_data.door_debounce_ms only takes effect on the next boot. */
  s_door_debounce_ms = cal_data_get()->door_debounce_ms;

  /* Seed the polled-pin debounce state from the actual levels so the first
   * poll is not treated as an edge against the zero-initialised defaults.
   * Without this, a lock detect reading high at boot posts a spurious
   * APP_EV_LOCK_CONFIRMED, and a released reload switch (high) makes the
   * first press match the stale default and post a spurious
   * APP_EV_RELOAD_REQUEST. */
  s_lock_last_raw = HAL_GPIO_ReadPin(DOOR_LOCK_DETECT_SW_PORT, DOOR_LOCK_DETECT_SW_PIN);
  s_lock_last_change_tick = 0U;
  s_reload_last_raw = HAL_GPIO_ReadPin(RELOAD_SW_PORT, RELOAD_SW_PIN);
  s_reload_last_change_tick = 0U;
  s_shutdown_last_raw = HAL_GPIO_ReadPin(SHUTDOWN_SW_PORT, SHUTDOWN_SW_PIN);
  s_shutdown_last_change_tick = 0U;

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
 *        If the cal_data debounce window have elapsed since the last EXTI edge and the
 *        pin state has changed, an APP_EV_DOOR_OPENED or APP_EV_DOOR_CLOSED
 *        event is posted to the application event queue.
 */
static void input_door_debounce(void) {
  TickType_t elapsed;
  uint8_t raw;
  app_event_t door_event;

  elapsed = xTaskGetTickCount() - s_door_last_change_tick;

  if (elapsed < pdMS_TO_TICKS(s_door_debounce_ms)) {
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

  if (elapsed < pdMS_TO_TICKS(s_door_debounce_ms)) {
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

/**
 * @brief Poll and debounce the active-low reload switch.
 *
 *        Posts APP_EV_RELOAD_REQUEST when the switch transitions from
 *        inactive to active.  The release transition updates the stored
 *        state without posting an event.
 */
static void input_poll_reload_pin(void) {
  TickType_t elapsed;
  uint8_t raw;
  app_event_t reload_event;

  elapsed = xTaskGetTickCount() - s_reload_last_change_tick;

  if (elapsed < pdMS_TO_TICKS(s_door_debounce_ms)) {
    return; /* Still debouncing */
  }

  raw = HAL_GPIO_ReadPin(RELOAD_SW_PORT, RELOAD_SW_PIN);

  if (raw == s_reload_last_raw) {
    return; /* No state change */
  }

  s_reload_last_raw = raw;
  s_reload_last_change_tick = xTaskGetTickCount();

  if (raw == 0U) {
    reload_event.id = APP_EV_RELOAD_REQUEST;
    reload_event.slot = APP_NO_SLOT;
    reload_event.value = 0U;
    reload_event.axis_num = 0U;

    app_task_post(&reload_event);
  }
}

/**
 * @brief Poll and debounce the active-low shutdown switch.
 *
 *        Posts APP_EV_SHUTDOWN_REQUEST when the switch transitions from
 *        inactive to active, and APP_EV_SHUTDOWN_END on the release edge.
 */
static void input_poll_shutdown_pin(void) {
  TickType_t elapsed;
  uint8_t raw;
  app_event_t shutdown_event;

  elapsed = xTaskGetTickCount() - s_shutdown_last_change_tick;

  if (elapsed < pdMS_TO_TICKS(s_door_debounce_ms)) {
    return; /* Still debouncing */
  }

  raw = HAL_GPIO_ReadPin(SHUTDOWN_SW_PORT, SHUTDOWN_SW_PIN);

  if (raw == s_shutdown_last_raw) {
    return; /* No state change */
  }

  s_shutdown_last_raw = raw;
  s_shutdown_last_change_tick = xTaskGetTickCount();

  if (raw == 0U) {
    shutdown_event.id = APP_EV_SHUTDOWN_REQUEST;
    shutdown_event.slot = APP_NO_SLOT;
    shutdown_event.value = 0U;
    shutdown_event.axis_num = 0U;

    app_task_post(&shutdown_event);
  }
  else {
    shutdown_event.id = APP_EV_SHUTDOWN_END;
    shutdown_event.slot = APP_NO_SLOT;
    shutdown_event.value = 0U;
    shutdown_event.axis_num = 0U;

    app_task_post(&shutdown_event);
  }
}

static void input_poll_inputs(void) {
  input_poll_lock_pin();
  input_poll_reload_pin();
  input_poll_shutdown_pin();
}

/**
 * @brief Get the current door state.
 * @return true if the door is currently closed; otherwise false.
 */
bool input_get_door_closed(void) {
  return (s_door_last_raw == 0U);
}

/**
 * @brief Get the current lock state.
 * @return true if the lock is currently confirmed; otherwise false.
 */
bool input_get_lock_confirmed(void) {
  return (s_lock_last_raw != 0U);
}

/**
 * @brief Drive the door lock-control output high to lock the door.
 */
void input_lock_door(void) {
  HAL_GPIO_WritePin(DOOR_LOCK_CTRL_PORT, DOOR_LOCK_CTRL_PIN, DOOR_LOCK_CTRL_ACTIVE_STATE);
  s_lock_cmd_locked = true;
}

/**
 * @brief Drive the door lock-control output low to unlock the door.
 */
void input_unlock_door(void) {
  HAL_GPIO_WritePin(DOOR_LOCK_CTRL_PORT, DOOR_LOCK_CTRL_PIN, DOOR_LOCK_CTRL_INACTIVE_STATE);
  s_lock_cmd_locked = false;
}

/**
 * @brief Get the last commanded lock-control state.
 * @return true if the lock-control output is currently commanded high (locked); otherwise false.
 */
bool input_get_lock_cmd_locked(void) {
  return s_lock_cmd_locked;
}

/**
 * @brief Get the current reload-switch state.
 * @return true if reload is currently requested; otherwise false.
 */
bool input_get_reload_requested(void) {
  return (s_reload_last_raw == 0U);
}

/**
 * @brief Get the current shutdown-switch state.
 * @return true if shutdown is currently requested; otherwise false.
 */
bool input_get_shutdown_requested(void) {
  return (s_shutdown_last_raw == 0U);
}
