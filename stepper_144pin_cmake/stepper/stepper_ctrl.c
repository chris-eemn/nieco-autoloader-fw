/**
 * @file stepper_ctrl.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief Shared control interface between the CLI and stepper task.
 * @version 0.3
 * @date 2026-07-02
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include "stepper_ctrl.h"
#include "axis.h"

#include "FreeRTOS.h"
#include "queue.h"

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

static volatile uint32_t s_rpm            = STEPPER_CTRL_DEFAULT_RPM;
static volatile uint32_t s_steps          = STEPPER_CTRL_DEFAULT_STEPS;
static volatile uint8_t  s_stop_requested = 0U;
static volatile uint8_t  s_running        = 0U;
static QueueHandle_t     s_cmd_q          = NULL;
static axis_t           *s_axes[STEPPER_CTRL_MAX_MOTORS] = { NULL };

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

void stepper_ctrl_init(void) {
  s_cmd_q = xQueueCreate(1U, sizeof(stepper_cmd_enum));
  configASSERT(s_cmd_q != NULL);
}

void stepper_ctrl_set_axis(uint8_t motor_num, axis_t *axis) {
  if ((motor_num < 1U) || (motor_num > STEPPER_CTRL_MAX_MOTORS)) {
    return;
  }

  s_axes[motor_num - 1U] = axis;
}

axis_t *stepper_ctrl_get_axis(uint8_t motor_num) {
  if ((motor_num < 1U) || (motor_num > STEPPER_CTRL_MAX_MOTORS)) {
    return NULL;
  }

  return s_axes[motor_num - 1U];
}

void stepper_ctrl_set_rpm(uint32_t rpm) {
  s_rpm = rpm;
}

uint32_t stepper_ctrl_get_rpm(void) {
  return s_rpm;
}

void stepper_ctrl_set_steps(uint32_t steps) {
  s_steps = steps;
}

uint32_t stepper_ctrl_get_steps(void) {
  return s_steps;
}

void stepper_ctrl_send_cmd(stepper_cmd_enum cmd) {
  s_running = 1U;
  /* xQueueOverwrite is safe on a queue of depth 1 — replaces any unread command */
  (void)xQueueOverwrite(s_cmd_q, &cmd);
}

uint8_t stepper_ctrl_recv_cmd(stepper_cmd_enum *cmd_out, uint32_t timeout_ms) {
  if (cmd_out == NULL) {
    return 0U;
  }

  TickType_t ticks =
      (timeout_ms == STEPPER_CTRL_WAIT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);

  return (xQueueReceive(s_cmd_q, cmd_out, ticks) == pdTRUE) ? 1U : 0U;
}

void stepper_ctrl_request_stop(void) {
  s_running        = 0U;
  s_stop_requested = 1U;

  for (uint8_t i = 0U; i < STEPPER_CTRL_MAX_MOTORS; i++) {
    if (s_axes[i] != NULL) {
      axis_stop(s_axes[i]);
    }
  }
}

uint8_t stepper_ctrl_is_stop_requested(void) {
  return s_stop_requested;
}

void stepper_ctrl_notify_idle(void) {
  s_running        = 0U;
  s_stop_requested = 0U;
}

uint8_t stepper_ctrl_is_running(void) {
  return s_running;
}
