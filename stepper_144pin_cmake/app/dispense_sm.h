/**
 * @file dispense_sm.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief handles cartridge dispense sequences for simultaneous patty requests and dispenses
 * @version 0.1
 * @date 2026-08-08
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef DISPENSE_SM_H_
#define DISPENSE_SM_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include "patty_types.h"
#include <stdbool.h>
#include <stdint.h>

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/
typedef enum {
  DISPENSE_IDLE = 0,
  DISPENSE_PUSH_EXTEND,
  DISPENSE_PUSH_RETRACT,
  DISPENSE_LIFT_SEEK,
  DISPENSE_LIFT_BACKOFF,
  DISPENSE_COMPLETE,
  DISPENSE_FAILED
} dispense_state_enum;

typedef enum {
  DISPENSE_EVENT_PUSH_EXTENDED = 0,
  DISPENSE_EVENT_PUSH_RETRACTED,
  DISPENSE_EVENT_LIFT_STALLED,
  DISPENSE_EVENT_LIFT_BACKOFF_COMPLETE,
  DISPENSE_EVENT_TIMEOUT,
  DISPENSE_EVENT_MOTION_FAULT
} dispense_event_id_enum;

#if 0
typedef struct {
  dispense_event_id_enum id;
  uint8_t slot_index;  // slots are 1-4, but this indexes into an array that starts at 0
  uint16_t fault_code;
  uint32_t measured_lift_travel_counts;
} dispense_event_t;
#endif

/*
 * TODO: Add a command-generation value to dispense_event_t and
 * dispense_sm_t if the production Motion layer can deliver delayed or
 * duplicate events for the same slot.
 */

typedef struct {
  dispense_state_enum state;
  uint8_t slot_index;  // slots are 1-4, but this indexes into an array that starts at 0
  uint16_t fault_code;
  uint32_t measured_lift_travel_counts;
  app_sm_timeout_id_enum timeout_id;
  bool product_may_have_dispensed;
} dispense_sm_t;
/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 *Function Prototypes
 *******************************************************************************/
/**
 * @brief Initializes one cartridge dispense state-machine context.
 *
 * @param dispense_sm State-machine context to initialize.
 * @param slot Zero-based cartridge slot. slots are numbered 1-4, but this indexes into an array that starts at 0
 */
void dispense_sm_init(dispense_sm_t* dispense_sm, uint8_t slot);

/**
 * @brief Starts one nonblocking mechanical dispense cycle for a cartridge.
 *
 * @param dispense_sm State-machine context for the selected cartridge.
 * @return PATTY_HANDLER_RESULT_OK when started or an error result when
 * rejected.
 */
patty_handler_result_enum dispense_sm_start(dispense_sm_t* dispense_sm);

/**
 * @brief Handles one event for an individual cartridge dispense cycle.
 *
 * @param dispense_sm State-machine context for the selected cartridge.
 * @param event Slot-specific event to handle.
 * @return PATTY_HANDLER_RESULT_OK while handled, PATTY_HANDLER_RESULT_NO_ACTION
 * for an unrelated event, or an error result if the state machine fails.
 */
patty_handler_result_enum dispense_sm_dispatch(dispense_sm_t* dispense_sm, const app_event_t* event);

#endif /* DISPENSE_SM_H_ */
