/**
 * @file stepper_auto_task.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief CLI-driven automatic stepper test task.
 * @version 0.1
 * @date 2026-08-07
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC. All Rights Reserved.
 */

#ifndef STEPPER_AUTO_TASK_H_
#define STEPPER_AUTO_TASK_H_

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
 * @brief Create the task that handles the CLI automatic CW/CCW test mode.
 * @return true when the task was created; otherwise false.
 */
bool stepper_auto_task_start(void);

#endif /* STEPPER_AUTO_TASK_H_ */
