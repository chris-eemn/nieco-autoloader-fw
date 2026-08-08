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

static bool start_motion(uint8_t slot, dispense_state_enum next_state);
static bool halt_motion(uint8_t slot);
/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

void dispense_sm_init(dispense_sm_t* dispense_sm, uint8_t slot) {
  if (dispense_sm != NULL) {
    dispense_sm->state = DISPENSE_IDLE;
    dispense_sm->slot_index = slot;
    dispense_sm->fault_code = 0U;
    dispense_sm->measured_lift_travel_counts = 0U;
    dispense_sm->product_may_have_dispensed = false;
    switch (slot) {
      case 1:
        dispense_sm->timeout_id = APP_SM_CART1_DISPENSE;
        break;
      case 2:
        dispense_sm->timeout_id = APP_SM_CART2_DISPENSE;
        break;
      case 3:
        dispense_sm->timeout_id = APP_SM_CART3_DISPENSE;
        break;
      case 4:
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

    if ((slot_zero_based == dispense_sm->slot_index) && (dispense_sm->state >= DISPENSE_PUSH_EXTEND) &&
        (dispense_sm->state <= DISPENSE_LIFT_BACKOFF)) {
      uint8_t fault_code = get_dispense_fault_code_from_event(event);

      if (fault_code != DISPENSE_FAULT_NONE) {
        result = dispense_fail(dispense_sm, fault_code);
      }
      else {
        switch (dispense_sm->state) {
          case DISPENSE_PUSH_EXTEND:
            if (event->id == APP_EV_MOTION_DONE) {
              // todo: implement
              // app_sm_port_cancel_timeout_id(dispense_sm->slot);
              dispense_sm->product_may_have_dispensed = true;
              result = dispense_start_motion(dispense_sm, DISPENSE_PUSH_RETRACT);
            }
            break;

          case DISPENSE_PUSH_RETRACT:
            if (event->id == APP_EV_MOTION_DONE) {
              // todo: implement
              // app_sm_port_cancel_timeout_id(dispense_sm->slot);
              result = dispense_start_motion(dispense_sm, DISPENSE_LIFT_SEEK);
            }
            break;

          case DISPENSE_LIFT_SEEK:
            if (event->id == APP_EV_MOTION_DONE) {
              // todo: implement
              // app_sm_port_cancel_timeout_id(dispense_sm->slot);
              // dispense_sm->measured_lift_travel_counts = event->measured_lift_travel_counts;
              result = dispense_start_motion(dispense_sm, DISPENSE_LIFT_BACKOFF);
            }
            break;

          case DISPENSE_LIFT_BACKOFF:
            if (event->id == APP_EV_MOTION_DONE) {
              // todo: implement
              // app_sm_port_cancel_timeout_id(dispense_sm->slot);
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
  command_started = start_motion(dispense_sm->slot_index, next_state);

  if (command_started == true) {
    timer_started = app_sm_port_arm_timeout(dispense_sm->timeout_id, 5000U);
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
  bool motion_halted;

  // app_sm_port_cancel_timeout_id(dispense_sm->slot);
  motion_halted = halt_motion(dispense_sm->slot_index);

  if (motion_halted == false) {
    dispense_sm->fault_code = DISPENSE_FAULT_HALT_REJECT;
  }
  else {
    dispense_sm->fault_code = fault_code;
  }

  dispense_sm->state = DISPENSE_FAILED;

  return PATTY_HANDLER_RESULT_MOTION_REJECTED;
}

// todo: placeholder so i can get the project to build
static bool start_motion(uint8_t slot, dispense_state_enum next_state) {
  (void)slot;
  (void)next_state;

  switch (next_state) {
    case DISPENSE_PUSH_EXTEND:
      // determine axis number from slot number
      break;
    case DISPENSE_PUSH_RETRACT:
      // determine axis number from slot number
      break;
    case DISPENSE_LIFT_SEEK:
      // determine axis number from slot number
      break;
    case DISPENSE_LIFT_BACKOFF:
      // determine axis number from slot number
      break;
    default:
      break;
  }

  return true;
}

// todo: placeholder so i can get the project to build
static bool halt_motion(uint8_t slot) {
  (void)slot;

  return true;
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
    app_console_print("[Dispense SM] Fault detected for slot %d: event id=%d, value=%d, fault_code=%d, axis_event name=%d\r\n", event->slot,
                      event->id, event->value, fault_code, axis_event_name((axis_event_enum)event->value));
  }

  return fault_code;
}
