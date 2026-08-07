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

#endif /* STEPPER_SYSTEM_H_ */
