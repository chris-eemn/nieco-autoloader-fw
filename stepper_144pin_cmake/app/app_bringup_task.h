/**
 * @file app_bringup_task.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief One-time application and hardware bring-up task.
 * @version 0.1
 * @date 2026-08-07
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC. All Rights Reserved.
 */

#ifndef APP_BRINGUP_TASK_H_
#define APP_BRINGUP_TASK_H_

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
 * @brief Create the one-time task that initialises application hardware and starts runtime tasks.
 * @return true when the bring-up task was created; otherwise false.
 */
bool app_bringup_task_start(void);

#endif /* APP_BRINGUP_TASK_H_ */
