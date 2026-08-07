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

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

typedef enum {
  APP_SM_TIMEOUT_NONE = 0,
  APP_SM_TIMEOUT_LOCK,
  APP_SM_TIMEOUT_UNLOCK,
  APP_SM_TIMEOUT_MOTION,
  APP_SM_TIMEOUT_DOOR,
} app_sm_timeout_id_enum;

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

/** @brief Command all pusher axes to home. */
void app_sm_port_home_pushers(void);

/** @brief Command all lift axes to home. */
void app_sm_port_home_lifts(void);

/** @brief Begin cartridge stack measurement. */
void app_sm_port_count_cartridges(void);

/**
 * @brief Extend the pusher for one cartridge.
 * @param slot Zero-based cartridge slot.
 */
void app_sm_port_push_extend(uint8_t slot);

/**
 * @brief Retract the pusher for one cartridge.
 * @param slot Zero-based cartridge slot.
 */
void app_sm_port_push_retract(uint8_t slot);

/**
 * @brief Move one lift upward until stall is detected.
 * @param slot Zero-based cartridge slot.
 */
void app_sm_port_lift_seek(uint8_t slot);

/**
 * @brief Back one lift away from the top plate.
 * @param slot Zero-based cartridge slot.
 */
void app_sm_port_lift_backoff(uint8_t slot);

/**
 * @brief Commit successful dispense accounting.
 * @param slot Zero-based cartridge slot.
 */
void app_sm_port_commit_dispense(uint8_t slot);

/** @brief Halt every axis immediately. */
void app_sm_port_halt_all_motion(void);

/** @brief Save persistent application state. */
void app_sm_port_save_state(void);

/**
 * @brief Arm the timeout associated with the current sequence step.
 * @param timeout Timeout category whose configured duration should be used.
 */
void app_sm_port_arm_timeout(app_sm_timeout_id_enum timeout);

/** @brief Cancel the currently armed sequence timeout. */
void app_sm_port_cancel_timeout(void);

/**
 * @brief Publish application state for Modbus and diagnostics.
 * @param sm Current application state model.
 */
void app_sm_port_publish_state(const app_sm_t* sm);

#endif /* APP_SM_PORT_H_ */
