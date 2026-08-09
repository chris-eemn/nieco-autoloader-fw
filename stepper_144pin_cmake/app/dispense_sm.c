/**
 * @file dispense_sm.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief
 * @version 0.1
 * @date 2026-08-08
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights
 * Reserved.
 *
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include "axis.h"
#include "app_console.h"
#include "app_sm_port.h"
#include "patty_handler.h"

#include <stddef.h>

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

#define DISPENSE_FAULT_NONE (0U)
#define DISPENSE_FAULT_TIMEOUT (1U)
#define DISPENSE_FAULT_MOTION (2U)
#define DISPENSE_FAULT_COMMAND_REJECT (3U)
#define DISPENSE_FAULT_TIMER_REJECT (4U)
#define DISPENSE_FAULT_HALT_REJECT (5U)
#define DISPENSE_FAULT_INVALID_STATE (6U)

#define DISPENSE_PUSH_RETRACT_TIMEOUT_MS (3000U)
#define DISPENSE_LIFT_TIMEOUT_MS (5000U)

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

static patty_handler_result_enum dispense_start_motion(dispense_sm_t* dispense_sm, dispense_state_enum next_state);
static uint8_t get_dispense_fault_code_from_event(const app_event_t* event);
static patty_handler_result_enum dispense_fail(dispense_sm_t* dispense_sm, uint16_t fault_code);

static bool start_motion(cartridge_t* cartridge, dispense_state_enum next_state);
static void halt_motion(cartridge_t* cartridge);
static uint32_t get_dispense_timeout_ms_for_state(dispense_state_enum state);
/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

void dispense_sm_init(dispense_sm_t* dispense_sm, cartridge_t* cartridge) {
  if (dispense_sm != NULL) {
    dispense_sm->state = DISPENSE_IDLE;
    dispense_sm->cartridge = cartridge;
    dispense_sm->slot_index = cartridge->num - 1;
    dispense_sm->fault_code = 0U;
    dispense_sm->measured_lift_travel_counts = 0U;
    dispense_sm->product_may_have_dispensed = false;
    switch (dispense_sm->slot_index) {
      case 0:
        dispense_sm->timeout_id = APP_SM_CART1_DISPENSE;
        break;
      case 1:
        dispense_sm->timeout_id = APP_SM_CART2_DISPENSE;
        break;
      case 2:
        dispense_sm->timeout_id = APP_SM_CART3_DISPENSE;
        break;
      case 3:
        dispense_sm->timeout_id = APP_SM_CART4_DISPENSE;
        break;
      default:
        dispense_sm->timeout_id = APP_SM_TIMEOUT_NONE;
        break;
    }
  }
}

patty_handler_result_enum dispense_sm_start(dispense_sm_t* dispense_sm) {
  patty_handler_result_enum result = PATTY_HANDLER_RESULT_INVALID_ARGUMENT;

  if ((dispense_sm != NULL)) {
    if (dispense_sm->state == DISPENSE_IDLE) {
      dispense_sm->fault_code = 0U;
      dispense_sm->measured_lift_travel_counts = 0U;
      dispense_sm->product_may_have_dispensed = false;
      app_console_print("[Dispense SM] Starting dispense for slot %d\r\n", dispense_sm->slot_index + 1U);
      result = dispense_start_motion(dispense_sm, DISPENSE_PUSH_EXTEND);
    }
    else {
      result = PATTY_HANDLER_RESULT_NOT_READY;
    }
  }

  return result;
}

patty_handler_result_enum dispense_sm_dispatch(dispense_sm_t* dispense_sm, const app_event_t* event) {
  patty_handler_result_enum result = PATTY_HANDLER_RESULT_INVALID_ARGUMENT;

  if ((dispense_sm != NULL) && (event != NULL)) {
    result = PATTY_HANDLER_RESULT_NO_ACTION;
    uint8_t slot_zero_based = event->slot - 1U;  // convert to zero-based index

    if ((slot_zero_based == dispense_sm->slot_index) && (dispense_sm->state >= DISPENSE_PUSH_EXTEND) && (dispense_sm->state <= DISPENSE_LIFT_SEEK)) {
      uint8_t fault_code = get_dispense_fault_code_from_event(event);

      if (fault_code != DISPENSE_FAULT_NONE) {
        result = dispense_fail(dispense_sm, fault_code);
      }
      else {
        switch (dispense_sm->state) {
          case DISPENSE_PUSH_EXTEND:
            if (event->id == APP_EV_MOTION_DONE) {
              // todo: implement
              app_sm_port_cancel_timeout_id(dispense_sm->timeout_id);
              dispense_sm->product_may_have_dispensed = true;
              result = dispense_start_motion(dispense_sm, DISPENSE_PUSH_RETRACT);
            }
            break;

          case DISPENSE_PUSH_RETRACT:
            if (event->id == APP_EV_MOTION_DONE) {
              // todo: implement
              app_sm_port_cancel_timeout_id(dispense_sm->timeout_id);
              result = dispense_start_motion(dispense_sm, DISPENSE_LIFT_SEEK);
            }
            break;

          case DISPENSE_LIFT_SEEK:
            if (event->id == APP_EV_MOTION_DONE) {
              // todo: implement
              app_sm_port_cancel_timeout_id(dispense_sm->timeout_id);
              // dispense_sm->measured_lift_travel_counts = event->measured_lift_travel_counts;
              // lift home also will backoff automatically
              dispense_sm->state = DISPENSE_COMPLETE;
              result = PATTY_HANDLER_RESULT_OK;
            }
            break;

          case DISPENSE_IDLE:
          case DISPENSE_COMPLETE:
          case DISPENSE_FAILED:
          default:
            break;
        }
      }
    }
  }

  return result;
}

static const char* const dispense_fault_names[] = {
    [DISPENSE_FAULT_NONE] = "DISPENSE_FAULT_NONE",
    [DISPENSE_FAULT_TIMEOUT] = "DISPENSE_FAULT_TIMEOUT",
    [DISPENSE_FAULT_MOTION] = "DISPENSE_FAULT_MOTION",
    [DISPENSE_FAULT_COMMAND_REJECT] = "DISPENSE_FAULT_COMMAND_REJECT",
    [DISPENSE_FAULT_TIMER_REJECT] = "DISPENSE_FAULT_TIMER_REJECT",
    [DISPENSE_FAULT_HALT_REJECT] = "DISPENSE_FAULT_HALT_REJECT",
    [DISPENSE_FAULT_INVALID_STATE] = "DISPENSE_FAULT_INVALID_STATE",
};
const char* dispense_sm_fault_to_str(uint8_t fault) {
  if ((size_t)fault >= (sizeof(dispense_fault_names) / sizeof(dispense_fault_names[0]))) {
    return "DISPENSE_FAULT_UNKNOWN";
  }

  return dispense_fault_names[fault];
}
/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/

/**
 * @brief Enters a motion state, starts its command, and arms its slot-specific
 * timeout.
 *
 * @param dispense_sm State-machine context.
 * @param next_state State associated with the new motion command.
 * @param motion_function Motion callback to invoke.
 * @return PATTY_HANDLER_RESULT_OK on success or
 * PATTY_HANDLER_RESULT_MOTION_REJECTED on failure.
 */
static patty_handler_result_enum dispense_start_motion(dispense_sm_t* dispense_sm, dispense_state_enum next_state) {
  patty_handler_result_enum result = PATTY_HANDLER_RESULT_MOTION_REJECTED;
  bool command_started;
  bool timer_started;

  dispense_sm->state = next_state;
  command_started = start_motion(dispense_sm->cartridge, next_state);

  if (command_started == true) {
    app_console_print("[Dispense SM] Started motion for slot %d, state=%d\r\n", dispense_sm->slot_index + 1U, next_state);
    uint32_t timeout_ms = get_dispense_timeout_ms_for_state(next_state);
    timer_started = app_sm_port_arm_timeout(dispense_sm->timeout_id, timeout_ms);
    if (timer_started == true) {
      result = PATTY_HANDLER_RESULT_OK;
    }
    else {
      result = dispense_fail(dispense_sm, DISPENSE_FAULT_TIMER_REJECT);
    }
  }
  else {
    result = dispense_fail(dispense_sm, DISPENSE_FAULT_COMMAND_REJECT);
  }

  return result;
}

/**
 * @brief Stops only this cartridge and records a terminal dispense failure.
 *
 * Other cartridge state machines are intentionally not changed.
 *
 * @param dispense_sm State-machine context.
 * @param fault_code Fault code to save for application diagnostics.
 * @return PATTY_HANDLER_RESULT_MOTION_REJECTED.
 */
static patty_handler_result_enum dispense_fail(dispense_sm_t* dispense_sm, uint16_t fault_code) {
  app_sm_port_cancel_timeout_id(dispense_sm->timeout_id);
  app_console_print("[Dispense SM] Halting motion for slot %d due to fault code (%d) %s\r\n", dispense_sm->slot_index + 1U, fault_code,
                    dispense_sm_fault_to_str(fault_code));
  halt_motion(dispense_sm->cartridge);
  dispense_sm->fault_code = fault_code;

  dispense_sm->state = DISPENSE_FAILED;
  app_console_print("[Dispense SM] Dispense failed for slot %d, state=%d, fault_code=(%d) %s\r\n", dispense_sm->slot_index + 1U, dispense_sm->state,
                    dispense_sm->fault_code, dispense_sm_fault_to_str(dispense_sm->fault_code));

  return PATTY_HANDLER_RESULT_MOTION_REJECTED;
}

// todo: placeholder so i can get the project to build
static bool start_motion(cartridge_t* cartridge, dispense_state_enum next_state) {
  (void)cartridge;
  (void)next_state;
  app_console_print("[Dispense SM] Starting motion for slot %d, state=%d.\r\n", cartridge->num, next_state);

  switch (next_state) {
    case DISPENSE_PUSH_EXTEND:
      app_console_print("[Dispense SM] Starting push extend for slot %d\r\n", cartridge->num);
      app_sm_port_push_extend(cartridge);
      break;
    case DISPENSE_PUSH_RETRACT:
      app_console_print("[Dispense SM] Starting push retract for slot %d\r\n", cartridge->num);
      app_sm_port_push_retract(cartridge);
      break;
    case DISPENSE_LIFT_SEEK:
      // determine axis number from slot number
      app_console_print("[Dispense SM] Starting lift seek for slot %d\r\n", cartridge->num);
      app_sm_port_home_lift(cartridge, DIR_LIFTER_UP);
      break;
    default:
      break;
  }

  return true;
}

static void halt_motion(cartridge_t* cartridge) {
  app_sm_port_halt_motion(cartridge);
}
// axis event ids will be placed in the value of an app_event
static uint8_t get_dispense_fault_code_from_event(const app_event_t* event) {
  uint8_t fault_code = DISPENSE_FAULT_NONE;

  if (event->id == APP_EV_FAULT) {
    switch ((uint8_t)event->value) {
      case AXIS_EVENT_MOVE_FAILED:
      case AXIS_EVENT_HOME_FAILED:
      case AXIS_EVENT_HOME_ABORTED:
      case AXIS_EVENT_IDLE_FAULT:
        fault_code = DISPENSE_FAULT_MOTION;
        break;
      default:
        fault_code = DISPENSE_FAULT_INVALID_STATE;
        break;
    }
  }
  else if (event->id == APP_EV_TIMEOUT) {
    fault_code = DISPENSE_FAULT_TIMEOUT;
  }

  if (fault_code != DISPENSE_FAULT_NONE) {
    if (fault_code == DISPENSE_FAULT_TIMEOUT) {
      app_console_print("[Dispense SM] Fault detected for slot %d: event id=%d, value=%d, fault_code=%d\r\n", event->slot, event->id, event->value,
                        fault_code);
    }
    else {
      app_console_print("[Dispense SM] Fault detected for slot %d: event id=%d, value=%d, fault_code=%d, axis_event name=%s\r\n", event->slot,
                        event->id, event->value, fault_code, axis_event_name((axis_event_enum)event->value));
    }
  }

  return fault_code;
}

static uint32_t get_dispense_timeout_ms_for_state(dispense_state_enum state) {
  uint32_t timeout_ms = 0U;

  switch (state) {
    case DISPENSE_PUSH_EXTEND:
      timeout_ms = DISPENSE_PUSH_RETRACT_TIMEOUT_MS;
      break;
    case DISPENSE_PUSH_RETRACT:
      timeout_ms = DISPENSE_PUSH_RETRACT_TIMEOUT_MS;
      break;
    case DISPENSE_LIFT_SEEK:
      timeout_ms = DISPENSE_LIFT_TIMEOUT_MS;
      break;
    default:
      break;
  }

  return timeout_ms;
}
