/**
 * @file stepper_cli.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief Stepper CLI command handlers for the "test" command group.
 *        Exposes get/set/list function pointers consumed by app_console_commands.
 * @version 0.2
 * @date 2026-06-25
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 */

#ifndef STEPPER_CLI_H_
#define STEPPER_CLI_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include <stdint.h>

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

typedef enum {
  STEPPER_CLI_AUTO = 0,
  STEPPER_CLI_CW,
  STEPPER_CLI_CCW,
  STEPPER_CLI_RPM,
  STEPPER_CLI_STEP,
  STEPPER_CLI_CLEAR,
  STEPPER_CLI_ENC,
  STEPPER_CLI_HOME,
  STEPPER_CLI_QUEUE,
  STEPPER_CLI_NUM_PARAMS,
} stepper_cli_param_enum;

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

/**
 * @brief CLI get handler — prints the current value of a stepper parameter.
 * @param param Parameter name string ("auto", "cw", "ccw", "rpm", "step").
 */
void stepper_cli_get_handler(char* param);

/**
 * @brief CLI set handler — applies the given value to a stepper parameter.
 * @param param Parameter name string ("auto", "cw", "ccw", "rpm", "step").
 * @param val   Value to apply.
 */
void stepper_cli_set_handler(char* param, int32_t val);

/**
 * @brief CLI list handler — prints all available parameter names and usage.
 */
void stepper_cli_list_handler(void);

#endif /* STEPPER_CLI_H_ */
