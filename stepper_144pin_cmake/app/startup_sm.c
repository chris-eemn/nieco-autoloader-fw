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
    sm->startup.state = STARTUP_WAIT_DOOR;
  }
}

void startup_sm_dispatch(app_sm_t* sm, const app_event_t* event) {
  if ((sm == NULL) || (event == NULL)) {
    return;
  }

  switch (sm->startup.state) {
    case STARTUP_WAIT_DOOR:
      if (event->id == APP_EV_DOOR_CLOSED) {
        app_console_print("[Startup SM] Door closed\r\n");
        sm->startup.state = STARTUP_LOCK_DOOR;
        app_sm_port_lock_door();
        app_sm_port_arm_timeout(APP_SM_TIMEOUT_LOCK, 1000);
      }
      break;

    case STARTUP_LOCK_DOOR:
      if (event->id == APP_EV_LOCK_CONFIRMED) {
        app_console_print("[Startup SM] Door locked\r\n");
        app_sm_port_cancel_timeout();
        sm->startup.state = STARTUP_HOME_PUSHERS;
        for (uint8_t i = 0U; i < APP_SLOT_COUNT; i++) {
          app_sm_port_home_pusher(i);
          app_sm_port_arm_timeout(APP_SM_TIMEOUT_MOTION, 10000);
        }
      }
      else if (event->id == APP_EV_TIMEOUT) {
        sm->startup.state = STARTUP_FAILED;
      }
      break;

    case STARTUP_HOME_PUSHERS:
      if (event->id == APP_EV_MOTION_DONE) {
        sm->cartridge[event->axis_num - 1U].pusher_homed = true;

        bool all_pushers_homed = true;
        for (uint8_t i = 0U; i < APP_SLOT_COUNT; i++) {
          if (sm->cartridge[i].pusher_homed == false) {
            all_pushers_homed = false;
            break;
          }
        }

        if (all_pushers_homed) {
          app_console_print("[Startup SM] Pushers homed\r\n");
          sm->startup.state = STARTUP_HOME_LIFTS_DOWN;
          app_sm_port_cancel_timeout();
          for (uint8_t i = 0U; i < APP_SLOT_COUNT; i++) {
            app_sm_port_home_lift(i);
            app_sm_port_arm_timeout(APP_SM_TIMEOUT_MOTION, 1000);
          }
        }
      }
      else if (event->id == APP_EV_TIMEOUT) {
        app_console_print("[Startup SM] Pushers home timeout\r\n");
        sm->startup.state = STARTUP_FAILED;
      }
      break;

    case STARTUP_HOME_LIFTS_DOWN:
      if (event->id == APP_EV_MOTION_DONE) {
        app_console_print("[Startup SM] Lifts homed down\r\n");
        app_sm_port_cancel_timeout();
        sm->startup.state = STARTUP_COUNT_CARTRIDGES;
        app_sm_port_count_cartridges();
        app_sm_port_arm_timeout(APP_SM_TIMEOUT_MOTION, 1000);
      }
      else if (event->id == APP_EV_TIMEOUT) {
        sm->startup.state = STARTUP_FAILED;
      }
      break;

    case STARTUP_HOME_LIFTS_UP:
      if (event->id == APP_EV_MOTION_DONE) {
        app_console_print("[Startup SM] Lifts homed up\r\n");
        app_sm_port_cancel_timeout();
        sm->startup.state = STARTUP_COUNT_CARTRIDGES;
        app_sm_port_count_cartridges();
        app_sm_port_arm_timeout(APP_SM_TIMEOUT_MOTION, 1000);
      }
      else if (event->id == APP_EV_TIMEOUT) {
        sm->startup.state = STARTUP_FAILED;
      }
      break;

    case STARTUP_COUNT_CARTRIDGES:
      if (event->id == APP_EV_COUNT_DONE) {
        app_sm_port_cancel_timeout();
        sm->startup.state = STARTUP_COMPLETE;
      }
      else if (event->id == APP_EV_TIMEOUT) {
        sm->startup.state = STARTUP_FAILED;
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
