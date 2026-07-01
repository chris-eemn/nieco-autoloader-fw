/**
 * @file    stepper.h
 * @brief   Multi-axis stepper motor control via DRV8424 (Stepper 19 Click)
 *
 * Generates STEP pulses from a shared timer ISR at 100kHz tick rate.
 * DIR, nEN, nSLP, and nFAULT are controlled via GPIO.
 * Up to STEPPER_MAX_MOTORS motors can be registered, each with its own
 * GPIO pin assignments supplied at init time.
 *
 * Typical call sequence:
 *   // Once, before any stepper_init():
 *   stepper_module_init(step_timer_gethandle());
 *
 *   // Once per motor:
 *   stepper_gpio_config_t m1_pins = { ... };
 *   stepper_t *m1 = stepper_init(&m1_pins);
 *
 *   // Per move:
 *   stepper_move_start(m1, steps, rpm, STEPPER_DIR_CW);
 *   stepper_wait_done(m1, STEPPER_TIMEOUT_FOREVER);
 */

#ifndef STEPPER_H
#define STEPPER_H

#include <stdint.h>
#include "stm32_hal.h"

/* -----------------------------------------------------------------------
 * Configuration
 * ----------------------------------------------------------------------- */

/** Maximum number of motor instances that can be registered. */
#define STEPPER_MAX_MOTORS         8U

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
  STEPPER_FULL    = 4,  /* Motor pool exhausted */
  STEPPER_INVALID = 5,  /* NULL handle or config pointer */
} stepper_status_enum;

/** A single GPIO port/pin pair. */
typedef struct {
  hal_gpio_t port;
  uint32_t   pin;
} gpio_pin_t;

/**
 * GPIO pin assignments for one DRV8424 driver instance.
 * Only port and pin vary per motor — signal polarities are fixed by the
 * DRV8424 silicon and are defined as constants inside stepper.c.
 */
typedef struct {
  gpio_pin_t step;
  gpio_pin_t dir;
  gpio_pin_t en;
  gpio_pin_t nslp;
  gpio_pin_t nfault;
} stepper_gpio_config_t;

/** Opaque motor instance handle. Allocated from an internal static pool by stepper_init(). */
typedef struct stepper_s stepper_t;

/**
 * Callback invoked from ISR context when a move completes.
 * Must be ISR-safe: no blocking, no FreeRTOS API except *FromISR variants.
 *
 * @param motor  Handle of the motor whose move just finished.
 */
typedef void (*stepper_done_cb_t)(stepper_t *motor);

/* -----------------------------------------------------------------------
 * Module-level init (call once before any stepper_init)
 * ----------------------------------------------------------------------- */

/**
 * @brief  Register the ISR callback and start the shared step timer.
 *         Call once before any stepper_init() call.
 *
 * @param  htim  Pointer to the configured step timer handle (e.g. TIM12 at 100kHz).
 *               Must not be NULL.
 */
void stepper_module_init(hal_tim_handle_t *htim);

/* -----------------------------------------------------------------------
 * Per-motor API
 * ----------------------------------------------------------------------- */

/**
 * @brief  Allocate and initialise a motor instance from the internal pool.
 *         Configures initial GPIO states and wakes the driver (1ms delay —
 *         call from FreeRTOS task context only).
 *
 * @param  pins  GPIO pin assignments for this motor. Must not be NULL.
 * @return Opaque motor handle on success.
 *         NULL if pins is NULL or the motor pool is exhausted.
 */
stepper_t *stepper_init(const stepper_gpio_config_t *pins);

/**
 * @brief  Start a move and return immediately (non-blocking).
 *         Use stepper_is_busy() to poll, or stepper_wait_done() to block.
 *
 * @param  motor      Handle returned by stepper_init(). Must not be NULL.
 * @param  steps      Number of microsteps to move.
 * @param  rpm        Motor speed in RPM.
 * @param  direction  STEPPER_DIR_CW or STEPPER_DIR_CCW.
 * @return STEPPER_OK      if move started,
 *         STEPPER_BUSY    if a move is already in progress,
 *         STEPPER_FAULT   if nFAULT is asserted,
 *         STEPPER_INVALID if motor is NULL.
 */
stepper_status_enum stepper_move_start(stepper_t *motor, uint32_t steps, uint32_t rpm,
                                       uint8_t direction);

/**
 * @brief  Block the calling task until the current move completes.
 *         Must be called from a FreeRTOS task context.
 *
 * @param  motor       Handle returned by stepper_init(). Must not be NULL.
 * @param  timeout_ms  Maximum wait time in milliseconds.
 *                     Pass STEPPER_TIMEOUT_FOREVER to wait indefinitely.
 * @return STEPPER_OK      on completion,
 *         STEPPER_TIMEOUT if the wait expired,
 *         STEPPER_INVALID if motor is NULL.
 */
stepper_status_enum stepper_wait_done(stepper_t *motor, uint32_t timeout_ms);

/**
 * @brief  Check whether a move is currently in progress.
 *
 * @param  motor  Handle returned by stepper_init(). Must not be NULL.
 * @return 1 if a move is running, 0 if idle or motor is NULL.
 */
uint8_t stepper_is_busy(stepper_t *motor);

/**
 * @brief  Register a callback invoked from ISR context when a move completes.
 *         Pass NULL to clear a previously registered callback.
 *
 * @param  motor  Handle returned by stepper_init(). Must not be NULL.
 * @param  cb     ISR-safe callback, or NULL.
 */
void stepper_register_done_cb(stepper_t *motor, stepper_done_cb_t cb);

/**
 * @brief  Immediately stop the motor and disable driver outputs.
 *
 * @param  motor  Handle returned by stepper_init(). Must not be NULL.
 */
void stepper_stop(stepper_t *motor);

/**
 * @brief  Enable driver outputs (nEN active).
 *
 * @param  motor  Handle returned by stepper_init(). Must not be NULL.
 */
void stepper_enable(stepper_t *motor);

/**
 * @brief  Disable driver outputs (nEN inactive). Motor will freewheel.
 *
 * @param  motor  Handle returned by stepper_init(). Must not be NULL.
 */
void stepper_disable(stepper_t *motor);

/**
 * @brief  Wake the driver (nSLP active).
 *         Blocks 1ms (DRV8424 wake-up requirement). Call from task context only.
 *
 * @param  motor  Handle returned by stepper_init(). Must not be NULL.
 */
void stepper_wake(stepper_t *motor);

/**
 * @brief  Put the driver to sleep (nSLP inactive). Clears any latched fault.
 *
 * @param  motor  Handle returned by stepper_init(). Must not be NULL.
 */
void stepper_sleep(stepper_t *motor);

/**
 * @brief  Check the nFAULT pin.
 *
 * @param  motor  Handle returned by stepper_init(). Must not be NULL.
 * @return 1 if fault is asserted, 0 if clear or motor is NULL.
 */
uint8_t stepper_is_fault(stepper_t *motor);

/**
 * @brief  Convert RPM to timer ticks between step pulse edges.
 *         Exposed for debug/logging purposes.
 *
 * @param  rpm  Desired speed in RPM.
 * @return Number of timer ticks per pulse edge, or UINT32_MAX if rpm is 0.
 */
uint32_t stepper_rpm_to_ticks(uint32_t rpm);

#endif /* STEPPER_H */
