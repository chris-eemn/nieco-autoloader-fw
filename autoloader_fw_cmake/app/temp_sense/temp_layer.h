/**
 * @file temp_layer.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief Per-channel temperature data store for the MCP342x thermocouple service.
 * @version 0.1
 * @date 2026-09-10
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC. All Rights Reserved.
 *
 */

#ifndef TEMP_LAYER_H_
#define TEMP_LAYER_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include <stdbool.h>
#include <stdint.h>

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
#define TEMP_LAYER_CHANNEL_COUNT 4

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 *Function Prototypes
 *******************************************************************************/

/**
 * @brief Store the raw ADC voltage reading for a channel.
 * @param ch Channel index (0-based).
 * @param v Raw ADC voltage in volts.
 */
void temp_layer_set_sense_v(uint8_t ch, float v);

/**
 * @brief Store the computed hot-junction temperature for a channel.
 * @param ch Channel index (0-based).
 * @param temp_c Hot-junction temperature in degrees Celsius.
 */
void temp_layer_set_temp_c(uint8_t ch, float temp_c);

/**
 * @brief Retrieve the computed hot-junction temperature for a channel.
 * @param ch Channel index (0-based).
 * @param out Pointer to receive the temperature in degrees Celsius.
 * @return true when the channel index is valid; otherwise false.
 */
bool temp_layer_get_temp_c(uint8_t ch, float *out);

/**
 * @brief Retrieve the hot-junction temperature for a channel in whole degrees F.
 * @param ch Channel index (0-based).
 * @return Temperature in whole degrees Fahrenheit (rounded to nearest), or
 *         INT16_MIN when the channel index is invalid or no reading exists.
 */
int16_t temp_layer_get_temp_f(uint8_t ch);

/**
 * @brief Convert a temperature from degrees Celsius to whole degrees Fahrenheit.
 * @param temp_c Temperature in degrees Celsius.
 * @return Temperature in whole degrees Fahrenheit (rounded to nearest).
 */
int16_t temp_layer_c_to_f(float temp_c);

/**
 * @brief Convert a temperature from whole degrees Fahrenheit to degrees Celsius.
 * @param temp_f Temperature in whole degrees Fahrenheit.
 * @return Temperature in degrees Celsius.
 */
float temp_layer_f_to_c(int16_t temp_f);

/**
 * @brief Retrieve the cold-junction compensation temperature.
 * @return Cold-junction temperature in degrees Celsius. Stub: returns 0.0f.
 */
float temp_layer_get_cjc_temp_c(void);

#endif /* TEMP_LAYER_H_ */
