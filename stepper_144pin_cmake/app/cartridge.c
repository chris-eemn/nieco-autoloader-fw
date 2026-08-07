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
uint8_t cartridge_get_axis_num(cartridge_id_t slot, cartridge_actuator_type_t type) {
  uint8_t axis_num = 0U;
  /*
    CARTRIDGE SLOT 0 -> PUSHER AXIS 1, LIFTER AXIS 2
    CARTRIDGE SLOT 1 -> PUSHER AXIS 3, LIFTER AXIS 4
    CARTRIDGE SLOT 2 -> PUSHER AXIS 5, LIFTER AXIS 6
    CARTRIDGE SLOT 3 -> PUSHER AXIS 7, LIFTER AXIS 8
  */

  if (slot < APP_SLOT_COUNT) {
    if (type == PUSHER) {
      axis_num = (uint8_t)((slot * 2U) + 1U);
    }
    else if (type == LIFTER) {
      axis_num = (uint8_t)((slot * 2U) + 2U);
    }
  }

  return axis_num;
}

uint8_t cartridge_get_slot_from_axis_num(uint8_t axis_num, cartridge_actuator_type_t type) {
  int8_t slot = -1;

  if ((axis_num > 0U) && (axis_num <= (APP_SLOT_COUNT * 2U))) {
    if (type == PUSHER) {
      if ((axis_num % 2U) == 1U) {
        slot = (int8_t)((axis_num - 1U) / 2U);
      }
    }
    else if (type == LIFTER) {
      if ((axis_num % 2U) == 0U) {
        slot = (int8_t)((axis_num - 2U) / 2U);
      }
    }
  }

  return slot;
}
/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/
