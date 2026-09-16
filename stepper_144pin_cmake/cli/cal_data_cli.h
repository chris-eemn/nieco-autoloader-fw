/**
 * @file cal_data_cli.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief CLI handlers for the "param" command group, backed by the real cal-data store.
 *        Exposes get/set/list function pointers consumed by app_console_commands, plus
 *        save and reset actions.
 *
 *        This replaces cal_data_example.c, which held its own private table of placeholder
 *        parameters unrelated to anything the application actually uses. Every parameter here
 *        is a field of cal_data_params_t, so "param set" changes the same RAM copy that
 *        cal_data_save() writes to flash.
 *
 *          param list                    -- all parameters, with ids and current values
 *          param get  <name|id>          -- print one value
 *          param set  <name|id> <value>  -- update the value and write it to flash
 *          param reset                   -- load factory defaults and write them to flash
 *
 *        Two pseudo-params act on RAM-only dispense state instead of the
 *        cal-data store (they are never saved to flash and are excluded from
 *        "param list"):
 *          param get thickness           -- per-slot thickness report: last raw lift travel,
 *                                           last stored sample, sample count, rolling average
 *          param set reset_thickness <n> -- clear slot <n>'s thickness ring buffer and average
 *
 * @version 0.1
 * @date 2026-07-27
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef CAL_DATA_CLI_H_
#define CAL_DATA_CLI_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include <stdint.h>

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/**
 * One entry per field of cal_data_params_t, in the same order as the struct.
 *
 * Numbering starts at 1 so that 0 stays available as the "not found" result, matching the
 * numeric ids the param command already accepted. These ids are printed by "param list" and
 * accepted by "param get"/"param set" in place of a name.
 */
typedef enum {
  CAL_DATA_CLI_PARAM_INVALID = 0,
  CAL_DATA_CLI_PUSHER_RPM,
  CAL_DATA_CLI_LIFTER_RPM,
  CAL_DATA_CLI_STALL_ERROR_COUNTS,
  CAL_DATA_CLI_HOME_ERROR_COUNTS,
  CAL_DATA_CLI_HOME_RPM,
  CAL_DATA_CLI_HOME_BACKOFF_STEPS,
  CAL_DATA_CLI_HOME_SETTLE_DELAY_MS,
  CAL_DATA_CLI_HOME_MAX_STEPS,
  CAL_DATA_CLI_SUPERVISOR_PERIOD_MS,
  CAL_DATA_CLI_DEFAULT_MOVE_RPM,
  CAL_DATA_CLI_DEFAULT_MOVE_STEPS,
  CAL_DATA_CLI_LOAD_OFFSET,
  CAL_DATA_CLI_PUSH_RETRACT_TIMEOUT_MS,
  CAL_DATA_CLI_LIFT_TIMEOUT_MS,
  CAL_DATA_CLI_PATTY_THICKNESS_COUNTS,
  CAL_DATA_CLI_RECOUNT_TIMEOUT_MS,
  CAL_DATA_CLI_PATTY2_THICKNESS_COUNTS,
  CAL_DATA_CLI_AUTO_CLEAR_FAULTS,
  CAL_DATA_CLI_AUTO_CLEAR_DELAY_MS,
  CAL_DATA_CLI_TEMP_MAX_F,
  CAL_DATA_CLI_THICKNESS_OFFSET_COUNTS,
  CAL_DATA_CLI_USE_MEASURED_THICKNESS,
  /* Action-only parameters have command handlers instead of cal_data_params_t
   * fields. They remain after the stored parameters to preserve existing ids. */
  CAL_DATA_CLI_THICKNESS,        /* "param get thickness" -- per-slot thickness report. */
  CAL_DATA_CLI_RESET_THICKNESS,  /* "param set reset_thickness <slot>" -- clear a slot's ring. */
  CAL_DATA_CLI_NUM_PARAMS,
} cal_data_cli_param_enum;

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 *Function Prototypes
 *******************************************************************************/

/**
 * @brief CLI get handler -- prints the current value of one cal-data parameter.
 * @param param Parameter name string, or its numeric id as a decimal string.
 */
void cal_data_cli_get_handler(char* param);

/**
 * @brief CLI set handler -- writes one cal-data parameter and persists it to SPI flash.
 * @note the value is NOT range checked; see the validation note in this file's header
 * @note the save is queued on the w25q driver and the handler waits for it to land before
 *       returning -- see the flash write cost and concurrency notes in this file's header
 * @param param Parameter name string, or its numeric id as a decimal string.
 * @param val   New value, stored as-is. See the validation note in this file's header for what
 *              a negative value does.
 */
void cal_data_cli_set_handler(char* param, int32_t val);

/**
 * @brief CLI list handler -- prints every parameter with its id and current value.
 */
void cal_data_cli_list_handler(void);

/**
 * @brief CLI reset handler -- loads the factory defaults and persists them to SPI flash.
 */
void cal_data_cli_reset_handler(void);

#endif /* CAL_DATA_CLI_H_ */
