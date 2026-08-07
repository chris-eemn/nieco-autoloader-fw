/**
 * @file cartridge.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief
 * @version 0.1
 * @date 2026-08-07
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef CARTRIDGE_H_
#define CARTRIDGE_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include <stdbool.h>
#include <stdint.h>
/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/
typedef enum { PUSHER = 0U, LIFTER = 1U } cartridge_actuator_type_t;
typedef enum { CARTRIDGE_1 = 0U, CARTRIDGE_2 = 1U, CARTRIDGE_3 = 2U, CARTRIDGE_4 = 3U } cartridge_id_t;
typedef enum {
  DIR_PUSHER_IN = 0U,
  DIR_PUSHER_OUT = 1U,
  DIR_LIFTER_DOWN = 2U,
  DIR_LIFTER_UP = 3U,
  DIR_COUNT = 4U,
} cartridge_direction_t;

typedef struct {
  uint8_t num;
  uint8_t type;
  uint16_t remaining;
  uint16_t pending;
  bool faulted;
  bool pusher_homed;
  bool lifter_homed_down;
  bool lifter_homed_up;
} cartridge_t;
/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 *Function Prototypes
 *******************************************************************************/
uint8_t cartridge_get_axis_num(cartridge_id_t slot, cartridge_actuator_type_t type);
uint8_t cartridge_get_slot_from_axis_num(uint8_t axis_num, cartridge_actuator_type_t type);
#endif /* CARTRIDGE_H_ */
