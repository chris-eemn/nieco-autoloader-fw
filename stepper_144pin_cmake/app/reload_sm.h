/**
 * @file reload_sm.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief
 * @version 0.1
 * @date 2026-08-24
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef RELOAD_SM_H_
#define RELOAD_SM_H_
#include <stdint.h>
#include "autoloader_types.h"
#include "cartridge.h"
/*******************************************************************************
 * Includes
 *******************************************************************************/
typedef struct {
  uint32_t status;
  uint32_t fault_code;
} reload_result_t;

typedef enum {
  RELOAD_STATUS_OK = 0,
  RELOAD_STATUS_IN_PROGRESS = 1,
  RELOAD_STATUS_DONE = 2,  // success, can transition to ready
  RELOAD_STATUS_FAILED = 3,
} reload_status_enum;

typedef struct {
  reload_state_enum state;
  uint32_t lift_homing_end_time_ms;
  uint32_t lift_homing_timeout_ms;
  uint8_t recount_pending_mask;  // bit i set while slot i still awaits its recount COUNT_DONE
} reload_sm_t;
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
 * @brief Enter the reload sequence at its first step.
 * @param sm Application state model.
 * @param cartridges Array of cartridges.
 */
void reload_sm_start(reload_sm_t* sm, cartridge_t cartridges[APP_SLOT_COUNT]);

/**
 * @brief Dispatch one event to the reload sequence.
 * @param reload Application state model.
 * @param cartridges Array of cartridges.
 * @param event Event to process.
 * @return Result of the reload sm
 */
reload_result_t reload_sm_dispatch(reload_sm_t* reload, cartridge_t cartridges[APP_SLOT_COUNT], const app_event_t* event);

/**
 * @brief Abort the reload sequence.
 * @param sm Application state model.
 */
void reload_sm_abort(reload_sm_t* reload);
#endif /* RELOAD_SM_H_ */
