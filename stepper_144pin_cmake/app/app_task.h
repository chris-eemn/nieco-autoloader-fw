/**
 * @file app_task.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief FreeRTOS Control task for the auto-loader application state machine.
 * @version 0.1
 * @date 2026-08-07
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC. All Rights Reserved.
 */

#ifndef APP_TASK_H_
#define APP_TASK_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include <stdbool.h>

#include "autoloader_sm.h"

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
 * @brief Create the Control task and its event queue.
 * @return true when both objects were created successfully; otherwise false.
 */
bool app_task_start(void);

/**
 * @brief Post the one-time event that starts the machine after hardware bring-up.
 * @return true when the event was queued; otherwise false.
 */
bool app_task_begin(void);

/**
 * @brief Post an application event from task context without blocking.
 * @param event Event to copy into the Control queue.
 * @return true when the event was queued; otherwise false.
 */
bool app_task_post(const app_event_t* event);

/**
 * @brief Post an application event from interrupt context.
 * @param event Event to copy into the Control queue.
 * @return true when the event was queued; otherwise false.
 */
bool app_task_post_from_isr(const app_event_t* event);

/**
 * @brief Register the callback invoked when any axis reaches a terminal state.
 *
 *        Replaces any previous registration; only one callback per axis.
 *        Safe to call while an operation is in progress — the new callback
 *        receives that operation's completion event.
 */
void app_task_register_axis_event_cb(void);
#endif /* APP_TASK_H_ */
