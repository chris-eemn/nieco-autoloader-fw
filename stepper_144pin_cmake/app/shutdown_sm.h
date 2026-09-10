/**
 * @file shutdown_sm.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief Graceful shutdown sequence state machine.
 * @version 0.1
 * @date 2026-09-10
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef SHUTDOWN_SM_H_
#define SHUTDOWN_SM_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include <stdbool.h>
#include <stdint.h>

#include "autoloader_types.h"
#include "cartridge.h"
#include "patty_handler.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

typedef enum {
  SHUTDOWN_STATUS_OK = 0,
  SHUTDOWN_STATUS_IN_PROGRESS = 1,
  SHUTDOWN_STATUS_DONE = 2, /* Success: sequence finished and switch released. */
  SHUTDOWN_STATUS_FAILED = 3,
} shutdown_status_enum;

typedef struct {
  shutdown_status_enum status;
  uint32_t fault_code;
} shutdown_result_t;

typedef struct {
  shutdown_state_enum state;
  bool release_seen; /* APP_EV_SHUTDOWN_END arrived before the sequence finished. */
} shutdown_sm_t;

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 *Function Prototypes
 *******************************************************************************/

/**
 * @brief Enter the shutdown sequence at its first step.
 *
 * Enters SHUTDOWN_WAIT_DISPENSES: disables dispensing on the handler (no
 * new cycles start; already-active cycles finish) and posts an
 * APP_EV_CONTINUE so the dispatcher evaluates the first state immediately.
 *
 * @param sm Shutdown state model.
 * @param patty_handler Dispense handler (dispensing-enable gate).
 */
void shutdown_sm_start(shutdown_sm_t* sm, patty_handler_t* patty_handler);

/**
 * @brief Dispatch one event to the shutdown sequence.
 * @param shutdown Shutdown state model.
 * @param cartridges Array of cartridges.
 * @param patty_handler Dispense handler (active-dispense check).
 * @param event Event to process.
 * @return Result of the shutdown sm
 */
shutdown_result_t shutdown_sm_dispatch(shutdown_sm_t* shutdown, cartridge_t cartridges[APP_SLOT_COUNT], patty_handler_t* patty_handler,
                                       const app_event_t* event);

/**
 * @brief Abort the shutdown sequence.
 *
 * Resets the sub-state only; no cancel path is wired to any event today.
 *
 * @param shutdown Shutdown state model.
 */
void shutdown_sm_abort(shutdown_sm_t* shutdown);

#endif /* SHUTDOWN_SM_H_ */
