/**
 * @file shutdown_sm.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief Graceful shutdown sequence state machine.
 * @version 0.1
 * @date 2026-09-10
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include "shutdown_sm.h"

#include <stdbool.h>
#include <stddef.h>

#include "app_console.h"
#include "app_sm_port.h"
#include "app_task.h"
#include "homing.h"
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

/** @brief Pusher homing phase configuration. */
static const homing_phase_cfg_t homing_cfg_pusher = {
    .name = "Pusher",
    .target = HOMING_TARGET_PUSHER,
    .next_state = SHUTDOWN_HOME_LIFTS_DOWN,
    .fault_code = APP_FAULT_CODE_NONE,
    .success_log = "[Shutdown SM] Pushers homed\r\n",
    .timeout_log = "[Shutdown SM] Pushers home timeout\r\n",
};

/** @brief Lifter-down homing phase configuration. */
static const homing_phase_cfg_t homing_cfg_lifter_down = {
    .name = "Lift",
    .target = HOMING_TARGET_LIFTER_DOWN,
    .next_state = SHUTDOWN_UNLOCK_DOOR,
    .fault_code = APP_FAULT_CODE_NONE,
    .success_log = "[Shutdown SM] Lifts homed down\r\n",
    .timeout_log = "[Shutdown SM] Lift homing down timeout\r\n",
};

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

/**
 * @brief Begin pusher homing for the shutdown sequence.
 *
 * Commands pusher homing for every slot and arms the motion watchdog. A
 * failed arm does not fail the sequence: the moves just commanded are
 * halted and the sequence advances to the next phase, because shutdown
 * must always reach the door unlock.
 *
 * @param sm Shutdown state model.
 * @param cartridges Array of cartridges.
 */
static void shutdown_begin_homing(shutdown_sm_t* sm, cartridge_t cartridges[APP_SLOT_COUNT]);

/**
 * @brief Begin lifter-down homing for the shutdown sequence.
 *
 * Commands lifter-down homing for every slot and arms the motion watchdog.
 * A failed arm does not fail the sequence: the moves just commanded are
 * halted and the sequence advances to the door unlock, because shutdown
 * must always reach the unlock.
 *
 * @param sm Shutdown state model.
 * @param cartridges Array of cartridges.
 */
static void shutdown_begin_lift_homing(shutdown_sm_t* sm, cartridge_t cartridges[APP_SLOT_COUNT]);

/**
 * @brief Advance the shutdown sequence to the next homing phase after a skip.
 *
 * Halts any motion still in flight (the stuck axis), cancels the motion
 * watchdog, and starts the next phase. Used when a homing phase timed out
 * or reported a fault: unlike reload, shutdown skips and continues. When
 * the next phase is the door unlock, the unlock is commanded here so no
 * path can park in SHUTDOWN_UNLOCK_DOOR awaiting a further event.
 *
 * @param sm Shutdown state model.
 * @param cartridges Array of cartridges.
 * @param next_state State of the next phase to enter.
 */
static void shutdown_skip_to_next_phase(shutdown_sm_t* sm, cartridge_t cartridges[APP_SLOT_COUNT], shutdown_state_enum next_state);

/**
 * @brief Command the door unlock and park the sequence in SHUTDOWN_HOLD.
 *
 * The unlock is driven once, at the moment the sequence reaches the unlock
 * phase — never deferred to a later event, so no path can leave the door
 * locked while waiting. save_state() runs here, once the mechanics are
 * parked, not while they are moving.
 *
 * @param sm Shutdown state model.
 */
static void shutdown_enter_unlock(shutdown_sm_t* sm);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

void shutdown_sm_start(shutdown_sm_t* sm, patty_handler_t* patty_handler) {
  if ((sm != NULL) && (patty_handler != NULL)) {
    /* Shutdown may be entered directly from APP_RELOAD, bypassing
     * reload_sm_abort(): clear the recount flag here so the axis glue maps
     * AXIS_EVENT_HOME_DONE to APP_EV_MOTION_DONE instead of COUNT_DONE.
     * Without this, shutdown homing completions are never recognized and
     * every phase ends by watchdog-skip with the axes left unhomed. */
    app_sm_port_set_recount_active(false);
    sm->state = SHUTDOWN_WAIT_DISPENSES;
    sm->release_seen = false;
    /* Block new dispense cycles here rather than relying on the caller:
     * the setter's door gate keeps the flag false whenever the door is not
     * closed and locked, so this call is safe from every entry state. */
    (void)patty_handler_set_dispensing_enabled(patty_handler, false);
    // feed dispatcher to evaluate the active-dispense condition immediately
    app_task_post(&(app_event_t){.id = APP_EV_CONTINUE, .slot = APP_NO_SLOT, .value = 0U});
  }
}

shutdown_result_t shutdown_sm_dispatch(shutdown_sm_t* shutdown, cartridge_t cartridges[APP_SLOT_COUNT], patty_handler_t* patty_handler,
                                       const app_event_t* event) {
  shutdown_result_t result = {
      .status = SHUTDOWN_STATUS_IN_PROGRESS,
      .fault_code = 0U,
  };

  if ((shutdown == NULL) || (cartridges == NULL) || (patty_handler == NULL) || (event == NULL)) {
    result.status = SHUTDOWN_STATUS_FAILED;
  }
  else {
    app_console_print("[Shutdown SM] dispatch event %s slot %u value 0x%04X\r\n", app_event_id_to_str(event->id), event->slot, event->value);

    /* The switch is edge-driven and may be released while the sequence is
     * still running. The sequence must still complete — mechanics parked,
     * door unlocked, state saved — so the release edge is latched here and
     * honored at the point SHUTDOWN_HOLD would otherwise park. */
    if (event->id == APP_EV_SHUTDOWN_END) {
      shutdown->release_seen = true;
    }
    // if the shutdown switch is pressed, clear the release_seen flag
    else if (event->id == APP_EV_SHUTDOWN_REQUEST) {
      shutdown->release_seen = false;
    }

    switch (shutdown->state) {
      case SHUTDOWN_WAIT_DISPENSES:
        /* Dispensing was disabled at entry, so queued-but-unstarted requests
         * cannot start. Forward motion events to the handler so in-flight
         * cycles keep advancing their retract/lift phases and settle on a
         * stall (the handler maps APP_EV_FAULT to a dispense failure); the
         * cycle's own slot watchdog was cancelled on APP_SHUTDOWN entry, so
         * this feed is also the only thing that can bound a cycle whose
         * motion event was in flight when shutdown began. */
        if ((event->id == APP_EV_MOTION_DONE) || (event->id == APP_EV_MOTION_FAILED) || (event->id == APP_EV_FAULT) ||
            ((event->id == APP_EV_TIMEOUT) && (event->value >= APP_SM_CART1_DISPENSE) && (event->value <= APP_SM_CART4_DISPENSE))) {
          (void)patty_handler_dispatch_event(patty_handler, event);
        }

        /* A dispense SM parked in DISPENSE_FAILED is not counted as active
         * but still holds a pending reservation: retire it here so the wait
         * cannot strand on it. The handler's process pass would fault the
         * cartridge on pending > remaining anyway; doing it here keeps the
         * wait bounded. */
        for (uint8_t slot = 0U; slot < APP_SLOT_COUNT; slot++) {
          if (patty_handler->dispense_sm[slot].state == DISPENSE_FAILED) {
            patty_handler->cartridge[slot].pending = 0U;
          }
        }

        if (patty_handler_has_active_dispenses(patty_handler) == false) {
          /* The queue is drained, not run: queued-but-unstarted requests are
           * skipped per the plan, and the queue must not resurface after the
           * shutdown returns through startup. Zero pending only once no cycle
           * is active: an in-flight cycle's commit path reads pending, so
           * clearing it earlier would fault a cartridge whose cycle would
           * otherwise complete cleanly. */
          for (uint8_t slot = 0U; slot < APP_SLOT_COUNT; slot++) {
            patty_handler->cartridge[slot].pending = 0U;
          }

          app_console_print("[Shutdown SM] No active dispenses, retracting pushers\r\n");
          shutdown_begin_homing(shutdown, cartridges);
        }
        break;

      case SHUTDOWN_HOME_PUSHERS: {
        /* A raw axis fault (e.g. a stall reported as APP_EV_FAULT) is the
         * axis layer's terminal event for the move just commanded: treat it
         * like a homing failure and skip, so the phase never waits for a
         * MOTION_DONE that can no longer arrive. */
        if (event->id == APP_EV_FAULT) {
          app_console_print("[Shutdown SM] Axis fault during pusher homing, skipping to lift homing\r\n");
          shutdown_skip_to_next_phase(shutdown, cartridges, (shutdown_state_enum)homing_cfg_pusher.next_state);
        }
        else if ((event->id == APP_EV_MOTION_DONE) || (event->id == APP_EV_MOTION_FAILED) ||
                 ((event->id == APP_EV_TIMEOUT) && (event->value == APP_SM_TIMEOUT_MOTION))) {
          uint32_t fault_code = APP_FAULT_CODE_NONE;
          homing_event_result_enum outcome = homing_handle_event(cartridges, event, &fault_code, &homing_cfg_pusher);

          if (outcome == HOMING_EVENT_COMPLETE) {
            app_console_print("%s", homing_cfg_pusher.success_log);
            (void)app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_MOTION);
            shutdown_begin_lift_homing(shutdown, cartridges);
          }
          else if (outcome == HOMING_EVENT_FAILED) {
            /* Motion fault or homing timeout: skip the stuck axis, keep going. */
            app_console_print("[Shutdown SM] Pusher homing incomplete, skipping to lift homing\r\n");
            shutdown_skip_to_next_phase(shutdown, cartridges, (shutdown_state_enum)homing_cfg_pusher.next_state);
          }
        }
        break;
      }

      case SHUTDOWN_HOME_LIFTS_DOWN: {
        /* As in the pusher phase, a raw axis fault is the terminal event for
         * the lift move: skip the stuck axis and still unlock the door. */
        if (event->id == APP_EV_FAULT) {
          app_console_print("[Shutdown SM] Axis fault during lift homing, skipping to unlock\r\n");
          (void)app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_MOTION);
          app_sm_port_halt_all_motion(cartridges);
          shutdown_enter_unlock(shutdown);
        }
        else if ((event->id == APP_EV_MOTION_DONE) || (event->id == APP_EV_MOTION_FAILED) ||
                 ((event->id == APP_EV_TIMEOUT) && (event->value == APP_SM_TIMEOUT_MOTION))) {
          uint32_t fault_code = APP_FAULT_CODE_NONE;
          homing_event_result_enum outcome = homing_handle_event(cartridges, event, &fault_code, &homing_cfg_lifter_down);

          if (outcome == HOMING_EVENT_COMPLETE) {
            app_console_print("%s", homing_cfg_lifter_down.success_log);
            (void)app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_MOTION);
            shutdown_enter_unlock(shutdown);
          }
          else if (outcome == HOMING_EVENT_FAILED) {
            /* Motion fault or homing timeout: skip the stuck axis, still unlock. */
            app_console_print("[Shutdown SM] Lift homing incomplete, skipping to unlock\r\n");
            (void)app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_MOTION);
            app_sm_port_halt_all_motion(cartridges);
            shutdown_enter_unlock(shutdown);
          }
        }
        break;
      }

      case SHUTDOWN_UNLOCK_DOOR:
        /* Defensive re-entry: every normal path commands the unlock when it
         * enters this state and parks in SHUTDOWN_HOLD in the same dispatch.
         * If a re-entered state is ever observed, complete the park now. */
        shutdown_enter_unlock(shutdown);
        break;

      case SHUTDOWN_HOLD:
        if (shutdown->release_seen == true) {
          app_console_print("[Shutdown SM] Shutdown switch released, returning to startup\r\n");
          result.status = SHUTDOWN_STATUS_DONE;
        }
        break;

      default:
        result.status = SHUTDOWN_STATUS_FAILED;
        break;
    }
  }

  return result;
}

void shutdown_sm_abort(shutdown_sm_t* shutdown) {
  if (shutdown != NULL) {
    /* Belt-and-braces against the blanket cancel in app_sm_enter_state for
     * callers that abort without a state transition. */
    (void)app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_MOTION);
    (void)app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_UNLOCK);
    shutdown->state = SHUTDOWN_WAIT_DISPENSES;
    shutdown->release_seen = false;
  }
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/

static void shutdown_begin_homing(shutdown_sm_t* sm, cartridge_t cartridges[APP_SLOT_COUNT]) {
  sm->state = SHUTDOWN_HOME_PUSHERS;

  for (uint8_t slot = 0U; slot < APP_SLOT_COUNT; slot++) {
    /* A rejected pusher home leaves pusher_homed false: the phase then
     * ends on the motion watchdog below, which skips and continues. */
    (void)app_sm_port_home_pusher(&cartridges[slot]);
  }

  /* A failed arm does not fail the sequence: halt the moves just commanded
   * (no watchdog will bound them) and advance to the next phase. */
  if (app_sm_port_arm_timeout(APP_SM_TIMEOUT_MOTION, app_sm_port_get_motion_timeout_ms()) == false) {
    app_console_print("[Shutdown SM] Failed to arm pusher homing timeout, skipping pusher homing\r\n");
    shutdown_skip_to_next_phase(sm, cartridges, (shutdown_state_enum)homing_cfg_pusher.next_state);
  }
}

static void shutdown_begin_lift_homing(shutdown_sm_t* sm, cartridge_t cartridges[APP_SLOT_COUNT]) {
  sm->state = SHUTDOWN_HOME_LIFTS_DOWN;

  for (uint8_t slot = 0U; slot < APP_SLOT_COUNT; slot++) {
    /* As in the pusher phase, a rejected home is bounded by the watchdog. */
    (void)app_sm_port_home_lift(&cartridges[slot], DIR_LIFTER_DOWN);
  }

  /* A failed arm does not fail the sequence: halt the moves just commanded
   * (no watchdog will bound them) and advance to the door unlock. */
  if (app_sm_port_arm_timeout(APP_SM_TIMEOUT_MOTION, app_sm_port_get_motion_timeout_ms()) == false) {
    app_console_print("[Shutdown SM] Failed to arm lift homing timeout, skipping lift homing\r\n");
    (void)app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_MOTION);
    app_sm_port_halt_all_motion(cartridges);
    shutdown_enter_unlock(sm);
  }
}

static void shutdown_skip_to_next_phase(shutdown_sm_t* sm, cartridge_t cartridges[APP_SLOT_COUNT], shutdown_state_enum next_state) {
  (void)app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_MOTION);
  app_sm_port_halt_all_motion(cartridges);
  sm->state = next_state;

  if (next_state == SHUTDOWN_HOME_LIFTS_DOWN) {
    shutdown_begin_lift_homing(sm, cartridges);
  }
  else if (next_state == SHUTDOWN_UNLOCK_DOOR) {
    shutdown_enter_unlock(sm);
  }
  else {
    /* SHUTDOWN_HOME_PUSHERS is not a skip target: nothing to start here. */
  }
}

static void shutdown_enter_unlock(shutdown_sm_t* sm) {
  app_sm_port_unlock_door();
  sm->state = SHUTDOWN_HOLD;
  app_sm_port_save_state();

  /* The switch may have been released partway through the sequence. The
   * sequence is complete at this point, so the latched release is honored
   * immediately: the dispatch returns DONE and the main SM returns to
   * startup without waiting for a release edge that already happened. */
  if (sm->release_seen == true) {
    app_console_print("[Shutdown SM] Shutdown switch released during sequence, returning to startup\r\n");
  }
}
