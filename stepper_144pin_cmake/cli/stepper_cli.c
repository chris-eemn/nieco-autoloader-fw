/**
 * @file stepper_cli.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief Stepper CLI command handlers for the "test" command group.
 *
 *   test set auto  1          -- start continuous CW->3s->CCW->3s loop
 *   test set auto  0          -- stop immediately (also aborts manual moves)
 *   test set cw    1          -- run one CW move, then idle
 *   test set ccw   1          -- run one CCW move, then idle
 *   test set rpm   <value>    -- set move speed in RPM
 *   test set step  <value>    -- set microstep count per leg
 *   test set clear 1          -- full fault reset (axis latch + DRV8424 sleep/wake)
 *   test get enc              -- print the current encoder count
 *   test get <param>          -- print the current value
 *   test list                 -- list all parameters
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

extern axis_t *axis2;
extern axis_t *axis3;
extern axis_t *axis4;
extern axis_t *axis5;
/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

static stepper_cli_param_enum lookup_param(const char *name);

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

    case STEPPER_CLI_CW:
      if (val != 0) {
        axis_t *ax = stepper_ctrl_get_axis();
        if (ax == NULL) {
          app_console_print("[STEPPER] Axis not initialised.\r\n");
          break;
        }
        stepper_status_enum ret = axis_move(ax, stepper_ctrl_get_steps(),
                                            stepper_ctrl_get_rpm(), STEPPER_DIR_CW);
        if (ret == STEPPER_OK) {
          app_console_print("[STEPPER] Manual CW move started.\r\n");
        }
        else {
          app_console_print("[STEPPER] CW start failed: %d (axis=%d)\r\n",
                            (int)ret, (int)axis_get_status(ax));
        }
      }
      break;

    case STEPPER_CLI_CCW:
      if (val != 0) {
        axis_t *ax = stepper_ctrl_get_axis();
        if (ax == NULL) {
          app_console_print("[STEPPER] Axis not initialised.\r\n");
          break;
        }
        stepper_status_enum ret = axis_move(ax, stepper_ctrl_get_steps(),
                                            stepper_ctrl_get_rpm(), STEPPER_DIR_CCW);
        if (ret == STEPPER_OK) {
          app_console_print("[STEPPER] Manual CCW move started.\r\n");
        }
        else {
          app_console_print("[STEPPER] CCW start failed: %d (axis=%d)\r\n",
                            (int)ret, (int)axis_get_status(ax));
        }
      }
      break;

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

    case STEPPER_CLI_CLEAR:
      if (val != 0) {
        axis_t *ax = stepper_ctrl_get_axis();
        if (ax == NULL) {
          app_console_print("[STEPPER] Axis not initialised.\r\n");
          break;
        }
        axis_fault_reset(ax);
        app_console_print("[STEPPER] Fault reset requested — supervisor will complete in ~2 ticks.\r\n");
      }
      break;

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
      axis_t *ax = stepper_ctrl_get_axis();
      axis_status_enum st = (ax != NULL) ? axis_get_status(ax) : AXIS_STATUS_FAULT;
      app_console_print("[STEPPER] axis status = %d\r\n", (int)st);
      break;
    }

    case STEPPER_CLI_RPM:
      app_console_print("[STEPPER] rpm = %lu\r\n", stepper_ctrl_get_rpm());
      break;

    case STEPPER_CLI_STEP:
      app_console_print("[STEPPER] steps = %lu\r\n", stepper_ctrl_get_steps());
      break;

    case STEPPER_CLI_ENC: {
      axis_t *ax = stepper_ctrl_get_axis();
      if (ax == NULL) {
        app_console_print("[STEPPER] Axis not initialised.\r\n");
        break;
      }
      app_console_print("[STEPPER] encoder1 count = %ld\r\n", axis_get_encoder_count(ax));
      app_console_print("[STEPPER] encoder2 count = %ld\r\n", axis_get_encoder_count(axis2));
      app_console_print("[STEPPER] encoder3 count = %ld\r\n", axis_get_encoder_count(axis3));
      app_console_print("[STEPPER] encoder4 count = %ld\r\n", axis_get_encoder_count(axis4));
      app_console_print("[STEPPER] encoder5 count = %ld\r\n", axis_get_encoder_count(axis5));
      break;
    }

    default:
      app_console_print("[STEPPER] Unknown param: %s\r\n", param);
      break;
  }
}

void stepper_cli_list_handler(void) {
  app_console_print("Stepper test parameters:\r\n");
  app_console_print("  auto  1|0      start/stop continuous CW->3s->CCW->3s loop\r\n");
  app_console_print("  cw    1        run one CW move then idle\r\n");
  app_console_print("  ccw   1        run one CCW move then idle\r\n");
  app_console_print("  rpm   <value>  move speed in RPM          (default %u)\r\n",
                    STEPPER_CTRL_DEFAULT_RPM);
  app_console_print("  step  <value>  microsteps per leg         (default %u)\r\n",
                    STEPPER_CTRL_DEFAULT_STEPS);
  app_console_print("  clear 1        full fault reset (axis latch + DRV8424 sleep/wake)\r\n");
  app_console_print("  enc            current encoder count\r\n");
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/

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
