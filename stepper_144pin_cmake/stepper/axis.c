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
#define AXIS_SUPERVISOR_PRIORITY (tskIDLE_PRIORITY + 1U)

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

static axis_t s_axes[AXIS_MAX_INSTANCES];
static uint8_t s_axis_count = 0U;

/* Indexed by axis_event_enum. Both the pointers and the strings are const so
 * the whole table stays in flash. Keep in step with axis_event_enum. */
static const char* const s_event_names[] = {
    [AXIS_EVENT_MOVE_DONE] = "MOVE_DONE", [AXIS_EVENT_MOVE_STOPPED] = "MOVE_STOPPED", [AXIS_EVENT_MOVE_FAILED] = "MOVE_FAILED",
    [AXIS_EVENT_HOME_DONE] = "HOME_DONE", [AXIS_EVENT_HOME_ABORTED] = "HOME_ABORTED", [AXIS_EVENT_HOME_FAILED] = "HOME_FAILED",
};

/* Supervisor task: created once by the first axis_init(), shared by all axes.
 * Using a dedicated low-priority task (rather than a FreeRTOS software timer or
 * a sub-rate ISR callback) keeps the supervisor logic cleanly separate from the
 * step-pulse ISR and avoids stack constraints of the timer daemon task. */
static TaskHandle_t s_supervisor_handle = NULL;
static uint32_t s_supervisor_period_ms = 0U;
/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

static void axis_fault_isr_cb(stepper_t* motor);
static void axis_emit(axis_t* axis, axis_event_enum event);
static void axis_emit_failure(axis_t* axis);
static void start_homing_backoff(axis_t* axis);
static void start_homing_settle(axis_t* axis);
static uint8_t handle_sync_event(axis_t* axis);
static void handle_homing_tick(axis_t* axis);
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
  axis->num = s_axis_count; /* Axis number 1-8 */

  axis->motor = motor;
  axis->encoder = encoder;

  /* Apply defaults for zero-valued config fields. */
  axis->config = *config;
  if (axis->config.supervisor_period_ms == 0U) {
    axis->config.supervisor_period_ms = AXIS_DEFAULT_SUPERVISOR_PERIOD_MS;
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

  axis->status = AXIS_STATUS_NOT_HOMED;
  axis->fault_from_isr = 0U;
  axis->fault_reset_pending = 0U;
  axis->homing_substate = HOMING_IDLE;
  axis->home_direction = STEPPER_DIR_CW;
  axis->active_op = AXIS_OP_NONE;
  axis->stop_pending = 0U;
  axis->event_cb = NULL;
  axis->event_ctx = NULL;

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

  app_console_print("[AXIS] Instance %u init OK. period=%lums encoder=%lu/%lu counts/ustep max_error=%lu counts.\r\n", (unsigned)(s_axis_count - 1U),
                    axis->config.supervisor_period_ms, axis->config.encoder_counts_numerator, axis->config.encoder_counts_denominator,
                    axis->config.max_sync_error_counts);

  return axis;
}

void axis_update_config(axis_t* axis, const axis_config_t* config) {
  if ((axis == NULL) || (config == NULL)) {
    return;
  }

  /* Apply defaults for zero-valued config fields. */
  axis->config = *config;
  if (axis->config.supervisor_period_ms == 0U) {
    axis->config.supervisor_period_ms = AXIS_DEFAULT_SUPERVISOR_PERIOD_MS;
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

  stepper_sync_config_t sync_config = {
      .encoder_counts_numerator = axis->config.encoder_counts_numerator,
      .encoder_counts_denominator = axis->config.encoder_counts_denominator,
      .max_error_counts = axis->config.max_sync_error_counts,
  };

  (void)stepper_sync_configure(axis->motor, axis->encoder, &sync_config);
}

void axis_register_event_cb(axis_t* axis, axis_event_cb_t cb, void* ctx) {
  if (axis == NULL) {
    return;
  }

  /* The callback and its context must be swapped as a unit: a supervisor tick
   * landing between the two stores would otherwise pair one registration's
   * callback with the other's context. */
  taskENTER_CRITICAL();
  axis->event_cb = cb;
  axis->event_ctx = ctx;
  taskEXIT_CRITICAL();
}

const char* axis_event_name(axis_event_enum event) {
  /* The NULL test covers a gap left in the designated initialisers if a new
   * event is added to the enum without a matching name. */
  if (((uint32_t)event >= (sizeof(s_event_names) / sizeof(s_event_names[0]))) || (s_event_names[event] == NULL)) {
    return "UNKNOWN";
  }

  return s_event_names[event];
}

axis_status_enum axis_get_status(const axis_t* axis) {
  if (axis == NULL) {
    return AXIS_STATUS_INVALID;
  }

  return axis->status;
}

int32_t axis_get_encoder_count(const axis_t* axis) {
  if (axis == NULL) {
    return 0;
  }

  return encoder_get_count(axis->encoder);
}

int32_t axis_get_home_travel_counts(const axis_t* axis) {
  if (axis == NULL) {
    return 0;
  }

  return axis->home_travel_counts;
}

stepper_status_enum axis_move(axis_t* axis, uint32_t steps, uint32_t rpm, uint8_t direction) {
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

  /* Claimed only after the motor is confirmed running. Claiming it earlier would
   * let a supervisor tick observe an active move on an idle motor and report a
   * completion that never happened. */
  axis->stop_pending = 0U;
  axis->active_op = AXIS_OP_MOVE;

  return STEPPER_OK;
}

uint8_t axis_is_busy(const axis_t* axis) {
  if (axis == NULL) {
    return 0U;
  }

  return stepper_is_busy(axis->motor);
}

stepper_status_enum axis_home(axis_t* axis, uint8_t direction) {
  if (axis == NULL) {
    return STEPPER_INVALID;
  }

  /* The back-off move is derived by inverting this direction, so an out-of-range
   * value is rejected here rather than silently taken as CCW by the driver. */
  if ((direction != STEPPER_DIR_CW) && (direction != STEPPER_DIR_CCW)) {
    return STEPPER_INVALID;
  }

  if (axis->status == AXIS_STATUS_HOMING) {
    return STEPPER_BUSY;
  }

  if (stepper_is_busy(axis->motor) != 0U) {
    return STEPPER_BUSY;
  }

  if ((axis->status == AXIS_STATUS_FAULT) || (axis->status == AXIS_STATUS_STALLED) || (stepper_is_fault(axis->motor) != 0U)) {
    return STEPPER_FAULT;
  }

  stepper_status_enum ret = stepper_move_start(axis->motor, axis->config.home_max_steps, axis->config.home_rpm, direction);

  if (ret != STEPPER_OK) {
    return ret;
  }

  axis->stop_pending = 0U;
  axis->active_op = AXIS_OP_HOME;
  axis->home_direction = direction;
  axis->homing_substate = HOMING_SEEK;
  axis->status = AXIS_STATUS_HOMING;

  /* Reference for the travel distance reported by axis_get_home_travel_counts().
   * The result is cleared here so a sequence that faults or aborts reports 0
   * instead of the distance measured by the previous sequence. */
  axis->home_start_counts = encoder_get_count(axis->encoder);
  axis->home_travel_counts = 0;

  app_console_print("[AXIS %d] Homing started. dir=%u rpm=%lu max=%lu usteps.\r\n", axis->num, direction, axis->config.home_rpm,
                    axis->config.home_max_steps);

  return STEPPER_OK;
}

void axis_clear_fault(axis_t* axis) {
  if (axis == NULL) {
    return;
  }

  axis->homing_substate = HOMING_IDLE;
  axis->fault_from_isr = 0U;
  axis->status = AXIS_STATUS_NOT_HOMED;

  /* Any operation interrupted by the fault has already reported its failure;
   * dropping the claim here keeps the recovery itself from emitting a second
   * event for the same operation. */
  axis->active_op = AXIS_OP_NONE;
  axis->stop_pending = 0U;

  (void)stepper_sync_take_event(axis->motor, NULL);
}

void axis_fault_reset(axis_t* axis) {
  if (axis == NULL) {
    return;
  }

  axis->fault_reset_pending = 1U;
}

void axis_stop(axis_t* axis) {
  if (axis == NULL) {
    return;
  }

  stepper_stop(axis->motor);

  if (axis->status == AXIS_STATUS_HOMING) {
    axis->homing_substate = HOMING_IDLE;
    axis->status = AXIS_STATUS_NOT_HOMED;
  }

  /* The motor is already stopped; the event is left to the supervisor so that
   * callbacks never run in the context of whichever task called stop. */
  if (axis->active_op != AXIS_OP_NONE) {
    axis->stop_pending = 1U;
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
 * @brief Release the active operation and deliver its terminal event.
 *
 * Called only from the supervisor task. The claim is released before the
 * callback runs so that a callback which starts the next move on this axis
 * sees an idle axis rather than one still owing an event.
 *
 * @param axis  Axis whose operation reached a terminal state.
 * @param event Outcome to report.
 */
static void axis_emit(axis_t* axis, axis_event_enum event) {
  axis_event_cb_t cb;
  void* ctx;

  axis->active_op = AXIS_OP_NONE;
  axis->stop_pending = 0U;

  /* Snapshot the pair together so a concurrent axis_register_event_cb() cannot
   * split it across this call. */
  taskENTER_CRITICAL();
  cb = axis->event_cb;
  ctx = axis->event_ctx;
  taskEXIT_CRITICAL();

  if (cb != NULL) {
    cb(axis, event, ctx);
  }
}

/**
 * @brief Report the failure of whichever operation is active.
 *
 * Emits nothing when the axis is idle, so a fault detected between operations
 * updates status without inventing a completion event for a move or homing
 * sequence that was never started.
 *
 * @param axis Axis whose active operation has failed.
 */
static void axis_emit_failure(axis_t* axis) {
  if (axis->active_op == AXIS_OP_HOME) {
    axis_emit(axis, AXIS_EVENT_HOME_FAILED);
  }
  else if (axis->active_op == AXIS_OP_MOVE) {
    axis_emit(axis, AXIS_EVENT_MOVE_FAILED);
  }
  else {
    /* Idle: the fault changes status only. */
    axis_emit(axis, AXIS_EVENT_IDLE_FAULT);
  }
}

/**
 * @brief Start the configured backoff move after homing seek finds the endstop.
 * @param axis Axis currently in HOMING_SEEK.
 */
static void start_homing_backoff(axis_t* axis) {
  uint8_t backoff_dir = (axis->home_direction == STEPPER_DIR_CW) ? STEPPER_DIR_CCW : STEPPER_DIR_CW;
  stepper_status_enum ret = stepper_move_start(axis->motor, axis->config.backoff_steps, axis->config.home_rpm, backoff_dir);

  if (ret == STEPPER_OK) {
    axis->homing_substate = HOMING_BACKOFF;
    app_console_print("[AXIS %d] Starting homing back-off (%lu usteps).\r\n", axis->num, axis->config.backoff_steps);
  }
  else {
    app_console_print("[AXIS %d] Back-off start failed: %d\r\n", axis->num, (int)ret);
    axis->status = AXIS_STATUS_FAULT;
    axis->homing_substate = HOMING_IDLE;
    axis_emit(axis, AXIS_EVENT_HOME_FAILED);
  }
}

/**
 * @brief Start the settle period after the homing back-off move.
 * @param axis Axis currently in HOMING_SETTLE.
 */
static void start_homing_settle(axis_t* axis) {
  axis->settle_end_time_ms = xTaskGetTickCount() + pdMS_TO_TICKS(axis->config.settle_delay_ms);
  axis->homing_substate = HOMING_SETTLE;
  app_console_print("[AXIS %d] Settling after endstop hit for %lu ms.\r\n", axis->num, axis->config.settle_delay_ms);
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
    app_console_print("[AXIS %d] Warning: encoder is ahead by %lu counts; motion continuing.\r\n", axis->num, deviation_counts);
  }
  else if (event == STEPPER_SYNC_EVENT_LAG) {
    event_handled = 1U;

    if ((axis->status == AXIS_STATUS_HOMING) && (axis->homing_substate == HOMING_SEEK)) {
      app_console_print("[AXIS %d] Endstop found: encoder lagged by %lu counts.\r\n", axis->num, deviation_counts);
      start_homing_settle(axis);
    }
    else if ((axis->status == AXIS_STATUS_HOMING) && (axis->homing_substate == HOMING_BACKOFF)) {
      axis->status = AXIS_STATUS_FAULT;
      axis->homing_substate = HOMING_IDLE;
      app_console_print("[AXIS %d] Homing back-off stalled: encoder lagged by %lu counts.\r\n", axis->num, deviation_counts);
      axis_emit(axis, AXIS_EVENT_HOME_FAILED);
    }
    else if ((axis->status == AXIS_STATUS_OK) || (axis->status == AXIS_STATUS_NOT_HOMED)) {
      axis->status = AXIS_STATUS_STALLED;
      app_console_print("[AXIS %d] Stall detected: encoder lagged by %lu counts. Motor stopped.\r\n", axis->num, deviation_counts);
      axis_emit_failure(axis);
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
 * @param axis Axis instance in AXIS_STATUS_HOMING.
 */
static void handle_homing_tick(axis_t* axis) {
  switch (axis->homing_substate) {
    case HOMING_SEEK: {
      if (stepper_is_busy(axis->motor) == 0U) {
        /* The seek used all home_max_steps without an ISR following-error event. */
        app_console_print("[AXIS %d] Homing timeout — endstop not reached.\r\n", axis->num);
        axis->status = AXIS_STATUS_FAULT;
        axis->homing_substate = HOMING_IDLE;
        axis_emit(axis, AXIS_EVENT_HOME_FAILED);
      }
      break;
    }

    case HOMING_SETTLE: {
      /* Wait for the seek deceleration to finish before timing the settle. */
      if (stepper_is_busy(axis->motor) != 0U) {
        break;
      }
      if ((int32_t)(xTaskGetTickCount() - axis->settle_end_time_ms) >= 0) {
        axis->settle_end_time_ms = 0U;
        start_homing_backoff(axis);
        axis->homing_substate = HOMING_BACKOFF;
      }
      break;
    }

    case HOMING_BACKOFF: {
      if (stepper_is_busy(axis->motor) == 0U) {
        /* Latch the seek-plus-backoff distance before zeroing, which discards it.
         * Ordered ahead of axis_emit() so the callback can read the result. */
        axis->home_travel_counts = encoder_get_count(axis->encoder) - axis->home_start_counts;

        encoder_zero(axis->encoder);
        axis->homing_substate = HOMING_IDLE;
        axis->status = AXIS_STATUS_OK;
        app_console_print("[AXIS %d] Homing complete. Travelled %ld counts. Encoder zeroed.\r\n", axis->num, axis->home_travel_counts);
        axis_emit(axis, AXIS_EVENT_HOME_DONE);
      }
      break;
    }

    default:
      break;
  }
}

/**
 * @brief Process one supervisor tick for a single axis instance.
 */
static void supervisor_tick(axis_t* axis) {
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
    app_console_print("[AXIS %d] Fault reset complete. Re-home before moving.\r\n", axis->num);
    return;
  }

  /* Fault from ISR takes priority over all other state. A stop requested in the
   * same window is reported as the fault instead, since the axis did not stop
   * for the reason the caller asked it to. */
  if (axis->fault_from_isr != 0U) {
    axis->fault_from_isr = 0U;
    axis->status = AXIS_STATUS_FAULT;
    axis->homing_substate = HOMING_IDLE;
    app_console_print("[AXIS %d] Stepper fault: axis halted.\r\n", axis->num);
    axis_emit_failure(axis);
    return;
  }

  if (axis->stop_pending != 0U) {
    if (axis->active_op == AXIS_OP_HOME) {
      axis_emit(axis, AXIS_EVENT_HOME_ABORTED);
    }
    else if (axis->active_op == AXIS_OP_MOVE) {
      axis_emit(axis, AXIS_EVENT_MOVE_STOPPED);
    }
    else {
      /* The operation ended on its own before the stop was serviced; its own
       * event was already delivered. */
      axis->stop_pending = 0U;
    }
    return;
  }

  if (handle_sync_event(axis) != 0U) {
    return;
  }

  if (axis->status == AXIS_STATUS_HOMING) {
    handle_homing_tick(axis);
  }
  else if ((axis->active_op == AXIS_OP_MOVE) && (stepper_is_busy(axis->motor) == 0U)) {
    axis_emit(axis, AXIS_EVENT_MOVE_DONE);
  }
  else {
    /* Idle, or a move still in progress. */
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
static void axis_supervisor_task(void* pv) {
  (void)pv;

  TickType_t last_wake = xTaskGetTickCount();

  for (;;) {
    vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(s_supervisor_period_ms));

    for (uint8_t i = 0U; i < s_axis_count; i++) {
      supervisor_tick(&s_axes[i]);
    }
  }
}
