/**
 * @file startup_sm.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief Auto-loader startup sequence state machine.
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

#include "app_console.h"
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

void startup_sm_start(app_sm_t* sm) {
  if (sm != NULL) {
    sm->startup = STARTUP_WAIT_DOOR;
  }
}

void startup_sm_dispatch(app_sm_t* sm, const app_event_t* event) {
  if ((sm == NULL) || (event == NULL)) {
    return;
  }

  switch (sm->startup) {
    case STARTUP_WAIT_DOOR:
      if (event->id == APP_EV_DOOR_CLOSED) {
        app_console_print("[Startup SM] Door closed\r\n");
        sm->startup = STARTUP_LOCK_DOOR;
        app_sm_port_lock_door();
        app_sm_port_arm_timeout(APP_SM_TIMEOUT_LOCK);
      }
      break;

    case STARTUP_LOCK_DOOR:
      if (event->id == APP_EV_LOCK_CONFIRMED) {
        app_console_print("[Startup SM] Door locked\r\n");
        app_sm_port_cancel_timeout();
        sm->startup = STARTUP_HOME_PUSHERS;
        app_sm_port_home_pushers();
        app_sm_port_arm_timeout(APP_SM_TIMEOUT_MOTION);
      }
      else if (event->id == APP_EV_TIMEOUT) {
        sm->startup = STARTUP_FAILED;
      }
      break;

    case STARTUP_HOME_PUSHERS:
      if (event->id == APP_EV_MOTION_DONE) {
        app_console_print("[Startup SM] Pushers homed\r\n");
        app_sm_port_cancel_timeout();
        sm->startup = STARTUP_HOME_LIFTS;
        app_sm_port_home_lifts();
        app_sm_port_arm_timeout(APP_SM_TIMEOUT_MOTION);
      }
      else if (event->id == APP_EV_TIMEOUT) {
        app_console_print("[Startup SM] Pushers home timeout\r\n");
        sm->startup = STARTUP_FAILED;
      }
      break;

    case STARTUP_HOME_LIFTS:
      if (event->id == APP_EV_MOTION_DONE) {
        app_sm_port_cancel_timeout();
        sm->startup = STARTUP_COUNT_CARTRIDGES;
        app_sm_port_count_cartridges();
        app_sm_port_arm_timeout(APP_SM_TIMEOUT_MOTION);
      }
      else if (event->id == APP_EV_TIMEOUT) {
        sm->startup = STARTUP_FAILED;
      }
      break;

    case STARTUP_COUNT_CARTRIDGES:
      if (event->id == APP_EV_COUNT_DONE) {
        app_sm_port_cancel_timeout();
        sm->startup = STARTUP_COMPLETE;
      }
      else if (event->id == APP_EV_TIMEOUT) {
        sm->startup = STARTUP_FAILED;
      }
      break;

    case STARTUP_COMPLETE:
    case STARTUP_FAILED:
    default:
      break;
  }
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/
