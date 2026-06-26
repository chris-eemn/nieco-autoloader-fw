/**
 * @file stepper_ctrl.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief Shared control interface between the CLI and stepper task.
 *
 *  CLI side  — calls stepper_ctrl_send_cmd() and stepper_ctrl_request_stop().
 *  Task side — calls stepper_ctrl_recv_cmd(), stepper_ctrl_is_stop_requested(),
 *              and stepper_ctrl_notify_idle().
 *
 * @version 0.2
 * @date 2026-06-25
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 */

#ifndef STEPPER_CTRL_H_
#define STEPPER_CTRL_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include <stdint.h>

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/** Default speed used until overridden via CLI. */
#define STEPPER_CTRL_DEFAULT_RPM   5U

/** Default step count per leg (half revolution at 1/8 microstep = 800 usteps). */
#define STEPPER_CTRL_DEFAULT_STEPS 800U

/** Pass to stepper_ctrl_recv_cmd() to block indefinitely. */
#define STEPPER_CTRL_WAIT_FOREVER  UINT32_MAX

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

typedef enum {
  STEPPER_CMD_AUTO_START = 0, /* begin continuous CW->pause->CCW->pause loop */
  STEPPER_CMD_RUN_CW,         /* execute one CW move then return to idle     */
  STEPPER_CMD_RUN_CCW,        /* execute one CCW move then return to idle    */
} stepper_cmd_enum;

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

/**
 * @brief Initialise the control module and create internal FreeRTOS objects.
 *        Call once from the stepper task before entering the main loop.
 */
void stepper_ctrl_init(void);

/**
 * @brief Set the target RPM used for the next move.
 * @param rpm Revolutions per minute (must be > 0).
 */
void stepper_ctrl_set_rpm(uint32_t rpm);

/**
 * @brief Get the currently configured RPM.
 */
uint32_t stepper_ctrl_get_rpm(void);

/**
 * @brief Set the microstep count used for each move leg.
 * @param steps Number of microsteps (must be > 0).
 */
void stepper_ctrl_set_steps(uint32_t steps);

/**
 * @brief Get the currently configured step count.
 */
uint32_t stepper_ctrl_get_steps(void);

/**
 * @brief Send a command to the stepper task. Safe to call from any task context.
 *        Overwrites any pending unprocessed command.
 * @param cmd Command to send.
 */
void stepper_ctrl_send_cmd(stepper_cmd_enum cmd);

/**
 * @brief Block the calling task until a command arrives or timeout expires.
 *        Pass STEPPER_CTRL_WAIT_FOREVER to block indefinitely.
 * @param cmd_out    Receives the command if one arrives. Must not be NULL.
 * @param timeout_ms Milliseconds to wait.
 * @return 1 if a command was received, 0 on timeout or invalid argument.
 */
uint8_t stepper_ctrl_recv_cmd(stepper_cmd_enum *cmd_out, uint32_t timeout_ms);

/**
 * @brief Request an immediate stop. Safe to call from any task context.
 *        Sets the stop flag and halts the motor immediately via stepper_stop().
 */
void stepper_ctrl_request_stop(void);

/**
 * @brief Check whether a stop has been requested.
 * @return 1 if stop is pending, 0 otherwise.
 */
uint8_t stepper_ctrl_is_stop_requested(void);

/**
 * @brief Notify that the task has returned to idle.
 *        Clears the stop flag and the running indicator.
 *        Call from the stepper task each time it enters the IDLE state.
 */
void stepper_ctrl_notify_idle(void);

/**
 * @brief Check whether a move or auto cycle is currently active.
 * @return 1 if running, 0 if idle or stopped.
 */
uint8_t stepper_ctrl_is_running(void);

#endif /* STEPPER_CTRL_H_ */
