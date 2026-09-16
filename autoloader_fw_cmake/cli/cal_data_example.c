/**
 * @file cal_data_example.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief Example calibration parameter store with CLI handler functions.
 *        Parameters: gain, motor_speed, on_time.
 * @version 0.1
 * @date 2026-06-23
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include "cal_data_example.h"
#include "app_console.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

typedef struct {
  const char* name;
  int32_t value;
  int32_t default_val;
} cal_entry_t;

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

static cal_entry_t s_cal[CAL_NUM_PARAMS] = {
    [CAL_PARAM_INVALID] = {"invalid", 0, 0},
    [CAL_GAIN] = {"gain", 100, 100},
    [CAL_MOTOR_SPEED] = {"motor_speed", 500, 500},
    [CAL_ON_TIME] = {"on_time", 200, 200},
};

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

int32_t get_cal_param(cal_param_enum param) {
  if ((param <= CAL_PARAM_INVALID) || (param >= CAL_NUM_PARAMS))
    return -1;
  return s_cal[param].value;
}

void set_cal_param(cal_param_enum param, int32_t val) {
  if ((param <= CAL_PARAM_INVALID) || (param >= CAL_NUM_PARAMS))
    return;
  s_cal[param].value = val;
}

cal_param_enum get_cal_enum(const char* name) {
  for (int32_t i = 1; i < CAL_NUM_PARAMS; i++) {
    if (strcmp(s_cal[i].name, name) == 0)
      return (cal_param_enum)i;
  }
  return CAL_PARAM_INVALID;
}

const char* get_cal_string(int32_t id) {
  if ((id <= 0) || (id >= CAL_NUM_PARAMS))
    return "invalid";
  return s_cal[id].name;
}

void caldata_set_default_data(void) {
  for (int32_t i = 1; i < CAL_NUM_PARAMS; i++) {
    s_cal[i].value = s_cal[i].default_val;
  }
}

void caldata_save(void) {
  /* TODO: write s_cal to non-volatile storage */
}

void caldata_get_handler(char* param) {
  char* pEnd;
  int32_t id = (int32_t)strtol(param, &pEnd, 10);
  bool is_id = (*pEnd == '\0');
  cal_param_enum e = is_id ? (cal_param_enum)id : get_cal_enum(param);

  if ((e <= CAL_PARAM_INVALID) || (e >= CAL_NUM_PARAMS)) {
    app_console_print("Unknown param: %s\r\n", param);
    return;
  }

  app_console_print("param get: %s = %d\r\n", get_cal_string((int32_t)e), get_cal_param(e));
}

void caldata_set_handler(char* param, int32_t val) {
  cal_param_enum e = get_cal_enum(param);

  if (e == CAL_PARAM_INVALID) {
    app_console_print("Unknown param: %s\r\n", param);
    return;
  }

  set_cal_param(e, val);
  app_console_print("param set: %s = %d\r\n", param, val);
}

void caldata_list_handler(void) {
  app_console_print("CAL parameters:\r\n");
  for (int32_t i = 1; i < CAL_NUM_PARAMS; i++) {
    app_console_print("  %d: %-16s = %d\r\n", i, get_cal_string(i), get_cal_param((cal_param_enum)i));
  }
}

void caldata_reset_to_defaults(void) {
  caldata_set_default_data();
  app_console_print("Factory reset complete.\r\n");
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/
