/**
 * @file app_console_commands.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief Registers all CLI command groups with the console module.
 *        Call app_console_commands_register() once from main() before
 *        vTaskStartScheduler().
 * @version 0.1
 * @date 2026-06-23
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include "app_console.h"
#include "app_console_commands.h"
#include "cal_data_cli.h"
#include "stepper_cli.h"
#include "temp_layer.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/* Largest magnitude (in hundredths of a degree) the fixed-point temp
 * printer will display. Guards against negating INT32_MIN. */
#define TEMP_PRINT_MAX_HUNDREDTHS (99999999L)

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

static void cmd_param_handler(int32_t argc, char** argv);
static void cmd_test_handler(int32_t argc, char** argv);
static void cmd_temp_handler(int32_t argc, char** argv);
static void print_temp_c(int32_t ch, float temp_c);
static void cmd_dispatch(console_command_get_fn_t get_fn, console_command_set_fn_t set_fn, console_command_list_fn_t list_fn, int32_t argc,
                         char** argv);

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

static const console_command_t s_paramCmd = {
    .name = "param",
    .help = "[set/get/list/reset] [param] [value]\t get/set/list/reset cal-data parameters",
    .handler = cmd_param_handler,
    .get_fn = cal_data_cli_get_handler,
    .set_fn = cal_data_cli_set_handler,
    .list_fn = cal_data_cli_list_handler,
};

static const console_command_t s_testCmd = {
    .name = "test",
    .help = "[set/get/list] [auto|start|rpm|step] [value]\t control stepper cycle",
    .handler = cmd_test_handler,
    .get_fn = stepper_cli_get_handler,
    .set_fn = stepper_cli_set_handler,
    .list_fn = stepper_cli_list_handler,
};

static const console_command_t s_tempCmd = {
    .name = "temp",
    .help = "[get <ch>|list]\t get thermocouple temperature",
    .handler = cmd_temp_handler,
};

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

void app_console_commands_register(void) {
  app_console_register_command(&s_paramCmd);
  app_console_register_command(&s_testCmd);
  app_console_register_command(&s_tempCmd);
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/

/**
 * @brief Top-level handler for the "param" CLI command.
 * @param argc Argument count.
 * @param argv Argument vector; argv[1] is the subcommand (get/set/list/reset).
 */
static void cmd_param_handler(int32_t argc, char** argv) {
  if (argv == NULL) {
    return;
  }

  if (argc == 1) {
    app_console_print(
        "Please specify task:\r\n"
        "  set   <name|id> <value> : Update value, saved to flash immediately\r\n"
        "  get   <name|id>         : Print value\r\n"
        "  list                    : List all parameters\r\n"
        "  reset                   : Load defaults, saved to flash immediately\r\n");
    return;
  }

  if (strcmp(argv[1], "reset") == 0) {
    cal_data_cli_reset_handler();
  }
  else {
    cmd_dispatch(s_paramCmd.get_fn, s_paramCmd.set_fn, s_paramCmd.list_fn, argc, argv);
  }
}

/**
 * @brief Top-level handler for the "test" CLI command.
 * @param argc Argument count.
 * @param argv Argument vector; argv[1] is the subcommand (get/set/list).
 */
static void cmd_test_handler(int32_t argc, char** argv) {
  if (argv == NULL) {
    return;
  }

  if (argc == 1) {
    app_console_print(
        "Please specify task:\r\n"
        "  set  <name> <value> : Run test with value\r\n"
        "  get  <name>         : Show last test result\r\n"
        "  list                : List all available tests\r\n");
    return;
  }

  cmd_dispatch(s_testCmd.get_fn, s_testCmd.set_fn, s_testCmd.list_fn, argc, argv);
}

/**
 * @brief Print a temperature in degrees C with two decimal places.
 *
 * The project links newlib-nano without float printf support, so the
 * value is converted to hundredths and printed with integer formats.
 *
 * @param ch     Channel number to label the line with (1-based).
 * @param temp_c Temperature in degrees Celsius.
 */
static void print_temp_c(int32_t ch, float temp_c) {
  /* Clamp before negating: an out-of-range double->int32_t cast can yield
   * INT32_MIN, whose negation overflows. */
  int32_t h = (int32_t)(temp_c * 100.0f);

  if (h > TEMP_PRINT_MAX_HUNDREDTHS) {
    h = TEMP_PRINT_MAX_HUNDREDTHS;
  }
  else if (h < -TEMP_PRINT_MAX_HUNDREDTHS) {
    h = -TEMP_PRINT_MAX_HUNDREDTHS;
  }

  bool    neg = (h < 0);
  int32_t ah  = (neg ? -h : h);

  app_console_print("CH%ld: %s%ld.%02ld C\r\n", (long)ch, (neg ? "-" : ""), (long)(ah / 100), (long)(ah % 100));
}

/**
 * @brief Top-level handler for the "temp" CLI command.
 * @param argc Argument count.
 * @param argv Argument vector; argv[1] is the subcommand (get/list).
 */
static void cmd_temp_handler(int32_t argc, char** argv) {
  if (argv == NULL) {
    return;
  }

  if (argc == 1) {
    app_console_print(
        "Please specify task:\r\n"
        "  get <ch> : Print temperature for channel 1 or 2\r\n"
        "  list     : Print all channel temperatures\r\n");
    return;
  }

  if (strcmp(argv[1], "get") == 0) {
    if (argc != 3) {
      app_console_print("Usage: temp get <ch>\r\n");
      return;
    }
    int32_t ch = (int32_t)atoi(argv[2]);
    if ((ch < 1) || (ch > 2)) {
      app_console_print("Channel must be 1 or 2\r\n");
      return;
    }
    float temp_c;
    if (temp_layer_get_temp_c((uint8_t)(ch - 1), &temp_c)) {
      print_temp_c(ch, temp_c);
    }
    else {
      app_console_print("Invalid channel\r\n");
    }
  }
  else if (strcmp(argv[1], "list") == 0) {
    for (int32_t ch = 1; ch <= 2; ch++) {
      float temp_c;
      if (temp_layer_get_temp_c((uint8_t)(ch - 1), &temp_c)) {
        print_temp_c(ch, temp_c);
      }
      else {
        app_console_print("CH%ld: unavailable\r\n", (long)ch);
      }
    }
  }
  else {
    app_console_print("Unknown subcommand: %s\r\n", argv[1]);
  }
}

/**
 * @brief Route a get/set/list subcommand to the appropriate function pointer.
 * @param get_fn  Handler called for "get <param>".
 * @param set_fn  Handler called for "set <param> <val>".
 * @param list_fn Handler called for "list".
 * @param argc    Argument count from the CLI.
 * @param argv    Argument vector from the CLI.
 */
static void cmd_dispatch(console_command_get_fn_t get_fn, console_command_set_fn_t set_fn, console_command_list_fn_t list_fn, int32_t argc,
                         char** argv) {
  if (argv == NULL) {
    return;
  }

  if (strcmp(argv[1], "set") == 0) {
    if (argc != 4) {
      app_console_print("Usage: %s set <name|id> <value>\r\n", argv[0]);
    }
    else {
      if (set_fn != NULL) {
        set_fn(argv[2], (int32_t)atoi(argv[3]));
      }
    }
  }
  else if (strcmp(argv[1], "get") == 0) {
    if (argc != 3) {
      app_console_print("Usage: %s get <name|id>\r\n", argv[0]);
    }
    else {
      if (get_fn != NULL) {
        get_fn(argv[2]);
      }
    }
  }
  else if (strcmp(argv[1], "list") == 0) {
    if (list_fn != NULL) {
      list_fn();
    }
  }
  else {
    app_console_print("Unknown subcommand: %s\r\n", argv[1]);
  }
}
