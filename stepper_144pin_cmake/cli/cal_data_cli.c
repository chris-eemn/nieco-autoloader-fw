/**
 * @file cal_data_cli.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief CLI handlers for the "param" command group. See cal_data_cli.h for the command
 *        summary, for why a set persists to flash inline rather than via a separate save
 *        command, and for the outstanding SPI flash concurrency note that applies to it.
 * @version 0.1
 * @date 2026-07-27
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include "cal_data_cli.h"
#include "cal_data.h"
#include "app_console.h"
#include "stepper_system.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/** Names accepted by "param get"/"param set", matching the cal_data_params_t field names. */
static const char* s_param_names[CAL_DATA_CLI_NUM_PARAMS] = {[CAL_DATA_CLI_PARAM_INVALID] = "invalid",
                                                             [CAL_DATA_CLI_PUSHER_RPM] = "pusher_rpm",
                                                             [CAL_DATA_CLI_LIFTER_RPM] = "lifter_rpm",
                                                             [CAL_DATA_CLI_STALL_ERROR_COUNTS] = "stall_error_counts",
                                                             [CAL_DATA_CLI_HOME_ERROR_COUNTS] = "home_error_counts",
                                                             [CAL_DATA_CLI_HOME_RPM] = "home_rpm",
                                                             [CAL_DATA_CLI_HOME_BACKOFF_STEPS] = "home_backoff_steps",
                                                             [CAL_DATA_CLI_HOME_MAX_STEPS] = "home_max_steps",
                                                              [CAL_DATA_CLI_SUPERVISOR_PERIOD_MS] = "supervisor_period_ms",
                                                              [CAL_DATA_CLI_DEFAULT_MOVE_RPM] = "default_move_rpm",
                                                              [CAL_DATA_CLI_DEFAULT_MOVE_STEPS] = "default_move_steps",
                                                              [CAL_DATA_CLI_HOME_SETTLE_DELAY_MS] = "home_settle_delay_ms",
                                                              [CAL_DATA_CLI_LOAD_OFFSET] = "load_offset",
                                                               [CAL_DATA_CLI_PUSH_RETRACT_TIMEOUT_MS] = "push_retract_timeout_ms",
                                                               [CAL_DATA_CLI_LIFT_TIMEOUT_MS] = "lift_timeout_ms",
                                                               [CAL_DATA_CLI_PATTY_THICKNESS_COUNTS] = "patty_thickness_counts",
                                                               [CAL_DATA_CLI_RECOUNT_TIMEOUT_MS] = "recount_timeout_ms"};

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

static uint32_t* param_value_ptr(cal_data_cli_param_enum param);
static cal_data_cli_param_enum lookup_param(const char* name);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

void cal_data_cli_get_handler(char* param) {
  if (param != NULL) {
    cal_data_cli_param_enum id = lookup_param(param);
    uint32_t* value = param_value_ptr(id);

    if (value == NULL) {
      app_console_print("[PARAM] Unknown param: %s\r\n", param);
    }
    else {
      app_console_print("[PARAM] %s = %lu\r\n", s_param_names[id], (unsigned long)*value);
    }
  }
}

void cal_data_cli_set_handler(char* param, int32_t val) {
  if (param != NULL) {
    cal_data_cli_param_enum id = lookup_param(param);
    uint32_t* value = param_value_ptr(id);

    if (value == NULL) {
      app_console_print("[PARAM] Unknown param: %s\r\n", param);
    }
    else {
      /* No range or sign checking by design -- see the validation note in cal_data_cli.h. */
      *value = (uint32_t)val;

      /* Persisted here rather than on a separate command: a set the operator has to remember to
       * follow with a save is a set that silently reverts on the next power cycle. On failure the
       * RAM copy still holds the new value, so say so rather than implying nothing happened. */

      if (cal_data_save() == true) {
        app_console_print("[PARAM] %s = %lu (saved)\r\n", s_param_names[id], (unsigned long)*value);
        stepper_system_update_configs(); /* Apply the new values to the axes immediately. */
      }
      else {
        app_console_print("[PARAM] %s = %lu -- FLASH SAVE FAILED, RAM only\r\n", s_param_names[id], (unsigned long)*value);
      }
    }
  }
}

void cal_data_cli_list_handler(void) {
  const char* source = (cal_data_is_valid() == true) ? "loaded from flash" : "defaults, unsaved";

  app_console_print("Cal-data parameters (%s):\r\n", source);

  for (int32_t i = 1; i < (int32_t)CAL_DATA_CLI_NUM_PARAMS; i++) {
    uint32_t* value = param_value_ptr((cal_data_cli_param_enum)i);

    if (value != NULL) {
      app_console_print("  %2ld %-21s %lu\r\n", (long)i, s_param_names[i], (unsigned long)*value);
    }
    vTaskDelay(pdMS_TO_TICKS(10U)); /* Yield to the console task so it can flush the output. */
  }

  app_console_print("  a 'param set' is written to flash immediately; 'param reset' for defaults\r\n");
}

void cal_data_cli_reset_handler(void) {
  cal_data_load_defaults();

  if (cal_data_save() == true) {
    app_console_print("[PARAM] Factory defaults loaded and saved.\r\n");
  }
  else {
    app_console_print("[PARAM] Factory defaults loaded -- FLASH SAVE FAILED, RAM only\r\n");
  }
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/

/**
 * @brief Resolve a parameter id to the field it names inside the cal-data RAM copy.
 *
 *        This is the only place that maps an id to a field, so get/set/list all stay a few
 *        lines each. It hands back a pointer rather than assigning directly so that one switch
 *        serves all three handlers instead of each carrying its own copy of the list.
 *
 * @param param Parameter to resolve. CAL_DATA_CLI_PARAM_INVALID and any value outside the
 *              enum fall through to the default case and yield NULL.
 * @return Pointer to the live field in cal_data_params_t, or NULL if param names nothing.
 */
static uint32_t* param_value_ptr(cal_data_cli_param_enum param) {
  cal_data_params_t* params = cal_data_get();
  uint32_t* value = NULL;

  if (params != NULL) {
    switch (param) {
      case CAL_DATA_CLI_PUSHER_RPM:
        value = &params->pusher_rpm;
        break;

      case CAL_DATA_CLI_LIFTER_RPM:
        value = &params->lifter_rpm;
        break;

      case CAL_DATA_CLI_STALL_ERROR_COUNTS:
        value = &params->stall_error_counts;
        break;

      case CAL_DATA_CLI_HOME_ERROR_COUNTS:
        value = &params->home_error_counts;
        break;

      case CAL_DATA_CLI_HOME_RPM:
        value = &params->home_rpm;
        break;

      case CAL_DATA_CLI_HOME_SETTLE_DELAY_MS:
        value = &params->home_settle_delay_ms;
        break;

      case CAL_DATA_CLI_HOME_BACKOFF_STEPS:
        value = &params->home_backoff_steps;
        break;

      case CAL_DATA_CLI_HOME_MAX_STEPS:
        value = &params->home_max_steps;
        break;

      case CAL_DATA_CLI_SUPERVISOR_PERIOD_MS:
        value = &params->supervisor_period_ms;
        break;

      case CAL_DATA_CLI_DEFAULT_MOVE_RPM:
        value = &params->default_move_rpm;
        break;

      case CAL_DATA_CLI_DEFAULT_MOVE_STEPS:
        value = &params->default_move_steps;
        break;

      case CAL_DATA_CLI_LOAD_OFFSET:
        value = &params->load_offset;
        break;

      case CAL_DATA_CLI_PUSH_RETRACT_TIMEOUT_MS:
        value = &params->push_retract_timeout_ms;
        break;

      case CAL_DATA_CLI_LIFT_TIMEOUT_MS:
        value = &params->lift_timeout_ms;
        break;

      case CAL_DATA_CLI_PATTY_THICKNESS_COUNTS:
        value = &params->patty_thickness_counts;
        break;

      case CAL_DATA_CLI_RECOUNT_TIMEOUT_MS:
        value = &params->recount_timeout_ms;
        break;

      default:
        break;
    }
  }

  return value;
}

/**
 * @brief Look up a parameter by name, or by numeric id if the token is entirely digits.
 * @param name Token from the CLI. NULL yields CAL_DATA_CLI_PARAM_INVALID.
 * @return Matching enum value, or CAL_DATA_CLI_PARAM_INVALID if the token matches nothing.
 */
static cal_data_cli_param_enum lookup_param(const char* name) {
  cal_data_cli_param_enum found = CAL_DATA_CLI_PARAM_INVALID;

  if (name != NULL) {
    char* end_ptr = NULL;
    uint32_t id = (uint32_t)strtoul(name, &end_ptr, 10);

    if ((end_ptr != name) && (*end_ptr == '\0')) {
      /* Token parsed as a complete decimal number, so treat it as an id rather than a name. */
      if ((id > (uint32_t)CAL_DATA_CLI_PARAM_INVALID) && (id < (uint32_t)CAL_DATA_CLI_NUM_PARAMS)) {
        found = (cal_data_cli_param_enum)id;
      }
    }
    else {
      for (int32_t i = 1; i < (int32_t)CAL_DATA_CLI_NUM_PARAMS; i++) {
        if (strcmp(s_param_names[i], name) == 0) {
          found = (cal_data_cli_param_enum)i;
          break;
        }
      }
    }
  }

  return found;
}
