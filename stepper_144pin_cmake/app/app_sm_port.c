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
  app_sm_timeout_id_enum timeout;
  volatile bool armed;
} timeout_slot_t;

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/
static timeout_slot_t timeout_slots[MAX_PENDING_TIMER_EVENTS];

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/
static void timer_cb(TimerHandle_t timer);
static bool cancel_slot(timeout_slot_t* slot, bool match_id, app_sm_timeout_id_enum timeout);
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
#define FAKE_PUSH_EXTEND_COUNTS (3000U)
#define FAKE_PUSH_EXTEND_RPM 15
#define FAKE_PUSH_EXTEND_DIR STEPPER_DIR_CCW

  uint8_t axis_num = cartridge_get_axis_num(slot, PUSHER);
  stepper_status_enum status = axis_move(stepper_ctrl_get_axis(axis_num), FAKE_PUSH_EXTEND_COUNTS, FAKE_PUSH_EXTEND_RPM, FAKE_PUSH_EXTEND_DIR);
  if (status != STEPPER_OK) {
    app_console_print("[app_sm_port] Failed to extend pusher for slot %d: %d\r\n", slot->num, (int)status);
  }
}

void app_sm_port_push_retract(cartridge_t* slot) {
#define FAKE_PUSH_RETRACT_COUNTS (3000U)
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

void app_sm_port_halt_all_motion(void) {
  /* TODO: Stop all axes after the final motor-to-slot map is available. */
}

void app_sm_port_save_state(void) {
  /* TODO: Queue the required nonvolatile records through the existing W25Q stack. */
}

bool app_sm_port_arm_timeout(app_sm_timeout_id_enum timeout, uint32_t delay_ms) {
  TickType_t period = pdMS_TO_TICKS(delay_ms);
  bool armed = false;

  if (period == 0U) {
    period = 1U;
  }

  for (uint32_t i = 0U; (i < MAX_PENDING_TIMER_EVENTS) && (armed == false); i++) {
    timeout_slot_t* slot = &timeout_slots[i];
    bool claimed;

    taskENTER_CRITICAL();
    claimed = (slot->armed == false);
    if (claimed) {
      slot->armed = true;
    }
    taskEXIT_CRITICAL();

    if (claimed) {
      slot->timeout = timeout;

      if (slot->handle == NULL) {
        slot->handle = xTimerCreate("AppSMTimeout", period, pdFALSE, slot, timer_cb);
      }

      /* Sets the period and starts the timer; xTimerStart alone would reuse the previous period. */
      if ((slot->handle != NULL) && (xTimerChangePeriod(slot->handle, period, 0U) == pdPASS)) {
        armed = true;
      }
      else {
        taskENTER_CRITICAL();
        slot->armed = false;
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
    (void)cancel_slot(&timeout_slots[i], false, (app_sm_timeout_id_enum)0);
  }
}

bool app_sm_port_cancel_timeout_id(app_sm_timeout_id_enum timeout) {
  bool cancelled = false;

  for (uint32_t i = 0U; i < MAX_PENDING_TIMER_EVENTS; i++) {
    if (cancel_slot(&timeout_slots[i], true, timeout)) {
      cancelled = true;
    }
  }

  return cancelled;
}

void app_sm_port_publish_state(const app_sm_t* sm) {
  if (sm != NULL) {
    /* TODO: Copy the fields required by the Modbus status registers. */
  }
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/

static void timer_cb(TimerHandle_t timer) {
  timeout_slot_t* slot = (timeout_slot_t*)pvTimerGetTimerID(timer);
  bool fire;

  taskENTER_CRITICAL();
  fire = slot->armed;
  slot->armed = false;
  taskEXIT_CRITICAL();

  if (fire) {
    app_event_t event = {
        .id = APP_EV_TIMEOUT,
        .slot = APP_NO_SLOT,
        .value = (uint32_t)slot->timeout,
    };

    bool queued = app_task_post(&event);
    configASSERT(queued == true);
  }
}

/**
 * @brief Clear a slot's armed flag and stop its timer.
 * @param slot Slot to cancel.
 * @param match_id When true, cancel only if the slot carries the given timeout ID.
 * @param timeout Timeout ID to match when match_id is true.
 * @return true if the slot was armed and is now cancelled.
 */
static bool cancel_slot(timeout_slot_t* slot, bool match_id, app_sm_timeout_id_enum timeout) {
  bool cancel;

  /* Clearing the flag suppresses the event; the stop below is best effort. */
  taskENTER_CRITICAL();
  cancel = (slot->armed == true) && ((match_id == false) || (slot->timeout == timeout));
  if (cancel) {
    slot->armed = false;
  }
  taskEXIT_CRITICAL();

  if (cancel) {
    (void)xTimerStop(slot->handle, 0U);
  }

  return cancel;
}
