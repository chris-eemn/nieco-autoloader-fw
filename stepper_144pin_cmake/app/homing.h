/**
 * @file homing.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief Shared homing-phase event handling for the application state machines.
 * @version 0.1
 * @date 2026-08-29
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef HOMING_H_
#define HOMING_H_

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

/**
 * @brief Cartridge homing flag managed by a homing phase.
 */
typedef enum {
  HOMING_TARGET_PUSHER = 0,
  HOMING_TARGET_LIFTER_DOWN,
  HOMING_TARGET_LIFTER_UP,
} homing_target_enum;

/**
 * @brief Outcome of processing one event in a homing phase.
 */
typedef enum {
  HOMING_EVENT_WAITING = 0,
  HOMING_EVENT_COMPLETE,
  HOMING_EVENT_FAILED,
} homing_event_result_enum;

/**
 * @brief Configuration for a generic homing phase.
 *
 * Each homing phase (pushers, lifters down, lifters up) is described by
 * a small config struct so the event handler can be shared.
 */
typedef struct {
  const char* name;          // human-readable name for log messages
  homing_target_enum target; // cartridge homing flag managed by this phase
  uint8_t next_state;        // state to transition to when all homed (caller's state enum)
  uint32_t fault_code;       // fault code to report on failure (caller's fault enum)
  const char* success_log;   // log message when all homed
  const char* timeout_log;   // log message on timeout
} homing_phase_cfg_t;

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 *Function Prototypes
 *******************************************************************************/

/**
 * @brief Check if all cartridges have a given homing flag set.
 * @param cartridges Array of cartridges.
 * @param target Homing flag to check.
 * @return true if all cartridges pass the check.
 */
bool homing_all_homed(const cartridge_t cartridges[APP_SLOT_COUNT], homing_target_enum target);

/**
 * @brief Mark the selected homing target complete for a cartridge.
 * @param cartridge Cartridge to update.
 * @param target Homing flag to set.
 */
void homing_mark_complete(cartridge_t* cartridge, homing_target_enum target);

/**
 * @brief Handle a generic homing-phase event.
 *
 * Processes motion-failed, timeout, and motion-done events for any homing
 * phase. On motion-done, marks the axis as homed and checks if all axes
 * are done. The returned outcome tells the caller whether to wait,
 * transition, or propagate a failure. On failure, the configured fault
 * code is written to *fault_code_out.
 *
 * @param cartridges Array of cartridges.
 * @param event Event to process.
 * @param fault_code_out Output fault code (written on failure).
 * @param cfg Homing phase configuration.
 * @return Outcome indicating whether the phase is waiting, complete, or failed.
 */
homing_event_result_enum homing_handle_event(cartridge_t cartridges[APP_SLOT_COUNT], const app_event_t* event,
                                             uint32_t* fault_code_out, const homing_phase_cfg_t* cfg);

#endif /* HOMING_H_ */
