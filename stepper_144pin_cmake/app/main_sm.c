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
#include "app_sm_port.h"
#include "patty_handler.h"

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

static void app_sm_enter_state(app_sm_t* sm, app_state_enum next);
static void app_sm_enter_sequence_fault(app_sm_t* sm);
static void handle_patty_request(app_sm_t* sm, const app_event_t* event);

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

  if (event->id == APP_EV_SHUTDOWN_REQUEST) {
    app_sm_enter_state(sm, APP_SHUTDOWN);
    return;
  }

  if ((event->id == APP_EV_FAULT) && (sm->state != APP_SHUTDOWN)) {
    sm->fault_code = event->value;
    app_sm_enter_state(sm, APP_FAULT);
    return;
  }

  switch (sm->state) {
    case APP_INIT:
      if (event->id == APP_EV_START) {
        app_sm_enter_state(sm, APP_STARTUP);
      }
      break;

    case APP_STARTUP:
      startup_sm_dispatch(sm, event);
      if (sm->startup.state == STARTUP_COMPLETE) {
        app_console_print("[Startup SM] Startup complete\r\n");
        app_sm_enter_state(sm, APP_READY);
      }
      else if (sm->startup.state == STARTUP_FAILED) {
        app_sm_enter_sequence_fault(sm);
      }
      break;

    case APP_READY:
      app_console_print("[Main SM] Ready for dispense or reload\r\n");

      if (event->id == APP_EV_DISPENSE_REQUEST) {
        app_console_print("[Main SM] Dispense request received: product_type=%s, count=%d\r\n", cartridge_type_to_string(event->product_type),
                          event->value);
        handle_patty_request(sm, event);
      }
      else if (event->id == APP_EV_RELOAD_REQUEST) {
        app_sm_enter_state(sm, APP_RELOAD);
      }
      break;

    case APP_DISPENSE:
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
        patty_handler_dispatch_event(&sm->patty_handler, event);
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
      if (event->id == APP_EV_FAULT_CLEARED) {
        sm->fault_code = 0U;
        app_sm_enter_state(sm, APP_STARTUP);
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
        startup_sm_start(sm);
        break;

      case APP_DISPENSE:
        // handled by patty_handler
        break;

      case APP_RELOAD:
        sm->reload_pending = false;
        reload_sm_start(sm);
        break;

      case APP_FAULT:
        app_sm_port_halt_all_motion();
        break;

      case APP_SHUTDOWN:
        app_sm_port_halt_all_motion();
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
  }
}
