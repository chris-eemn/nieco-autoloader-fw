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

static void app_sm_enter_state(app_sm_t* sm, app_state_enum next);
static void app_sm_enter_sequence_fault(app_sm_t* sm);

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
        app_sm_enter_state(sm, APP_READY);
      }
      else if (sm->startup.state == STARTUP_FAILED) {
        app_sm_enter_sequence_fault(sm);
      }
      break;

    case APP_READY:
      if ((event->id == APP_EV_DISPENSE_REQUEST) && (event->slot < APP_SLOT_COUNT)) {
        sm->active_slot = event->slot;
        app_sm_enter_state(sm, APP_DISPENSE);
      }
      else if (event->id == APP_EV_RELOAD_REQUEST) {
        app_sm_enter_state(sm, APP_RELOAD);
      }
      break;

    case APP_DISPENSE:
      if (event->id == APP_EV_RELOAD_REQUEST) {
        sm->reload_pending = true;
      }
      else {
        dispense_sm_dispatch(sm, event);
      }

      if (sm->dispense == DISPENSE_COMPLETE) {
        if (sm->reload_pending == true) {
          app_sm_enter_state(sm, APP_RELOAD);
        }
        else {
          app_sm_enter_state(sm, APP_READY);
        }
      }
      else if (sm->dispense == DISPENSE_FAILED) {
        app_sm_enter_sequence_fault(sm);
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
        dispense_sm_start(sm, sm->active_slot);
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
