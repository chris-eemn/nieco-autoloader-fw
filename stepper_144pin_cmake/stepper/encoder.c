/**
 * @file encoder.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief Generic quadrature encoder wrapper (STM32 timer in encoder mode, x4).
 * @version 0.1
 * @date 2026-07-02
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include "encoder.h"
#include "app_console.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/* Which timer peripheral backs a given encoder instance. Needed because TIM and
 * LPTIM expose different HAL APIs for reading/zeroing the hardware counter
 * (e.g. m8_encoder_timer is LPTIM-backed; all other encoder timers are TIM-backed). */
typedef enum {
  ENCODER_SRC_TIM = 0,
  ENCODER_SRC_LPTIM,
} encoder_src_enum;

struct encoder_s {
  encoder_src_enum src; /* Which HAL API to use for this instance's timer handle */
  union {
    hal_tim_handle_t   *htim;   /* Valid when src == ENCODER_SRC_TIM             */
    hal_lptim_handle_t *hlptim; /* Valid when src == ENCODER_SRC_LPTIM           */
  } handle;
  int32_t           accumulated; /* Signed count from last encoder_zero()            */
  uint32_t          last_raw;    /* Raw counter captured at last get_count()         */
  int32_t           prev_count;  /* Accumulated count at last encoder_get_delta() call */
  void (*index_cb)(encoder_t *); /* Index/Z pulse callback (stub, not yet wired)     */
};

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

static encoder_t s_encoders[ENCODER_MAX_INSTANCES];
static uint8_t   s_encoder_count = 0U;

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

encoder_t *encoder_init(hal_tim_handle_t *htim) {
  if (htim == NULL) {
    return NULL;
  }

  if (s_encoder_count >= ENCODER_MAX_INSTANCES) {
    app_console_print("[ENCODER] ERROR: pool full (%u/%u)\r\n", (unsigned)s_encoder_count, (unsigned)ENCODER_MAX_INSTANCES);
    return NULL;
  }

  encoder_t *enc = &s_encoders[s_encoder_count];
  s_encoder_count++;

  enc->src         = ENCODER_SRC_TIM;
  enc->handle.htim = htim;
  enc->accumulated = 0;
  enc->last_raw    = HAL_TIM_GetCounter(htim);
  enc->prev_count  = 0;
  enc->index_cb    = NULL;

  app_console_print("[ENCODER] Instance %u init OK. raw=%lu\r\n", (unsigned)(s_encoder_count - 1U), enc->last_raw);

  return enc;
}

encoder_t *encoder_init_lptim(hal_lptim_handle_t *hlptim) {
  if (hlptim == NULL) {
    return NULL;
  }

  if (s_encoder_count >= ENCODER_MAX_INSTANCES) {
    app_console_print("[ENCODER] ERROR: pool full (%u/%u)\r\n", (unsigned)s_encoder_count, (unsigned)ENCODER_MAX_INSTANCES);
    return NULL;
  }

  encoder_t* enc = &s_encoders[s_encoder_count];
  s_encoder_count++;

  enc->src = ENCODER_SRC_LPTIM;
  enc->handle.hlptim = hlptim;
  enc->accumulated = 0;
  enc->last_raw = HAL_LPTIM_GetCounter(hlptim);
  enc->prev_count = 0;
  enc->index_cb = NULL;

  app_console_print("[ENCODER] Instance %u init OK (LPTIM). raw=%lu\r\n", (unsigned)(s_encoder_count - 1U), enc->last_raw);

  return enc;
}

int32_t encoder_get_count(encoder_t* enc) {
  if (enc == NULL) {
    return 0;
  }

  uint32_t raw = encoder_get_raw(enc);
  /* Cast through uint16_t so the subtraction wraps correctly for a 16-bit
   * free-running counter. The int16_t cast yields the signed displacement. */
  // this trick only works if the timer ARR/period is 0xffff
  int16_t delta = (int16_t)((uint16_t)raw - (uint16_t)enc->last_raw);

  enc->last_raw = raw;
  enc->accumulated += (int32_t)delta;

  return enc->accumulated;
}

uint32_t encoder_get_raw(const encoder_t* enc) {
  uint32_t raw = 0U;

  if (enc != NULL) {
    raw = (enc->src == ENCODER_SRC_LPTIM) ? HAL_LPTIM_GetCounter(enc->handle.hlptim) : HAL_TIM_GetCounter(enc->handle.htim);
  }

  return raw;
}

int32_t encoder_get_delta(encoder_t* enc) {
  if (enc == NULL) {
    return 0;
  }

  int32_t current = encoder_get_count(enc);
  int32_t delta = current - enc->prev_count;
  enc->prev_count    = current;

  return delta;
}

void encoder_zero(encoder_t *enc) {
  if (enc == NULL) {
    return;
  }

  if (enc->src == ENCODER_SRC_LPTIM) {
    /* LPTIM only exposes a reset-to-zero, not an arbitrary SetCounter like TIM.
     * Zero is still a valid symmetric-range reference point since encoder_get_count()
     * computes deltas via unsigned 16-bit wraparound regardless of the starting value. */
    if (HAL_LPTIM_ResetCounter(enc->handle.hlptim) != HAL_OK) {
      app_console_print("[ENCODER] ERROR: LPTIM counter reset failed.\r\n");
    }
    enc->last_raw = 0U;
  }
  else {
    /* Set hardware counter to mid-range so the full 16-bit counter range is
     * available for bidirectional travel before overflow/underflow. */
    HAL_TIM_SetCounter(enc->handle.htim, 0x8000U);
    enc->last_raw = 0x8000U;
  }

  enc->accumulated = 0;
  enc->prev_count  = 0;
}

void encoder_register_index_cb(encoder_t *enc, void (*cb)(encoder_t *enc)) {
  /* TODO: Index/Z pulse support is not yet implemented. Hardware wiring
   * (EXTI vs. secondary timer channel capture) is not yet confirmed.
   * This stub reserves the API surface for a future non-breaking addition. */
  if (enc != NULL) {
    enc->index_cb = cb;
  }
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/
