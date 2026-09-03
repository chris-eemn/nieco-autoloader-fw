/**
 * @file cartridge_recount.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief Shared recount and lane-pair type validation for cartridge slots.
 * @version 0.1
 * @date 2026-08-29
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef CARTRIDGE_RECOUNT_H_
#define CARTRIDGE_RECOUNT_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "cartridge.h"
#include "autoloader_types.h"

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
 *Function Prototypes
 *******************************************************************************/
/**
 * @brief Begin the recount for one slot: raise its lift to stall.
 *
 * Issues the lift-up homing (stall + backoff + encoder zero) move. The caller
 * is responsible for arming APP_SM_TIMEOUT_RECOUNT. On stall, the axis event
 * path posts APP_EV_COUNT_DONE for this slot. A false return means the move
 * was rejected and no COUNT_DONE event will arrive — do not wait for one.
 * @param slot Cartridge slot to recount (num field is 1-4).
 * @return true when the homing move was started; false when the axis rejected
 *         it (busy, latched fault, or invalid).
 */
bool cartridge_recount_start(cartridge_t* slot);

/**
 * @brief Apply one completed recount for a slot.
 *
 * Reads the lift travel counts, computes remaining patties from the configured
 * patty thickness, and stores the result in slot->remaining. The result is
 * clamped to at least slot->pending so in-flight dispenses are never
 * invalidated, and to UINT16_MAX to prevent truncation.
 * @param slot Cartridge slot whose recount move just completed.
 * @param fault_out Output fault code (written on failure).
 * @return true on success; false when patty thickness is unconfigured (0) or
 *         the lifter axis is faulted/invalid, in which case *fault_out is set.
 */
bool cartridge_recount_apply(cartridge_t* slot, uint32_t* fault_out);

/**
 * @brief Validate cartridge types as lane pairs (slots 1/2 and 3/4).
 *
 * Empty slots never constitute a mismatch. On a mismatch, both cartridges in
 * the lane are inhibited (faulted = true) and a console error is logged.
 * @param cartridges Array of cartridges.
 * @return true if any lane mismatched; false if all lanes are consistent.
 */
bool cartridge_validate_lane_pairs(cartridge_t cartridges[APP_SLOT_COUNT]);

#endif /* CARTRIDGE_RECOUNT_H_ */
