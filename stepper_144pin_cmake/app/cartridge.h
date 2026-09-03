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

// product types that can be dispensed from the autoloader. Whoppers, Jr. patties, and LTOs are the only supported types.
typedef enum { CARTRIDGE_TYPE_EMPTY, CARTRIDGE_TYPE_WHOPPER, CARTRIDGE_TYPE_JR, CARTRIDGE_TYPE_LTO } cartridge_type_t;

typedef enum {
  DIR_PUSHER_IN = 0U,
  DIR_PUSHER_OUT = 1U,
  DIR_LIFTER_DOWN = 2U,
  DIR_LIFTER_UP = 3U,
  DIR_COUNT = 4U,
} cartridge_direction_t;

typedef struct {
  uint8_t num;  // 1-4, if you need to index into an array that starts 0 this must be decremented by 1.
  cartridge_type_t type;
  uint32_t lifter_home_up_encoder_counts;  // counts before stationary up, used to calculate patty thickness
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
/**
 * @brief Gets the axis number for a given cartridge slot and actuator type.
 *
 * @param slot Cartridge slot identifier.
 * @param type Actuator type (pusher or lifter).
 * @return uint8_t Axis number associated with the specified slot and actuator.
 */
uint8_t cartridge_get_axis_num(cartridge_t* slot, cartridge_actuator_type_t type);

/**
 * @brief Gets the cartridge slot identifier from an axis number and actuator type.
 *
 * @param axis_num Axis number to convert.
 * @return uint8_t Cartridge slot identifier corresponding to the specified axis. Note that this function returns 1-4 for valid slots, and -1 for
 * invalid axis numbers. but the catridge array is indexed at 0-3.
 */
uint8_t cartridge_get_slot_from_axis_num(uint8_t axis_num);

/**
 * @brief Reads the raw type-sensor GPIO levels for a cartridge slot.
 *
 * @param cartridge Cartridge whose slot selects the sensor pins.
 * @param sensor1_out Output raw level (0/1) of sensor 1.
 * @param sensor2_out Output raw level (0/1) of sensor 2.
 */
void cartridge_read_type_sensors(const cartridge_t* cartridge, uint8_t* sensor1_out, uint8_t* sensor2_out);

/**
 * @brief Determines and updates the cartridge type metadata.
 *
 * @param cartridge Pointer to the cartridge structure to evaluate and update.
 */
void cartridge_determine_type(cartridge_t* cartridge);

/**
 * @brief Converts a cartridge type enum value to a human-readable string.
 *
 * @param type Cartridge type enum value.
 * @return const char* Pointer to a null-terminated string representing the cartridge type.
 */
const char* cartridge_type_to_string(cartridge_type_t type);
#endif /* CARTRIDGE_H_ */
