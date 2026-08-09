/**
 * @file app_sm_port.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief Hardware and service actions used by the application state machines.
 * @version 0.1
 * @date 2026-08-07
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC. All Rights Reserved.
 */

#ifndef APP_SM_PORT_H_
#define APP_SM_PORT_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include <stdint.h>

#include "autoloader_sm.h"
#include "cartridge.h"

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
 * Function Prototypes
 *******************************************************************************/

/** @brief Command the door lock. */
void app_sm_port_lock_door(void);

/** @brief Command the door to unlock. */
void app_sm_port_unlock_door(void);

/**
 * @brief Home the pusher axis for a cartridge slot.
 * @param slot Pointer to the cartridge slot structure (num field is 1-4).
 */
void app_sm_port_home_pusher(cartridge_t* slot);

/**
 * @brief Home the lift axis for a cartridge slot.
 * @param slot Pointer to the cartridge slot structure (num field is 1-4).
 * @param direction Lift homing direction.
 */
void app_sm_port_home_lift(cartridge_t* slot, cartridge_direction_t direction);

/**
 * @brief Begin cartridge stack measurement.
 * @param slot Pointer to the cartridge slot structure (num field is 1-4).
 */
void app_sm_port_count_cartridges(cartridge_t* slot);

/**
 * @brief Extend the pusher for a cartridge slot.
 * @param slot Pointer to the cartridge slot structure (num field is 1-4).
 */
void app_sm_port_push_extend(cartridge_t* slot);

/**
 * @brief Retract the pusher for a cartridge slot.
 * @param slot Pointer to the cartridge slot structure (num field is 1-4).
 */
void app_sm_port_push_retract(cartridge_t* slot);

/**
 * @brief Move the lift upward until stall is detected.
 * @param slot Pointer to the cartridge slot structure (num field is 1-4).
 */
void app_sm_port_lift_seek(cartridge_t* slot);

/**
 * @brief Back the lift away from the top plate.
 * @param slot Pointer to the cartridge slot structure (num field is 1-4).
 */
void app_sm_port_lift_backoff(cartridge_t* slot);

/**
 * @brief Commit successful dispense accounting.
 * @param slot Pointer to the cartridge slot structure (num field is 1-4).
 */
void app_sm_port_commit_dispense(cartridge_t* slot);

/**
 * @brief Halt motion for a specific cartridge slot.
 * @param slot Pointer to the cartridge slot structure (num field is 1-4).
 * @return true if the motion was successfully halted; otherwise false.
 */
void app_sm_port_halt_motion(cartridge_t* slot);

/** @brief Halt every axis immediately. */
void app_sm_port_halt_all_motion(void);

/** @brief Save persistent application state. */
void app_sm_port_save_state(void);

/**
 * @brief Arm a timeout that posts APP_EV_TIMEOUT after a delay.
 * @param timeout Timeout ID carried in the event value field.
 * @param delay_ms Delay in milliseconds before the event is posted.
 * @return true if the timeout timer was successfully started; otherwise false.
 * @note Up to MAX_PENDING_TIMER_EVENTS timeouts may be armed at once.
 */
bool app_sm_port_arm_timeout(app_sm_timeout_id_enum timeout, uint32_t delay_ms);

/** @brief Cancel the currently armed sequence timeout. */
void app_sm_port_cancel_timeout(void);

/**
 * @brief Cancel one pending timeout by ID.
 * @param timeout Timeout ID to cancel.
 * @return true if a pending timeout was cancelled, false if it had already fired.
 */
bool app_sm_port_cancel_timeout_id(app_sm_timeout_id_enum timeout);
/**
 * @brief Publish application state for Modbus and diagnostics.
 * @param sm Current application state model.
 */
void app_sm_port_publish_state(const app_sm_t* sm);

/**
 * @brief Convert a timeout ID to a human-readable string.
 * @param timeout_id Timeout ID to convert.
 * @return Pointer to a constant string describing the timeout ID.
 */
const char* app_sm_port_timeout_id_to_str(app_sm_timeout_id_enum timeout_id);

#endif /* APP_SM_PORT_H_ */
