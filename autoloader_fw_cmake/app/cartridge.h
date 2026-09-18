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
/** Number of thickness samples kept in each slot's rolling-average ring buffer. */
#define CARTRIDGE_THICKNESS_RING_SIZE (8U)

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/
/**
 * Actuator type for a cartridge slot. This enum is the single source of truth for
 * how actuators map onto axis numbers: each slot owns CARTRIDGE_AXIS_COUNT
 * consecutive axes, starting at the actuator listed first.
 *
 *   Slot 1 -> LIFTER axis 1, PUSHER axis 2
 *   Slot 2 -> LIFTER axis 3, PUSHER axis 4
 *   Slot 3 -> LIFTER axis 5, PUSHER axis 6
 *   Slot 4 -> LIFTER axis 7, PUSHER axis 8
 *
 * Reordering these enumerators reorders the axis assignment. Every axis-number
 * calculation and every axis-to-actuator test derives from this enum, so nothing
 * else may hardcode the parity.
 */
typedef enum { LIFTER = 0U, PUSHER = 1U, CARTRIDGE_AXIS_COUNT = 2U } cartridge_actuator_type_t;

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
  /* Per-slot rolling patty-thickness average, measured during dispense lift seeks.
   * RAM-only: cleared on recount/reload and fault clear, never persisted. */
  uint32_t thickness_samples[CARTRIDGE_THICKNESS_RING_SIZE];  // ring buffer of valid samples (encoder counts)
  uint8_t thickness_sample_count;                             // samples valid so far, saturates at CARTRIDGE_THICKNESS_RING_SIZE
  uint8_t thickness_sample_next;                              // ring write index
  uint32_t thickness_avg_counts;                              // mean of stored samples, 0 = no valid average yet
  /* Dynamic stack count computed from the measured stack height and the rolling
   * average at the last dispense commit, BEFORE the inventory clamps. Latched
   * every commit even when use_measured_thickness is off (shadow mode), so the
   * customer can compare it against remaining. It may exceed remaining by
   * design: it is a comparison number, not inventory. 0 = never computed. */
  uint32_t thickness_shadow_remaining;
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
uint8_t cartridge_get_axis_num(const cartridge_t* slot, cartridge_actuator_type_t type);

/**
 * @brief Gets the cartridge slot identifier from an axis number and actuator type.
 *
 * @param axis_num Axis number to convert.
 * @return uint8_t Cartridge slot identifier corresponding to the specified axis. Note that this function returns 1-4 for valid slots, and -1 for
 * invalid axis numbers. but the catridge array is indexed at 0-3.
 */
uint8_t cartridge_get_slot_from_axis_num(uint8_t axis_num);

/**
 * @brief Tests whether an axis belongs to the given actuator type.
 *
 *        Derives the answer from cartridge_actuator_type_t, so the actuator/axis
 *        layout is defined in exactly one place. Callers must not test axis
 *        parity directly.
 *
 * @param axis_num Axis number to test (1-based; 0 is invalid and returns false).
 * @param type Actuator type to test against.
 * @return true when axis_num is that actuator of its slot, false otherwise.
 */
bool cartridge_axis_is_type(uint8_t axis_num, cartridge_actuator_type_t type);

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

/**
 * @brief Adds one measured patty-thickness sample to the slot's rolling average.
 *
 *        Ring-inserts the sample (CARTRIDGE_THICKNESS_RING_SIZE entries) and
 *        recomputes thickness_avg_counts as the plain mean of the stored
 *        samples. A sample_counts value of 0 means "no valid sample" (no
 *        capture, or the offset consumed the whole travel) and is ignored.
 *        Unit is encoder counts, the same unit as patty_thickness_counts.
 *
 * @param slot Cartridge slot receiving the sample. NULL is ignored.
 * @param sample_counts Thickness sample in encoder counts; 0 is ignored.
 */
void cartridge_add_thickness_sample(cartridge_t* slot, uint32_t sample_counts);

/**
 * @brief Converts a measured stack height into a patty count using a thickness average.
 *
 *        Single shared formula for the authoritative remaining recalculation and
 *        the shadow comparison count: round-to-nearest division
 *        (stack_height + avg/2) / avg.
 *
 * @param stack_height Stack height in encoder counts.
 * @param avg Thickness average in encoder counts; 0 returns 0.
 * @return uint32_t Patty count, 0 when avg is 0.
 */
uint32_t cartridge_stack_count(uint32_t stack_height, uint32_t avg);

/**
 * @brief Clears the slot's thickness ring buffer and rolling average.
 *
 *        Call wherever slot->remaining is (re)initialized: cartridge reload /
 *        recount completion and slot fault clear.
 *
 * @param slot Cartridge slot to reset. NULL is ignored.
 */
void cartridge_reset_thickness(cartridge_t* slot);

#endif /* CARTRIDGE_H_ */
