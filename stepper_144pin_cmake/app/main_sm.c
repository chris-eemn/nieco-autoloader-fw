/**
 * @file main_sm.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief Main auto-loader application state machine.
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
#include <string.h>

#include "app_console.h"
#include "app_event_simulator.h"
#include "app_sm_port.h"
#include "autoloader_types.h"
#include "axis.h"
#include "stepper_ctrl.h"
#include "cartridge.h"  // only need for my simulated type hack
#include "patty_handler.h"
#include "startup_sm.h"
#include "app_task.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/
static const char* const app_event_id_names[] = {
    [APP_EV_START] = "APP_EV_START",
    [APP_EV_DOOR_OPENED] = "APP_EV_DOOR_OPENED",
    [APP_EV_DOOR_CLOSED] = "APP_EV_DOOR_CLOSED",
    [APP_EV_LOCK_CONFIRMED] = "APP_EV_LOCK_CONFIRMED",
    [APP_EV_LOCK_RELEASED] = "APP_EV_LOCK_RELEASED",
    [APP_EV_MOTION_DONE] = "APP_EV_MOTION_DONE",
    [APP_EV_HOME_DONE] = "APP_EV_HOME_DONE",
    [APP_EV_DETERMINE_TYPE_DONE] = "APP_EV_DETERMINE_TYPE_DONE",
    [APP_EV_MOTION_FAILED] = "APP_EV_MOTION_FAILED",
    [APP_EV_STARTUP_DONE] = "APP_EV_STARTUP_DONE",
    [APP_EV_COUNT_DONE] = "APP_EV_COUNT_DONE",
    [APP_EV_DISPENSE_REQUEST] = "APP_EV_DISPENSE_REQUEST",
    [APP_EV_RELOAD_REQUEST] = "APP_EV_RELOAD_REQUEST",
    [APP_EV_FAULT] = "APP_EV_FAULT",
    [APP_EV_FAULT_CLEARED] = "APP_EV_FAULT_CLEARED",
    [APP_EV_SHUTDOWN_REQUEST] = "APP_EV_SHUTDOWN_REQUEST",
    [APP_EV_TIMEOUT] = "APP_EV_TIMEOUT",
};

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

static void app_sm_enter_state(app_sm_t* sm, app_state_enum next);
static void app_sm_enter_sequence_fault(app_sm_t* sm);
static void handle_patty_request(app_sm_t* sm, const app_event_t* event);
const char* app_event_id_to_str(app_event_id_enum event_id);
const char* app_sm_timeout_id_to_str(app_sm_timeout_id_enum timeout_id);
static void clear_all_axis_faults(void);
/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

void app_sm_init(app_sm_t* sm) {
  if (sm != NULL) {
    (void)memset(sm, 0, sizeof(*sm));
    sm->state = APP_INIT;
    sm->active_slot = APP_NO_SLOT;
    for (uint8_t slot = 0U; slot < APP_SLOT_COUNT; slot++) {
      sm->cartridge[slot].num = slot + 1;
    }
    patty_handler_init(&sm->patty_handler, sm->cartridge);

    // TODO; not safe but allows us to fake the door/lock for now
    patty_handler_set_safety_state(&sm->patty_handler, true, true, true);
    app_sm_port_publish_state(sm);
  }
}

void app_sm_dispatch(app_sm_t* sm, const app_event_t* event) {
  if ((sm == NULL) || (event == NULL)) {
    return;
  }

  if (event->id == APP_EV_TIMEOUT) {
    app_console_print("[Main SM] Timeout event received: timeout_id=%s\r\n", app_sm_port_timeout_id_to_str((app_sm_timeout_id_enum)event->value));
  }
  else {
    app_console_print("[Main SM] Event received: id=%s, slot=%d, value=%d\r\n", app_event_id_to_str(event->id), event->slot, event->value);
  }

  if (event->id == APP_EV_SHUTDOWN_REQUEST) {
    app_sm_enter_state(sm, APP_SHUTDOWN);
    return;
  }

  if (event->id == APP_EV_FAULT) {
    app_console_print("[Main SM] Fault event received: fault_code=%d\r\n", event->value);
    sm->fault_code = event->value;
    app_sm_enter_state(sm, APP_FAULT);
    // app_console_print("[Main SM] Simulating fault cleared event in 100ms for testing purposes\r\n");
    // app_simulate_event(100, APP_EV_FAULT_CLEARED);
    return;
  }

  switch (sm->state) {
    case APP_INIT:
      if (event->id == APP_EV_START) {
        // TODO: put back to startup

#if 0
        app_console_print("[Startup SM] Simulating cartridge %d type as WHOPPER for testing purposes\r\n", 0 + 1U);
        sm->cartridge[0].type = CARTRIDGE_TYPE_WHOPPER;
        app_console_print("[Startup SM] Simulating cartridge %d number of items as 10 for testing purposes\r\n", 0 + 1U);
        sm->cartridge[0].remaining = 10U;
        app_sm_enter_state(sm, APP_READY);
#else
        app_sm_enter_state(sm, APP_STARTUP);
#endif
      }
      break;

    case APP_STARTUP: {
      startup_result_t result;
      if ((event->id == APP_EV_DOOR_OPENED) || (event->id == APP_EV_LOCK_RELEASED)) {
        startup_sm_abort(&sm->startup);
        sm->fault_code = APP_FAULT_CODE_DOOR_OPENED;
        app_sm_enter_state(sm, APP_FAULT);
      }
      else {
        result = startup_sm_dispatch(&sm->startup, sm->cartridge, event);
        if (result.status == STARTUP_STATUS_DONE) {
          app_console_print("[Startup SM] Startup complete\r\n");
          app_sm_enter_state(sm, APP_READY);
        }
        else if (result.status == STARTUP_STATUS_FAILED) {
          app_console_print("[Startup SM] Startup failed: fault_code=%d\r\n", result.fault_code);
          app_sm_enter_sequence_fault(sm);
        }
        sm->fault_code = result.fault_code;
      }
    } break;

    case APP_READY:
      app_console_print("[Main SM] In ready state\r\n");

      if ((event->id == APP_EV_DOOR_OPENED) || (event->id == APP_EV_LOCK_RELEASED)) {
        sm->fault_code = APP_FAULT_CODE_DOOR_OPENED;
        patty_handler_abort_all(&sm->patty_handler);
        app_sm_enter_state(sm, APP_FAULT);
      }
      else if (event->id == APP_EV_DISPENSE_REQUEST) {
        app_console_print("[Main SM] Dispense request received: product_type=%s, count=%d\r\n", cartridge_type_to_string(event->product_type),
                          event->value);
        handle_patty_request(sm, event);
      }
      else if (event->id == APP_EV_RELOAD_REQUEST) {
        app_sm_enter_state(sm, APP_RELOAD);
      }
      break;

    case APP_DISPENSE:
      app_console_print("[Main SM] In dispense state\r\n");
      if ((event->id == APP_EV_DOOR_OPENED) || (event->id == APP_EV_LOCK_RELEASED)) {
        sm->fault_code = APP_FAULT_CODE_DOOR_OPENED;
        patty_handler_abort_all(&sm->patty_handler);
        app_sm_enter_state(sm, APP_FAULT);
      }
      if (event->id == APP_EV_DISPENSE_REQUEST) {
        /* New requests can be accepted while other cartridges are running. */
        handle_patty_request(sm, event);
      }
      else if (event->id == APP_EV_RELOAD_REQUEST) {
        sm->reload_pending = true;

        /*
         * Prevent the handler from starting another queued cycle.
         * Already-active cartridge cycles are allowed to finish.
         */
        patty_handler_set_dispensing_enabled(&sm->patty_handler, false);
      }
      else {
        patty_handler_result_enum result;
        result = patty_handler_dispatch_event(&sm->patty_handler, event);

        app_console_print("[Main SM] Patty handler processed event: result=%d (%s)\r\n", result, patty_handler_result_enum_to_str(result));
      }

      if ((sm->reload_pending == true) && (patty_handler_has_active_dispenses(&sm->patty_handler) == false)) {
        app_sm_enter_state(sm, APP_RELOAD);
      }
      else if (patty_handler_has_work(&sm->patty_handler) == false) {
        app_sm_enter_state(sm, APP_READY);
      }
      break;

    case APP_RELOAD:
      reload_sm_dispatch(sm, event);
      if (sm->reload == RELOAD_COMPLETE) {
        app_sm_enter_state(sm, APP_READY);
      }
      else if (sm->reload == RELOAD_FAILED) {
        app_sm_enter_sequence_fault(sm);
      }
      break;

    case APP_FAULT:
      app_console_print("[Main SM] Fault state: fault_code=%d\r\n", sm->fault_code);
      if (event->id == APP_EV_FAULT_CLEARED) {
        sm->fault_code = 0U;
        app_console_print("[Main SM] Fault cleared, returning to startup\r\n");
        // this is a hack so do not have to power cycle machine to dispense again
        clear_all_axis_faults();
        patty_handler_init(&sm->patty_handler, sm->cartridge);
        patty_handler_set_safety_state(&sm->patty_handler, true, true, true);
        for (uint8_t slot = 0U; slot < APP_SLOT_COUNT; slot++) {
          sm->cartridge[slot].faulted = false;
        }
        app_sm_enter_state(sm, APP_STARTUP);
      }
      else if (event->id == APP_EV_DOOR_CLOSED) {
        if (sm->fault_code == APP_FAULT_CODE_DOOR_OPENED) {
          app_console_print("[Main SM] Door closed, returning to startup\r\n");
          sm->fault_code = 0U;
          patty_handler_set_safety_state(&sm->patty_handler, true, true, true);
          app_sm_enter_state(sm, APP_STARTUP);
          app_task_post(&(app_event_t){.id = APP_EV_CONTINUE, .slot = APP_NO_SLOT, .value = 0U});
        }
      }
      break;

    case APP_SHUTDOWN:
      break;

    default:
      sm->fault_code = APP_FAULT_CODE_INVALID_STATE;
      app_sm_enter_state(sm, APP_FAULT);
      break;
  }

  app_sm_port_publish_state(sm);
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/

static void clear_all_axis_faults(void) {
  for (uint8_t motor_num = 1U; motor_num <= STEPPER_CTRL_MAX_MOTORS; motor_num++) {
    axis_t* ax = stepper_ctrl_get_axis(motor_num);
    if (ax != NULL) {
      axis_fault_reset(ax);
    }
  }
}

/**
 * @brief Perform the entry actions for a main application state.
 * @param sm Application state model.
 * @param next State to enter.
 */
static void app_sm_enter_state(app_sm_t* sm, app_state_enum next) {
  if (sm != NULL) {
    app_sm_port_cancel_timeout();
    sm->state = next;

    switch (next) {
      case APP_STARTUP:
        startup_sm_start(&sm->startup);
        break;

      case APP_DISPENSE:
        // handled by patty_handler
        break;

      case APP_RELOAD:
        sm->reload_pending = false;
        reload_sm_start(sm);
        break;

      case APP_FAULT:
        app_sm_port_halt_all_motion(sm->cartridge);
        break;

      case APP_SHUTDOWN:
        app_sm_port_halt_all_motion(sm->cartridge);
        app_sm_port_save_state();
        break;

      case APP_INIT:
      case APP_READY:
      default:
        break;
    }

    app_sm_port_publish_state(sm);
  }
}

/**
 * @brief Enter the fault state after a startup, dispense, or reload failure.
 * @param sm Application state model.
 */
static void app_sm_enter_sequence_fault(app_sm_t* sm) {
  if (sm != NULL) {
    if (sm->fault_code == 0U) {
      sm->fault_code = APP_FAULT_CODE_SEQUENCE_ERROR;
    }
    app_sm_enter_state(sm, APP_FAULT);
  }
}

/**
 * @brief Submits a patty request and starts all eligible cartridges.
 *
 * @param sm Application state-machine context.
 * @param event Patty request event.
 */
static void handle_patty_request(app_sm_t* sm, const app_event_t* event) {
  patty_handler_result_enum request_result;
  patty_handler_result_enum process_result;

  request_result = patty_handler_add_request(&sm->patty_handler, event->product_type, event->value, PATTY_REQUEST_SOURCE_QUEUE);

  if (request_result == PATTY_HANDLER_RESULT_OK) {
    process_result = patty_handler_process(&sm->patty_handler);

    if (process_result == PATTY_HANDLER_RESULT_MOTION_REJECTED) {
      /* One or more cartridges rejected their initial motion command.
       * The handler has already faulted those individual cartridges.
       */
    }

    app_sm_enter_state(sm, APP_DISPENSE);
  }
  else {
    /* TODO: Publish the appropriate rejected-request result to Modbus. */
    app_console_print("[Main SM] Dispense request rejected: result=%d\r\n", request_result);
  }
}

const char* app_event_id_to_str(app_event_id_enum event_id) {
  if ((size_t)event_id >= (sizeof(app_event_id_names) / sizeof(app_event_id_names[0]))) {
    return "APP_EV_UNKNOWN";
  }

  return app_event_id_names[event_id];
}
