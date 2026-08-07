/**
 * @file dispense_sm.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief Auto-loader single-patty dispense sequence state machine.
 * @version 0.1
 * @date 2026-08-07
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC. All Rights Reserved.
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include "autoloader_sm.h"

#include <stddef.h>

#include "app_sm_port.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

void dispense_sm_start(app_sm_t* sm, uint8_t slot) {
  if (sm != NULL) {
    sm->active_slot = slot;
    sm->dispense = DISPENSE_PUSH_EXTEND;
    app_sm_port_push_extend(slot);
    app_sm_port_arm_timeout(APP_SM_TIMEOUT_MOTION, 1000);
  }
}

void dispense_sm_dispatch(app_sm_t* sm, const app_event_t* event) {
  if ((sm == NULL) || (event == NULL)) {
    return;
  }

  if (event->id == APP_EV_TIMEOUT) {
    sm->dispense = DISPENSE_FAILED;
    return;
  }

  switch (sm->dispense) {
    case DISPENSE_PUSH_EXTEND:
      if (event->id == APP_EV_MOTION_DONE) {
        app_sm_port_cancel_timeout();
        sm->dispense = DISPENSE_PUSH_RETRACT;
        app_sm_port_push_retract(sm->active_slot);
        app_sm_port_arm_timeout(APP_SM_TIMEOUT_MOTION, 1000);
      }
      break;

    case DISPENSE_PUSH_RETRACT:
      if (event->id == APP_EV_MOTION_DONE) {
        app_sm_port_cancel_timeout();
        sm->dispense = DISPENSE_LIFT_SEEK;
        app_sm_port_lift_seek(sm->active_slot);
        app_sm_port_arm_timeout(APP_SM_TIMEOUT_MOTION, 1000);
      }
      break;

    case DISPENSE_LIFT_SEEK:
      if (event->id == APP_EV_STALL_DETECTED) {
        app_sm_port_cancel_timeout();
        sm->dispense = DISPENSE_LIFT_BACKOFF;
        app_sm_port_lift_backoff(sm->active_slot);
        app_sm_port_arm_timeout(APP_SM_TIMEOUT_MOTION, 1000);
      }
      break;

    case DISPENSE_LIFT_BACKOFF:
      if (event->id == APP_EV_MOTION_DONE) {
        app_sm_port_cancel_timeout();
        app_sm_port_commit_dispense(sm->active_slot);
        sm->dispense = DISPENSE_COMPLETE;
      }
      break;

    case DISPENSE_COMPLETE:
    case DISPENSE_FAILED:
    default:
      break;
  }
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/
