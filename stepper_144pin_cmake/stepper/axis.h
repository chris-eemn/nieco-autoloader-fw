/**
 * @file axis.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief Generic single-axis abstraction — stepper + encoder with closed-loop supervision.
 * @version 0.1
 * @date 2026-07-02
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 * Composes one stepper_t and one encoder_t into a single, generic axis. Adds:
 *   - Continuous stall detection (encoder stopped while motor commanded to run).
 *   - Homing (drive to endstop, back off, zero encoder).
 *   - Fault integration (stepper fault surfaces into combined axis status).
 *
 * All logic is generic. No assumptions are made about what the axis moves.
 *
 * A shared supervisor task is created automatically on the first axis_init()
 * call and runs at a fixed period (default 50 ms, configurable via the first
 * axis's config). All subsequent axes share that period regardless of their own
 * supervisor_period_ms field. Homing and stall windows (in ms) are converted to
 * sample counts at axis_init() time using the period that was established.
 *
 * NOTE: Do not call stepper_wait_done() directly on an axis's motor while using
 * axis_home() — use axis_get_status() to poll for completion instead. The
 * supervisor drives all state transitions; mixing the raw stepper blocking API
 * with axis supervision leads to undefined behaviour.
 *
 * Typical call sequence:
 *   stepper_t  *motor = stepper_init(&pins);
 *   encoder_t  *enc   = encoder_init(htim);
 *
 *   axis_config_t cfg = {
 *     .home_direction       = STEPPER_DIR_CW,
 *     .home_rpm             = 10,
 *     .home_max_steps       = 50000,
 *     .backoff_steps        = 800,
 *     .stationary_window_ms = 50,
 *     .stall_window_ms      = 100,
 *   };
 *   axis_t *ax = axis_init(motor, enc, m1_fault_exti_gethandle(), &cfg);
 *
 *   axis_home(ax);
 *   while (axis_get_status(ax) == AXIS_STATUS_HOMING) { vTaskDelay(10); }
 */

#ifndef AXIS_H_
#define AXIS_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include <stdint.h>
#include "stepper.h"
#include "encoder.h"
#include "stm32_hal.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/** Maximum number of axis instances that can be registered. */
#define AXIS_MAX_INSTANCES 8U

/** Default supervisor task period in ms, applied when config.supervisor_period_ms == 0. */
#define AXIS_DEFAULT_SUPERVISOR_PERIOD_MS 25U

/** Default stationary/stall window in ms, applied when the config field is 0. */
#define AXIS_DEFAULT_WINDOW_MS 150U

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/**
 * Combined axis status.
 * Transitions:
 *   NOT_HOMED → HOMING (via axis_home())
 *   HOMING    → OK     (homing succeeded)
 *   HOMING    → FAULT  (homing timeout or stepper fault)
 *   OK        → STALLED (encoder stopped while commanded moving)
 *   OK/HOMING → FAULT  (stepper fault detected)
 *   STALLED   → NOT_HOMED (via axis_clear_fault())
 *   FAULT     → NOT_HOMED (via axis_fault_reset(), deferred over two supervisor ticks)
 */
typedef enum {
  AXIS_STATUS_NOT_HOMED = 0, /*!< Initial state; encoder position has not been zeroed.
                              *!< axis_move() is still permitted in this state — homing
                              *!< is not a precondition for motion, only for having a
                              *!< known absolute position/encoder reference. */
  AXIS_STATUS_HOMING,        /*!< Homing move in progress                      */
  AXIS_STATUS_OK,            /*!< Normal operating state                        */
  AXIS_STATUS_STALLED=66,       /*!< Encoder did not move while motor was running  */
  AXIS_STATUS_FAULT=99,         /*!< Stepper fault or homing timeout               */
  AXIS_STATUS_INVALID=255,      /*!< Returned by axis_get_status() if axis is NULL */
} axis_status_enum;

/**
 * Per-axis configuration supplied at axis_init() time.
 * Any field set to 0 receives the documented default.
 */
typedef struct {
  /** Period of the supervisor encoder-sample tick. Default: AXIS_DEFAULT_SUPERVISOR_PERIOD_MS.
   *  Only honoured for the first axis registered; all subsequent axes share that period. */
  uint32_t supervisor_period_ms;

  /** How long the encoder must read zero-delta before declaring the endstop reached
   *  during homing. Default: AXIS_DEFAULT_WINDOW_MS. */
  uint32_t stationary_window_ms;

  /** How long the encoder must read zero-delta while the motor is commanded running
   *  before declaring a stall. Default: AXIS_DEFAULT_WINDOW_MS. */
  uint32_t stall_window_ms;

  /** Back-off distance in microsteps after endstop is detected (no default — must be set). */
  uint32_t backoff_steps;

  /** Direction to drive during homing seek. STEPPER_DIR_CW or STEPPER_DIR_CCW. */
  uint8_t home_direction;

  /** Speed for both the homing seek and back-off moves, in RPM (must be > 0). */
  uint32_t home_rpm;

  /** Maximum microsteps allowed during the homing seek before declaring a timeout fault.
   *  The stepper enforces this limit automatically at the driver level (must be > 0). */
  uint32_t home_max_steps;
} axis_config_t;

/** Opaque axis handle. Allocated from an internal static pool by axis_init(). */
typedef struct axis_s axis_t;

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

/**
 * @brief  Allocate and initialise an axis instance from the internal pool.
 *         Creates the shared supervisor task on the first call.
 *         Registers continuous nFAULT EXTI monitoring for the motor as part
 *         of init — hexti is a required parameter (not an optional follow-up
 *         call) so that fault monitoring cannot be silently left unwired for
 *         a given axis.
 *         Must be called from a FreeRTOS task context (task creation requires
 *         the scheduler to not yet have been started, OR to be running already
 *         when called from a task — see FreeRTOS xTaskCreate() requirements).
 *
 * @param  motor   Stepper handle from stepper_init(). Must not be NULL.
 * @param  encoder Encoder handle from encoder_init(). Must not be NULL.
 * @param  hexti   EXTI handle pre-configured for falling-edge on this motor's
 *                 nFAULT GPIO line (e.g. from mN_fault_exti_gethandle()).
 *                 Must not be NULL.
 * @param  config  Per-axis configuration. Must not be NULL.
 * @return Opaque axis handle on success.
 *         NULL if any argument is NULL or the axis pool is exhausted.
 */
axis_t *axis_init(stepper_t *motor, encoder_t *encoder, hal_exti_handle_t *hexti, const axis_config_t *config);

/**
 * @brief  Return the current combined axis status.
 *
 * @param  axis  Handle returned by axis_init(). Must not be NULL.
 * @return Current status, or AXIS_STATUS_FAULT if axis is NULL.
 */
axis_status_enum axis_get_status(const axis_t *axis);

/**
 * @brief  Return the current encoder count for the axis.
 *
 * @param  axis  Handle returned by axis_init(). Must not be NULL.
 * @return Signed encoder count, or 0 if axis is NULL.
 */
int32_t axis_get_encoder_count(const axis_t *axis);

/**
 * @brief  Start a non-blocking move on the axis.
 *         Does NOT require the axis to have been homed first — AXIS_STATUS_NOT_HOMED
 *         only reflects that the encoder position reference is not yet zeroed;
 *         it does not block motion.
 *         The supervisor's stall detection runs automatically during the move
 *         for axes in both AXIS_STATUS_OK and AXIS_STATUS_NOT_HOMED states.
 *
 * @param  axis       Handle returned by axis_init(). Must not be NULL.
 * @param  steps      Number of microsteps to move.
 * @param  rpm        Speed in RPM.
 * @param  direction  STEPPER_DIR_CW or STEPPER_DIR_CCW.
 * @return STEPPER_OK      if the move started,
 *         STEPPER_BUSY    if a move or homing sequence is already in progress,
 *         STEPPER_FAULT   if a fault or stall is latched (clear it first),
 *         STEPPER_INVALID if axis is NULL.
 */
stepper_status_enum axis_move(axis_t *axis, uint32_t steps, uint32_t rpm, uint8_t direction);

/**
 * @brief Report whether the axis motor is currently executing a move.
 * @param axis Handle returned by axis_init(). Must not be NULL.
 * @return 1 when a move is active, or 0 when idle or axis is NULL.
 */
uint8_t axis_is_busy(const axis_t *axis);

/**
 * @brief  Start a non-blocking homing sequence.
 *         Drives in config.home_direction until the encoder is stationary for
 *         config.stationary_window_ms, then backs off by config.backoff_steps,
 *         then zeros the encoder. Transitions axis status to AXIS_STATUS_HOMING.
 *
 *         Use axis_get_status() to poll for AXIS_STATUS_OK (success) or
 *         AXIS_STATUS_FAULT (timeout / stepper fault).
 *
 * @param  axis  Handle returned by axis_init(). Must not be NULL.
 * @return STEPPER_OK      if homing was started,
 *         STEPPER_BUSY    if homing or a move is already in progress,
 *         STEPPER_FAULT   if a fault is latched (clear it first),
 *         STEPPER_INVALID if axis is NULL.
 */
stepper_status_enum axis_home(axis_t *axis);

/**
 * @brief  Clear a latched AXIS_STATUS_STALLED or AXIS_STATUS_FAULT and return
 *         the axis to AXIS_STATUS_NOT_HOMED. Intended for stall recovery where
 *         no hardware reset is needed. For stepper fault recovery use
 *         axis_fault_reset() instead, which also resets the DRV8424 hardware.
 *
 * @param  axis  Handle returned by axis_init(). Must not be NULL.
 */
void axis_clear_fault(axis_t *axis);

/**
 * @brief  Request a full fault recovery sequence.
 *         Safe to call from any context (task or ISR) — sets a flag and
 *         returns immediately. The supervisor task executes the actual reset
 *         over two ticks:
 *           Tick 1: asserts nSLP low (DRV8424 sleep).
 *           Tick 2: deasserts nSLP, clears the stepper and axis fault latches,
 *                   and transitions the axis to AXIS_STATUS_NOT_HOMED.
 *         Using the supervisor period as the sleep pulse (≥25ms) satisfies the
 *         DRV8424 minimum without requiring an explicit delay. Any nFAULT edge
 *         that fires during the sleep pulse is consumed by the latch clear on
 *         tick 2 and does not re-fault the axis.
 *
 * @param  axis  Handle returned by axis_init(). Must not be NULL.
 */
void axis_fault_reset(axis_t *axis);

/**
 * @brief  Immediately stop any in-progress move on the axis.
 *         Clears stall/stationary counters. If the axis was homing, transitions
 *         to AXIS_STATUS_NOT_HOMED (homing is aborted cleanly, not a fault).
 *         For AXIS_STATUS_OK and AXIS_STATUS_NOT_HOMED the status is unchanged —
 *         a voluntary stop is not a fault condition.
 *
 * @param  axis  Handle returned by axis_init(). Must not be NULL.
 */
void axis_stop(axis_t *axis);

#endif /* AXIS_H_ */
