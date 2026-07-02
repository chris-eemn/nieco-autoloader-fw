/**
 * @file encoder.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief Generic quadrature encoder wrapper (STM32 timer in encoder mode, x4).
 * @version 0.1
 * @date 2026-07-02
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 * One encoder instance wraps one timer handle that has already been configured
 * in encoder mode and started by the caller (e.g. via m1_encoder_timer_init()
 * followed by HAL_TIM_IC_StartChannel() and HAL_TIM_Start()).
 *
 * This module is intentionally "dumb" — it is a position source only.
 * Homing, stall detection, and closed-loop logic belong in axis.c.
 *
 * Typical call sequence:
 *   hal_tim_handle_t *htim = m1_encoder_timer_init();
 *   HAL_TIM_IC_StartChannel(htim, HAL_TIM_CHANNEL_1);
 *   HAL_TIM_IC_StartChannel(htim, HAL_TIM_CHANNEL_2);
 *   HAL_TIM_Start(htim);
 *
 *   encoder_t *enc = encoder_init(htim);
 *   encoder_zero(enc);
 *
 *   int32_t pos = encoder_get_count(enc);
 */

#ifndef ENCODER_H_
#define ENCODER_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include <stdint.h>
#include "stm32_hal.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/** Maximum number of encoder instances that can be registered. */
#define ENCODER_MAX_INSTANCES 8U

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/** Opaque encoder instance handle. Allocated from an internal static pool. */
typedef struct encoder_s encoder_t;

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

/**
 * @brief  Allocate and initialise an encoder instance from the internal pool.
 *         The timer must already be configured in encoder mode and started.
 *
 * @param  htim  Pointer to the timer handle configured in encoder mode (x4).
 *               Must not be NULL.
 * @return Opaque encoder handle on success.
 *         NULL if htim is NULL or the encoder pool is exhausted.
 */
encoder_t *encoder_init(hal_tim_handle_t *htim);

/**
 * @brief  Read the current accumulated encoder count since the last zero.
 *
 *         Internally reads the hardware counter and updates the accumulator
 *         using a signed 16-bit delta so that counter wrap-around is handled
 *         transparently (valid for displacements < 32768 counts between calls).
 *
 * @param  enc  Handle returned by encoder_init(). Must not be NULL.
 * @return Accumulated signed count from the last encoder_zero(), or 0 if NULL.
 */
int32_t encoder_get_count(encoder_t *enc);

/**
 * @brief  Read the signed delta since the last call to encoder_get_delta().
 *
 *         Internally calls encoder_get_count() to update the accumulator,
 *         then returns the change since the previous encoder_get_delta() call.
 *
 * @param  enc  Handle returned by encoder_init(). Must not be NULL.
 * @return Signed delta count, or 0 if NULL.
 */
int32_t encoder_get_delta(encoder_t *enc);

/**
 * @brief  Zero the encoder position reference.
 *         Sets the hardware counter to 0x8000 (mid-range of the 16-bit counter)
 *         to allow symmetric bidirectional travel before wrap-around, then
 *         resets the software accumulator to zero.
 *
 * @param  enc  Handle returned by encoder_init(). Must not be NULL.
 */
void encoder_zero(encoder_t *enc);

/**
 * @brief  Register an index (Z) pulse callback.
 *
 *         TODO: Not yet implemented — index wiring (EXTI vs. secondary timer
 *         channel) is not yet confirmed in hardware. This stub reserves the API
 *         so that axis.c callers can be written against the final interface
 *         without a rework when the wiring is finalised.
 *
 * @param  enc  Handle returned by encoder_init(). Must not be NULL.
 * @param  cb   Callback to invoke on index pulse. Pass NULL to clear.
 */
void encoder_register_index_cb(encoder_t *enc, void (*cb)(encoder_t *enc));

#endif /* ENCODER_H_ */
