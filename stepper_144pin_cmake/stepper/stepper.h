/**
 * @file stepper.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief Multi-axis stepper motor control via DRV8424 (Stepper 19 Click).
 * @version 0.1
 * @date 2026-07-02
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
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
 *   while (stepper_is_busy(m1) != 0U) { vTaskDelay(1); }
 */

#ifndef STEPPER_H_
#define STEPPER_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include <stdint.h>
#include "encoder.h"
#include "stm32_hal.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/** Maximum number of motor instances that can be registered. */
#define STEPPER_MAX_MOTORS 8U

/** Timer tick rate configured in CubeMX (Hz). ARR=1439 @ 144MHz = 100kHz */
#define STEPPER_TIMER_TICK_HZ 100000UL

/** Microsteps per full step — must match M0/M1 jumper config on Stepper 19 Click */
#define STEPPER_MICROSTEPS 8U

/** Full steps per revolution for this motor (1.8 deg/step) */
#define STEPPER_FULL_STEPS_PER_REV 200U

/** Derived: total microsteps per revolution */
#define STEPPER_USTEPS_PER_REV (STEPPER_FULL_STEPS_PER_REV * STEPPER_MICROSTEPS)

#define STEPPER_DIR_CW 1U
#define STEPPER_DIR_CCW 0U

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

typedef enum {
  STEPPER_OK = 0,
  STEPPER_FAULT = 1,
  STEPPER_BUSY = 2,
  STEPPER_FULL = 4,    /* Motor pool exhausted */
  STEPPER_INVALID = 5, /* NULL handle or config pointer */
} stepper_status_enum;

/** Per-step encoder following-error event reported from the step ISR. */
typedef enum {
  STEPPER_SYNC_EVENT_NONE = 0,
  STEPPER_SYNC_EVENT_LAG,  /*!< Encoder fell behind; the ISR stopped the motor. */
  STEPPER_SYNC_EVENT_LEAD, /*!< Encoder ran ahead; motion was allowed to continue. */
} stepper_sync_event_enum;

/** Integer-ratio configuration for per-step encoder following-error monitoring. */
typedef struct {
  uint32_t encoder_counts_numerator;   /*!< Encoder counts per microstep numerator. */
  uint32_t encoder_counts_denominator; /*!< Encoder counts per microstep denominator. */
  uint32_t max_error_counts;           /*!< Lag/lead threshold in encoder counts. */
} stepper_sync_config_t;

/** A single GPIO port/pin pair. */
typedef struct {
  hal_gpio_t port;
  uint32_t pin;
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
typedef void (*stepper_done_cb_t)(stepper_t* motor);

/**
 * Callback invoked from ISR context when a fault is detected.
 * Must be ISR-safe: no blocking, no FreeRTOS API except *FromISR variants.
 *
 * @param motor  Handle of the motor on which the fault was detected.
 */
typedef void (*stepper_fault_cb_t)(stepper_t* motor);

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

/**
 * @brief  Register the ISR callback and start the shared step timer.
 *         Call once before any stepper_init() call.
 *
 * @param  htim  Pointer to the configured step timer handle (e.g. TIM12 at 100kHz).
 *               Must not be NULL.
 */
void stepper_module_init(hal_tim_handle_t* htim);

/**
 * @brief  Allocate and initialise a motor instance from the internal pool.
 *         Configures initial GPIO states and wakes the driver (1ms delay —
 *         call from FreeRTOS task context only).
 *
 * @param  pins  GPIO pin assignments for this motor. Must not be NULL.
 * @return Opaque motor handle on success.
 *         NULL if pins is NULL or the motor pool is exhausted.
 */
stepper_t* stepper_init(const stepper_gpio_config_t* pins);

/**
 * @brief Configure per-step encoder following-error monitoring for a motor.
 *
 *        Once configured, the shared step ISR reads the encoder
 * after every
 *        completed microstep. Encoder lag at the configured limit immediately
 *        stops and disables the motor. Encoder lead
 * records a warning event but
 *        does not stop motion.
 *
 * @param motor   Handle returned by stepper_init(). Must not be NULL or running.
 *
 * @param encoder Encoder paired with the motor. Must not be NULL.
 * @param config  Non-zero encoder ratio and error threshold. Must not be NULL.
 *
 * @return STEPPER_OK on success, STEPPER_BUSY if the motor is running, or
 *         STEPPER_INVALID for a NULL argument or zero-valued config
 * field.
 */
stepper_status_enum stepper_sync_configure(stepper_t* motor, encoder_t* encoder, const stepper_sync_config_t* config);

/**
 * @brief Take one pending encoder synchronization event.
 *
 *        Must be called from task context. The event is cleared atomically after
 *
 * it is copied. A lag event means the step ISR has already stopped the
 *        motor; a lead event is informational only.
 *
 * @param motor Handle
 * returned by stepper_init(). Must not be NULL.
 * @param deviation_counts Receives the following error in encoder counts. May be NULL.
 * @return
 * Pending event, or STEPPER_SYNC_EVENT_NONE if no event is pending.
 */
stepper_sync_event_enum stepper_sync_take_event(stepper_t* motor, uint32_t* deviation_counts);

/**
 * @brief  Start a move and return immediately (non-blocking).
 *         Use stepper_is_busy() to poll for completion.
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
stepper_status_enum stepper_move_start(stepper_t *motor, uint32_t steps, uint32_t rpm, uint8_t direction);

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
 * @brief  Register an EXTI handle for continuous nFAULT monitoring.
 *         Attaches an ISR-safe fault callback to the EXTI handle so that
 *         any falling edge on nFAULT is detected and latched immediately,
 *         even during a move. Call after stepper_init() but before any move.
 *
 * @param  motor  Handle returned by stepper_init(). Must not be NULL.
 * @param  hexti  EXTI handle pre-configured for falling-edge on this motor's
 *                nFAULT GPIO line (e.g. from m1_fault_exti_gethandle()).
 *                Must not be NULL.
 */
void stepper_register_fault_exti(stepper_t *motor, hal_exti_handle_t *hexti);

/**
 * @brief  Register a callback invoked from ISR context when a fault is detected.
 *         The callback fires for both EXTI-detected faults and faults latched
 *         during stepper_move_start(). Pass NULL to clear a prior callback.
 *
 * @param  motor  Handle returned by stepper_init(). Must not be NULL.
 * @param  cb     ISR-safe callback, or NULL.
 */
void stepper_register_fault_cb(stepper_t *motor, stepper_fault_cb_t cb);

/**
 * @brief  Clear the software fault latch.
 *         Does NOT clear the DRV8424 hardware fault — the caller must also
 *         perform a stepper_sleep() / stepper_wake() cycle to reset the
 *         hardware (see stepper_sleep() note about latched-fault clearing).
 *
 * @param  motor  Handle returned by stepper_init(). Must not be NULL.
 */
void stepper_clear_fault(stepper_t *motor);

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
 * @brief  Check whether a fault has been latched for this motor.
 *
 *         Returns the latched fault state rather than sampling the live nFAULT pin.
 *         The latch is set by:
 *           - The EXTI ISR when nFAULT falls during or between moves (if registered),
 *           - A live-pin check inside stepper_move_start() at move-start time.
 *         The latch persists until stepper_clear_fault() is called explicitly.
 *
 * @param  motor  Handle returned by stepper_init(). Must not be NULL.
 * @return 1 if a fault is latched, 0 if no fault or motor is NULL.
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

#endif /* STEPPER_H_ */
