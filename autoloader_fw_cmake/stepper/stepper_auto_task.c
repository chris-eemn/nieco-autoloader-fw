/**
 * @file stepper_auto_task.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief CLI-driven automatic stepper test task.
 * @version 0.1
 * @date 2026-08-07
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC. All Rights Reserved.
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include "stepper_auto_task.h"

#include <stdint.h>

#include "FreeRTOS.h"
#include "app_console.h"
#include "axis.h"
#include "stepper.h"
#include "stepper_ctrl.h"
#include "task.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

#define STEPPER_AUTO_TASK_STACK_SIZE (512U)
#define STEPPER_AUTO_TASK_PRIORITY (tskIDLE_PRIORITY + 1U)
#define STEPPER_AUTO_PAUSE_MS (3000U)
#define STEPPER_AUTO_POLL_MS (25U)
#define STEPPER_AUTO_MOVE_TIMEOUT_MS (30000U)

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

typedef enum {
  STEPPER_AUTO_STATE_IDLE = 0,
  STEPPER_AUTO_STATE_RUN_CW,
  STEPPER_AUTO_STATE_WAIT_CW,
  STEPPER_AUTO_STATE_PAUSE_AFTER_CW,
  STEPPER_AUTO_STATE_RUN_CCW,
  STEPPER_AUTO_STATE_WAIT_CCW,
  STEPPER_AUTO_STATE_PAUSE_AFTER_CCW,
  STEPPER_AUTO_STATE_ERROR,
} stepper_auto_state_enum;

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

static void stepper_auto_task_run(void* parameters);
static bool stepper_auto_task_move_timed_out(TickType_t move_started_at);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

bool stepper_auto_task_start(void) {
  BaseType_t result = xTaskCreate(stepper_auto_task_run, "StepperAuto", STEPPER_AUTO_TASK_STACK_SIZE, NULL, STEPPER_AUTO_TASK_PRIORITY, NULL);

  return (result == pdPASS);
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/

/**
 * @brief Service CLI auto-cycle commands for the already-initialised test axis.
 * @param parameters Unused FreeRTOS task parameter.
 */
static void stepper_auto_task_run(void* parameters) {
  stepper_auto_state_enum state = STEPPER_AUTO_STATE_IDLE;
  stepper_cmd_enum command = STEPPER_CMD_AUTO_START;
  axis_t* axis = NULL;
  uint32_t current_rpm = 0U;
  uint32_t current_steps = 0U;
  TickType_t move_started_at = 0U;
  TickType_t pause_started_at = 0U;

  (void)parameters;

  axis = stepper_ctrl_get_axis(1U);
  if (axis == NULL) {
    app_console_print("[ERROR] Auto-test axis is unavailable.\r\n");
    state = STEPPER_AUTO_STATE_ERROR;
  }

  for (;;) {
    switch (state) {
      case STEPPER_AUTO_STATE_IDLE:
        stepper_ctrl_notify_idle();
        app_console_print("[STEPPER] Idle.\r\n");
        if (stepper_ctrl_recv_cmd(&command, STEPPER_CTRL_WAIT_FOREVER) != 0U) {
          if (command == STEPPER_CMD_AUTO_START) {
            current_rpm = stepper_ctrl_get_rpm();
            current_steps = stepper_ctrl_get_steps();
            state = STEPPER_AUTO_STATE_RUN_CW;
          }
        }
        break;

      case STEPPER_AUTO_STATE_RUN_CW:
        if (stepper_ctrl_is_stop_requested() != 0U) {
          state = STEPPER_AUTO_STATE_IDLE;
        }
        else if (axis_move(axis, current_steps, current_rpm, STEPPER_DIR_CW) == STEPPER_OK) {
          app_console_print("[INFO] MOVE CW -- steps=%lu rpm=%lu\r\n", current_steps, current_rpm);
          move_started_at = xTaskGetTickCount();
          state = STEPPER_AUTO_STATE_WAIT_CW;
        }
        else {
          app_console_print("[ERROR] CW start failed (axis=%d).\r\n", (int32_t)axis_get_status(axis));
          state = STEPPER_AUTO_STATE_IDLE;
        }
        break;

      case STEPPER_AUTO_STATE_WAIT_CW:
        if (stepper_ctrl_is_stop_requested() != 0U) {
          app_console_print("[INFO] CW stopped. Encoder = %ld\r\n", axis_get_encoder_count(axis));
          state = STEPPER_AUTO_STATE_IDLE;
        }
        else if ((axis_get_status(axis) == AXIS_STATUS_STALLED) || (axis_get_status(axis) == AXIS_STATUS_FAULT)) {
          app_console_print("[ERROR] CW failed (axis=%d). Encoder = %ld\r\n", (int32_t)axis_get_status(axis), axis_get_encoder_count(axis));
          state = STEPPER_AUTO_STATE_IDLE;
        }
        else if (stepper_auto_task_move_timed_out(move_started_at) != false) {
          axis_stop(axis);
          app_console_print("[ERROR] CW timed out. Encoder = %ld\r\n", axis_get_encoder_count(axis));
          state = STEPPER_AUTO_STATE_IDLE;
        }
        else if (axis_is_busy(axis) == 0U) {
          app_console_print("[INFO] CW done. Encoder = %ld\r\n", axis_get_encoder_count(axis));
          pause_started_at = xTaskGetTickCount();
          state = STEPPER_AUTO_STATE_PAUSE_AFTER_CW;
        }
        else {
          vTaskDelay(pdMS_TO_TICKS(STEPPER_AUTO_POLL_MS));
        }
        break;

      case STEPPER_AUTO_STATE_PAUSE_AFTER_CW:
        if (stepper_ctrl_is_stop_requested() != 0U) {
          state = STEPPER_AUTO_STATE_IDLE;
        }
        else if ((xTaskGetTickCount() - pause_started_at) >= pdMS_TO_TICKS(STEPPER_AUTO_PAUSE_MS)) {
          state = STEPPER_AUTO_STATE_RUN_CCW;
        }
        else {
          vTaskDelay(pdMS_TO_TICKS(STEPPER_AUTO_POLL_MS));
        }
        break;

      case STEPPER_AUTO_STATE_RUN_CCW:
        if (stepper_ctrl_is_stop_requested() != 0U) {
          state = STEPPER_AUTO_STATE_IDLE;
        }
        else if (axis_move(axis, current_steps, current_rpm, STEPPER_DIR_CCW) == STEPPER_OK) {
          app_console_print("[INFO] MOVE CCW -- steps=%lu rpm=%lu\r\n", current_steps, current_rpm);
          move_started_at = xTaskGetTickCount();
          state = STEPPER_AUTO_STATE_WAIT_CCW;
        }
        else {
          app_console_print("[ERROR] CCW start failed (axis=%d).\r\n", (int32_t)axis_get_status(axis));
          state = STEPPER_AUTO_STATE_IDLE;
        }
        break;

      case STEPPER_AUTO_STATE_WAIT_CCW:
        if (stepper_ctrl_is_stop_requested() != 0U) {
          app_console_print("[INFO] CCW stopped. Encoder = %ld\r\n", axis_get_encoder_count(axis));
          state = STEPPER_AUTO_STATE_IDLE;
        }
        else if ((axis_get_status(axis) == AXIS_STATUS_STALLED) || (axis_get_status(axis) == AXIS_STATUS_FAULT)) {
          app_console_print("[ERROR] CCW failed (axis=%d). Encoder = %ld\r\n", (int32_t)axis_get_status(axis), axis_get_encoder_count(axis));
          state = STEPPER_AUTO_STATE_IDLE;
        }
        else if (stepper_auto_task_move_timed_out(move_started_at) != false) {
          axis_stop(axis);
          app_console_print("[ERROR] CCW timed out. Encoder = %ld\r\n", axis_get_encoder_count(axis));
          state = STEPPER_AUTO_STATE_IDLE;
        }
        else if (axis_is_busy(axis) == 0U) {
          app_console_print("[INFO] CCW done. Encoder = %ld\r\n\r\n", axis_get_encoder_count(axis));
          pause_started_at = xTaskGetTickCount();
          state = STEPPER_AUTO_STATE_PAUSE_AFTER_CCW;
        }
        else {
          vTaskDelay(pdMS_TO_TICKS(STEPPER_AUTO_POLL_MS));
        }
        break;

      case STEPPER_AUTO_STATE_PAUSE_AFTER_CCW:
        if (stepper_ctrl_is_stop_requested() != 0U) {
          state = STEPPER_AUTO_STATE_IDLE;
        }
        else if ((xTaskGetTickCount() - pause_started_at) >= pdMS_TO_TICKS(STEPPER_AUTO_PAUSE_MS)) {
          state = STEPPER_AUTO_STATE_RUN_CW;
        }
        else {
          vTaskDelay(pdMS_TO_TICKS(STEPPER_AUTO_POLL_MS));
        }
        break;

      case STEPPER_AUTO_STATE_ERROR:
      default:
        vTaskDelay(pdMS_TO_TICKS(1000U));
        break;
    }
  }
}

/**
 * @brief Test whether the active move has exceeded the auto-test timeout.
 * @param move_started_at Tick count captured when the move started.
 * @return true when the timeout elapsed; otherwise false.
 */
static bool stepper_auto_task_move_timed_out(TickType_t move_started_at) {
  return ((xTaskGetTickCount() - move_started_at) >= pdMS_TO_TICKS(STEPPER_AUTO_MOVE_TIMEOUT_MS));
}
