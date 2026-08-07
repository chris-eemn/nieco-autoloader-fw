/**
 * @file axis.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief Generic single-axis abstraction — stepper + encoder with closed-loop supervision.
 * @version 0.1
 * @date 2026-07-02
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include "axis.h"
#include "app_console.h"

#include "FreeRTOS.h"
#include "task.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

#define AXIS_SUPERVISOR_STACK_SIZE 256U
#define AXIS_SUPERVISOR_PRIORITY   (tskIDLE_PRIORITY + 1U)

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/* Internal homing sub-states, only meaningful when status == AXIS_STATUS_HOMING. */
typedef enum {
  HOMING_IDLE   = 0, /* Not homing                                          */
  HOMING_SEEK,       /* Driving toward endstop, waiting for stationary enc  */
  HOMING_BACKOFF,    /* Backing off from endstop                            */
} homing_state_enum;

struct axis_s {
  stepper_t        *motor;
  encoder_t        *encoder;
  axis_config_t     config;

  volatile axis_status_enum status;
  volatile uint8_t          fault_from_isr;     /* Set by fault ISR callback; observed by supervisor */
  volatile uint8_t          fault_reset_pending; /* 1=sleep requested, 2=sleeping (wake next tick)   */

  /* Homing */
  homing_state_enum homing_substate;
  uint32_t          enc_stationary_count; /* Consecutive zero-delta ticks during SEEK   */
  uint32_t          stationary_samples;   /* Required ticks to declare stationary        */

  /* Stall detection */
  uint32_t stall_count;   /* Consecutive zero-delta ticks while running  */
  uint32_t stall_samples; /* Required ticks to declare stall             */

  /* Shared between homing and stall — encoder sample at previous supervisor tick */
  int32_t last_enc_count;
};

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

static axis_t  s_axes[AXIS_MAX_INSTANCES];
static uint8_t s_axis_count = 0U;

/* Supervisor task: created once by the first axis_init(), shared by all axes.
 * Using a dedicated low-priority task (rather than a FreeRTOS software timer or
 * a sub-rate ISR callback) keeps the supervisor logic cleanly separate from the
 * step-pulse ISR and avoids stack constraints of the timer daemon task. */
static TaskHandle_t s_supervisor_handle    = NULL;
static uint32_t     s_supervisor_period_ms = 0U;

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

static void axis_fault_isr_cb(stepper_t* motor);
static void start_homing_backoff(axis_t* axis);
static uint8_t handle_sync_event(axis_t* axis);
static void handle_homing_tick(axis_t* axis, int32_t delta);
static void handle_stall_tick(axis_t* axis, int32_t delta);
static void supervisor_tick(axis_t* axis);
static void axis_supervisor_task(void* pv);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

axis_t* axis_init(stepper_t* motor, encoder_t* encoder, hal_exti_handle_t* hexti, const axis_config_t* config) {
  if ((motor == NULL) || (encoder == NULL) || (hexti == NULL) || (config == NULL)) {
    return NULL;
  }

  if (s_axis_count >= AXIS_MAX_INSTANCES) {
    app_console_print("[AXIS] ERROR: pool full (%u/%u)\r\n", (unsigned)s_axis_count, (unsigned)AXIS_MAX_INSTANCES);
    return NULL;
  }

  axis_t* axis = &s_axes[s_axis_count];
  s_axis_count++;

  axis->motor = motor;
  axis->encoder = encoder;

  /* Apply defaults for zero-valued config fields. */
  axis->config = *config;
  if (axis->config.supervisor_period_ms == 0U) {
    axis->config.supervisor_period_ms = AXIS_DEFAULT_SUPERVISOR_PERIOD_MS;
  }
  if (axis->config.stationary_window_ms == 0U) {
    axis->config.stationary_window_ms = AXIS_DEFAULT_WINDOW_MS;
  }
  if (axis->config.stall_window_ms == 0U) {
    axis->config.stall_window_ms = AXIS_DEFAULT_WINDOW_MS;
  }
  if (axis->config.encoder_counts_numerator == 0U) {
    axis->config.encoder_counts_numerator = AXIS_DEFAULT_ENCODER_COUNTS_NUMERATOR;
  }
  if (axis->config.encoder_counts_denominator == 0U) {
    axis->config.encoder_counts_denominator = AXIS_DEFAULT_ENCODER_COUNTS_DENOMINATOR;
  }
  if (axis->config.max_sync_error_counts == 0U) {
    axis->config.max_sync_error_counts = AXIS_DEFAULT_MAX_SYNC_ERROR_COUNTS;
  }

  /* Establish the supervisor period from the first axis registered. */
  if (s_supervisor_handle == NULL) {
    s_supervisor_period_ms = axis->config.supervisor_period_ms;
  }
  else if (axis->config.supervisor_period_ms != s_supervisor_period_ms) {
    app_console_print(
        "[AXIS] WARNING: axis %u requested supervisor period %lums but %lums is "
        "already running (request ignored — one shared period for all axes).\r\n",
        (unsigned)s_axis_count, axis->config.supervisor_period_ms, s_supervisor_period_ms);
  }

  uint32_t period = (s_supervisor_period_ms > 0U) ? s_supervisor_period_ms : 1U;

  /* Convert ms windows to sample counts (ceiling division so a 50ms window
   * at a 50ms period requires exactly 1 consecutive zero-delta sample). */
  axis->stationary_samples = (axis->config.stationary_window_ms + period - 1U) / period;
  if (axis->stationary_samples == 0U) {
    axis->stationary_samples = 1U;
  }

  axis->stall_samples = (axis->config.stall_window_ms + period - 1U) / period;
  if (axis->stall_samples == 0U) {
    axis->stall_samples = 1U;
  }

  axis->status = AXIS_STATUS_NOT_HOMED;
  axis->fault_from_isr = 0U;
  axis->fault_reset_pending = 0U;
  axis->homing_substate = HOMING_IDLE;
  axis->enc_stationary_count = 0U;
  axis->stall_count = 0U;
  axis->last_enc_count = encoder_get_count(encoder);

  stepper_sync_config_t sync_config = {
      .encoder_counts_numerator = axis->config.encoder_counts_numerator,
      .encoder_counts_denominator = axis->config.encoder_counts_denominator,
      .max_error_counts = axis->config.max_sync_error_counts,
  };

  if (stepper_sync_configure(motor, encoder, &sync_config) != STEPPER_OK) {
    s_axis_count--;
    return NULL;
  }

  /* Wire the stepper fault callback so that DRV8424 faults surface here.
   * EXTI registration is mandatory (hexti is a required, non-NULL parameter)
   * so continuous fault monitoring cannot be silently skipped per-axis the
   * way an optional/separate wire-up call could be forgotten. */
  stepper_register_fault_exti(motor, hexti);
  stepper_register_fault_cb(motor, axis_fault_isr_cb);

  /* Create the shared supervisor task on the first axis_init() call. */
  if (s_supervisor_handle == NULL) {
    BaseType_t ret = xTaskCreate(axis_supervisor_task, "AxisSupv", AXIS_SUPERVISOR_STACK_SIZE, NULL, AXIS_SUPERVISOR_PRIORITY, &s_supervisor_handle);
    configASSERT(ret == pdPASS);
  }

  app_console_print("[AXIS] Instance %u init OK. period=%lums stat=%lu stall=%lu samples.\r\n", (unsigned)(s_axis_count - 1U),
                    axis->config.supervisor_period_ms, axis->stationary_samples, axis->stall_samples);

  return axis;
}

axis_status_enum axis_get_status(const axis_t *axis) {
  if (axis == NULL) {
    return AXIS_STATUS_INVALID;
  }

  return axis->status;
}

int32_t axis_get_encoder_count(const axis_t *axis) {
  if (axis == NULL) {
    return 0;
  }

  return encoder_get_count(axis->encoder);
}

stepper_status_enum axis_move(axis_t *axis, uint32_t steps, uint32_t rpm, uint8_t direction) {
  if (axis == NULL) {
    return STEPPER_INVALID;
  }

  if (axis->status == AXIS_STATUS_HOMING) {
    return STEPPER_BUSY;
  }

  if (stepper_is_busy(axis->motor) != 0U) {
    return STEPPER_BUSY;
  }

  if ((axis->status == AXIS_STATUS_FAULT) || (axis->status == AXIS_STATUS_STALLED)) {
    return STEPPER_FAULT;
  }

  stepper_status_enum ret = stepper_move_start(axis->motor, steps, rpm, direction);
  if (ret != STEPPER_OK) {
    return ret;
  }

  axis->stall_count = 0U;

  return STEPPER_OK;
}

uint8_t axis_is_busy(const axis_t *axis) {
  if (axis == NULL) {
    return 0U;
  }

  return stepper_is_busy(axis->motor);
}

stepper_status_enum axis_home(axis_t *axis) {
  if (axis == NULL) {
    return STEPPER_INVALID;
  }

  if (axis->status == AXIS_STATUS_HOMING) {
    return STEPPER_BUSY;
  }

  if (stepper_is_busy(axis->motor) != 0U) {
    return STEPPER_BUSY;
  }

  if ((axis->status == AXIS_STATUS_FAULT) || (axis->status == AXIS_STATUS_STALLED) ||
      (stepper_is_fault(axis->motor) != 0U)) {
    return STEPPER_FAULT;
  }

  stepper_status_enum ret =
      stepper_move_start(axis->motor, axis->config.home_max_steps, axis->config.home_rpm, axis->config.home_direction);

  if (ret != STEPPER_OK) {
    return ret;
  }

  axis->enc_stationary_count = 0U;
  axis->stall_count          = 0U;
  axis->homing_substate      = HOMING_SEEK;
  axis->status               = AXIS_STATUS_HOMING;

  app_console_print("[AXIS] Homing started. dir=%u rpm=%lu max=%lu usteps.\r\n", axis->config.home_direction,
                    axis->config.home_rpm, axis->config.home_max_steps);

  return STEPPER_OK;
}

void axis_clear_fault(axis_t *axis) {
  if (axis == NULL) {
    return;
  }

  axis->stall_count          = 0U;
  axis->enc_stationary_count = 0U;
  axis->homing_substate      = HOMING_IDLE;
  axis->fault_from_isr       = 0U;
  axis->status               = AXIS_STATUS_NOT_HOMED;
  (void)stepper_sync_take_event(axis->motor, NULL);
}

void axis_fault_reset(axis_t *axis) {
  if (axis == NULL) {
    return;
  }

  axis->fault_reset_pending = 1U;
}

void axis_stop(axis_t *axis) {
  if (axis == NULL) {
    return;
  }

  stepper_stop(axis->motor);
  axis->stall_count          = 0U;
  axis->enc_stationary_count = 0U;

  if (axis->status == AXIS_STATUS_HOMING) {
    axis->homing_substate = HOMING_IDLE;
    axis->status          = AXIS_STATUS_NOT_HOMED;
  }
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/

/**
 * @brief Fault callback registered with the stepper driver for each axis motor.
 *
 * Called from ISR context. Sets a volatile flag rather than modifying axis
 * state directly so that all axis-state writes happen in the supervisor task.
 */
static void axis_fault_isr_cb(stepper_t* motor) {
  for (uint8_t i = 0U; i < s_axis_count; i++) {
    if (s_axes[i].motor == motor) {
      s_axes[i].fault_from_isr = 1U;
      return;
    }
  }
}

/**
 * @brief Start the configured backoff move after homing seek finds the endstop.
 * @param axis Axis currently in HOMING_SEEK.
 */
static void start_homing_backoff(axis_t* axis) {
  uint8_t backoff_dir = (axis->config.home_direction == STEPPER_DIR_CW) ? STEPPER_DIR_CCW : STEPPER_DIR_CW;
  stepper_status_enum ret = stepper_move_start(axis->motor, axis->config.backoff_steps, axis->config.home_rpm, backoff_dir);

  if (ret == STEPPER_OK) {
    axis->homing_substate = HOMING_BACKOFF;
    axis->enc_stationary_count = 0U;
    axis->stall_count = 0U;
    app_console_print("[AXIS] Starting homing back-off (%lu usteps).\r\n", axis->config.backoff_steps);
  }
  else {
    app_console_print("[AXIS] Back-off start failed: %d\r\n", (int)ret);
    axis->status = AXIS_STATUS_FAULT;
    axis->homing_substate = HOMING_IDLE;
  }
}

/**
 * @brief Consume and interpret one following-error event raised by the step ISR.
 * @param axis Axis whose motor generated the event.
 * @return 1 when a lag event changed axis state and completed this supervisor tick; otherwise 0.
 */
static uint8_t handle_sync_event(axis_t* axis) {
  uint32_t deviation_counts = 0U;
  stepper_sync_event_enum event = stepper_sync_take_event(axis->motor, &deviation_counts);
  uint8_t event_handled = 0U;

  if (event == STEPPER_SYNC_EVENT_LEAD) {
    app_console_print("[AXIS] Warning: encoder is ahead by %lu counts; motion continuing.\r\n", deviation_counts);
  }
  else if (event == STEPPER_SYNC_EVENT_LAG) {
    event_handled = 1U;

    if ((axis->status == AXIS_STATUS_HOMING) && (axis->homing_substate == HOMING_SEEK)) {
      app_console_print("[AXIS] Endstop found: encoder lagged by %lu counts.\r\n", deviation_counts);
      start_homing_backoff(axis);
    }
    else if ((axis->status == AXIS_STATUS_HOMING) && (axis->homing_substate == HOMING_BACKOFF)) {
      axis->status = AXIS_STATUS_FAULT;
      axis->homing_substate = HOMING_IDLE;
      app_console_print("[AXIS] Homing back-off stalled: encoder lagged by %lu counts.\r\n", deviation_counts);
    }
    else if ((axis->status == AXIS_STATUS_OK) || (axis->status == AXIS_STATUS_NOT_HOMED)) {
      axis->status = AXIS_STATUS_STALLED;
      axis->stall_count = 0U;
      app_console_print("[AXIS] Stall detected: encoder lagged by %lu counts. Motor stopped.\r\n", deviation_counts);
    }
    else {
      /* The ISR has already stopped the motor; no state transition is needed. */
    }
  }
  else {
    /* No synchronization event is pending. */
  }

  return event_handled;
}

/**
 * @brief Handle one supervisor tick for an axis that is currently homing.
 *
 * @param axis   Axis instance in AXIS_STATUS_HOMING.
 * @param delta  Signed encoder delta since the previous supervisor tick.
 */
static void handle_homing_tick(axis_t* axis, int32_t delta) {
  switch (axis->homing_substate) {
    case HOMING_SEEK: {
      if (delta == 0) {
        axis->enc_stationary_count++;
      }
      else {
        axis->enc_stationary_count = 0U;
      }

      if (axis->enc_stationary_count >= axis->stationary_samples) {
        /* Endstop detected — stop seek and start back-off. */
        stepper_stop(axis->motor);

        app_console_print("[AXIS] Endstop found: encoder stationary.\r\n");
        start_homing_backoff(axis);
      }
      else if (stepper_is_busy(axis->motor) == 0U) {
        /* Motor finished its max-steps move without detecting a stationary encoder
         * — homing timeout (mechanical failure or encoder disconnected). */
        app_console_print("[AXIS] Homing timeout — endstop not reached.\r\n");
        axis->status = AXIS_STATUS_FAULT;
        axis->homing_substate = HOMING_IDLE;
      }
      break;
    }

    case HOMING_BACKOFF: {
      /* Back-off is still open-loop step generation — without this check, a jam
       * during back-off would go undetected: the stepper completes its commanded
       * steps on schedule regardless of whether the mechanism actually moved, and
       * homing would report success with a zeroed encoder at the wrong position. */
      if (delta == 0) {
        axis->stall_count++;
      }
      else {
        axis->stall_count = 0U;
      }

      if (axis->stall_count >= axis->stall_samples) {
        stepper_stop(axis->motor);
        axis->status          = AXIS_STATUS_FAULT;
        axis->homing_substate = HOMING_IDLE;
        axis->stall_count     = 0U;
        app_console_print("[AXIS] Homing back-off stalled — encoder did not move.\r\n");
        break;
      }

      if (stepper_is_busy(axis->motor) == 0U) {
        encoder_zero(axis->encoder);
        axis->last_enc_count  = 0;
        axis->homing_substate = HOMING_IDLE;
        axis->status          = AXIS_STATUS_OK;
        app_console_print("[AXIS] Homing complete. Encoder zeroed.\r\n");
      }
      break;
    }

    default:
      break;
  }
}

/**
 * @brief Handle one supervisor tick for stall detection.
 *
 * Called when the stepper reports busy and axis status is OK or NOT_HOMED.
 *
 * @param axis   Axis instance.
 * @param delta  Signed encoder delta since the previous supervisor tick.
 */
static void handle_stall_tick(axis_t *axis, int32_t delta) {
  /* The stall check is intentionally all-or-nothing: delta must be exactly
   * zero for the full stall window. A proportional step-count vs encoder-count
   * ratio check is explicitly out of scope for this revision — the structure
   * accommodates adding it here without reworking the supervisor loop. */
  if (delta == 0) {
    axis->stall_count++;
    if (axis->stall_count % 10 == 0U) {
      app_console_print("[AXIS] Warning: potential stall detected . "
            "Stall count = %lu/%lu, Encoder ticks = %ld\r\n",
            axis->stall_count, axis->stall_samples, (long)encoder_get_count(axis->encoder));
    }
  }
  else {
    axis->stall_count = 0U;
  }

  if (axis->stall_count >= axis->stall_samples) {
    stepper_stop(axis->motor);
    axis->status      = AXIS_STATUS_STALLED;
    axis->stall_count = 0U;
    app_console_print("[AXIS] Stall detected. Motor stopped.\r\n");
  }
}

/**
 * @brief Process one supervisor tick for a single axis instance.
 */
static void supervisor_tick(axis_t *axis) {
  /* Two-tick hardware reset sequence driven by axis_fault_reset().
   * Tick 1 (pending==1): assert nSLP low; the supervisor period (≥25ms) acts as
   * the sleep pulse — no explicit delay needed.
   * Tick 2 (pending==2): deassert nSLP, then clear all software latches so any
   * fault_from_isr that fired during the sleep pulse is consumed here rather
   * than immediately re-faulting the axis. */
  if (axis->fault_reset_pending == 1U) {
    stepper_sleep(axis->motor);
    axis->fault_reset_pending = 2U;
    return;
  }
  if (axis->fault_reset_pending == 2U) {
    stepper_wake(axis->motor);
    stepper_clear_fault(axis->motor);
    axis_clear_fault(axis);
    axis->fault_reset_pending = 0U;
    app_console_print("[AXIS] Fault reset complete. Re-home before moving.\r\n");
    return;
  }

  /* Fault from ISR takes priority over all other state. */
  if (axis->fault_from_isr != 0U) {
    axis->fault_from_isr = 0U;
    axis->status = AXIS_STATUS_FAULT;
    axis->homing_substate = HOMING_IDLE;
    app_console_print("[AXIS] Stepper fault: axis halted.\r\n");
    return;
  }

  if (handle_sync_event(axis) != 0U) {
    return;
  }

  /* Sample encoder and compute signed delta since last tick. */
  int32_t curr_count = encoder_get_count(axis->encoder);
  int32_t delta = curr_count - axis->last_enc_count;
  axis->last_enc_count = curr_count;

  switch (axis->status) {
    case AXIS_STATUS_HOMING:
      handle_homing_tick(axis, delta);
      break;

    case AXIS_STATUS_NOT_HOMED:
    case AXIS_STATUS_OK:
      if (stepper_is_busy(axis->motor) != 0U) {
        handle_stall_tick(axis, delta);
      }
      else {
        axis->stall_count = 0U;
      }
      break;

    default:
      /* STALLED, FAULT — nothing to supervise until cleared. */
      break;
  }
}

/**
 * @brief Axis supervisor task.
 *
 * Wakes at s_supervisor_period_ms intervals and calls supervisor_tick() for
 * each registered axis. A dedicated task is used (rather than a FreeRTOS
 * software timer) so that the supervisor stack is independent of the timer
 * daemon, avoiding stack overflow in that shared context.
 */
static void axis_supervisor_task(void *pv) {
  (void)pv;

  TickType_t last_wake = xTaskGetTickCount();

  for (;;) {
    vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(s_supervisor_period_ms));

    for (uint8_t i = 0U; i < s_axis_count; i++) {
      supervisor_tick(&s_axes[i]);
    }
  }
}
