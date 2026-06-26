/**
 * @file stepper_cli.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief Stepper CLI command handlers for the "test" command group.
 *
 *   test set auto 1          -- start continuous CW->3s->CCW->3s loop
 *   test set auto 0          -- stop immediately (also aborts manual moves)
 *   test set cw   1          -- run one CW move, then idle
 *   test set ccw  1          -- run one CCW move, then idle
 *   test set rpm  <value>    -- set move speed in RPM
 *   test set step <value>    -- set microstep count per leg
 *   test get <param>         -- print the current value
 *   test list                -- list all parameters
 *
 * @version 0.2
 * @date 2026-06-25
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include "stepper_cli.h"
#include "stepper_ctrl.h"
#include "app_console.h"
#include <string.h>

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

static const char *s_param_names[STEPPER_CLI_NUM_PARAMS] = {
  [STEPPER_CLI_AUTO] = "auto",
  [STEPPER_CLI_CW]   = "cw",
  [STEPPER_CLI_CCW]  = "ccw",
  [STEPPER_CLI_RPM]  = "rpm",
  [STEPPER_CLI_STEP] = "step",
};

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
        stepper_ctrl_send_cmd(STEPPER_CMD_RUN_CW);
        app_console_print("[STEPPER] Manual CW move started.\r\n");
      }
      break;

    case STEPPER_CLI_CCW:
      if (val != 0) {
        stepper_ctrl_send_cmd(STEPPER_CMD_RUN_CCW);
        app_console_print("[STEPPER] Manual CCW move started.\r\n");
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

    case STEPPER_CLI_RPM:
      app_console_print("[STEPPER] rpm = %lu\r\n", stepper_ctrl_get_rpm());
      break;

    case STEPPER_CLI_STEP:
      app_console_print("[STEPPER] steps = %lu\r\n", stepper_ctrl_get_steps());
      break;

    case STEPPER_CLI_CW:
    case STEPPER_CLI_CCW:
      app_console_print("[STEPPER] running = %u\r\n", stepper_ctrl_is_running());
      break;

    default:
      app_console_print("[STEPPER] Unknown param: %s\r\n", param);
      break;
  }
}

void stepper_cli_list_handler(void) {
  app_console_print("Stepper test parameters:\r\n");
  app_console_print("  auto 1|0       start/stop continuous CW->3s->CCW->3s loop\r\n");
  app_console_print("  cw   1         run one CW move then idle\r\n");
  app_console_print("  ccw  1         run one CCW move then idle\r\n");
  app_console_print("  rpm  <value>   move speed in RPM          (default %u)\r\n",
                    STEPPER_CTRL_DEFAULT_RPM);
  app_console_print("  step <value>   microsteps per leg         (default %u)\r\n",
                    STEPPER_CTRL_DEFAULT_STEPS);
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
