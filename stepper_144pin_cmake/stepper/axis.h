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
 *   - Per-step following-error stall detection in the step ISR.
 *   - Homing (drive to endstop, back off, zero encoder).
 *   - Fault integration (stepper fault surfaces into combined axis status).
 *
 * All logic is generic. No assumptions are made about what the axis moves.
 *
 * A shared supervisor task is created automatically on the first axis_init()
 * call and runs at a fixed period (default 25 ms, configurable via the first
 * axis's config). All subsequent axes share that period regardless of their own
 * supervisor_period_ms field. Stall and homing-endstop detection occur directly
 * in the step ISR using max_sync_error_counts.
 *
 * Use axis_get_status() and axis_is_busy() to observe non-blocking motion, or
 * register a callback with axis_register_event_cb() to be told when a move or
 * homing sequence reaches a terminal state. The supervisor drives all homing
 * and fault state transitions, and delivers every event.
 *
 * Typical call sequence:
 *   stepper_t  *motor = stepper_init(&pins);
 *   encoder_t  *enc   = encoder_init(htim);
 *
 *   axis_config_t cfg = {
 *     .home_rpm             = 10,
 *     .home_max_steps       = 50000,
 *     .backoff_steps        = 800,
 *     .max_sync_error_counts = 10,
 *   };
 *   axis_t *ax = axis_init(motor, enc, m1_fault_exti_gethandle(), &cfg);
 *   axis_register_event_cb(ax, on_axis_event, (void *)AXIS_ID_LIFT);
 *
 *   axis_home(ax, STEPPER_DIR_CW);
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

/** Bench encoder produces 2.5 counts per microstep at the current 8-microstep setting. */
#define AXIS_DEFAULT_ENCODER_COUNTS_NUMERATOR 5U
#define AXIS_DEFAULT_ENCODER_COUNTS_DENOMINATOR 2U

#define AXIS_DEFAULT_MAX_SYNC_ERROR_COUNTS 10U

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/**
 * Combined axis status.
 * Transitions:
 *   NOT_HOMED → HOMING (via axis_home())
 *   HOMING    → OK     (homing succeeded)
 *   HOMING    → FAULT  (homing timeout or stepper fault)
 *   OK        → STALLED (encoder lag exceeds max_sync_error_counts)
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
  AXIS_STATUS_STALLED = 66,  /*!< Encoder lag exceeded the following-error limit */
  AXIS_STATUS_FAULT = 99,    /*!< Stepper fault or homing timeout               */
  AXIS_STATUS_INVALID = 255, /*!< Returned by axis_get_status() if axis is NULL */
} axis_status_enum;

/**
 * Axis instance handle.
 *
 * Declared ahead of its body to break the cycle between the struct and the
 * event callback: the callback signature takes an axis_t*, and the struct
 * stores an axis_event_cb_t. A pointer to an incomplete type is all the
 * callback typedef needs, so the tag alone is enough here. The body follows
 * once axis_event_cb_t exists.
 */
typedef struct axis_s axis_t;

/* Internal homing sub-states, only meaningful when status == AXIS_STATUS_HOMING. */
typedef enum {
  HOMING_IDLE = 0, /* Not homing                                          */
  HOMING_SEEK,     /* Driving toward endstop, waiting for ISR lag event   */
  HOMING_SETTLE,   /* Settling after endstop hit, before backoff           */
  HOMING_BACKOFF,  /* Backing off from endstop                            */
} homing_state_enum;

/* Operation currently owed a completion event. Distinguishes a user move from
 * the internal seek and back-off moves that homing issues, so that homing
 * reports one AXIS_EVENT_HOME_* rather than an event per stepper move. */
typedef enum {
  AXIS_OP_NONE = 0, /* Idle; no event outstanding */
  AXIS_OP_MOVE,     /* axis_move() in progress    */
  AXIS_OP_HOME,     /* axis_home() in progress    */
} axis_op_enum;

/**
 * Terminal outcome of an axis_move() or axis_home() operation.
 *
 * One event is delivered per accepted operation: a call that returns STEPPER_OK
 * is followed by one of these, and a call that returns any other status produces
 * none. Faults detected while the axis is idle change axis_get_status() but
 * generate no event.
 *
 * Events are delivered on the supervisor tick following the operation's end, so
 * a caller that polls axis_is_busy() and immediately starts the next operation
 * can replace a pending event before it is delivered. Drive the axis from the
 * events or from polling, not from both at once.
 */
typedef enum {
  AXIS_EVENT_MOVE_DONE = 0, /*!< Commanded steps completed                      */
  AXIS_EVENT_MOVE_STOPPED,  /*!< Move ended early by axis_stop()                */
  AXIS_EVENT_MOVE_FAILED,   /*!< Stall or stepper fault ended the move          */
  AXIS_EVENT_HOME_DONE,     /*!< Back-off finished and the encoder was zeroed   */
  AXIS_EVENT_HOME_ABORTED,  /*!< Homing ended early by axis_stop()              */
  AXIS_EVENT_HOME_FAILED,   /*!< Seek timeout, back-off stall, or stepper fault */
} axis_event_enum;
/**
 * Callback invoked when an axis operation reaches a terminal state.
 *
 * Always called from the shared supervisor task, never from ISR context, so
 * blocking FreeRTOS APIs are legal here. It nonetheless runs on the
 * supervisor's stack and delays every other axis's tick, so it must return
 * promptly — post to a queue and return rather than doing work inline.
 *
 * @param axis  Axis that completed the operation.
 * @param event Outcome of the operation.
 * @param ctx   Opaque pointer supplied to axis_register_event_cb().
 */
typedef void (*axis_event_cb_t)(axis_t* axis, axis_event_enum event, void* ctx);

/**
 * Per-axis configuration supplied at axis_init() time.
 * Any field set to 0 receives the documented default.
 */
typedef struct {
  /** Period of the supervisor state-processing tick. Default: AXIS_DEFAULT_SUPERVISOR_PERIOD_MS.
   *  Only honoured for the first axis registered; all subsequent axes share that period. */
  uint32_t supervisor_period_ms;

  /** Integer encoder-counts-per-microstep ratio used by the step ISR. Default: 5/2. */
  uint32_t encoder_counts_numerator;
  uint32_t encoder_counts_denominator;

  /** Encoder following-error threshold. Lag stops immediately; lead only reports. */
  uint32_t max_sync_error_counts;

  /** Back-off distance in microsteps after endstop is detected (no default — must be set). */
  uint32_t backoff_steps;

  /** Speed for both the homing seek and back-off moves, in RPM (must be > 0). */
  uint32_t home_rpm;

  /** Maximum microsteps allowed during the homing seek before declaring a timeout fault.
   *  The stepper enforces this limit automatically at the driver level (must be > 0). */
  uint32_t home_max_steps;

  uint32_t settle_delay_ms; /**< Time to wait after hitting the endstop before starting the back-off move. */
} axis_config_t;

struct axis_s {
  uint8_t num; /**< Axis number 1-8 */
  stepper_t* motor;
  encoder_t* encoder;
  axis_config_t config;

  volatile axis_status_enum status;
  volatile uint8_t fault_from_isr;      /* Set by fault ISR callback; observed by supervisor */
  volatile uint8_t fault_reset_pending; /* 1=sleep requested, 2=sleeping (wake next tick)   */

  /* Completion reporting */
  volatile axis_op_enum active_op; /* Operation awaiting a terminal event              */
  volatile uint8_t stop_pending;   /* Set by axis_stop(); observed by supervisor       */
  axis_event_cb_t event_cb;        /* Registered completion callback, or NULL          */
  void* event_ctx;                 /* Opaque context passed back to event_cb           */

  /* Homing */
  homing_state_enum homing_substate;
  uint8_t home_direction; /**< Seek direction of the active homing sequence, captured from axis_home().
                           *!< The back-off move drives opposite to it.                                */

  int32_t home_start_counts;  /**< Encoder count sampled when axis_home() accepted the sequence.       */
  int32_t home_travel_counts; /**< Net encoder displacement from the start of the seek to the end of
                               *!< the back-off, latched just before the encoder is zeroed. Reported by
                               *!< axis_get_home_travel_counts().                                      */

  uint32_t settle_delay_ms;    /**< Time to wait after hitting the endstop before starting the back-off move. */
  uint32_t settle_end_time_ms; /**< Absolute time when the settle delay ends. Written by the supervisor task. */
};

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
axis_t* axis_init(stepper_t* motor, encoder_t* encoder, hal_exti_handle_t* hexti, const axis_config_t* config);

/**
 * @brief  Return the current combined axis status.
 *
 * @param  axis  Handle returned by axis_init(). Must not be NULL.
 * @return Current status, or AXIS_STATUS_FAULT if axis is NULL.
 */
axis_status_enum axis_get_status(const axis_t* axis);

/**
 * @brief  Register the callback invoked when a move or homing sequence reaches
 *         a terminal state. Pass NULL to clear a previously registered callback.
 *         Replaces any previous registration; only one callback per axis.
 *
 *         Safe to call while an operation is in progress — the new callback
 *         receives that operation's completion event.
 *
 * @param  axis  Handle returned by axis_init(). Must not be NULL.
 * @param  cb    Callback invoked from the supervisor task, or NULL.
 * @param  ctx   Opaque pointer passed back to the callback unmodified. Useful
 *               for carrying a caller-side axis identifier or queue handle.
 */
void axis_register_event_cb(axis_t* axis, axis_event_cb_t cb, void* ctx);

/**
 * @brief  Return a printable name for an event code, for logging.
 *         The table lives in axis.c, so including this header does not place a
 *         copy of it in every translation unit.
 *
 * @param  event  Event code received by an axis_event_cb_t.
 * @return Static string such as "MOVE_DONE", or "UNKNOWN" if event is out of
 *         range. Never NULL, and safe to pass straight to a %s format.
 */
const char* axis_event_name(axis_event_enum event);

/**
 * @brief  Return the current encoder count for the axis.
 *
 * @param  axis  Handle returned by axis_init(). Must not be NULL.
 * @return Signed encoder count, or 0 if axis is NULL.
 */
int32_t axis_get_encoder_count(const axis_t* axis);

/**
 * @brief  Start a non-blocking move on the axis.
 *         Does NOT require the axis to have been homed first — AXIS_STATUS_NOT_HOMED
 *         only reflects that the encoder position reference is not yet zeroed;
 *         it does not block motion.
 *         The step ISR's following-error detection runs automatically during the move
 *         for axes in both AXIS_STATUS_OK and AXIS_STATUS_NOT_HOMED states.
 *
 *         On STEPPER_OK, exactly one AXIS_EVENT_MOVE_* event is delivered to the
 *         registered callback when the move ends.
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
stepper_status_enum axis_move(axis_t* axis, uint32_t steps, uint32_t rpm, uint8_t direction);

/**
 * @brief Report whether the axis motor is currently executing a move.
 * @param axis Handle returned by axis_init(). Must not be NULL.
 * @return 1 when a move is active, or 0 when idle or axis is NULL.
 */
uint8_t axis_is_busy(const axis_t* axis);

/**
 * @brief  Start a non-blocking homing sequence.
 *         Drives in the requested direction until encoder lag reaches
 *         config.max_sync_error_counts, then backs off by config.backoff_steps
 *         in the opposite direction and zeros the encoder. Transitions axis
 *         status to AXIS_STATUS_HOMING.
 *
 *         Use axis_get_status() to poll for AXIS_STATUS_OK (success) or
 *         AXIS_STATUS_FAULT (timeout / stepper fault). Alternatively, on
 *         STEPPER_OK exactly one AXIS_EVENT_HOME_* event is delivered to the
 *         registered callback when the sequence ends.
 *
 *         On success the distance covered is available from
 *         axis_get_home_travel_counts().
 *
 * @param  axis       Handle returned by axis_init(). Must not be NULL.
 * @param  direction  Seek direction: STEPPER_DIR_CW or STEPPER_DIR_CCW.
 *                    The back-off move drives the other way.
 * @return STEPPER_OK      if homing was started,
 *         STEPPER_BUSY    if homing or a move is already in progress,
 *         STEPPER_FAULT   if a fault is latched (clear it first),
 *         STEPPER_INVALID if axis is NULL or direction is not a valid
 *                         STEPPER_DIR_* value.
 */
stepper_status_enum axis_home(axis_t* axis, uint8_t direction);

/**
 * @brief  Return how far the axis travelled during the last successful homing
 *         sequence: the net encoder displacement from the start of the seek to
 *         the end of the back-off. A seek of 10000 counts followed by a 200
 *         count back-off reports 9800.
 *
 *         Latched immediately before the encoder is zeroed, so it is already
 *         valid when AXIS_EVENT_HOME_DONE is delivered and can be read from
 *         inside the event callback.
 *
 *         The value is signed and follows the encoder's own sign convention, so
 *         a seek in the negative-counting direction reports a negative number.
 *         Take the absolute value for a direction-independent distance.
 *
 *         Cleared to 0 by each axis_home() call, so a sequence that ends in
 *         AXIS_EVENT_HOME_FAILED or AXIS_EVENT_HOME_ABORTED reports 0 rather
 *         than the previous sequence's distance.
 *
 * @param  axis  Handle returned by axis_init(). Must not be NULL.
 * @return Net encoder counts travelled, or 0 if axis is NULL or the last homing
 *         sequence did not complete.
 */
int32_t axis_get_home_travel_counts(const axis_t* axis);

/**
 * @brief  Clear a latched AXIS_STATUS_STALLED or AXIS_STATUS_FAULT and return
 *         the axis to AXIS_STATUS_NOT_HOMED. Intended for stall recovery where
 *         no hardware reset is needed. For stepper fault recovery use
 *         axis_fault_reset() instead, which also resets the DRV8424 hardware.
 *
 * @param  axis  Handle returned by axis_init(). Must not be NULL.
 */
void axis_clear_fault(axis_t* axis);

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
void axis_fault_reset(axis_t* axis);

/**
 * @brief  Immediately stop any in-progress move on the axis.
 *         If the axis was homing, transitions to AXIS_STATUS_NOT_HOMED
 *         (homing is aborted cleanly, not a fault).
 *         For AXIS_STATUS_OK and AXIS_STATUS_NOT_HOMED the status is unchanged —
 *         a voluntary stop is not a fault condition.
 *
 *         The motor stops synchronously, but the resulting AXIS_EVENT_MOVE_STOPPED
 *         or AXIS_EVENT_HOME_ABORTED event is deferred to the next supervisor tick
 *         so that all events originate from the supervisor task. Stopping an idle
 *         axis produces no event.
 *
 * @param  axis  Handle returned by axis_init(). Must not be NULL.
 */
void axis_stop(axis_t* axis);

/**
 * @brief  Update the axis configuration at runtime.
 *
 * @param  axis  Handle returned by axis_init(). Must not be NULL.
 * @param  config  New configuration to apply. Must not be NULL.
 */
void axis_update_config(axis_t* axis, const axis_config_t* config);
#endif /* AXIS_H_ */
