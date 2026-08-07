/**
 * @file stepper_system.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief Board-level construction and registration of the stepper axes.
 * @version 0.1
 * @date 2026-08-07
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC. All Rights Reserved.
 */

#ifndef STEPPER_SYSTEM_H_
#define STEPPER_SYSTEM_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include "axis.h"
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
 * Function Prototypes
 *******************************************************************************/

/**
 * @brief Initialise the encoder timers, stepper drivers, and axis instances.
 *
 *        Successfully constructed axes are registered with stepper_ctrl and can
 *        be obtained with stepper_ctrl_get_axis(). Must be called from task
 *        context because axis construction creates the axis supervisor task.
 *
 * @return true when all configured hardware was initialised; otherwise false.
 */
bool stepper_system_init(void);

/**
 * @brief Update stepper related configs from cal_data_params_t when parameters change.
 *
 *        Called after cal_data_init() and whenever a cal_data parameter is
 *        changed to apply user-modified values without requiring a power cycle.
 *        This updates all relevant configuration structures, not just
 *        axis_config_t. Axes must have been constructed already
 *        (stepper_system_init()).
 */
void stepper_system_update_configs(void);

/**
 * @brief Register a callback invoked when any axis reaches a terminal state.
 *
 *        Replaces any previous registration; only one callback per axis.
 *        Safe to call while an operation is in progress — the new callback
 *        receives that operation's completion event.
 *
 * @param cb  Callback invoked from the supervisor task, or NULL.
 * @param ctx Opaque pointer passed back to the callback unmodified. Useful
 *            for carrying a caller-side axis identifier or queue handle.
 */
void stepper_system_register_axis_event_cb(axis_event_cb_t cb, void* ctx);

#endif /* STEPPER_SYSTEM_H_ */
