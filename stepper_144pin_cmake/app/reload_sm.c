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
  else if (event->id == APP_EV_TIMEOUT) {
    reload->state = RELOAD_FAILED;
    result.status = RELOAD_STATUS_FAILED;
  }
  else {
    switch (reload->state) {
      case RELOAD_HOME_PUSHERS:
        if (event->id == APP_EV_MOTION_DONE) {
          app_sm_port_cancel_timeout();
          reload->state = RELOAD_HOME_LIFTS;
          for (uint8_t slot = 0U; slot < APP_SLOT_COUNT; slot++) {
            app_sm_port_home_lift(&cartridges[slot], DIR_LIFTER_DOWN);
          }
          app_sm_port_arm_timeout(APP_SM_TIMEOUT_MOTION, 1000);
        }
        break;

      case RELOAD_HOME_LIFTS:
        if (event->id == APP_EV_MOTION_DONE) {
          app_sm_port_cancel_timeout();
          reload->state = RELOAD_UNLOCK_DOOR;
          app_sm_port_unlock_door();
          app_sm_port_arm_timeout(APP_SM_TIMEOUT_UNLOCK, 1000);
        }
        break;

      case RELOAD_UNLOCK_DOOR:
        if (event->id == APP_EV_LOCK_RELEASED) {
          app_sm_port_cancel_timeout();
          reload->state = RELOAD_WAIT_OPEN;
          app_sm_port_arm_timeout(APP_SM_TIMEOUT_UNLOCK, 1000);
        }
        break;

      case RELOAD_WAIT_OPEN:
        if (event->id == APP_EV_DOOR_OPENED) {
          app_sm_port_cancel_timeout();
          reload->state = RELOAD_WAIT_CLOSE;
          app_sm_port_arm_timeout(APP_SM_TIMEOUT_UNLOCK, 1000);
        }
        break;

      case RELOAD_WAIT_CLOSE:
        if (event->id == APP_EV_DOOR_CLOSED) {
          app_sm_port_cancel_timeout();
          reload->state = RELOAD_LOCK_DOOR;
          app_sm_port_lock_door();
          app_sm_port_arm_timeout(APP_SM_TIMEOUT_LOCK, 1000);
        }
        break;

      case RELOAD_LOCK_DOOR:
        if (event->id == APP_EV_LOCK_CONFIRMED) {
          app_sm_port_cancel_timeout();
          reload->state = RELOAD_COUNT_CARTRIDGES;
          // app_sm_port_count_cartridges();
          app_sm_port_arm_timeout(APP_SM_TIMEOUT_MOTION, 1000);
        }
        break;

      case RELOAD_COUNT_CARTRIDGES:
        if (event->id == APP_EV_COUNT_DONE) {
          app_sm_port_cancel_timeout();
          reload->state = RELOAD_COMPLETE;
          result.status = RELOAD_STATUS_DONE;
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
