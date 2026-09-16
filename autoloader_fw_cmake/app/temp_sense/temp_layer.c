/**
 * @file temp_layer.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief Per-channel temperature data store for the MCP342x thermocouple service.
 * @version 0.1
 * @date 2026-09-10
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC. All Rights Reserved.
 *
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include "temp_layer.h"

#include <limits.h>
#include <stddef.h>

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/
static float s_sense_v[TEMP_LAYER_CHANNEL_COUNT];
static float s_temp_c[TEMP_LAYER_CHANNEL_COUNT];
static float s_cjc_temp_c = 0.0f;

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

void temp_layer_set_sense_v(uint8_t ch, float v) {
  if (ch < TEMP_LAYER_CHANNEL_COUNT) {
    s_sense_v[ch] = v;
  }
}

void temp_layer_set_temp_c(uint8_t ch, float temp_c) {
  if (ch < TEMP_LAYER_CHANNEL_COUNT) {
    s_temp_c[ch] = temp_c;
  }
}

bool temp_layer_get_temp_c(uint8_t ch, float *out) {
  if ((ch < TEMP_LAYER_CHANNEL_COUNT) && (out != NULL)) {
    *out = s_temp_c[ch];
    return true;
  }
  return false;
}

int16_t temp_layer_get_temp_f(uint8_t ch) {
  float temp_c = 0.0f;
  int16_t temp_f = INT16_MIN;

  if (temp_layer_get_temp_c(ch, &temp_c) == true) {
    temp_f = temp_layer_c_to_f(temp_c);
  }

  return temp_f;
}

int16_t temp_layer_c_to_f(float temp_c) {
  float temp_f = (temp_c * 1.8f) + 32.0f;
  int32_t rounded = 0;
  int16_t out = 0;

  /* Round to nearest whole degree, then clamp to the int16_t range so the
   * caller can hand the value straight to a modbus register. */
  rounded = (temp_f >= 0.0f) ? (int32_t)(temp_f + 0.5f) : (int32_t)(temp_f - 0.5f);

  if (rounded > INT16_MAX) {
    rounded = INT16_MAX;
  }
  else if (rounded < INT16_MIN) {
    rounded = INT16_MIN;
  }

  out = (int16_t)rounded;
  return out;
}

float temp_layer_f_to_c(int16_t temp_f) {
  return (((float)temp_f) - 32.0f) / 1.8f;
}

float temp_layer_get_cjc_temp_c(void) {
  return s_cjc_temp_c;
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/
