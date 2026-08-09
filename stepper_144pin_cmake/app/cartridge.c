/**
 * @file cartridge.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief
 * @version 0.1
 * @date 2026-08-07
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include <stdint.h>
#include "autoloader_sm.h"
#include "cartridge.h"
#include "mx_gpio_default.h"
#include "stm32_hal.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/
uint8_t cartridge_get_axis_num(cartridge_t* slot, cartridge_actuator_type_t type) {
  uint8_t axis_num = 0U;
  /*
    CARTRIDGE SLOT 1 -> PUSHER AXIS 1, LIFTER AXIS 2
    CARTRIDGE SLOT 2 -> PUSHER AXIS 3, LIFTER AXIS 4
    CARTRIDGE SLOT 3 -> PUSHER AXIS 5, LIFTER AXIS 6
    CARTRIDGE SLOT 4 -> PUSHER AXIS 7, LIFTER AXIS 8
  */

  // slot nums start from 1-4
  if (slot->num <= APP_SLOT_COUNT) {
    if (type == PUSHER) {
      axis_num = (uint8_t)(((slot->num - 1) * 2U) + 1U);
    }
    else if (type == LIFTER) {
      axis_num = (uint8_t)(((slot->num - 1) * 2U) + 2U);
    }
  }

  return axis_num;
}

uint8_t cartridge_get_slot_from_axis_num(uint8_t axis_num) {
  uint8_t slot = 0U;

  if ((axis_num > 0U) && (axis_num <= (APP_SLOT_COUNT * 2U))) {
    slot = ((axis_num - 1U) / 2U) + 1U;
  }

  return slot;
}

void cartridge_determine_type(cartridge_t* cartridge) {
  uint8_t sensor1_value = 0;
  uint8_t sensor2_value = 0;

  // todo: these pins are hardcoded for now, but should be mapped to the cartridge slot number in the future
  // these pins are internally pulled HIGH, so expecting catridge to pull low when present
  sensor1_value = HAL_GPIO_ReadPin(CARTRIDGE_SENSOR_1_PORT, CARTRIDGE_SENSOR_1_PIN);
  sensor2_value = HAL_GPIO_ReadPin(CARTRIDGE_SENSOR_2_PORT, CARTRIDGE_SENSOR_2_PIN);

  if ((sensor1_value == 1) && (sensor2_value == 1)) {
    cartridge->type = CARTRIDGE_TYPE_EMPTY;
  }
  else if ((sensor1_value == 0) && (sensor2_value == 1)) {
    cartridge->type = CARTRIDGE_TYPE_WHOPPER;
  }
  else if ((sensor1_value == 1) && (sensor2_value == 0)) {
    cartridge->type = CARTRIDGE_TYPE_JR;
  }
  else if ((sensor1_value == 0) && (sensor2_value == 0)) {
    cartridge->type = CARTRIDGE_TYPE_LTO;
  }
}

const char* cartridge_type_to_string(cartridge_type_t type) {
  const char* type_str = "CARTRIDGE_TYPE_UNKNOWN";

  switch (type) {
    case CARTRIDGE_TYPE_EMPTY:
      type_str = "CARTRIDGE_TYPE_EMPTY";
      break;
    case CARTRIDGE_TYPE_WHOPPER:
      type_str = "CARTRIDGE_TYPE_WHOPPER";
      break;
    case CARTRIDGE_TYPE_JR:
      type_str = "CARTRIDGE_TYPE_JR";
      break;
    case CARTRIDGE_TYPE_LTO:
      type_str = "CARTRIDGE_TYPE_LTO";
      break;
    default:
      break;
  }

  return type_str;
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/
