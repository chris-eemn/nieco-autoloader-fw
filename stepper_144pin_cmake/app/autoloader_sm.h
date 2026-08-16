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

#include "cartridge.h"
#include "patty_handler.h"
#include "autoloader_types.h"
#include "startup_sm.h"
#include <stdint.h>

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

typedef struct app_sm {
  app_state_enum state;
  startup_sm_t startup;
  reload_state_enum reload;
  patty_handler_t patty_handler;
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

#if 0
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
#endif

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
