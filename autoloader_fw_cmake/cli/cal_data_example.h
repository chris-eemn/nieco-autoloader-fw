/**
 * @file cal_data_example.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief Example calibration parameter store. Exposes CLI handler functions
 *        for use as console_command_t function pointers.
 * @version 0.1
 * @date 2026-06-23
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef CAL_DATA_EXAMPLE_H_
#define CAL_DATA_EXAMPLE_H_

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

typedef enum { CAL_PARAM_INVALID = 0, CAL_GAIN, CAL_MOTOR_SPEED, CAL_ON_TIME, CAL_NUM_PARAMS } cal_param_enum;

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

/**
 * @brief Get the value of a calibration parameter.
 * @param param The parameter to read.
 * @return The current value, or -1 if param is out of range.
 */
int32_t get_cal_param(cal_param_enum param);

/**
 * @brief Set the value of a calibration parameter.
 * @param param The parameter to write.
 * @param val The new value.
 */
void set_cal_param(cal_param_enum param, int32_t val);

/**
 * @brief Look up a calibration parameter enum by name string.
 * @param name The parameter name string.
 * @return The matching enum, or CAL_PARAM_INVALID if not found.
 */
cal_param_enum get_cal_enum(const char* name);

/**
 * @brief Get the name string for a calibration parameter by numeric id.
 * @param id The numeric parameter id.
 * @return The parameter name string, or "invalid" if id is out of range.
 */
const char* get_cal_string(int32_t id);

/**
 * @brief Reset all calibration parameters to their factory default values.
 */
void caldata_set_default_data(void);

/**
 * @brief Persist the current calibration data to non-volatile storage.
 */
void caldata_save(void);

/**
 * @brief CLI get handler — prints the current value of a calibration parameter.
 * @param param Parameter name or numeric id string.
 */
void caldata_get_handler(char* param);

/**
 * @brief CLI set handler — updates the value of a calibration parameter.
 * @param param Parameter name string.
 * @param val New value.
 */
void caldata_set_handler(char* param, int32_t val);

/**
 * @brief CLI list handler — prints all calibration parameters and their values.
 */
void caldata_list_handler(void);

/**
 * @brief CLI reset handler — resets all calibration parameters to defaults.
 */
void caldata_reset_to_defaults(void);

#endif /* CAL_DATA_EXAMPLE_H_ */
