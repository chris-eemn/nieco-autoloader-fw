/**
 * @file app_event_simulator.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief
 * @version 0.1
 * @date 2026-08-07
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef APP_EVENT_SIMULATOR_H_
#define APP_EVENT_SIMULATOR_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include "autoloader_sm.h"
#include "FreeRTOS.h"
#include "timers.h"

#include <stdbool.h>
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

bool app_simulate_event(uint32_t delay_ms, app_event_id_enum event_id);
#endif /* APP_EVENT_SIMULATOR_H_ */
