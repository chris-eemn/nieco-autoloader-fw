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
#include <stdbool.h>

#include "app_console.h"
#include "app_sm_port.h"
#include "app_task.h"
#include "autoloader_types.h"
#include "input.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
#define LIFT_HOMING_SETTLE_DELAY_MS (250U)  // time to wait after lift homing before starting the next step.

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/
static void startup_begin_homing(app_sm_t* sm);

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
      if ((input_get_door_closed() == true) || (event->id == APP_EV_DOOR_CLOSED)) {
        if (input_get_lock_confirmed() == true) {
          app_console_print("[Startup SM] Door already locked\r\n");
          startup_begin_homing(sm);
          sm->startup.state = STARTUP_HOME_PUSHERS;
        }
        else {
          app_console_print("[Startup SM] Locking door\r\n");

          sm->startup.state = STARTUP_LOCK_DOOR;
          app_sm_port_lock_door();
          app_sm_port_arm_timeout(APP_SM_TIMEOUT_LOCK, 1000U);
        }
      }
      break;

    case STARTUP_LOCK_DOOR:
      if (event->id == APP_EV_LOCK_CONFIRMED) {
        app_console_print("[Startup SM] Door locked\r\n");
        app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_LOCK);
        sm->startup.state = STARTUP_HOME_PUSHERS;
        startup_begin_homing(sm);
      }
      else if ((event->id == APP_EV_TIMEOUT) && (event->value == APP_SM_TIMEOUT_LOCK)) {
        sm->startup.state = STARTUP_FAILED;
      }
      else {
        app_console_print("[Startup SM] Unexpected event in STARTUP_LOCK_DOOR: id=%d, value=%d\r\n", event->id, event->value);
      }
      break;

    case STARTUP_HOME_PUSHERS:
      if ((event->id == APP_EV_DOOR_OPENED) || (event->id == APP_EV_LOCK_RELEASED)) {
        for (size_t i = 0U; i < APP_SLOT_COUNT; i++) {
          app_sm_port_halt_motion(&sm->cartridge[i]);
        }
        sm->fault_code = APP_FAULT_CODE_DOOR_OPENED;
        app_task_post(&(app_event_t){.id = APP_EV_FAULT, .slot = APP_NO_SLOT, .value = 0U});
        sm->startup.state = STARTUP_FAILED;
      }
      else if (event->id == APP_EV_MOTION_DONE) {
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
          app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_MOTION);
          for (uint8_t i = 0U; i < APP_SLOT_COUNT; i++) {
            app_sm_port_home_lift(&sm->cartridge[i], DIR_LIFTER_DOWN);
            app_sm_port_arm_timeout(APP_SM_TIMEOUT_MOTION, 10000);
          }
        }
      }
      else if ((event->id == APP_EV_TIMEOUT) && (event->value == APP_SM_TIMEOUT_MOTION)) {
        app_console_print("[Startup SM] Pushers home timeout\r\n");
        sm->startup.state = STARTUP_FAILED;
      }
      else {
        app_console_print("[Startup SM] Unexpected event in STARTUP_HOME_PUSHERS: id=%d, value=%d\r\n", event->id, event->value);
      }
      break;

    case STARTUP_HOME_LIFTS_DOWN:
      if (event->id == APP_EV_MOTION_DONE) {
        // cartridge nums are labeled 1-4, but the array is indexed at 0
        uint8_t cartridge = cartridge_get_slot_from_axis_num(event->axis_num) - 1;
        sm->cartridge[cartridge].lifter_homed_down = true;

        bool all_lifts_homed_down = true;
        for (uint8_t i = 0U; i < APP_SLOT_COUNT; i++) {
          if (sm->cartridge[i].lifter_homed_down == false) {
            all_lifts_homed_down = false;
            break;
          }
        }

        if (all_lifts_homed_down) {
          app_console_print("[Startup SM] Lifts homed down\r\n");
          sm->startup.state = STARTUP_HOME_LIFTS_DELAY;
          app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_MOTION);
          app_sm_port_arm_timeout(APP_SM_TIMEOUT_STARTUP_DELAY, LIFT_HOMING_SETTLE_DELAY_MS);
        }
      }
      else if ((event->id == APP_EV_TIMEOUT) && (event->value == APP_SM_TIMEOUT_MOTION)) {
        app_console_print("[Startup SM] Lifts home down timeout\r\n");
        sm->startup.state = STARTUP_FAILED;
      }
      else {
        app_console_print("[Startup SM] Unexpected event in STARTUP_HOME_LIFTS_DOWN: id=%d, value=%d\r\n", event->id, event->value);
      }
      break;

    case STARTUP_HOME_LIFTS_DELAY:
      if ((event->id == APP_EV_TIMEOUT) && (event->value == APP_SM_TIMEOUT_STARTUP_DELAY)) {
        sm->startup.state = STARTUP_HOME_LIFTS_UP;
        for (uint8_t i = 0U; i < APP_SLOT_COUNT; i++) {
          app_sm_port_home_lift(&sm->cartridge[i], DIR_LIFTER_UP);
          app_sm_port_arm_timeout(APP_SM_TIMEOUT_MOTION, 10000);
        }
      }
      else {
        app_console_print("[Startup SM] Unexpected event in STARTUP_HOME_LIFTS_DELAY: id=%d, value=%d\r\n", event->id, event->value);
      }
      break;

    case STARTUP_HOME_LIFTS_UP:
      if (event->id == APP_EV_MOTION_DONE) {
        // cartridge nums are labeled 1-4, but the array is indexed at 0
        uint8_t cartridge = cartridge_get_slot_from_axis_num(event->axis_num) - 1;
        sm->cartridge[cartridge].lifter_homed_up = true;

        bool all_lifts_homed_up = true;
        for (uint8_t i = 0U; i < APP_SLOT_COUNT; i++) {
          if (sm->cartridge[i].lifter_homed_up == false) {
            all_lifts_homed_up = false;
            break;
          }
        }

        if (all_lifts_homed_up) {
          app_console_print("[Startup SM] Lifts homed up\r\n");
          sm->startup.state = STARTUP_DETERMINE_TYPE;
          app_sm_port_cancel_timeout();
          const app_event_t new_event = {
              .id = APP_EV_HOME_DONE,
              .slot = APP_NO_SLOT,
              .value = 0U,
          };
          app_task_post(&new_event);
        }
      }
      else if (event->id == APP_EV_TIMEOUT) {
        sm->startup.state = STARTUP_FAILED;
      }
      break;

    case STARTUP_DETERMINE_TYPE:
      if (event->id == APP_EV_HOME_DONE) {
        for (uint8_t i = 0U; i < APP_SLOT_COUNT; i++) {
          cartridge_determine_type(&sm->cartridge[i]);
          app_console_print("[Startup SM] Cartridge %d type determined: %s\r\n", i + 1U, cartridge_type_to_string(sm->cartridge[i].type));

          app_console_print("[Startup SM] Simulating cartridge %d type as WHOPPER for testing purposes\r\n", i + 1U);
          sm->cartridge[i].type = CARTRIDGE_TYPE_WHOPPER;
        }

        const app_event_t new_event = {
            .id = APP_EV_DETERMINE_TYPE_DONE,
            .slot = APP_NO_SLOT,
            .value = 0U,
        };
        app_task_post(&new_event);

        sm->startup.state = STARTUP_COUNT_CARTRIDGES;
      }
      else {
        app_console_print("[Startup SM] Unexpected event in STARTUP_DETERMINE_TYPE: id=%d, value=%d\r\n", event->id, event->value);
      }

      break;

    case STARTUP_COUNT_CARTRIDGES:
      if (event->id == APP_EV_DETERMINE_TYPE_DONE) {
        app_console_print("[Startup SM] Counting cartridges\r\n");
        for (uint8_t i = 0U; i < APP_SLOT_COUNT; i++) {
          app_sm_port_count_cartridges(&sm->cartridge[i]);
          app_console_print("[Startup SM] Counting cartridges for slot %d, homing_enc_ticks = %d\r\n", i + 1U,
                            sm->cartridge[i].lifter_home_up_encoder_counts);

          app_console_print("[Startup SM] Simulating cartridge %d number of items as 10 for testing purposes\r\n", i + 1U);
          sm->cartridge[i].remaining = 10U;
        }
        sm->startup.state = STARTUP_COMPLETE;
      }
      else if (event->id == APP_EV_TIMEOUT) {
        sm->startup.state = STARTUP_FAILED;
      }
      else {
        app_console_print("[Startup SM] Unexpected event in STARTUP_COUNT_CARTRIDGES: id=%d, value=%d\r\n", event->id, event->value);
      }

      break;

    case STARTUP_COMPLETE:
    case STARTUP_FAILED:
    default:
      app_console_print("[Startup SM] Unexpected event in state %d: id=%d, value=%d\r\n", sm->startup.state, event->id, event->value);
      break;
  }
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/
static void startup_begin_homing(app_sm_t* sm) {
  sm->startup.state = STARTUP_HOME_PUSHERS;
  for (uint8_t i = 0U; i < APP_SLOT_COUNT; i++) {
    app_sm_port_home_pusher(&sm->cartridge[i]);
    app_sm_port_arm_timeout(APP_SM_TIMEOUT_MOTION, 10000);
  }
}
