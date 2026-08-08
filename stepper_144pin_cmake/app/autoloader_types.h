/**
 * @file autoloader_types.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief
 * @version 0.1
 * @date 2026-08-08
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef AUTOLOADER_TYPES_H_
#define AUTOLOADER_TYPES_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
#define APP_SLOT_COUNT (1U)  // number of cartridges/slots available in the auto-loader.
#define APP_NO_SLOT (0xFFU)
#define APP_FAULT_CODE_INVALID_STATE (1U)
#define APP_FAULT_CODE_SEQUENCE_ERROR (2U)
/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/
typedef enum {
  APP_EV_START = 0,
  APP_EV_DOOR_OPENED,
  APP_EV_DOOR_CLOSED,
  APP_EV_LOCK_CONFIRMED,
  APP_EV_LOCK_RELEASED,
  APP_EV_MOTION_DONE,
  APP_EV_HOME_DONE,
  APP_EV_DETERMINE_TYPE_DONE,
  APP_EV_STALL_DETECTED,
  APP_EV_STARTUP_DONE,
  APP_EV_COUNT_DONE,
  APP_EV_DISPENSE_REQUEST,
  APP_EV_RELOAD_REQUEST,
  APP_EV_FAULT,
  APP_EV_FAULT_CLEARED,
  APP_EV_SHUTDOWN_REQUEST,
  APP_EV_TIMEOUT,
} app_event_id_enum;

typedef enum {
  APP_INIT = 0,
  APP_STARTUP,
  APP_READY,
  APP_DISPENSE,
  APP_RELOAD,
  APP_FAULT,
  APP_SHUTDOWN,
} app_state_enum;

typedef enum {
  STARTUP_WAIT_DOOR = 0,
  STARTUP_LOCK_DOOR,
  STARTUP_HOME_PUSHERS,
  STARTUP_HOME_LIFTS_DOWN,
  STARTUP_HOME_LIFTS_DELAY,
  STARTUP_HOME_LIFTS_UP,
  STARTUP_COUNT_CARTRIDGES,
  STARTUP_DETERMINE_TYPE,
  STARTUP_COMPLETE,
  STARTUP_FAILED,
} startup_state_enum;

typedef enum {
  RELOAD_HOME_PUSHERS = 0,
  RELOAD_HOME_LIFTS,
  RELOAD_UNLOCK_DOOR,
  RELOAD_WAIT_OPEN,
  RELOAD_WAIT_CLOSE,
  RELOAD_LOCK_DOOR,
  RELOAD_COUNT_CARTRIDGES,
  RELOAD_COMPLETE,
  RELOAD_FAILED,
} reload_state_enum;
typedef enum { PATTY_REQUEST_SOURCE_QUEUE = 0, PATTY_REQUEST_SOURCE_MANUAL } patty_request_source_enum;

typedef enum {
  PATTY_HANDLER_RESULT_OK = 0,
  PATTY_HANDLER_RESULT_NO_ACTION,
  PATTY_HANDLER_RESULT_INVALID_ARGUMENT,
  PATTY_HANDLER_RESULT_NOT_READY,
  PATTY_HANDLER_RESULT_NOT_ENOUGH_PRODUCT,
  PATTY_HANDLER_RESULT_MOTION_REJECTED
} patty_handler_result_enum;
/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 *Function Prototypes
 *******************************************************************************/

#endif /* AUTOLOADER_TYPES_H_ */
