/**
 * @file stepper_cli.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief Stepper CLI command handlers for the "test" command group.
 *
 *   test set auto  1          -- start continuous CW->3s->CCW->3s loop (motor 1 only)
 *   test set auto  0          -- stop immediately (also aborts manual moves on all motors)
 *   test set cw    <motor>    -- run one CW move on <motor> (1..STEPPER_CTRL_MAX_MOTORS), then idle
 *   test set ccw   <motor>    -- run one CCW move on <motor> (1..STEPPER_CTRL_MAX_MOTORS), then idle
 *   test set rpm   <value>    -- set move speed in RPM (shared by all motors)
 *   test set step  <value>    -- set microstep count per leg (shared by all motors)
 *   test set clear <motor>    -- full fault reset for <motor> (axis latch + DRV8424 sleep/wake)
 *   test get enc              -- print the current encoder count for every motor
 *   test get <param>          -- print the current value
 *   test list                 -- list all parameters
 *
 * cw/ccw/clear on different motor numbers may be issued back-to-back — each
 * addresses an independent axis/stepper instance, and the shared step-timer ISR
 * (see stepper.c) advances every running motor on every tick, so e.g.
 * "test set cw 1" followed immediately by "test set cw 2" runs both motors
 * concurrently rather than queuing one behind the other.
 *
 * @version 0.3
 * @date 2026-07-02
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
#include <string.h>

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

static const char *s_param_names[STEPPER_CLI_NUM_PARAMS] = {
  [STEPPER_CLI_AUTO]  = "auto",
  [STEPPER_CLI_CW]    = "cw",
  [STEPPER_CLI_CCW]   = "ccw",
  [STEPPER_CLI_RPM]   = "rpm",
  [STEPPER_CLI_STEP]  = "step",
  [STEPPER_CLI_CLEAR] = "clear",
  [STEPPER_CLI_ENC]   = "enc",
};

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

static stepper_cli_param_enum lookup_param(const char *name);
static axis_t *stepper_cli_lookup_axis(int32_t motor_num);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

void stepper_cli_set_handler(char *param, int32_t val) {
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
      axis_t *ax = stepper_cli_lookup_axis(val);
      if (ax == NULL) {
        break;
      }
      stepper_status_enum ret = axis_move(ax, stepper_ctrl_get_steps(),
                                          stepper_ctrl_get_rpm(), STEPPER_DIR_CW);
      if (ret == STEPPER_OK) {
        app_console_print("[STEPPER] Motor %ld: manual CW move started.\r\n", val);
      }
      else {
        app_console_print("[STEPPER] Motor %ld: CW start failed: %d (axis=%d)\r\n",
                          val, (int)ret, (int)axis_get_status(ax));
      }
      break;
    }

    case STEPPER_CLI_CCW: {
      axis_t *ax = stepper_cli_lookup_axis(val);
      if (ax == NULL) {
        break;
      }
      stepper_status_enum ret = axis_move(ax, stepper_ctrl_get_steps(),
                                          stepper_ctrl_get_rpm(), STEPPER_DIR_CCW);
      if (ret == STEPPER_OK) {
        app_console_print("[STEPPER] Motor %ld: manual CCW move started.\r\n", val);
      }
      else {
        app_console_print("[STEPPER] Motor %ld: CCW start failed: %d (axis=%d)\r\n",
                          val, (int)ret, (int)axis_get_status(ax));
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
      axis_t *ax = stepper_cli_lookup_axis(val);
      if (ax == NULL) {
        break;
      }
      axis_fault_reset(ax);
      app_console_print("[STEPPER] Motor %ld: fault reset requested — supervisor will complete in ~2 ticks.\r\n",
                        val);
      break;
    }

    default:
      app_console_print("[STEPPER] Unknown param: %s\r\n", param);
      break;
  }
}

void stepper_cli_get_handler(char *param) {
  if (param == NULL) {
    return;
  }

  switch (lookup_param(param)) {
    case STEPPER_CLI_AUTO:
      app_console_print("[STEPPER] running = %u\r\n", stepper_ctrl_is_running());
      break;

    case STEPPER_CLI_CW:
    case STEPPER_CLI_CCW: {
      axis_t *ax = stepper_ctrl_get_axis(1U);
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

    case STEPPER_CLI_ENC: {
      for (uint8_t motor_num = 1U; motor_num <= STEPPER_CTRL_MAX_MOTORS; motor_num++) {
        axis_t *ax = stepper_ctrl_get_axis(motor_num);
        app_console_print("[STEPPER] encoder%u count = %ld\r\n",
                          (uint32_t)motor_num, axis_get_encoder_count(ax));
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
  app_console_print("  cw    <motor>  run one CW move on <motor> (1-%u) then idle\r\n",
                    STEPPER_CTRL_MAX_MOTORS);
  app_console_print("  ccw   <motor>  run one CCW move on <motor> (1-%u) then idle\r\n",
                    STEPPER_CTRL_MAX_MOTORS);
  app_console_print("  rpm   <value>  move speed in RPM, shared by all motors (default %u)\r\n",
                    STEPPER_CTRL_DEFAULT_RPM);
  app_console_print("  step  <value>  microsteps per leg, shared by all motors (default %u)\r\n",
                    STEPPER_CTRL_DEFAULT_STEPS);
  app_console_print("  clear <motor>  full fault reset for <motor> (axis latch + DRV8424 sleep/wake)\r\n");
  app_console_print("  enc            current encoder count for every motor\r\n");
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/

/**
 * @brief Resolve a CLI-supplied motor number to its bound axis handle,
 *        printing a CLI error message if the number is out of range or
 *        no axis has been bound to that slot.
 * @param motor_num 1-based motor number, as parsed from the CLI value argument.
 * @return Axis handle on success, or NULL if motor_num is invalid/unbound.
 */
static axis_t *stepper_cli_lookup_axis(int32_t motor_num) {
  if ((motor_num < 1) || (motor_num > (int32_t)STEPPER_CTRL_MAX_MOTORS)) {
    app_console_print("[STEPPER] Motor number must be 1-%u.\r\n", STEPPER_CTRL_MAX_MOTORS);
    return NULL;
  }

  axis_t *ax = stepper_ctrl_get_axis((uint8_t)motor_num);
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
static stepper_cli_param_enum lookup_param(const char *name) {
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
