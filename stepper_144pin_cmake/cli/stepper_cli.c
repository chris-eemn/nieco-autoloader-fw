/**
 * @file stepper_cli.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief Stepper CLI command handlers for the "test" command group.
 *
 *   test set auto  1          -- start continuous CW->3s->CCW->3s loop (motor 1 only)
 *   test set auto  0          -- stop immediately (also aborts manual moves on all motors)
 *   test set cw    <motor>    -- run one CW move on <motor> (1..STEPPER_CTRL_MAX_MOTORS), then idle
 *   test set ccw   <motor>    -- run one CCW move on <motor> (1..STEPPER_CTRL_MAX_MOTORS), then idle
 *   test set home  <motor>    -- home <motor> (1..STEPPER_CTRL_MAX_MOTORS)
 *   test set rpm   <value>    -- set move speed in RPM (shared by all motors)
 *   test set step  <value>    -- set microstep count per leg (shared by all motors)
 *   test set clear <ignored>  -- full fault reset for all motors (axis latch + DRV8424 sleep/wake)
 *   test get enc              -- print the current encoder count for every motor
 *   test get <param>          -- print the current value
 *   test list                 -- list all parameters
 *
 * cw/ccw on different motor numbers may be issued back-to-back - each
 * addresses an independent axis/stepper instance, and the shared step-timer ISR
 * (see stepper.c) advances every running motor on every tick, so e.g.
 * "test set cw 1" followed immediately by "test set cw 2" runs both motors
 * concurrently rather than queuing one behind the other. "test set clear"
 * resets faults on every motor at once, regardless of the value argument.
 *
 * @version 0.4
 * @date 2026-08-07
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include "stepper_cli.h"
#include "stepper_ctrl.h"
#include "axis.h"
#include "app_console.h"
#include "app_task.h"
#include <string.h>

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

static const char* s_param_names[STEPPER_CLI_NUM_PARAMS] = {
    [STEPPER_CLI_AUTO] = "auto",             [STEPPER_CLI_CW] = "cw",       [STEPPER_CLI_CCW] = "ccw",           [STEPPER_CLI_HOME] = "home",
    [STEPPER_CLI_RPM] = "rpm",               [STEPPER_CLI_STEP] = "step",   [STEPPER_CLI_CLEAR] = "clear",       [STEPPER_CLI_ENC] = "enc",
    [STEPPER_CLI_QUEUE] = "queue",           [STEPPER_CLI_FAKE_HOME] = "fake_home", [STEPPER_CLI_FAKE_HOME_DIST] = "fake_home_dist",
};

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

static stepper_cli_param_enum lookup_param(const char* name);
static axis_t* stepper_cli_lookup_axis(int32_t motor_num);
static void stepper_cli_home(int32_t motor_num);
static void enqueue_patties(uint16_t n);
static void stepper_cli_print_fake_home(void);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

void stepper_cli_set_handler(char* param, int32_t val) {
  if (param == NULL) {
    return;
  }

  switch (lookup_param(param)) {
    case STEPPER_CLI_AUTO:
      if (val != 0) {
        stepper_ctrl_send_cmd(STEPPER_CMD_AUTO_START);
        app_console_print("[STEPPER] Auto cycle started.\r\n");
      }
      else {
        stepper_ctrl_request_stop();
        app_console_print("[STEPPER] Stopped.\r\n");
      }
      break;

    case STEPPER_CLI_CW: {
      axis_t* ax = stepper_cli_lookup_axis(val);
      if (ax == NULL) {
        break;
      }
      stepper_status_enum ret = axis_move(ax, stepper_ctrl_get_steps(), stepper_ctrl_get_rpm(), STEPPER_DIR_CW);
      if (ret == STEPPER_OK) {
        app_console_print("[STEPPER] Motor %ld: manual CW move started.\r\n", val);
      }
      else {
        app_console_print("[STEPPER] Motor %ld: CW start failed: %d (axis=%d)\r\n", val, (int)ret, (int)axis_get_status(ax));
      }
      break;
    }

    case STEPPER_CLI_CCW: {
      axis_t* ax = stepper_cli_lookup_axis(val);
      if (ax == NULL) {
        break;
      }
      stepper_status_enum ret = axis_move(ax, stepper_ctrl_get_steps(), stepper_ctrl_get_rpm(), STEPPER_DIR_CCW);
      if (ret == STEPPER_OK) {
        app_console_print("[STEPPER] Motor %ld: manual CCW move started.\r\n", val);
      }
      else {
        app_console_print("[STEPPER] Motor %ld: CCW start failed: %d (axis=%d)\r\n", val, (int)ret, (int)axis_get_status(ax));
      }
      break;
    }

    case STEPPER_CLI_HOME:
      stepper_cli_home(val);
      break;

    case STEPPER_CLI_QUEUE:
      if (val <= 0) {
        app_console_print("[STEPPER] Queue count must be > 0.\r\n");
      }
      else {
        enqueue_patties(val);
      }
      break;

    case STEPPER_CLI_FAKE_HOME: {
      if (val != 0) {
        uint32_t dist = 0U;
        axis_get_fake_homing(NULL, &dist);
        /* Rejected inside axis_set_fake_homing() if dist == 0. */
        axis_set_fake_homing(true, dist);
      }
      else {
        axis_set_fake_homing(false, 0U);
      }
      stepper_cli_print_fake_home();
      break;
    }

    case STEPPER_CLI_FAKE_HOME_DIST: {
      if (val <= 0) {
        app_console_print("[STEPPER] fake_home_dist must be > 0 usteps.\r\n");
      }
      else {
        bool enabled = false;
        axis_get_fake_homing(&enabled, NULL);
        axis_set_fake_homing(enabled, (uint32_t)val);
        stepper_cli_print_fake_home();
      }
      break;
    }

    case STEPPER_CLI_RPM:
      if (val <= 0) {
        app_console_print("[STEPPER] RPM must be > 0.\r\n");
      }
      else {
        stepper_ctrl_set_rpm((uint32_t)val);
        app_console_print("[STEPPER] rpm = %ld\r\n", val);
      }
      break;

    case STEPPER_CLI_STEP:
      if (val <= 0) {
        app_console_print("[STEPPER] Steps must be > 0.\r\n");
      }
      else {
        stepper_ctrl_set_steps((uint32_t)val);
        app_console_print("[STEPPER] steps = %ld\r\n", val);
      }
      break;

    case STEPPER_CLI_CLEAR: {
      for (uint8_t motor_num = 1U; motor_num <= STEPPER_CTRL_MAX_MOTORS; motor_num++) {
        axis_t* ax = stepper_ctrl_get_axis(motor_num);
        if (ax != NULL) {
          axis_fault_reset(ax);
        }
      }
      app_console_print("[STEPPER] All motors: fault reset requested - supervisor will complete in ~2 ticks.\r\n");
      app_task_post(&(app_event_t){.id = APP_EV_FAULT_CLEARED, .slot = APP_NO_SLOT, .value = 0U});
      break;
    }

    default:
      app_console_print("[STEPPER] Unknown param: %s\r\n", param);
      break;
  }
}

void stepper_cli_get_handler(char* param) {
  if (param == NULL) {
    return;
  }

  switch (lookup_param(param)) {
    case STEPPER_CLI_AUTO:
      app_console_print("[STEPPER] running = %u\r\n", stepper_ctrl_is_running());
      break;

    case STEPPER_CLI_CW:
    case STEPPER_CLI_CCW: {
      axis_t* ax = stepper_ctrl_get_axis(1U);
      axis_status_enum st = (ax != NULL) ? axis_get_status(ax) : AXIS_STATUS_FAULT;
      app_console_print("[STEPPER] motor 1 axis status = %d\r\n", (int)st);
      break;
    }

    case STEPPER_CLI_RPM:
      app_console_print("[STEPPER] rpm = %lu\r\n", stepper_ctrl_get_rpm());
      break;

    case STEPPER_CLI_STEP:
      app_console_print("[STEPPER] steps = %lu\r\n", stepper_ctrl_get_steps());
      break;

    case STEPPER_CLI_FAKE_HOME:
      stepper_cli_print_fake_home();
      break;

    case STEPPER_CLI_FAKE_HOME_DIST: {
      uint32_t dist = 0U;
      axis_get_fake_homing(NULL, &dist);
      app_console_print("[STEPPER] fake_home_dist = %lu usteps\r\n", dist);
      break;
    }

    case STEPPER_CLI_ENC: {
      for (uint8_t motor_num = 1U; motor_num <= STEPPER_CTRL_MAX_MOTORS; motor_num++) {
        axis_t* ax = stepper_ctrl_get_axis(motor_num);
        app_console_print("[STEPPER] encoder%u count = %ld\r\n", (uint32_t)motor_num, axis_get_encoder_count(ax));
      }
      break;
    }

    default:
      app_console_print("[STEPPER] Unknown param: %s\r\n", param);
      break;
  }
}

void stepper_cli_list_handler(void) {
  app_console_print("Stepper test parameters:\r\n");
  app_console_print("  auto  1|0      start/stop continuous CW->3s->CCW->3s loop (motor 1 only)\r\n");
  app_console_print("  cw    <motor>  run one CW move on <motor> (1-%u) then idle\r\n", STEPPER_CTRL_MAX_MOTORS);
  app_console_print("  ccw   <motor>  run one CCW move on <motor> (1-%u) then idle\r\n", STEPPER_CTRL_MAX_MOTORS);
  app_console_print("  home  <motor>  home <motor> (1-%u)\r\n", STEPPER_CTRL_MAX_MOTORS);
  app_console_print("  rpm   <value>  move speed in RPM, shared by all motors (default %u)\r\n", STEPPER_CTRL_DEFAULT_RPM);
  app_console_print("  step  <value>  microsteps per leg, shared by all motors (default %u)\r\n", STEPPER_CTRL_DEFAULT_STEPS);
  app_console_print("  clear <ignored>  full fault reset for all motors (axis latch + DRV8424 sleep/wake)\r\n");
  app_console_print("  enc            current encoder count for every motor\r\n");
  app_console_print("  queue <n>      enqueue patties\r\n");
  app_console_print("  fake_home <0|1>  simulated endstop for open-loop bench testing (all axes, RAM-only, OFF at boot)\r\n");
  app_console_print("  fake_home_dist <usteps>  seek distance in motor microsteps before the FAKE endstop fires\r\n");
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/

/**
 * @brief Validate a motor number and start its non-blocking homing sequence.
 * @param motor_num 1-based motor number, as parsed from the CLI value argument.
 */
static void stepper_cli_home(int32_t motor_num) {
  axis_t* ax = stepper_cli_lookup_axis(motor_num);
  if (ax == NULL) {
    return;
  }

  stepper_status_enum status = stepper_ctrl_home((uint8_t)motor_num, STEPPER_DIR_CW);
  if (status == STEPPER_OK) {
    app_console_print("[STEPPER] Motor %ld: homing started.\r\n", motor_num);
  }
  else {
    app_console_print("[STEPPER] Motor %ld: homing start failed: %d (axis=%d)\r\n", motor_num, (int)status, (int)axis_get_status(ax));
  }
}

/**
 * @brief Resolve a CLI-supplied motor number to its bound axis handle,
 *        printing a CLI error message if the number is out of range or
 *        no axis has been bound to that slot.
 * @param motor_num 1-based motor number, as parsed from the CLI value argument.
 * @return Axis handle on success, or NULL if motor_num is invalid/unbound.
 */
static axis_t* stepper_cli_lookup_axis(int32_t motor_num) {
  if ((motor_num < 1) || (motor_num > (int32_t)STEPPER_CTRL_MAX_MOTORS)) {
    app_console_print("[STEPPER] Motor number must be 1-%u.\r\n", STEPPER_CTRL_MAX_MOTORS);
    return NULL;
  }

  axis_t* ax = stepper_ctrl_get_axis((uint8_t)motor_num);
  if (ax == NULL) {
    app_console_print("[STEPPER] Motor %ld not initialised.\r\n", motor_num);
  }

  return ax;
}

/**
 * @brief Look up a parameter by name string.
 * @param name Parameter name. Must not be NULL.
 * @return Matching enum value, or STEPPER_CLI_NUM_PARAMS if not found.
 */
static stepper_cli_param_enum lookup_param(const char* name) {
  if (name == NULL) {
    return STEPPER_CLI_NUM_PARAMS;
  }

  for (int32_t i = 0; i < (int32_t)STEPPER_CLI_NUM_PARAMS; i++) {
    if (strcmp(s_param_names[i], name) == 0) {
      return (stepper_cli_param_enum)i;
    }
  }

  return STEPPER_CLI_NUM_PARAMS;
}

/**
 * @brief Print the current fake-homing enable flag and seek distance.
 */
static void stepper_cli_print_fake_home(void) {
  bool enabled = false;
  uint32_t dist = 0U;

  axis_get_fake_homing(&enabled, &dist);
  app_console_print("[STEPPER] fake_home = %u\r\n", enabled ? 1U : 0U);
  app_console_print("[STEPPER] fake_home_dist = %lu usteps\r\n", dist);
}

static void enqueue_patties(uint16_t n) {
  app_event_t event;

  event.id = APP_EV_DISPENSE_REQUEST;
  event.product_type = CARTRIDGE_TYPE_WHOPPER;
  event.value = n;
  app_task_post(&event);
}
