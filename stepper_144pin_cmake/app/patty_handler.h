/**
 * @file patty_handler.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief Routes patty requests and coordinates one dispense state machine per
 * cartridge.
 * @version 0.1
 * @date 2026-08-08
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights
 * Reserved.
 *
 */

#ifndef PATTY_HANDLER_H_
#define PATTY_HANDLER_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "autoloader_types.h"
#include "cartridge.h"
#include "dispense_sm.h"
#include "patty_types.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

#define PATTY_HANDLER_SLOT_COUNT (APP_SLOT_COUNT)
#define PATTY_HANDLER_NO_SLOT (0xFFU)

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/
typedef struct {
  cartridge_t* cartridge;
  //   patty_handler_port_t port;
  dispense_sm_t dispense_sm[PATTY_HANDLER_SLOT_COUNT];
  bool door_closed;
  bool door_locked;
  bool dispensing_enabled;
  bool lto_pause_active;
} patty_handler_t;
#if 0
typedef bool (*patty_handler_motion_fn_t)(uint8_t slot);
typedef bool (*patty_handler_timeout_arm_fn_t)(uint8_t slot, dispense_state_enum state);
typedef void (*patty_handler_timeout_cancel_fn_t)(uint8_t slot);
typedef void (*patty_handler_publish_fn_t)(uint8_t slot, const cartridge_t* cartridge, const dispense_sm_t* dispense_sm);

typedef struct {
  patty_handler_motion_fn_t push_extend;
  patty_handler_motion_fn_t push_retract;
  patty_handler_motion_fn_t lift_seek;
  patty_handler_motion_fn_t lift_backoff;
  patty_handler_motion_fn_t halt_motion;
  patty_handler_timeout_arm_fn_t arm_timeout;
  patty_handler_timeout_cancel_fn_t cancel_timeout;
  patty_handler_publish_fn_t publish_status;
} patty_handler_port_t;
#endif

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

/**
 * @brief Initializes a patty handler and its four cartridge dispense state
 * machines.
 *
 * @param handler Handler instance to initialize.
 * @param cartridge Array containing four application cartridge records.
 * @param port Application callbacks used to start motion, manage timers, and
 * publish status.
 * @return PATTY_HANDLER_RESULT_OK on success; otherwise
 * PATTY_HANDLER_RESULT_INVALID_ARGUMENT.
 */
patty_handler_result_enum patty_handler_init(patty_handler_t* handler, cartridge_t* cartridge);

/**
 * @brief Updates the safety conditions that permit a new dispense cycle to
 * start.
 *
 * An open door or unlocked door does not stop motion here. The application
 * safety layer must post a motion-fault event and remove motor power when
 * either condition is lost during motion.
 *
 * @param handler Initialized handler instance.
 * @param door_closed True when the door-closed input is active.
 * @param door_locked True when the door lock is confirmed.
 * @param dispensing_enabled True when the parent application permits
 * dispensing.
 */
void patty_handler_set_safety_state(patty_handler_t* handler, bool door_closed, bool door_locked, bool dispensing_enabled);

/**
 * @brief Enables or disables the customer-specified LTO pause for cartridge
 * slots 3 and 4.
 *
 * @param handler Initialized handler instance.
 * @param active True to prevent new cycles from starting on slots 3 and 4.
 */
void patty_handler_set_lto_pause(patty_handler_t* handler, bool active);

/**
 * @brief Validates and atomically allocates an incoming patty request.
 *
 * Product type is passed as uint8_t because the actual application's
 * cartridge_type_t definition is not part of this starter project. Change the
 * parameter type during integration if desired.
 *
 * @param handler Initialized handler instance.
 * @param product_type Product type value to match against cartridge_t.type.
 * @param requested_count Number of patties to reserve.
 * @param source Queue/POS or manual request source.
 * @return Request result. A rejected request does not modify any pending count.
 *
 * Call patty_handler_process() after an accepted request. Keeping admission and
 * starting separate makes it easy to route both operations through one Control
 * task without blocking the Modbus callback.
 */
patty_handler_result_enum patty_handler_add_request(patty_handler_t* handler, uint8_t product_type, uint16_t requested_count,
                                                    patty_request_source_enum source);

/**
 * @brief Starts one pending dispense on every eligible idle cartridge.
 *
 * Calling this function can start up to four cartridge state machines at the
 * same time.
 *
 * @param handler Initialized handler instance.
 * @return PATTY_HANDLER_RESULT_OK when at least one cycle starts,
 * PATTY_HANDLER_RESULT_NO_ACTION when no slot is ready, or an error result when
 * a motion command is rejected.
 */
patty_handler_result_enum patty_handler_process(patty_handler_t* handler);

/**
 * @brief Routes one slot-specific motion event to the corresponding cartridge
 * state machine.
 *
 * @param handler Initialized handler instance.
 * @param event Motion completion, stall, timeout, or fault event.
 * @return Current handling result.
 */
patty_handler_result_enum patty_handler_dispatch_event(patty_handler_t* handler, const dispense_event_t* event);

#endif /* PATTY_HANDLER_H_ */
