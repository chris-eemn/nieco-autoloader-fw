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
#include "cal_data.h"
#include "stepper_ctrl.h"
#include "cartridge.h"  // only need for my simulated type hack
#include "input.h"
#include "patty_handler.h"
#include "reload_sm.h"
#include "shutdown_sm.h"
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


/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

static void app_sm_enter_state(app_sm_t* sm, app_state_enum next);
static void app_sm_enter_sequence_fault(app_sm_t* sm);
static void app_sm_clear_fault(app_sm_t* sm);
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

  if (event->id == APP_EV_FAULT) {
    /* Shutdown owns the machine until the switch releases: a raw axis fault
     * event must not divert it to APP_FAULT with the door locked (plan
     * decision — faults during shutdown skip and continue). The event still
     * reaches the shutdown SM through the state switch below, where the
     * wait and homing states absorb it. */
    if (sm->state == APP_SHUTDOWN) {
      app_console_print("[Main SM] Fault event absorbed during shutdown\r\n");
    }
    else {
      app_console_print("[Main SM] Fault event received: fault_code=%d\r\n", event->value);
      sm->fault_code = event->value;
      app_sm_enter_state(sm, APP_FAULT);
      // app_console_print("[Main SM] Simulating fault cleared event in 100ms for testing purposes\r\n");
      // app_simulate_event(100, APP_EV_FAULT_CLEARED);
      return;
    }
  }

  /* The shutdown switch is edge-driven: a release outside APP_SHUTDOWN is
   * nothing to do (the switch was already inactive, or the machine already
   * left shutdown). Filter it here so it never reaches a sub-SM, whose
   * unexpected-event arms would fault the sequence. */
  if ((event->id == APP_EV_SHUTDOWN_END) && (sm->state != APP_SHUTDOWN)) {
    app_console_print("[Main SM] Shutdown-end event ignored in state %d\r\n", sm->state);
    app_sm_port_publish_state(sm);
    return;
  }

  switch (sm->state) {
    case APP_INIT:
      if (event->id == APP_EV_SHUTDOWN_REQUEST) {
        /* Plan: any state except APP_SHUTDOWN enters shutdown. The switch
         * posts only on edges, so a press before APP_EV_START must not be
         * dropped. */
        app_console_print("[Main SM] Shutdown request received during init\r\n");
        app_sm_enter_state(sm, APP_SHUTDOWN);
      }
      else if (event->id == APP_EV_START) {
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

      if ((event->id == APP_EV_CONTINUE) && (input_get_shutdown_requested() == true)) {
        /* The switch posts only on edges: if it was already held active when
         * startup began (e.g. after a door-closed fault recovery), no
         * SHUTDOWN_REQUEST edge will ever arrive. Poll the debounced level
         * on startup's own entry pump so the machine cannot dispense with
         * the switch engaged. The door poll timer armed by startup_sm_start
         * re-enters this arm, so the abort below cannot strand the sequence. */
        app_console_print("[Main SM] Shutdown switch held active at startup, entering shutdown\r\n");
        startup_sm_abort(&sm->startup);
        app_sm_enter_state(sm, APP_SHUTDOWN);
      }
      else if (event->id == APP_EV_SHUTDOWN_REQUEST) {
        app_console_print("[Main SM] Shutdown requested during startup\r\n");
        startup_sm_abort(&sm->startup);
        app_sm_enter_state(sm, APP_SHUTDOWN);
      }
      else if ((event->id == APP_EV_DOOR_OPENED) || (event->id == APP_EV_LOCK_RELEASED)) {
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
        app_console_print("[Main SM] Reload request received\r\n");
        app_sm_enter_state(sm, APP_RELOAD);
      }
      else if (event->id == APP_EV_SHUTDOWN_REQUEST) {
        app_console_print("[Main SM] Shutdown request received\r\n");
        app_sm_enter_state(sm, APP_SHUTDOWN);
      }
      break;

    case APP_DISPENSE:
      app_console_print("[Main SM] In dispense state\r\n");
      if ((event->id == APP_EV_DOOR_OPENED) || (event->id == APP_EV_LOCK_RELEASED)) {
        sm->fault_code = APP_FAULT_CODE_DOOR_OPENED;
        patty_handler_abort_all(&sm->patty_handler);
        app_sm_enter_state(sm, APP_FAULT);
      }
      else {
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
        else if (event->id == APP_EV_SHUTDOWN_REQUEST) {
          /*
           * Enter APP_SHUTDOWN directly. The shutdown SM disables dispensing
           * again at entry; already-active cartridge cycles finish first in
           * SHUTDOWN_WAIT_DISPENSES.
           */
          app_console_print("[Main SM] Shutdown request received during dispense\r\n");
          app_sm_enter_state(sm, APP_SHUTDOWN);
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
          app_console_print("[Main SM] All dispense work complete, returning to ready state\r\n");
          app_sm_enter_state(sm, APP_READY);
        }
      }
      break;

    case APP_RELOAD:
      if (event->id == APP_EV_SHUTDOWN_REQUEST) {
        /* Reload owns the door/lock sequence; shutdown takes over instead.
         * The blanket cancel in app_sm_enter_state clears reload's timers.
         * reload_pending clears with the abandoned sequence: the shutdown
         * drains the dispense queue, so a later reload request must come
         * from a fresh APP_EV_RELOAD_REQUEST, not a stale flag. */
        app_console_print("[Main SM] Shutdown requested during reload\r\n");
        sm->reload_pending = false;
        app_sm_enter_state(sm, APP_SHUTDOWN);
      }
      else {
        reload_result_t reload_result = reload_sm_dispatch(&sm->reload, sm->cartridge, event);
        if (reload_result.status == RELOAD_STATUS_DONE) {
          /*
           * Reload disabled dispensing when it was requested. Re-enable it now that
           * the reload completed and the door is closed and locked. The handler
           * refuses enable unless door_closed && door_locked, so a false return
           * means the safety state has not been applied yet.
           */
          if (patty_handler_set_dispensing_enabled(&sm->patty_handler, true) == false) {
            app_console_print("[Main SM] WARNING: reload done but dispensing re-enable refused (safety state not applied)\r\n");
          }
          sm->reload_pending = false;
          app_sm_enter_state(sm, APP_READY);
        }
        else if (reload_result.status == RELOAD_STATUS_FAILED) {
          /* Reload owns the fault code for this transition; a stale code from a
           * prior state must not survive into APP_FAULT. */
          sm->fault_code = reload_result.fault_code;
          app_sm_enter_sequence_fault(sm);
        }
      }
      break;

    case APP_FAULT:
      app_console_print("[Main SM] Fault state: fault_code=%d\r\n", sm->fault_code);
      if (event->id == APP_EV_SHUTDOWN_REQUEST) {
        /* Remember the request; app_sm_clear_fault runs the shutdown sequence
         * instead of startup once the fault is cleared. */
        app_console_print("[Main SM] Shutdown requested during fault, pending fault clear\r\n");
        sm->shutdown_pending = true;
      }
      else if (event->id == APP_EV_FAULT_CLEARED) {
        app_sm_clear_fault(sm);
      }
      else if (event->id == APP_EV_DOOR_CLOSED) {
        if (sm->fault_code == APP_FAULT_CODE_DOOR_OPENED) {
          app_console_print("[Main SM] Door closed, returning to startup\r\n");
          /* Belt-and-braces against the blanket cancel, same as app_sm_clear_fault. */
          (void)app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_AUTO_CLEAR_FAULT);
          sm->fault_code = 0U;
          /* The machine is recovering to startup, not to a shutdown the
           * switch asked for while faulted — a shutdown request arriving
           * after this point re-enters APP_SHUTDOWN from startup anyway. */
          sm->shutdown_pending = false;
          patty_handler_set_safety_state(&sm->patty_handler, true, true, true);
          app_sm_enter_state(sm, APP_STARTUP);
        }
      }
      else if ((event->id == APP_EV_TIMEOUT) && ((app_sm_timeout_id_enum)event->value == APP_SM_TIMEOUT_AUTO_CLEAR_FAULT)) {
        app_console_print("[Main SM] Auto-clear fault timer expired\r\n");
        app_sm_clear_fault(sm);
      }
      break;

    case APP_SHUTDOWN: {
      shutdown_result_t shutdown_result = shutdown_sm_dispatch(&sm->shutdown, sm->cartridge, &sm->patty_handler, event);
      if (shutdown_result.status == SHUTDOWN_STATUS_DONE) {
        /* Switch went inactive in SHUTDOWN_HOLD. Startup re-homes, re-counts,
         * and gates dispensing on door-closed + locked. startup_sm_start arms
         * the door poll timer, so no entry pump is posted here.
         *
         * shutdown_sm_start disabled dispensing on the handler at entry, and
         * unlike the fault path in app_sm_clear_fault, nothing re-inits the
         * handler on this return, so the flag would stay false and every
         * request after shutdown would be rejected. Re-apply the safety state
         * the same way the fault recovery does; the door was unlocked during
         * shutdown, so the handler's cached door flags are stale and must be
         * reset alongside the enable. */
        patty_handler_set_safety_state(&sm->patty_handler, true, true, true);
        app_sm_enter_state(sm, APP_STARTUP);
      }
      else if (shutdown_result.status == SHUTDOWN_STATUS_FAILED) {
        /* Internal error only (NULL args): the shutdown sequence itself never
         * fails. The blanket cancel in app_sm_enter_state clears shutdown's
         * timers on the transition. */
        app_console_print("[Main SM] Shutdown sequence internal error\r\n");
        sm->fault_code = APP_FAULT_CODE_SEQUENCE_ERROR;
        app_sm_enter_state(sm, APP_FAULT);
      }
    } break;

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
        reload_sm_start(&sm->reload);
        break;

      case APP_FAULT:
        reload_sm_abort(&sm->reload);
        app_sm_port_halt_all_motion(sm->cartridge);
        /* Auto-clear stopgap: arm on every fault entry, reading cal-data live so a
         * 'param set' takes effect on the next fault. The blanket
         * app_sm_port_cancel_timeout() at the top of this function cancels it on
         * the next transition, so a fault that clears early never leaves a
         * pending auto-clear timer. */
        if (cal_data_get()->auto_clear_faults != 0U) {
          (void)app_sm_port_arm_timeout(APP_SM_TIMEOUT_AUTO_CLEAR_FAULT, cal_data_get()->auto_clear_delay_ms);
        }
        break;

      case APP_SHUTDOWN:
        /* Graceful shutdown: let in-flight dispenses finish, retract pushers,
         * home lifters down, unlock the door, then park. save_state() runs on
         * SHUTDOWN_HOLD entry, once the mechanics are parked. shutdown_sm_start
         * disables dispensing on the handler itself. */
        shutdown_sm_start(&sm->shutdown, &sm->patty_handler);
        break;

      case APP_INIT:
      case APP_READY:
      default:
        break;
    }

    /* Cancel the per-slot dispense watchdogs after the state-specific entry
     * actions above: while APP_SHUTDOWN is active the shutdown SM owns the
     * event feed, and it forwards these slot timeouts to the dispense SMs
     * from SHUTDOWN_WAIT_DISPENSES, so a cycle whose motion event was in
     * flight when shutdown began can still settle. The dispense SMs
     * themselves are left untouched — their active states are exactly what
     * SHUTDOWN_WAIT_DISPENSES polls. */
    if (next == APP_SHUTDOWN) {
      for (uint8_t slot = 0U; slot < APP_SLOT_COUNT; slot++) {
        (void)app_sm_port_cancel_timeout_id((app_sm_timeout_id_enum)(APP_SM_CART1_DISPENSE + slot));
      }
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
 * @brief Clear the active fault and return the machine to startup.
 *
 * Shared by the manual clear (APP_EV_FAULT_CLEARED, posted by the CLI
 * 'test set clear 1' and the Modbus clear_faults register) and the
 * auto-clear timer. The state machine owns the axis reset and the
 * per-cartridge faulted-flag reset, so callers post only the event.
 *
 * @param sm Application state model.
 */
static void app_sm_clear_fault(app_sm_t* sm) {
  /* Belt-and-braces against the blanket cancel: the timer is normally cancelled
   * by app_sm_enter_state's app_sm_port_cancel_timeout() on the transition out
   * of APP_FAULT, but cancel it here too so a clear can never leave a pending
   * auto-clear behind even if the blanket cancel changes again. */
  (void)app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_AUTO_CLEAR_FAULT);
  sm->fault_code = 0U;
  if (sm->shutdown_pending == true) {
    /* The shutdown switch was toggled while faulted: run the graceful
     * shutdown sequence instead of returning to startup. Reset the axis
     * faults first: axis_home refuses to start on a faulted or stalled
     * axis, and a rejected homing command would divert the shutdown to
     * APP_FAULT with the door locked. The patty handler re-init below is
     * skipped: shutdown disables dispensing anyway, and startup re-inits
     * it when the machine returns. */
    sm->shutdown_pending = false;
    app_console_print("[Main SM] Fault cleared with shutdown pending, entering shutdown\r\n");
    clear_all_axis_faults();
    for (uint8_t slot = 0U; slot < APP_SLOT_COUNT; slot++) {
      sm->cartridge[slot].faulted = false;
      // inventory is re-established by the next startup recount: drop thickness history now
      cartridge_reset_thickness(&sm->cartridge[slot]);
    }
    app_sm_enter_state(sm, APP_SHUTDOWN);
    return;
  }
  app_console_print("[Main SM] Fault cleared, returning to startup\r\n");
  // this is a hack so do not have to power cycle machine to dispense again
  clear_all_axis_faults();
  patty_handler_init(&sm->patty_handler, sm->cartridge);
  patty_handler_set_safety_state(&sm->patty_handler, true, true, true);
  for (uint8_t slot = 0U; slot < APP_SLOT_COUNT; slot++) {
    sm->cartridge[slot].faulted = false;
    // slot fault clear re-inits inventory via startup recount: drop stale thickness history
    cartridge_reset_thickness(&sm->cartridge[slot]);
  }
  app_sm_enter_state(sm, APP_STARTUP);
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

