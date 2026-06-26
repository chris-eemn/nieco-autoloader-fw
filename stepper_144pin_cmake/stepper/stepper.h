/**
 * @file    stepper.h
 * @brief   Single-axis stepper motor control via DRV8424 (Stepper 19 Click)
 *
 * Generates STEP pulses from a TIM12 ISR at 100kHz tick rate.
 * DIR, nEN, nSLP controlled via GPIO.
 *
 * Non-blocking usage:
 *   stepper_move_start() returns immediately after arming the ISR.
 *   Poll stepper_is_busy() to check progress, or call stepper_wait_done()
 *   to block until completion. Optionally register a callback via
 *   stepper_register_done_cb() to be notified from ISR context.
 *
 * Pin assignments (via CubeMX labels):
 *   M1_STEP   - PB15  - Step pulse output
 *   M1_DIR    - PB14  - Direction output
 *   M1_EN     - PA10  - Enable (active low)
 *   M1_NSLP   - PC1   - Sleep (active low)
 *   M1_NFAULT - PB4   - Fault input (active low, open drain)
 */

#ifndef STEPPER_H
#define STEPPER_H

#include <stdint.h>
#include "mx_tim12.h"

/* -----------------------------------------------------------------------
 * Configuration
 * ----------------------------------------------------------------------- */

/** Timer tick rate configured in CubeMX (Hz). ARR=1439 @ 144MHz = 100kHz */
#define STEPPER_TIMER_TICK_HZ      100000UL

/** Microsteps per full step — must match M0/M1 jumper config on Stepper 19 Click */
#define STEPPER_MICROSTEPS         8U

/** Full steps per revolution for this motor (1.8 deg/step) */
#define STEPPER_FULL_STEPS_PER_REV 200U

/** Derived: total microsteps per revolution */
#define STEPPER_USTEPS_PER_REV     (STEPPER_FULL_STEPS_PER_REV * STEPPER_MICROSTEPS)

/** Pass to stepper_wait_done() to block indefinitely */
#define STEPPER_TIMEOUT_FOREVER    UINT32_MAX

/* -----------------------------------------------------------------------
 * Direction
 * ----------------------------------------------------------------------- */

#define STEPPER_DIR_CW  1U
#define STEPPER_DIR_CCW 0U

/* -----------------------------------------------------------------------
 * Types
 * ----------------------------------------------------------------------- */

typedef enum {
  STEPPER_OK      = 0,
  STEPPER_FAULT   = 1,
  STEPPER_BUSY    = 2,
  STEPPER_TIMEOUT = 3,
} stepper_status_enum;

/**
 * Callback invoked from ISR context when a move completes.
 * Must be ISR-safe: no blocking, no FreeRTOS API except *FromISR variants.
 */
typedef void (*stepper_done_cb_t)(void);

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

/**
 * @brief  Initialise GPIO states and start the step timer.
 *         Call once before any other stepper function.
 *         Must be called from a FreeRTOS task context (stepper_wake blocks 1ms).
 */
void stepper_init(void);

/**
 * @brief  Start a move and return immediately (non-blocking).
 *         Use stepper_is_busy() to poll, or stepper_wait_done() to block.
 *
 * @param  steps      Number of microsteps to move
 * @param  rpm        Motor speed in RPM
 * @param  direction  STEPPER_DIR_CW or STEPPER_DIR_CCW
 * @return STEPPER_OK if move started,
 *         STEPPER_BUSY if a move is already in progress,
 *         STEPPER_FAULT if nFAULT is asserted
 */
stepper_status_enum stepper_move_start(uint32_t steps, uint32_t rpm, uint8_t direction);

/**
 * @brief  Block the calling task until the current move completes.
 *         Must be called from a FreeRTOS task context.
 *
 * @param  timeout_ms  Maximum wait time in milliseconds.
 *                     Pass STEPPER_TIMEOUT_FOREVER to wait indefinitely.
 * @return STEPPER_OK on completion, STEPPER_TIMEOUT if the wait expired.
 */
stepper_status_enum stepper_wait_done(uint32_t timeout_ms);

/**
 * @brief  Check whether a move is currently in progress.
 * @return 1 if a move is running, 0 if idle.
 */
uint8_t stepper_is_busy(void);

/**
 * @brief  Register a callback invoked from ISR when a move completes.
 *         Pass NULL to clear a previously registered callback.
 *
 * @param  cb  Callback function pointer (must be ISR-safe), or NULL.
 */
void stepper_register_done_cb(stepper_done_cb_t cb);

/**
 * @brief  Immediately stop the motor and disable driver outputs.
 */
void stepper_stop(void);

/**
 * @brief  Enable driver outputs (nEN low).
 */
void stepper_enable(void);

/**
 * @brief  Disable driver outputs (nEN high). Motor will freewheel.
 */
void stepper_disable(void);

/**
 * @brief  Wake the driver (nSLP high).
 *         Blocks 1ms (DRV8424 wake-up requirement). Call from task context only.
 */
void stepper_wake(void);

/**
 * @brief  Put the driver to sleep (nSLP low). Clears any latched fault.
 */
void stepper_sleep(void);

/**
 * @brief  Check the nFAULT pin.
 * @return 1 if fault is asserted, 0 if clear.
 */
uint8_t stepper_is_fault(void);

/**
 * @brief  Convert RPM to timer ticks between steps.
 *         Exposed for debug/logging purposes.
 *
 * @param  rpm  Desired speed in RPM
 * @return Number of timer ticks between each step pulse
 */
uint32_t stepper_rpm_to_ticks(uint32_t rpm);

/**
 * @brief  Update callback registered directly against TIM12 handle.
 *         Registered via HAL_TIM_RegisterUpdateCallback() in stepper_init.
 *         Do not call directly.
 */
void stepper_tim_period_elapsed_cb(hal_tim_handle_t *htim);

#endif /* STEPPER_H */
