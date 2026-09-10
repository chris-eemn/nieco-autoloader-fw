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
  APP_EV_MOTION_FAILED,
  APP_EV_STARTUP_DONE,
  APP_EV_COUNT_DONE,
  APP_EV_DISPENSE_REQUEST,
  APP_EV_RELOAD_REQUEST,
  APP_EV_FAULT,
  APP_EV_FAULT_CLEARED,
  APP_EV_SHUTDOWN_REQUEST,
  APP_EV_SHUTDOWN_END,
  APP_EV_TIMEOUT,
  APP_EV_CONTINUE,
#if 0
  APP_DISP_EVENT_PUSH_EXTENDED = 0,
  APP_DISP_EVENT_PUSH_RETRACTED,
  APP_DISP_EVENT_LIFT_STALLED,
  APP_DISP_EVENT_LIFT_BACKOFF_COMPLETE,
  APP_DISP_EVENT_TIMEOUT,
  APP_DISP_EVENT_MOTION_FAULT
#endif
} app_event_id_enum;

typedef enum {
  APP_FAULT_CODE_NONE = 0,
  APP_FAULT_CODE_INVALID_STATE,
  APP_FAULT_CODE_SEQUENCE_ERROR,
  APP_FAULT_CODE_DOOR_OPENED,
  APP_FAULT_CODE_STARTUP_FAILED,
  APP_FAULT_CODE_RELOAD_FAILED,
  APP_FAULT_CODE_CAL_MISSING,
} app_fault_code_enum;

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
  RELOAD_RECOUNT,
  RELOAD_VALIDATE,
  RELOAD_COMPLETE,
  RELOAD_FAILED,
  RELOAD_WAIT_DOOR,         /* Entry wait for door closed + lock confirmed. */
  RELOAD_LOCK_DOOR_RECOUNT, /* Post-reload re-lock before recount. */
} reload_state_enum;

typedef enum {
  SHUTDOWN_WAIT_DISPENSES = 0, /* Let active cycles finish; block new ones. */
  SHUTDOWN_HOME_PUSHERS,       /* Retract all pushers. */
  SHUTDOWN_HOME_LIFTS_DOWN,    /* Home all lifters down. */
  SHUTDOWN_UNLOCK_DOOR,        /* Drive unlock, no door-open wait. */
  SHUTDOWN_HOLD,               /* Sit until APP_EV_SHUTDOWN_END. */
} shutdown_state_enum;

typedef enum { PATTY_REQUEST_SOURCE_QUEUE = 0, PATTY_REQUEST_SOURCE_MANUAL } patty_request_source_enum;

typedef enum {
  PATTY_HANDLER_RESULT_OK = 0,
  PATTY_HANDLER_RESULT_NO_ACTION,
  PATTY_HANDLER_RESULT_INVALID_ARGUMENT,
  PATTY_HANDLER_RESULT_NOT_READY,
  PATTY_HANDLER_RESULT_NOT_ENOUGH_PRODUCT,
  PATTY_HANDLER_RESULT_MOTION_REJECTED
} patty_handler_result_enum;

typedef enum {
  APP_SM_TIMEOUT_NONE = 0,
  APP_SM_TIMEOUT_LOCK,
  APP_SM_TIMEOUT_UNLOCK,
  APP_SM_TIMEOUT_STARTUP_PUSHER_HOME,
  APP_SM_TIMEOUT_STARTUP_LIFTER_HOME,
  APP_SM_TIMEOUT_STARTUP_DELAY,
  APP_SM_TIMEOUT_MOTION,
  APP_SM_TIMEOUT_RECOUNT,
  APP_SM_TIMEOUT_AUTO_CLEAR_FAULT,
  APP_SM_CART1_DISPENSE,
  APP_SM_CART2_DISPENSE,
  APP_SM_CART3_DISPENSE,
  APP_SM_CART4_DISPENSE,
  APP_SM_TIMEOUT_DOOR,
} app_sm_timeout_id_enum;

typedef struct {
  app_event_id_enum id;
  uint8_t slot;          // this is a cartridge slot number 1-4, cannot be used to index into an array that starts at 0
  uint8_t product_type;  // cartridge_type_enum
  uint16_t value;        // AXIS_EVENT_FAULT
  uint8_t axis_num;      // axis numer 1-8
} app_event_t;
/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 *Function Prototypes
 *******************************************************************************/

#endif /* AUTOLOADER_TYPES_H_ */
