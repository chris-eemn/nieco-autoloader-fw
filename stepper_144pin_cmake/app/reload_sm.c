/**
 * @file reload_sm.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief Auto-loader reload sequence state machine.
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

void reload_sm_start(app_sm_t* sm) {
  if (sm != NULL) {
    sm->reload = RELOAD_HOME_PUSHERS;
    app_sm_port_home_pushers();
    app_sm_port_arm_timeout(APP_SM_TIMEOUT_MOTION);
  }
}

void reload_sm_dispatch(app_sm_t* sm, const app_event_t* event) {
  if ((sm == NULL) || (event == NULL)) {
    return;
  }

  if (event->id == APP_EV_TIMEOUT) {
    sm->reload = RELOAD_FAILED;
    return;
  }

  switch (sm->reload) {
    case RELOAD_HOME_PUSHERS:
      if (event->id == APP_EV_MOTION_DONE) {
        app_sm_port_cancel_timeout();
        sm->reload = RELOAD_HOME_LIFTS;
        app_sm_port_home_lifts();
        app_sm_port_arm_timeout(APP_SM_TIMEOUT_MOTION);
      }
      break;

    case RELOAD_HOME_LIFTS:
      if (event->id == APP_EV_MOTION_DONE) {
        app_sm_port_cancel_timeout();
        sm->reload = RELOAD_UNLOCK_DOOR;
        app_sm_port_unlock_door();
        app_sm_port_arm_timeout(APP_SM_TIMEOUT_UNLOCK);
      }
      break;

    case RELOAD_UNLOCK_DOOR:
      if (event->id == APP_EV_LOCK_RELEASED) {
        app_sm_port_cancel_timeout();
        sm->reload = RELOAD_WAIT_OPEN;
        app_sm_port_arm_timeout(APP_SM_TIMEOUT_DOOR);
      }
      break;

    case RELOAD_WAIT_OPEN:
      if (event->id == APP_EV_DOOR_OPENED) {
        app_sm_port_cancel_timeout();
        sm->reload = RELOAD_WAIT_CLOSE;
        app_sm_port_arm_timeout(APP_SM_TIMEOUT_DOOR);
      }
      break;

    case RELOAD_WAIT_CLOSE:
      if (event->id == APP_EV_DOOR_CLOSED) {
        app_sm_port_cancel_timeout();
        sm->reload = RELOAD_LOCK_DOOR;
        app_sm_port_lock_door();
        app_sm_port_arm_timeout(APP_SM_TIMEOUT_LOCK);
      }
      break;

    case RELOAD_LOCK_DOOR:
      if (event->id == APP_EV_LOCK_CONFIRMED) {
        app_sm_port_cancel_timeout();
        sm->reload = RELOAD_COUNT_CARTRIDGES;
        app_sm_port_count_cartridges();
        app_sm_port_arm_timeout(APP_SM_TIMEOUT_MOTION);
      }
      break;

    case RELOAD_COUNT_CARTRIDGES:
      if (event->id == APP_EV_COUNT_DONE) {
        app_sm_port_cancel_timeout();
        sm->reload = RELOAD_COMPLETE;
      }
      break;

    case RELOAD_COMPLETE:
    case RELOAD_FAILED:
    default:
      break;
  }
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/
