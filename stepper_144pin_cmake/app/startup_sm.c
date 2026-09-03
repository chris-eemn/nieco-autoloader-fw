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

#include "startup_sm.h"

#include <stddef.h>
#include <stdbool.h>

#include "app_console.h"
#include "app_sm_port.h"
#include "app_task.h"
#include "autoloader_types.h"
#include "homing.h"
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

/** @brief Pusher homing phase configuration. */
static const homing_phase_cfg_t homing_cfg_pusher = {
    .name = "Pusher",
    .target = HOMING_TARGET_PUSHER,
    .next_state = STARTUP_HOME_LIFTS_DOWN,
    .fault_code = APP_FAULT_CODE_STARTUP_FAILED,
    .success_log = "[Startup SM] Pushers homed\r\n",
    .timeout_log = "[Startup SM] Pushers home timeout\r\n",
};

/** @brief Lifter-down homing phase configuration. */
static const homing_phase_cfg_t homing_cfg_lifter_down = {
    .name = "Lift",
    .target = HOMING_TARGET_LIFTER_DOWN,
    .next_state = STARTUP_HOME_LIFTS_DELAY,
    .fault_code = APP_FAULT_CODE_STARTUP_FAILED,
    .success_log = "[Startup SM] Lifts homed down\r\n",
    .timeout_log = "[Startup SM] Lift homing down timeout\r\n",
};

/** @brief Lifter-up homing phase configuration. */
static const homing_phase_cfg_t homing_cfg_lifter_up = {
    .name = "Lift",
    .target = HOMING_TARGET_LIFTER_UP,
    .next_state = STARTUP_DETERMINE_TYPE,
    .fault_code = APP_FAULT_CODE_STARTUP_FAILED,
    .success_log = "[Startup SM] Lifts homed up\r\n",
    .timeout_log = "[Startup SM] Lifts home up timeout\r\n",
};

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/
static void startup_begin_homing(startup_sm_t* sm, cartridge_t cartridges[APP_SLOT_COUNT]);
static void startup_begin_lifter_homing(cartridge_t cartridges[APP_SLOT_COUNT], homing_target_enum target);
static void startup_post_home_done(void);

/**
 * @brief Handle a generic homing-phase event for the startup sequence.
 *
 * Wraps the shared homing handler and applies the startup failure
 * side effects (result status, fault code, and state transition) when
 * the phase reports a failure.
 *
 * @param sm Application state model.
 * @param cartridges Array of cartridges.
 * @param event Event to process.
 * @param result Output result structure (modified on failure).
 * @param cfg Homing phase configuration.
 * @return Outcome indicating whether the phase is waiting, complete, or failed.
 */
static homing_event_result_enum startup_handle_homing_event(startup_sm_t* sm, cartridge_t cartridges[APP_SLOT_COUNT], const app_event_t* event,
                                                             startup_result_t* result, const homing_phase_cfg_t* cfg);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

void startup_sm_start(startup_sm_t* sm) {
  if (sm != NULL) {
    sm->state = STARTUP_WAIT_DOOR;
    // feed dispatcher to check door and lock status, and to start the sequence if both are satisfied
    app_task_post(&(app_event_t){.id = APP_EV_CONTINUE, .slot = APP_NO_SLOT, .value = 0U});
  }
}

void startup_sm_abort(startup_sm_t* sm) {
  if (sm != NULL) {
    sm->state = STARTUP_WAIT_DOOR;
    sm->lift_homing_end_time_ms = 0U;
    sm->lift_homing_timeout_ms = 0U;
    app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_LOCK);
    app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_STARTUP_DELAY);
    app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_MOTION);
  }
}

startup_result_t startup_sm_dispatch(startup_sm_t* sm, cartridge_t cartridges[APP_SLOT_COUNT], const app_event_t* event) {
  if ((sm == NULL) || (event == NULL) || (cartridges == NULL)) {
    return (startup_result_t){.status = STARTUP_STATUS_FAILED, .fault_code = APP_FAULT_CODE_STARTUP_FAILED};
  }

  startup_result_t result = {.status = STARTUP_STATUS_IN_PROGRESS, .fault_code = APP_FAULT_CODE_NONE};

  switch (sm->state) {
    case STARTUP_WAIT_DOOR:
      if ((input_get_door_closed() == true) || (event->id == APP_EV_DOOR_CLOSED)) {
        if (input_get_lock_confirmed() == true) {
          app_console_print("[Startup SM] Door already locked\r\n");
          startup_begin_homing(sm, cartridges);
          sm->state = STARTUP_HOME_PUSHERS;
        }
        else {
          app_console_print("[Startup SM] Locking door\r\n");

          sm->state = STARTUP_LOCK_DOOR;
          app_sm_port_lock_door();
          app_sm_port_arm_timeout(APP_SM_TIMEOUT_LOCK, 5000U);
        }
      }
      break;

    case STARTUP_LOCK_DOOR:
      if (event->id == APP_EV_LOCK_CONFIRMED) {
        app_console_print("[Startup SM] Door locked\r\n");
        app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_LOCK);
        sm->state = STARTUP_HOME_PUSHERS;
        startup_begin_homing(sm, cartridges);
      }
      else if ((event->id == APP_EV_TIMEOUT) && (event->value == APP_SM_TIMEOUT_LOCK)) {
        result.status = STARTUP_STATUS_FAILED;
        result.fault_code = APP_FAULT_CODE_STARTUP_FAILED;
        sm->state = STARTUP_FAILED;
      }
      else {
        app_console_print("[Startup SM] Unexpected event in STARTUP_LOCK_DOOR: id=%d, value=%d\r\n", event->id, event->value);
      }
      break;

    case STARTUP_HOME_PUSHERS: {
      if (startup_handle_homing_event(sm, cartridges, event, &result, &homing_cfg_pusher) != HOMING_EVENT_COMPLETE) {
        break;
      }
      // All homed — start lift-down homing inline
      app_console_print(homing_cfg_pusher.success_log);
      sm->state = (startup_state_enum)homing_cfg_pusher.next_state;
      app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_MOTION);
      startup_begin_lifter_homing(cartridges, HOMING_TARGET_LIFTER_DOWN);
      break;
    }

    case STARTUP_HOME_LIFTS_DOWN: {
      if (startup_handle_homing_event(sm, cartridges, event, &result, &homing_cfg_lifter_down) != HOMING_EVENT_COMPLETE) {
        break;
      }
      // All homed — transition to delay state
      app_console_print(homing_cfg_lifter_down.success_log);
      sm->state = (startup_state_enum)homing_cfg_lifter_down.next_state;
      app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_MOTION);
      app_sm_port_arm_timeout(APP_SM_TIMEOUT_STARTUP_DELAY, LIFT_HOMING_SETTLE_DELAY_MS);
      break;
    }

    case STARTUP_HOME_LIFTS_DELAY:
      if ((event->id == APP_EV_TIMEOUT) && (event->value == APP_SM_TIMEOUT_STARTUP_DELAY)) {
        sm->state = STARTUP_HOME_LIFTS_UP;
        startup_begin_lifter_homing(cartridges, HOMING_TARGET_LIFTER_UP);
      }
      else {
        app_console_print("[Startup SM] Unexpected event in STARTUP_HOME_LIFTS_DELAY: id=%d, value=%d\r\n", event->id, event->value);
      }
      break;

    case STARTUP_HOME_LIFTS_UP: {
      if (startup_handle_homing_event(sm, cartridges, event, &result, &homing_cfg_lifter_up) != HOMING_EVENT_COMPLETE) {
        break;
      }
      // All homed — transition to determine type
      app_console_print(homing_cfg_lifter_up.success_log);
      sm->state = (startup_state_enum)homing_cfg_lifter_up.next_state;
      app_sm_port_cancel_timeout();
      app_sm_port_cancel_timeout_id(APP_SM_TIMEOUT_MOTION);
      startup_post_home_done();
      break;
    }

    case STARTUP_DETERMINE_TYPE:
      if (event->id == APP_EV_HOME_DONE) {
        for (uint8_t i = 0U; i < APP_SLOT_COUNT; i++) {
          cartridge_determine_type(&cartridges[i]);
          app_console_print("[Startup SM] Cartridge %d type determined: %s\r\n", i + 1U, cartridge_type_to_string(cartridges[i].type));

          app_console_print("[Startup SM] Simulating cartridge %d type as WHOPPER for testing purposes\r\n", i + 1U);
          cartridges[i].type = CARTRIDGE_TYPE_WHOPPER;
        }

        const app_event_t new_event = {
            .id = APP_EV_DETERMINE_TYPE_DONE,
            .slot = APP_NO_SLOT,
            .value = 0U,
        };
        app_task_post(&new_event);

        sm->state = STARTUP_COUNT_CARTRIDGES;
      }
      else {
        app_console_print("[Startup SM] Unexpected event in STARTUP_DETERMINE_TYPE: id=%d, value=%d\r\n", event->id, event->value);
      }

      break;

    case STARTUP_COUNT_CARTRIDGES:
      if (event->id == APP_EV_DETERMINE_TYPE_DONE) {
        app_console_print("[Startup SM] Counting cartridges\r\n");
        for (uint8_t i = 0U; i < APP_SLOT_COUNT; i++) {
          app_sm_port_count_cartridges(&cartridges[i]);
          app_console_print("[Startup SM] Counting cartridges for slot %d, homing_enc_ticks = %d\r\n", i + 1U,
                            cartridges[i].lifter_home_up_encoder_counts);

          app_console_print("[Startup SM] Simulating cartridge %d number of items as 10 for testing purposes\r\n", i + 1U);
          cartridges[i].remaining = 10U;
        }
        sm->state = STARTUP_COMPLETE;
        result.status = STARTUP_STATUS_DONE;
      }
      else if (event->id == APP_EV_TIMEOUT) {
        sm->state = STARTUP_FAILED;
        result.status = STARTUP_STATUS_FAILED;
      }
      else {
        app_console_print("[Startup SM] Unexpected event in STARTUP_COUNT_CARTRIDGES: id=%d, value=%d\r\n", event->id, event->value);
      }

      break;

    case STARTUP_FAILED:
      app_console_print("[Startup SM] Startup failed\r\n");
      break;
    case STARTUP_COMPLETE:
      app_console_print("[Startup SM] Startup complete\r\n");
      result.status = STARTUP_STATUS_DONE;
      break;
    default:
      app_console_print("[Startup SM] Unexpected event in state %d: id=%d, value=%d\r\n", sm->state, event->id, event->value);
      break;
  }

  return result;
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/

/**
 * @brief Handle a generic homing-phase event for the startup sequence.
 *
 * Wraps the shared homing handler and applies the startup failure
 * side effects (result status, fault code, and state transition) when
 * the phase reports a failure.
 *
 * @param sm Application state model.
 * @param cartridges Array of cartridges.
 * @param event Event to process.
 * @param result Output result structure (modified on failure).
 * @param cfg Homing phase configuration.
 * @return Outcome indicating whether the phase is waiting, complete, or failed.
 */
static homing_event_result_enum startup_handle_homing_event(startup_sm_t* sm, cartridge_t cartridges[APP_SLOT_COUNT], const app_event_t* event,
                                                             startup_result_t* result, const homing_phase_cfg_t* cfg) {
  uint32_t fault_code = APP_FAULT_CODE_NONE;

  homing_event_result_enum outcome = homing_handle_event(cartridges, event, &fault_code, cfg);

  if (outcome == HOMING_EVENT_FAILED) {
    result->status = STARTUP_STATUS_FAILED;
    result->fault_code = fault_code;
    sm->state = STARTUP_FAILED;
  }

  return outcome;
}

/**
 * @brief Begin homing lifters in the direction selected by the target.
 * @param cartridges Array of cartridges.
 * @param target Lifter homing target.
 */
static void startup_begin_lifter_homing(cartridge_t cartridges[APP_SLOT_COUNT], homing_target_enum target) {
  if ((cartridges != NULL) && ((target == HOMING_TARGET_LIFTER_DOWN) || (target == HOMING_TARGET_LIFTER_UP))) {
    for (uint8_t i = 0U; i < APP_SLOT_COUNT; i++) {
      if (target == HOMING_TARGET_LIFTER_DOWN) {
        app_sm_port_home_lift(&cartridges[i], DIR_LIFTER_DOWN);
      }
      else {
        app_sm_port_home_lift(&cartridges[i], DIR_LIFTER_UP);
      }
      app_sm_port_arm_timeout(APP_SM_TIMEOUT_MOTION, 10000);
    }
  }
}

/**
 * @brief Post a HOME_DONE event to the task queue.
 * @param sm Application state model.
 */
static void startup_post_home_done(void) {
  const app_event_t new_event = {
      .id = APP_EV_HOME_DONE,
      .slot = APP_NO_SLOT,
      .value = 0U,
  };
  app_task_post(&new_event);
}

/**
 * @brief Begin pusher homing.
 * @param sm Application state model.
 * @param cartridges Array of cartridges.
 */
static void startup_begin_homing(startup_sm_t* sm, cartridge_t cartridges[APP_SLOT_COUNT]) {
  sm->state = STARTUP_HOME_PUSHERS;
  for (uint8_t i = 0U; i < APP_SLOT_COUNT; i++) {
    app_sm_port_home_pusher(&cartridges[i]);
    app_sm_port_arm_timeout(APP_SM_TIMEOUT_MOTION, 10000);
  }
}
