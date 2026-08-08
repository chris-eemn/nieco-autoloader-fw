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
#include "app_sm_port.h"
#include "patty_handler.h"

#include <stddef.h>

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

#define DISPENSE_FAULT_TIMEOUT (1U)
#define DISPENSE_FAULT_MOTION (2U)
#define DISPENSE_FAULT_COMMAND_REJECT (3U)
#define DISPENSE_FAULT_TIMER_REJECT (4U)
#define DISPENSE_FAULT_HALT_REJECT (5U)

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
static patty_handler_result_enum dispense_fail(dispense_sm_t* dispense_sm, uint16_t fault_code);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

void dispense_sm_init(dispense_sm_t* dispense_sm, uint8_t slot) {
  if (dispense_sm != NULL) {
    dispense_sm->state = DISPENSE_IDLE;
    dispense_sm->slot = slot;
    dispense_sm->fault_code = 0U;
    dispense_sm->measured_lift_travel_counts = 0U;
    dispense_sm->product_may_have_dispensed = false;
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

patty_handler_result_enum dispense_sm_dispatch(dispense_sm_t* dispense_sm, const dispense_event_t* event) {
  patty_handler_result_enum result = PATTY_HANDLER_RESULT_INVALID_ARGUMENT;

  if ((dispense_sm != NULL) && (event != NULL)) {
    result = PATTY_HANDLER_RESULT_NO_ACTION;

    if ((event->slot == dispense_sm->slot) && (dispense_sm->state >= DISPENSE_PUSH_EXTEND) && (dispense_sm->state <= DISPENSE_LIFT_BACKOFF)) {
      if (event->id == DISPENSE_EVENT_TIMEOUT) {
        result = dispense_fail(dispense_sm, DISPENSE_FAULT_TIMEOUT);
      }
      else if (event->id == DISPENSE_EVENT_MOTION_FAULT) {
        uint16_t fault_code = event->fault_code;

        if (fault_code == 0U) {
          fault_code = DISPENSE_FAULT_MOTION;
        }

        result = dispense_fail(dispense_sm, fault_code);
      }
      else {
        switch (dispense_sm->state) {
          case DISPENSE_PUSH_EXTEND:
            if (event->id == DISPENSE_EVENT_PUSH_EXTENDED) {
              // app_sm_port_cancel_timeout_id(dispense_sm->slot);
              dispense_sm->product_may_have_dispensed = true;
              result = dispense_start_motion(dispense_sm, DISPENSE_PUSH_RETRACT);
            }
            break;

          case DISPENSE_PUSH_RETRACT:
            if (event->id == DISPENSE_EVENT_PUSH_RETRACTED) {
              // app_sm_port_cancel_timeout_id(dispense_sm->slot);
              result = dispense_start_motion(dispense_sm, DISPENSE_LIFT_SEEK);
            }
            break;

          case DISPENSE_LIFT_SEEK:
            if (event->id == DISPENSE_EVENT_LIFT_STALLED) {
              // app_sm_port_cancel_timeout_id(dispense_sm->slot);
              dispense_sm->measured_lift_travel_counts = event->measured_lift_travel_counts;
              result = dispense_start_motion(dispense_sm, DISPENSE_LIFT_BACKOFF);
            }
            break;

          case DISPENSE_LIFT_BACKOFF:
            if (event->id == DISPENSE_EVENT_LIFT_BACKOFF_COMPLETE) {
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
  command_started = app_sm_port_start_motion(dispense_sm->slot, next_state);

  if (command_started == true) {
    timer_started = app_sm_port_arm_timeout(dispense_sm->slot, next_state);
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
  motion_halted = app_sm_port_halt_motion(dispense_sm->slot);

  if (motion_halted == false) {
    dispense_sm->fault_code = DISPENSE_FAULT_HALT_REJECT;
  }
  else {
    dispense_sm->fault_code = fault_code;
  }

  dispense_sm->state = DISPENSE_FAILED;

  return PATTY_HANDLER_RESULT_MOTION_REJECTED;
}
