/**
 * @file app_sm_port.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief Initial hardware-port stubs for the application state machines.
 * @version 0.1
 * @date 2026-08-07
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC. All Rights Reserved.
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include "axis.h"
#include "app_sm_port.h"
#include "app_console.h"
#include "app_task.h"
#include "cartridge.h"
#include "FreeRTOS.h"
#include "stepper.h"
#include "stepper_ctrl.h"
#include "timers.h"

#include <stddef.h>

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
#define MAX_PENDING_TIMER_EVENTS (4U)

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/
typedef struct {
  TimerHandle_t handle;
  app_sm_timeout_id_enum timeout_id;
  volatile bool armed;
} timeout_timer_t;

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/
static timeout_timer_t timeout_timers[MAX_PENDING_TIMER_EVENTS];
static const char* const app_sm_timeout_id_names[] = {
    [APP_SM_TIMEOUT_NONE] = "APP_SM_TIMEOUT_NONE",
    [APP_SM_TIMEOUT_LOCK] = "APP_SM_TIMEOUT_LOCK",
    [APP_SM_TIMEOUT_UNLOCK] = "APP_SM_TIMEOUT_UNLOCK",
    [APP_SM_TIMEOUT_STARTUP_PUSHER_HOME] = "APP_SM_TIMEOUT_STARTUP_PUSHER_HOME",
    [APP_SM_TIMEOUT_STARTUP_LIFTER_HOME] = "APP_SM_TIMEOUT_STARTUP_LIFTER_HOME",
    [APP_SM_TIMEOUT_STARTUP_DELAY] = "APP_SM_TIMEOUT_STARTUP_DELAY",
    [APP_SM_TIMEOUT_MOTION] = "APP_SM_TIMEOUT_MOTION",
    [APP_SM_CART1_DISPENSE] = "APP_SM_CART1_DISPENSE",
    [APP_SM_CART2_DISPENSE] = "APP_SM_CART2_DISPENSE",
    [APP_SM_CART3_DISPENSE] = "APP_SM_CART3_DISPENSE",
    [APP_SM_CART4_DISPENSE] = "APP_SM_CART4_DISPENSE",
    [APP_SM_TIMEOUT_DOOR] = "APP_SM_TIMEOUT_DOOR",
};
/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/
static void timer_cb(TimerHandle_t timer);
static bool cancel_timer(timeout_timer_t* slot, bool match_id, app_sm_timeout_id_enum timeout);
static uint8_t determine_slot_from_timer_id(app_sm_timeout_id_enum timeout_id);
/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

void app_sm_port_lock_door(void) {
  /* TODO: Drive the door-lock output. */
}

void app_sm_port_unlock_door(void) {
  /* TODO: Release the door-lock output. */
}

void app_sm_port_home_pusher(cartridge_t* slot) {
  axis_home(stepper_ctrl_get_axis(cartridge_get_axis_num(slot, PUSHER)), STEPPER_DIR_CW);
}

void app_sm_port_home_lift(cartridge_t* slot, cartridge_direction_t direction) {
  if (direction == DIR_LIFTER_DOWN) {
    axis_home(stepper_ctrl_get_axis(cartridge_get_axis_num(slot, LIFTER)), STEPPER_DIR_CW);
  }
  else if (direction == DIR_LIFTER_UP) {
    axis_home(stepper_ctrl_get_axis(cartridge_get_axis_num(slot, LIFTER)), STEPPER_DIR_CCW);
  }
  else {
    app_console_print("[app_sm_port] Invalid direction for lift homing\r\n");
  }
}

void app_sm_port_count_cartridges(cartridge_t* slot) {
  slot->lifter_home_up_encoder_counts = axis_get_home_travel_counts(stepper_ctrl_get_axis(cartridge_get_axis_num(slot, LIFTER)));
}

void app_sm_port_push_extend(cartridge_t* slot) {
#define FAKE_PUSH_EXTEND_COUNTS (1000U)
#define FAKE_PUSH_EXTEND_RPM 15
#define FAKE_PUSH_EXTEND_DIR STEPPER_DIR_CCW

  uint8_t axis_num = cartridge_get_axis_num(slot, PUSHER);
  stepper_status_enum status = axis_move(stepper_ctrl_get_axis(axis_num), FAKE_PUSH_EXTEND_COUNTS, FAKE_PUSH_EXTEND_RPM, FAKE_PUSH_EXTEND_DIR);
  if (status != STEPPER_OK) {
    app_console_print("[app_sm_port] Failed to extend pusher for slot %d: %d\r\n", slot->num, (int)status);
  }
}

void app_sm_port_push_retract(cartridge_t* slot) {
#define FAKE_PUSH_RETRACT_COUNTS (1000U)
#define FAKE_PUSH_RETRACT_RPM 20
#define FAKE_PUSH_RETRACT_DIR STEPPER_DIR_CW
  uint8_t axis_num = cartridge_get_axis_num(slot, PUSHER);
  stepper_status_enum status = axis_move(stepper_ctrl_get_axis(axis_num), FAKE_PUSH_RETRACT_COUNTS, FAKE_PUSH_RETRACT_RPM, FAKE_PUSH_RETRACT_DIR);
  if (status != STEPPER_OK) {
    app_console_print("[app_sm_port] Failed to retract pusher for slot %d: %d\r\n", slot->num, (int)status);
  }
}

void app_sm_port_lift_seek(cartridge_t* slot) {
  (void)slot;
  /* TODO: Map the slot to its lift axis and post APP_EV_STALL_DETECTED on expected stall. */
}

void app_sm_port_lift_backoff(cartridge_t* slot) {
  (void)slot;
  /* TODO: Map the slot to its lift axis. */
}

void app_sm_port_commit_dispense(cartridge_t* slot) {
  (void)slot;
  /* TODO: Update remaining and pending counts after confirmed mechanical completion. */
}

void app_sm_port_halt_motion(cartridge_t* slot) {
  axis_stop(stepper_ctrl_get_axis(cartridge_get_axis_num(slot, PUSHER)));
  axis_stop(stepper_ctrl_get_axis(cartridge_get_axis_num(slot, LIFTER)));
}

void app_sm_port_halt_all_motion(void) {
  /* TODO: Stop all axes after the final motor-to-slot map is available. */
}

void app_sm_port_save_state(void) {
  /* TODO: Queue the required nonvolatile records through the existing W25Q stack. */
}

bool app_sm_port_arm_timeout(app_sm_timeout_id_enum timeout_id, uint32_t delay_ms) {
  TickType_t period = pdMS_TO_TICKS(delay_ms);
  bool armed = false;

  if (period == 0U) {
    period = 1U;
  }

  for (uint32_t i = 0U; (i < MAX_PENDING_TIMER_EVENTS) && (armed == false); i++) {
    timeout_timer_t* timer = &timeout_timers[i];
    bool claimed;

    taskENTER_CRITICAL();
    claimed = (timer->armed == false);
    if (claimed) {
      timer->armed = true;
    }
    taskEXIT_CRITICAL();

    if (claimed) {
      timer->timeout_id = timeout_id;

      if (timer->handle == NULL) {
        timer->handle = xTimerCreate("AppSMTimeout", period, pdFALSE, timer, timer_cb);
      }

      /* Sets the period and starts the timer; xTimerStart alone would reuse the previous period. */
      if ((timer->handle != NULL) && (xTimerChangePeriod(timer->handle, period, 0U) == pdPASS)) {
        armed = true;
      }
      else {
        taskENTER_CRITICAL();
        timer->armed = false;
        taskEXIT_CRITICAL();
      }
    }
  }

  if (armed == false) {
    app_console_print("[app_sm_port] Failed to arm timeout timer\r\n");
  }

  return armed;
}

/**
 * @brief Cancel every pending timeout.
 */
void app_sm_port_cancel_timeout(void) {
  for (uint32_t i = 0U; i < MAX_PENDING_TIMER_EVENTS; i++) {
    (void)cancel_timer(&timeout_timers[i], true, (app_sm_timeout_id_enum)0);
  }
}

bool app_sm_port_cancel_timeout_id(app_sm_timeout_id_enum timeout) {
  bool cancelled = false;

  for (uint32_t i = 0U; i < MAX_PENDING_TIMER_EVENTS; i++) {
    if (cancel_timer(&timeout_timers[i], true, timeout)) {
      cancelled = true;
    }
  }

  app_console_print("[app_sm_port] Cancelled timeout ID %s: %s\r\n", app_sm_port_timeout_id_to_str(timeout), cancelled ? "true" : "false");

  return cancelled;
}

void app_sm_port_publish_state(const app_sm_t* sm) {
  if (sm != NULL) {
    /* TODO: Copy the fields required by the Modbus status registers. */
  }
}

const char* app_sm_port_timeout_id_to_str(app_sm_timeout_id_enum timeout_id) {
  if ((size_t)timeout_id >= (sizeof(app_sm_timeout_id_names) / sizeof(app_sm_timeout_id_names[0]))) {
    return "APP_SM_TIMEOUT_UNKNOWN";
  }

  return app_sm_timeout_id_names[timeout_id];
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/

static void timer_cb(TimerHandle_t timer) {
  timeout_timer_t* _timer = (timeout_timer_t*)pvTimerGetTimerID(timer);
  bool fire;

  taskENTER_CRITICAL();
  fire = _timer->armed;
  _timer->armed = false;
  taskEXIT_CRITICAL();

  if (fire) {
    app_event_t event = {
        .id = APP_EV_TIMEOUT,
        .slot = determine_slot_from_timer_id(_timer->timeout_id),
        .value = (uint32_t)_timer->timeout_id,
    };

    bool queued = app_task_post(&event);
    configASSERT(queued == true);
  }
}

/**
 * @brief Clear a timer's armed flag and stop its timer.
 * @param timer Timer to cancel.
 * @param match_id When true, cancel only if the slot carries the given timeout ID.
 * @param timeout Timeout ID to match when match_id is true.
 * @return true if the slot was armed and is now cancelled.
 */
static bool cancel_timer(timeout_timer_t* timer, bool match_id, app_sm_timeout_id_enum timeout) {
  bool cancel;

  /* Clearing the flag suppresses the event; the stop below is best effort. */
  taskENTER_CRITICAL();
  cancel = (timer->armed == true) && ((match_id == false) || (timer->timeout_id == timeout));
  if (cancel) {
    timer->armed = false;
  }
  taskEXIT_CRITICAL();

  if (cancel) {
    (void)xTimerStop(timer->handle, 0U);
  }

  return cancel;
}

/**
 * @brief Determine the cartridge slot associated with a timeout ID.
 * @param timeout_id Timeout identifier to evaluate.
 * @return Slot number (1-4) for cartridge dispense timeouts, or APP_NO_SLOT when not slot-related.
 */
static uint8_t determine_slot_from_timer_id(app_sm_timeout_id_enum timeout_id) {
  uint8_t slot = APP_NO_SLOT;

  switch (timeout_id) {
    case APP_SM_TIMEOUT_NONE:
    case APP_SM_TIMEOUT_LOCK:
    case APP_SM_TIMEOUT_UNLOCK:
    case APP_SM_TIMEOUT_STARTUP_PUSHER_HOME:
    case APP_SM_TIMEOUT_STARTUP_LIFTER_HOME:
    case APP_SM_TIMEOUT_STARTUP_DELAY:
    case APP_SM_TIMEOUT_MOTION:
    case APP_SM_TIMEOUT_DOOR:
    default:
      // do not relate to cartridge/slot
      break;
    case APP_SM_CART1_DISPENSE:
      slot = 1;
      break;
    case APP_SM_CART2_DISPENSE:
      slot = 2;
      break;
    case APP_SM_CART3_DISPENSE:
      slot = 3;
      break;
    case APP_SM_CART4_DISPENSE:
      slot = 4;
      break;
  }
  return slot;
}
