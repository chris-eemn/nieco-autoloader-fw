/**
 * @file autoloader_sm.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief Auto-loader application state-machine types and interfaces.
 * @version 0.1
 * @date 2026-08-07
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC. All Rights Reserved.
 */

#ifndef AUTOLOADER_SM_H_
#define AUTOLOADER_SM_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include <stdbool.h>
#include <stdint.h>

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
  APP_EV_STALL_DETECTED,
  APP_EV_COUNT_DONE,
  APP_EV_DISPENSE_REQUEST,
  APP_EV_RELOAD_REQUEST,
  APP_EV_FAULT,
  APP_EV_FAULT_CLEARED,
  APP_EV_SHUTDOWN_REQUEST,
  APP_EV_TIMEOUT,
} app_event_id_enum;

typedef struct {
  app_event_id_enum id;
  uint8_t slot;
  uint16_t value;
  uint8_t axis_num;
} app_event_t;

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
  STARTUP_HOME_LIFTS_UP,
  STARTUP_COUNT_CARTRIDGES,
  STARTUP_COMPLETE,
  STARTUP_FAILED,
} startup_state_enum;

typedef enum {
  DISPENSE_PUSH_EXTEND = 0,
  DISPENSE_PUSH_RETRACT,
  DISPENSE_LIFT_SEEK,
  DISPENSE_LIFT_BACKOFF,
  DISPENSE_COMPLETE,
  DISPENSE_FAILED,
} dispense_state_enum;

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

typedef struct {
  uint8_t num;
  uint8_t type;
  uint16_t remaining;
  uint16_t pending;
  bool faulted;
  bool pusher_homed;
  bool lifter_homed_down;
  bool lifter_homed_up;
} cartridge_t;

typedef struct {
  startup_state_enum state;
} startup_sm_t;

typedef struct {
  app_state_enum state;
  startup_sm_t startup;
  dispense_state_enum dispense;
  reload_state_enum reload;
  cartridge_t cartridge[APP_SLOT_COUNT];
  uint8_t active_slot;
  uint16_t fault_code;
  bool reload_pending;
} app_sm_t;

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

/**
 * @brief Initialize the complete application state model.
 * @param sm Application state model.
 */
void app_sm_init(app_sm_t* sm);

/**
 * @brief Dispatch one event to the main application state machine.
 * @param sm Application state model.
 * @param event Event to process.
 */
void app_sm_dispatch(app_sm_t* sm, const app_event_t* event);

/**
 * @brief Enter the startup sequence at its first step.
 * @param sm Application state model.
 */
void startup_sm_start(app_sm_t* sm);

/**
 * @brief Dispatch one event to the startup sequence.
 * @param sm Application state model.
 * @param event Event to process.
 */
void startup_sm_dispatch(app_sm_t* sm, const app_event_t* event);

/**
 * @brief Begin one dispense sequence.
 * @param sm Application state model.
 * @param slot Zero-based cartridge slot.
 */
void dispense_sm_start(app_sm_t* sm, uint8_t slot);

/**
 * @brief Dispatch one event to the dispense sequence.
 * @param sm Application state model.
 * @param event Event to process.
 */
void dispense_sm_dispatch(app_sm_t* sm, const app_event_t* event);

/**
 * @brief Enter the reload sequence at its first step.
 * @param sm Application state model.
 */
void reload_sm_start(app_sm_t* sm);

/**
 * @brief Dispatch one event to the reload sequence.
 * @param sm Application state model.
 * @param event Event to process.
 */
void reload_sm_dispatch(app_sm_t* sm, const app_event_t* event);

#endif /* AUTOLOADER_SM_H_ */
