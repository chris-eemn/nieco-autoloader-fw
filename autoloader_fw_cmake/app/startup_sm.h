/**
 * @file startup_sm.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief Startup state-machine types and interfaces.
 * @version 0.1
 * @date 2026-08-15
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef STARTUP_SM_H_
#define STARTUP_SM_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include <stdint.h>

#include "cartridge.h"
#include "autoloader_types.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

typedef struct {
  uint32_t status;
  uint32_t fault_code;
} startup_result_t;

typedef enum {
  STARTUP_STATUS_OK = 0,
  STARTUP_STATUS_IN_PROGRESS = 1,
  STARTUP_STATUS_DONE = 2,  // success, can transition to ready
  STARTUP_STATUS_FAILED = 3,
} startup_status_enum;

typedef struct {
  startup_state_enum state;
  uint32_t lift_homing_end_time_ms;
  uint32_t lift_homing_timeout_ms;
} startup_sm_t;

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 *Function Prototypes
 *******************************************************************************/

/**
 * @brief Enter the startup sequence at its first step.
 * @param sm Application state model.
 */
void startup_sm_start(startup_sm_t* sm);

/**
 * @brief Dispatch one event to the startup sequence.
 * @param sm Application state model.
 * @param event Event to process.
 * @return Result of the startup sm
 */
startup_result_t startup_sm_dispatch(startup_sm_t* startup, cartridge_t cartridges[APP_SLOT_COUNT], const app_event_t* event);

/**
 * @brief Abort the startup sequence.
 * @param sm Application state model.
 */
void startup_sm_abort(startup_sm_t* startup);
#endif /* STARTUP_SM_H_ */
