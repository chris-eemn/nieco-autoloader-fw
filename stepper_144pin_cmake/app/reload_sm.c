/**
 * @file reload_sm.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief Auto-loader reload sequence state machine.
 * @version 0.1
 * @date 2026-08-07
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include "reload_sm.h"

#include <stdbool.h>
#include <stddef.h>

#include "app_console.h"
#include "app_sm_port.h"
#include "homing.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/** @brief Pusher homing phase configuration. */
static const homing_phase_cfg_t homing_cfg_pusher = {
    .name = "Pusher",
    .target = HOMING_TARGET_PUSHER,
    .next_state = RELOAD_HOME_LIFTS,
    .fault_code = APP_FAULT_CODE_RELOAD_FAILED,
    .success_log = "[Reload SM] Pushers homed\r\n",
    .timeout_log = "[Reload SM] Pushers home timeout\r\n",
};

/** @brief Lifter-down homing phase configuration. */
static const homing_phase_cfg_t homing_cfg_lifter_down = {
    .name = "Lift",
    .target = HOMING_TARGET_LIFTER_DOWN,
    .next_state = RELOAD_UNLOCK_DOOR,
    .fault_code = APP_FAULT_CODE_RELOAD_FAILED,
    .success_log = "[Reload SM] Lifts homed down\r\n",
    .timeout_log = "[Reload SM] Lift homing down timeout\r\n",
};

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

/**
 * @brief Handle a generic homing-phase event for the reload sequence.
 *
 * Wraps the shared homing handler and applies the reload failure
 * side effects (result status, fault code, and state transition) when
 * the phase reports a failure.
 *
 * @param reload Application state model.
 * @param cartridges Array of cartridges.
 * @param event Event to process.
 * @param result Output result structure (modified on failure).
 * @param cfg Homing phase configuration.
 * @return Outcome indicating whether the phase is waiting, complete, or failed.
 */
static homing_event_result_enum reload_handle_homing_event(reload_sm_t* reload, cartridge_t cartridges[APP_SLOT_COUNT], const app_event_t* event,
                                                           reload_result_t* result, const homing_phase_cfg_t* cfg);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

void reload_sm_start(reload_sm_t* sm, cartridge_t cartridges[APP_SLOT_COUNT]) {
  if (sm != NULL) {
    sm->state = RELOAD_HOME_PUSHERS;
    sm->lift_homing_end_time_ms = 0U;
    sm->lift_homing_timeout_ms = 0U;

    for (uint8_t i = 0U; i < APP_SLOT_COUNT; i++) {
      app_sm_port_home_pusher(&cartridges[i]);
    }
    app_sm_port_arm_timeout(APP_SM_TIMEOUT_MOTION, 10000);
  }
}

reload_result_t reload_sm_dispatch(reload_sm_t* reload, cartridge_t cartridges[APP_SLOT_COUNT], const app_event_t* event) {
  reload_result_t result = {
      .status = RELOAD_STATUS_IN_PROGRESS,
      .fault_code = 0U,
  };

  if ((reload == NULL) || (cartridges == NULL) || (event == NULL)) {
    result.status = RELOAD_STATUS_FAILED;
  }
  else {
    switch (reload->state) {
      case RELOAD_HOME_PUSHERS: {
        if (reload_handle_homing_event(reload, cartridges, event, &result, &homing_cfg_pusher) != HOMING_EVENT_COMPLETE) {
          break;
        }
        // All homed — start lift-down homing inline
        app_console_print(homing_cfg_pusher.success_log);
        reload->state = (reload_state_enum)homing_cfg_pusher.next_state;
        app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_MOTION);
        for (uint8_t slot = 0U; slot < APP_SLOT_COUNT; slot++) {
          app_sm_port_home_lift(&cartridges[slot], DIR_LIFTER_DOWN);
        }
        app_sm_port_arm_timeout(APP_SM_TIMEOUT_MOTION, 1000);
        break;
      }

      case RELOAD_HOME_LIFTS: {
        if (reload_handle_homing_event(reload, cartridges, event, &result, &homing_cfg_lifter_down) != HOMING_EVENT_COMPLETE) {
          break;
        }
        // All homed — unlock the door
        app_console_print(homing_cfg_lifter_down.success_log);
        reload->state = (reload_state_enum)homing_cfg_lifter_down.next_state;
        app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_MOTION);
        app_sm_port_unlock_door();
        app_sm_port_arm_timeout(APP_SM_TIMEOUT_UNLOCK, 1000);
        break;
      }

      case RELOAD_UNLOCK_DOOR:
        if (event->id == APP_EV_LOCK_RELEASED) {
          (void)app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_UNLOCK);
          reload->state = RELOAD_WAIT_OPEN;
          app_sm_port_arm_timeout(APP_SM_TIMEOUT_UNLOCK, 1000);
        }
        else if ((event->id == APP_EV_TIMEOUT) && (event->value == APP_SM_TIMEOUT_UNLOCK)) {
          app_console_print("[Reload SM] Unlock door timeout\r\n");
          result.status = RELOAD_STATUS_FAILED;
          result.fault_code = APP_FAULT_CODE_RELOAD_FAILED;
          reload->state = RELOAD_FAILED;
        }
        break;

      case RELOAD_WAIT_OPEN:
        if (event->id == APP_EV_DOOR_OPENED) {
          (void)app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_UNLOCK);
          reload->state = RELOAD_WAIT_CLOSE;
          app_sm_port_arm_timeout(APP_SM_TIMEOUT_UNLOCK, 1000);
        }
        else if ((event->id == APP_EV_TIMEOUT) && (event->value == APP_SM_TIMEOUT_UNLOCK)) {
          app_console_print("[Reload SM] Wait door open timeout\r\n");
          result.status = RELOAD_STATUS_FAILED;
          result.fault_code = APP_FAULT_CODE_RELOAD_FAILED;
          reload->state = RELOAD_FAILED;
        }
        break;

      case RELOAD_WAIT_CLOSE:
        if (event->id == APP_EV_DOOR_CLOSED) {
          (void)app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_UNLOCK);
          reload->state = RELOAD_LOCK_DOOR;
          app_sm_port_lock_door();
          app_sm_port_arm_timeout(APP_SM_TIMEOUT_LOCK, 1000);
        }
        else if ((event->id == APP_EV_TIMEOUT) && (event->value == APP_SM_TIMEOUT_UNLOCK)) {
          app_console_print("[Reload SM] Wait door close timeout\r\n");
          result.status = RELOAD_STATUS_FAILED;
          result.fault_code = APP_FAULT_CODE_RELOAD_FAILED;
          reload->state = RELOAD_FAILED;
        }
        break;

      case RELOAD_LOCK_DOOR:
        if (event->id == APP_EV_LOCK_CONFIRMED) {
          (void)app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_LOCK);
          reload->state = RELOAD_RECOUNT;
          // app_sm_port_count_cartridges();
          app_sm_port_arm_timeout(APP_SM_TIMEOUT_MOTION, 1000);
        }
        else if ((event->id == APP_EV_TIMEOUT) && (event->value == APP_SM_TIMEOUT_LOCK)) {
          app_console_print("[Reload SM] Lock door timeout\r\n");
          result.status = RELOAD_STATUS_FAILED;
          result.fault_code = APP_FAULT_CODE_RELOAD_FAILED;
          reload->state = RELOAD_FAILED;
        }
        break;

      case RELOAD_RECOUNT:
        if (event->id == APP_EV_COUNT_DONE) {
          app_sm_port_cancel_timeout();
          reload->state = RELOAD_COMPLETE;
          result.status = RELOAD_STATUS_DONE;
        }
        else if ((event->id == APP_EV_TIMEOUT) && (event->value == APP_SM_TIMEOUT_MOTION)) {
          app_console_print("[Reload SM] Count cartridges timeout\r\n");
          result.status = RELOAD_STATUS_FAILED;
          result.fault_code = APP_FAULT_CODE_RELOAD_FAILED;
          reload->state = RELOAD_FAILED;
        }
        break;

      case RELOAD_COMPLETE:
        result.status = RELOAD_STATUS_DONE;
        break;

      case RELOAD_FAILED:
      default:
        result.status = RELOAD_STATUS_FAILED;
        break;
    }
  }

  return result;
}

void reload_sm_abort(reload_sm_t* reload) {
  if (reload != NULL) {
    app_sm_port_cancel_timeout();
    reload->state = RELOAD_FAILED;
    reload->lift_homing_end_time_ms = 0U;
    reload->lift_homing_timeout_ms = 0U;
  }
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/

static homing_event_result_enum reload_handle_homing_event(reload_sm_t* reload, cartridge_t cartridges[APP_SLOT_COUNT], const app_event_t* event,
                                                           reload_result_t* result, const homing_phase_cfg_t* cfg) {
  uint32_t fault_code = APP_FAULT_CODE_NONE;

  homing_event_result_enum outcome = homing_handle_event(cartridges, event, &fault_code, cfg);

  if (outcome == HOMING_EVENT_FAILED) {
    result->status = RELOAD_STATUS_FAILED;
    result->fault_code = fault_code;
    reload->state = RELOAD_FAILED;
  }

  return outcome;
}
