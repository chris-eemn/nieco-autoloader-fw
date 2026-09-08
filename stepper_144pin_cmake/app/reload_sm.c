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

#include <stdbool.h>
#include <stddef.h>

#include "app_console.h"
#include "app_sm_port.h"
#include "app_task.h"
#include "cal_data.h"
#include "cartridge_recount.h"
#include "homing.h"
#include "input.h"

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
    .next_state = RELOAD_HOME_LIFTS,
    .fault_code = APP_FAULT_CODE_RELOAD_FAILED,
    .success_log = "[Reload SM] Pushers homed\r\n",
    .timeout_log = "[Reload SM] Pushers home timeout\r\n",
};

/** @brief Lifter-down homing phase configuration. */
static const homing_phase_cfg_t homing_cfg_lifter_down = {
    .name = "Lift",
    .target = HOMING_TARGET_LIFTER_DOWN,
    .next_state = RELOAD_UNLOCK_DOOR,
    .fault_code = APP_FAULT_CODE_RELOAD_FAILED,
    .success_log = "[Reload SM] Lifts homed down\r\n",
    .timeout_log = "[Reload SM] Lift homing down timeout\r\n",
};

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

/**
 * @brief Handle a generic homing-phase event for the reload sequence.
 *
 * Wraps the shared homing handler and applies the reload failure
 * side effects (result status, fault code, and state transition) when
 * the phase reports a failure.
 *
 * @param reload Application state model.
 * @param cartridges Array of cartridges.
 * @param event Event to process.
 * @param result Output result structure (modified on failure).
 * @param cfg Homing phase configuration.
 * @return Outcome indicating whether the phase is waiting, complete, or failed.
 */
static homing_event_result_enum reload_handle_homing_event(reload_sm_t* reload, cartridge_t cartridges[APP_SLOT_COUNT], const app_event_t* event,
                                                           reload_result_t* result, const homing_phase_cfg_t* cfg);

/**
 * @brief Arm the recount timeout.
 *
 * Uses the configured recount timeout, falling back to a default when
 * calibration data is missing or unconfigured.
 *
 * @return true when the timer was armed, false otherwise.
 */
static bool reload_arm_recount_timeout(void);

/**
 * @brief Fail the reload from the recount phase.
 *
 * Clears the recount-active flag, cancels the recount timeout, halts all
 * motion (so in-flight lifters do not keep driving into stall), and records
 * the failure in the result.
 *
 * @param reload Application state model.
 * @param cartridges Array of cartridges.
 * @param result Output result structure (modified).
 * @param fault_code Fault code to report.
 */
static void reload_fail_recount(reload_sm_t* reload, cartridge_t cartridges[APP_SLOT_COUNT], reload_result_t* result, uint32_t fault_code);

/**
 * @brief Arm a reload timeout and fail the reload if arming fails.
 *
 * A state waiting without its watchdog armed can hang forever, so no reload
 * state may be entered unless its timeout is confirmed armed. On a failed
 * arm, the reload fails immediately with the given fault code.
 *
 * On failure the reload state and result are set here; the caller only needs
 * to stop processing its branch. The fault code travels in the result only
 * (not `last_fault_code`): main_sm consumes the failed result in the same
 * dispatch and enters APP_FAULT, so the shared RELOAD_FAILED dispatch arm —
 * which reports `last_fault_code` only when the in-call result carries no
 * code, per the 5.2 contract — never runs for these failures.
 *
 * @param reload Application state model.
 * @param result Output result structure (modified on failure).
 * @param id Timeout ID to arm.
 * @param timeout_ms Timeout duration in milliseconds.
 * @param fault_code Fault code to report when arming fails.
 * @return True when the timeout is armed, false when the reload was failed.
 */
static bool reload_arm_timeout(reload_sm_t* reload, reload_result_t* result, app_sm_timeout_id_enum id, uint32_t timeout_ms, uint32_t fault_code);

/**
 * @brief Begin pusher homing for the reload sequence.
 *
 * Commands pusher homing for every slot and arms the motion watchdog.
 * A failed arm fails the reload immediately: the pusher moves just
 * commanded are halted because a move is demonstrably in flight and the
 * arm failure means no watchdog will bound it.
 *
 * @param sm Application state model.
 * @param cartridges Array of cartridges.
 */
static void reload_begin_homing(reload_sm_t* sm, cartridge_t cartridges[APP_SLOT_COUNT]);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

void reload_sm_start(reload_sm_t* sm) {
  if (sm != NULL) {
    /* Like startup, the reload waits for the door instead of faulting at
     * entry: RELOAD_WAIT_DOOR polls the door level and the lock sensor,
     * commands the lock when the door is closed, and begins homing only
     * once the lock is confirmed. */
    sm->state = RELOAD_WAIT_DOOR;
    sm->lift_homing_end_time_ms = 0U;
    sm->lift_homing_timeout_ms = 0U;
    sm->recount_pending_mask = 0U;
    sm->last_fault_code = APP_FAULT_CODE_NONE;
    app_sm_port_set_recount_active(false);
    // feed dispatcher to check door and lock status, and to start the sequence if both are satisfied
    app_task_post(&(app_event_t){.id = APP_EV_CONTINUE, .slot = APP_NO_SLOT, .value = 0U});
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
  else {
    app_console_print("[Reload SM] dispatch event %s slot %u value 0x%04X\r\n", app_event_id_to_str(event->id), event->slot, event->value);
    switch (reload->state) {
      case RELOAD_WAIT_DOOR:
        /* No timeout while waiting for the door (matches STARTUP_WAIT_DOOR):
         * only a fault event or shutdown breaks out of an unclosed door. */
        if ((input_get_door_closed() == true) || (event->id == APP_EV_DOOR_CLOSED)) {
          if (input_get_lock_confirmed() == true) {
            app_console_print("[Reload SM] Door already locked\r\n");
            reload_begin_homing(reload, cartridges);
          }
          else {
            app_console_print("[Reload SM] Locking door\r\n");
            reload->state = RELOAD_LOCK_DOOR;
            app_sm_port_lock_door();
            (void)reload_arm_timeout(reload, &result, APP_SM_TIMEOUT_LOCK, app_sm_port_get_lock_timeout_ms(), APP_FAULT_CODE_RELOAD_FAILED);
          }
        }
        break;

      case RELOAD_LOCK_DOOR:
        if (event->id == APP_EV_LOCK_CONFIRMED) {
          app_console_print("[Reload SM] Door locked\r\n");
          (void)app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_LOCK);
          reload_begin_homing(reload, cartridges);
        }
        else if ((event->id == APP_EV_DOOR_OPENED) || (event->id == APP_EV_LOCK_RELEASED)) {
          /* Door reopened (or lock released) while locking: unlock the
           * actuator and wait for the user to close the door again. No
           * motion halt — homing has not started in the entry states.
           * Park in RELOAD_WAIT_OPEN (no timeout armed): the level poll in
           * RELOAD_WAIT_DOOR would re-command the lock on the very next
           * event if the door sensor still reads closed, pulsing the
           * actuator. RELOAD_WAIT_OPEN only leaves on a fresh
           * APP_EV_DOOR_CLOSED edge, so the lock is re-commanded once per
           * genuine close. */
          app_console_print("[Reload SM] Door reopened while locking, waiting for close\r\n");
          (void)app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_LOCK);
          reload->state = RELOAD_WAIT_OPEN;
          app_sm_port_unlock_door();
        }
        else if ((event->id == APP_EV_TIMEOUT) && (event->value == APP_SM_TIMEOUT_LOCK)) {
          app_console_print("[Reload SM] Lock door timeout\r\n");
          result.status = RELOAD_STATUS_FAILED;
          result.fault_code = APP_FAULT_CODE_RELOAD_FAILED;
          reload->state = RELOAD_FAILED;
        }
        break;

      case RELOAD_HOME_PUSHERS: {
        if (reload_handle_homing_event(reload, cartridges, event, &result, &homing_cfg_pusher) != HOMING_EVENT_COMPLETE) {
          break;
        }
        // All homed — start lift-down homing inline
        app_console_print(homing_cfg_pusher.success_log);
        reload->state = (reload_state_enum)homing_cfg_pusher.next_state;
        app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_MOTION);
        for (uint8_t slot = 0U; slot < APP_SLOT_COUNT; slot++) {
          app_sm_port_home_lift(&cartridges[slot], DIR_LIFTER_DOWN);
        }
        (void)reload_arm_timeout(reload, &result, APP_SM_TIMEOUT_MOTION, app_sm_port_get_motion_timeout_ms(), APP_FAULT_CODE_RELOAD_FAILED);
        break;
      }

      case RELOAD_HOME_LIFTS: {
        if (reload_handle_homing_event(reload, cartridges, event, &result, &homing_cfg_lifter_down) != HOMING_EVENT_COMPLETE) {
          break;
        }
        // All homed — unlock the door
        app_console_print(homing_cfg_lifter_down.success_log);
        reload->state = (reload_state_enum)homing_cfg_lifter_down.next_state;
        app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_MOTION);
        app_sm_port_unlock_door();
        (void)reload_arm_timeout(reload, &result, APP_SM_TIMEOUT_UNLOCK, app_sm_port_get_lock_timeout_ms(), APP_FAULT_CODE_RELOAD_FAILED);
        break;
      }

      case RELOAD_UNLOCK_DOOR:
        if (event->id == APP_EV_LOCK_RELEASED) {
          (void)app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_UNLOCK);
          reload->state = RELOAD_WAIT_OPEN;
          (void)reload_arm_timeout(reload, &result, APP_SM_TIMEOUT_UNLOCK, app_sm_port_get_door_timeout_ms(), APP_FAULT_CODE_RELOAD_FAILED);
        }
        else if ((event->id == APP_EV_TIMEOUT) && (event->value == APP_SM_TIMEOUT_UNLOCK)) {
          app_console_print("[Reload SM] Unlock door timeout\r\n");
          result.status = RELOAD_STATUS_FAILED;
          result.fault_code = APP_FAULT_CODE_RELOAD_FAILED;
          reload->state = RELOAD_FAILED;
        }
        break;

      case RELOAD_WAIT_OPEN:
        if (event->id == APP_EV_DOOR_OPENED) {
          (void)app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_UNLOCK);
          reload->state = RELOAD_WAIT_CLOSE;
          (void)reload_arm_timeout(reload, &result, APP_SM_TIMEOUT_UNLOCK, app_sm_port_get_door_timeout_ms(), APP_FAULT_CODE_RELOAD_FAILED);
        }
        else if ((event->id == APP_EV_TIMEOUT) && (event->value == APP_SM_TIMEOUT_UNLOCK)) {
          app_console_print("[Reload SM] Wait door open timeout\r\n");
          result.status = RELOAD_STATUS_FAILED;
          result.fault_code = APP_FAULT_CODE_RELOAD_FAILED;
          reload->state = RELOAD_FAILED;
        }
        break;

      case RELOAD_WAIT_CLOSE:
        if (event->id == APP_EV_DOOR_CLOSED) {
          (void)app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_UNLOCK);
          reload->state = RELOAD_LOCK_DOOR_RECOUNT;
          app_sm_port_lock_door();
          (void)reload_arm_timeout(reload, &result, APP_SM_TIMEOUT_LOCK, app_sm_port_get_lock_timeout_ms(), APP_FAULT_CODE_RELOAD_FAILED);
        }
        else if ((event->id == APP_EV_TIMEOUT) && (event->value == APP_SM_TIMEOUT_UNLOCK)) {
          app_console_print("[Reload SM] Wait door close timeout\r\n");
          result.status = RELOAD_STATUS_FAILED;
          result.fault_code = APP_FAULT_CODE_RELOAD_FAILED;
          reload->state = RELOAD_FAILED;
        }
        break;

      case RELOAD_LOCK_DOOR_RECOUNT:
        if (event->id == APP_EV_LOCK_CONFIRMED) {
          (void)app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_LOCK);
          reload->state = RELOAD_RECOUNT;
          /* Reload is global: all slots recount in parallel, each lifter home-done posts COUNT_DONE. */
          reload->recount_pending_mask = (uint8_t)((1U << APP_SLOT_COUNT) - 1U);
          app_sm_port_set_recount_active(true);

          for (uint8_t slot = 0U; slot < APP_SLOT_COUNT; slot++) {
            if (cartridge_recount_start(&cartridges[slot]) == false) {
              app_console_print("[Reload SM] Recount start rejected for slot %d\r\n", (int)(slot + 1U));
              reload_fail_recount(reload, cartridges, &result, APP_FAULT_CODE_RELOAD_FAILED);
              break;
            }
          }

          if (result.status == RELOAD_STATUS_IN_PROGRESS) {
            if (reload_arm_recount_timeout() == false) {
              app_console_print("[Reload SM] Failed to arm recount timeout\r\n");
              reload_fail_recount(reload, cartridges, &result, APP_FAULT_CODE_RELOAD_FAILED);
            }
          }
        }
        else if ((event->id == APP_EV_DOOR_OPENED) || (event->id == APP_EV_LOCK_RELEASED)) {
          /* Door reopened (or lock released, which the lock sensor reports
           * independently) while locking: unlock the actuator and wait for the
           * user to close the door again, with the close timeout re-armed. */
          app_console_print("[Reload SM] Door reopened while locking, waiting for close\r\n");
          (void)app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_LOCK);
          reload->state = RELOAD_WAIT_CLOSE;
          app_sm_port_unlock_door();
          (void)reload_arm_timeout(reload, &result, APP_SM_TIMEOUT_UNLOCK, app_sm_port_get_door_timeout_ms(), APP_FAULT_CODE_RELOAD_FAILED);
        }
        else if ((event->id == APP_EV_TIMEOUT) && (event->value == APP_SM_TIMEOUT_LOCK)) {
          app_console_print("[Reload SM] Lock door timeout\r\n");
          result.status = RELOAD_STATUS_FAILED;
          result.fault_code = APP_FAULT_CODE_RELOAD_FAILED;
          reload->state = RELOAD_FAILED;
        }
        break;

      case RELOAD_RECOUNT: {
        if (event->id == APP_EV_COUNT_DONE) {
          uint8_t event_slot = event->slot;

          if ((event_slot == APP_NO_SLOT) || (event_slot == 0U) || (event_slot > APP_SLOT_COUNT)) {
            app_console_print("[Reload SM] Recount done for invalid slot %d\r\n", (int)event_slot);
            reload_fail_recount(reload, cartridges, &result, APP_FAULT_CODE_RELOAD_FAILED);
            break;
          }

          uint8_t slot_idx = (uint8_t)(event_slot - 1U);
          uint8_t slot_bit = (uint8_t)(1U << slot_idx);

          if ((reload->recount_pending_mask & slot_bit) == 0U) {
            app_console_print("[Reload SM] Unexpected recount done for slot %d\r\n", (int)event_slot);
            reload_fail_recount(reload, cartridges, &result, APP_FAULT_CODE_RELOAD_FAILED);
            break;
          }

          uint32_t fault = APP_FAULT_CODE_NONE;
          if (cartridge_recount_apply(&cartridges[slot_idx], &fault) == false) {
            reload_fail_recount(reload, cartridges, &result, fault);
            break;
          }

          reload->recount_pending_mask = (uint8_t)(reload->recount_pending_mask & ~slot_bit);

          if (reload->recount_pending_mask != 0U) {
            break;
          }

          app_sm_port_set_recount_active(false);
          app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_RECOUNT);
          /* All recounts landed — validate cartridge types before completing. */
          reload->state = RELOAD_VALIDATE;
          
          // must feed the loop to run the RELOAD_VALIDATE branch, which is synchronous and does not wait for any events
          app_task_post(&(app_event_t){.id = APP_EV_CONTINUE, .slot = APP_NO_SLOT, .value = 0U});
        }
        else if ((event->id == APP_EV_TIMEOUT) && (event->value == APP_SM_TIMEOUT_RECOUNT)) {
          app_console_print("[Reload SM] Recount timeout, pending mask 0x%02X\r\n", (unsigned int)reload->recount_pending_mask);
          reload_fail_recount(reload, cartridges, &result, APP_FAULT_CODE_RELOAD_FAILED);
        }
        else if (event->id == APP_EV_MOTION_FAILED) {
          app_console_print("[Reload SM] Motion failed during recount (axis %d)\r\n", (int)event->axis_num);
          reload_fail_recount(reload, cartridges, &result, APP_FAULT_CODE_RELOAD_FAILED);
        }
        break;
      }

      case RELOAD_VALIDATE: {
        /* Synchronous validation: no motion, no external events, no timeout.
         * A lane mismatch inhibits the discrepant cartridges but does not
         * fail the reload — the machine returns to READY with compatible
         * cartridges still usable. */
        for (uint8_t slot = 0U; slot < APP_SLOT_COUNT; slot++) {
          cartridge_determine_type(&cartridges[slot]);
        }

        bool mismatch = cartridge_validate_lane_pairs(cartridges);

        if (mismatch) {
          app_console_print("[Reload SM] Lane-pair mismatch — discrepant cartridges inhibited\r\n");
        }
        else {
          app_console_print("[Reload SM] Cartridge validation passed\r\n");
        }

        reload->state = RELOAD_COMPLETE;
        result.status = RELOAD_STATUS_DONE;
        break;
      }

      case RELOAD_COMPLETE:
        result.status = RELOAD_STATUS_DONE;
        break;

      case RELOAD_FAILED:
      default:
        result.status = RELOAD_STATUS_FAILED;
        /* In-call failures set result.fault_code themselves; this arm only
         * reports the code recorded before this dispatch (e.g. by
         * reload_begin_homing on a failed motion-timeout arm). */
        if (result.fault_code == APP_FAULT_CODE_NONE) {
          result.fault_code = reload->last_fault_code;
        }
        break;
    }
  }

  return result;
}

void reload_sm_abort(reload_sm_t* reload) {
  if (reload != NULL) {
    /* Cancel only the timeouts reload owns; dispense timers must survive. */
    (void)app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_MOTION);
    (void)app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_UNLOCK);
    (void)app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_LOCK);
    (void)app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_RECOUNT);
    app_sm_port_set_recount_active(false);
    /* Park in the entry-wait state, not RELOAD_FAILED: a fault during the
     * wait phase resumes cleanly on the next reload_sm_start() instead of
     * dead-ending in the failed state. */
    reload->state = RELOAD_WAIT_DOOR;
    reload->lift_homing_end_time_ms = 0U;
    reload->lift_homing_timeout_ms = 0U;
    reload->recount_pending_mask = 0U;
    reload->last_fault_code = APP_FAULT_CODE_NONE;
  }
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/

static homing_event_result_enum reload_handle_homing_event(reload_sm_t* reload, cartridge_t cartridges[APP_SLOT_COUNT], const app_event_t* event,
                                                           reload_result_t* result, const homing_phase_cfg_t* cfg) {
  uint32_t fault_code = APP_FAULT_CODE_NONE;

  homing_event_result_enum outcome = homing_handle_event(cartridges, event, &fault_code, cfg);

  if (outcome == HOMING_EVENT_FAILED) {
    result->status = RELOAD_STATUS_FAILED;
    result->fault_code = fault_code;
    reload->state = RELOAD_FAILED;
  }

  return outcome;
}

static bool reload_arm_recount_timeout(void) {
  /* cal_data is the single source of truth: cal_data_init() provisions the
   * factory defaults whenever flash holds nothing usable. */
  return app_sm_port_arm_timeout(APP_SM_TIMEOUT_RECOUNT, cal_data_get()->recount_timeout_ms);
}

static void reload_begin_homing(reload_sm_t* sm, cartridge_t cartridges[APP_SLOT_COUNT]) {
  sm->state = RELOAD_HOME_PUSHERS;

  for (uint8_t i = 0U; i < APP_SLOT_COUNT; i++) {
    app_sm_port_home_pusher(&cartridges[i]);
  }

  /* A failed arm fails the reload before any homing event can arrive. Halt
   * the pusher moves just commanded — a move is demonstrably in flight, and
   * the arm failure means no watchdog will bound it. */
  if (app_sm_port_arm_timeout(APP_SM_TIMEOUT_MOTION, app_sm_port_get_motion_timeout_ms()) == false) {
    app_console_print("[Reload SM] Failed to arm pusher homing timeout\r\n");
    app_sm_port_halt_all_motion(cartridges);
    sm->state = RELOAD_FAILED;
    sm->last_fault_code = APP_FAULT_CODE_RELOAD_FAILED;
  }
}

static bool reload_arm_timeout(reload_sm_t* reload, reload_result_t* result, app_sm_timeout_id_enum id, uint32_t timeout_ms, uint32_t fault_code) {
  if (app_sm_port_arm_timeout(id, timeout_ms) == false) {
    app_console_print("[Reload SM] Failed to arm timeout %s\r\n", app_sm_port_timeout_id_to_str(id));
    reload->state = RELOAD_FAILED;
    result->status = RELOAD_STATUS_FAILED;
    result->fault_code = fault_code;
    return false;
  }

  return true;
}

static void reload_fail_recount(reload_sm_t* reload, cartridge_t cartridges[APP_SLOT_COUNT], reload_result_t* result, uint32_t fault_code) {
  app_sm_port_set_recount_active(false);
  app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_RECOUNT);

  /* Halt only when a recount move is actually in flight; halting an idle axis
   * would emit spurious HOME_ABORTED events for unrelated homing. */
  if (reload->recount_pending_mask != 0U) {
    app_sm_port_halt_all_motion(cartridges);
  }

  reload->recount_pending_mask = 0U;
  reload->state = RELOAD_FAILED;
  result->status = RELOAD_STATUS_FAILED;
  result->fault_code = fault_code;
}
