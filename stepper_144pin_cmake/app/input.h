/**
 * @file input.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief FreeRTOS input task for door debounce and periodic input polling.
 *
 *        The input task wakes immediately when notified by the door EXTI
 *        handler, services the door debounce logic, and wakes periodically
 *        to poll additional inputs.  Debounced input events are posted to
 *        the application event queue.  No state-machine decisions are made
 *        inside this module.
 * @version 0.1
 * @date 2026-08-08
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC. All Rights Reserved.
 */

#ifndef INPUT_H_
#define INPUT_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include <stddef.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "queue.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

#define INPUT_TASK_STACK_DEPTH (512U)
#define INPUT_TASK_PRIORITY (tskIDLE_PRIORITY + 3U)
#define INPUT_POLL_PERIOD_MS (10U)

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
 * @brief Initialise the input module.
 *
 *        Creates the binary semaphore used by the door EXTI ISR to wake the
 *        input task.  Must be called before the input task is created.
 */
void input_init(void);

/**
 * @brief Register the door EXTI callback and initialise debounce state.
 *
 *        Reads the door pin, stores the initial state, and registers the
 *        EXTI trigger callback.  Must be called after input_init().
 */
void input_register_exti_cb(void);

/**
 * @brief FreeRTOS input task entry point.
 *
 *        Wakes on door EXTI notification or on the periodic poll timer,
 *        services door debounce, and calls the input-polling placeholder.
 * @param parameters Unused FreeRTOS task parameter.
 */
void input_task_run(void* parameters);

/**
 * @brief ISR-safe entry point called from the door EXTI handler.
 *
 *        Reads the door pin, records the tick of the last edge, and gives
 *        the binary semaphore to wake the input task.  Must only be called
 *        from ISR context.
 */
void input_door_exti_callback_from_isr(void);

/**
 * @brief Get the current door state.
 *
 *        Returns the last-known door pin value read by the input task.
 * @return true if the door is currently closed; otherwise false.
 */
bool input_get_door_closed(void);

/**
 * @brief Get the current lock state.
 *
 *        Returns the last-known lock-detect pin value read by the input task.
 * @return true if the lock is currently confirmed; otherwise false.
 */
bool input_get_lock_confirmed(void);

/**
 * @brief Drive the door lock-control output high to lock the door.
 *
 *        Sets the DOOR_LOCK_CTRL pin high, which commands the lock actuator
 *        to engage.  Actual lock engagement is confirmed separately via
 *        input_get_lock_confirmed().
 */
void input_lock_door(void);

/**
 * @brief Drive the door lock-control output low to unlock the door.
 *
 *        Clears the DOOR_LOCK_CTRL pin, which commands the lock actuator
 *        to release.
 */
void input_unlock_door(void);

/**
 * @brief Get the last commanded lock-control state.
 *
 *        Returns the state last written by input_lock_door() or
 *        input_unlock_door(), not the physical lock feedback.
 * @return true if the lock-control output is currently commanded high (locked); otherwise false.
 */
bool input_get_lock_cmd_locked(void);

/**
 * @brief Get the current reload-switch state.
 *
 *        Returns the last-known reload-switch pin value read by the input task.
 * @return true if reload is currently requested; otherwise false.
 */
bool input_get_reload_requested(void);

#endif /* INPUT_H_ */
