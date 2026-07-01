/**
  ******************************************************************************
  * file           : main.c
  * brief          : Main program body
  *                  main() initializes the system and creates the app task.
  ******************************************************************************
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "app_console.h"
#include "stepper.h"
#include "stepper_ctrl.h"
#include "pca9538a.h"
#include "mx_i2c1.h"
#include <stdio.h>
#include "mb_regs.h"

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Private typedef -----------------------------------------------------------*/

typedef enum {
  ST_INIT = 0,
  ST_IDLE,
  ST_RUN_CW,
  ST_WAIT_CW,
  ST_PAUSE_POST_CW,
  ST_RUN_CCW,
  ST_WAIT_CCW,
  ST_PAUSE_POST_CCW,
  ST_ERROR,
} task_state_enum;

/* Private define ------------------------------------------------------------*/
#define APP_TASK_STACK_SIZE 512U
#define APP_TASK_PRIORITY   (tskIDLE_PRIORITY + 1U)

/** Number of 250 ms ticks that make up the 3-second inter-move pause. */
#define PAUSE_TICK_COUNT 12U

/* IO expander pin assignments */
#define IO_EXPANDER_M0        0x01U
#define IO_EXPANDER_M1        0x02U
#define IO_EXPANDER_DEC1      0x04U
#define IO_EXPANDER_TOFF      0x08U
#define IO_EXPANDER_STP       0x10U
#define IO_EXPANDER_DIR       0x20U
#define IO_EXPANDER_RESERVED0 0x40U

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Signals move completion from the stepper ISR callback to the stepper task. */
static QueueHandle_t s_move_done_q = NULL;

/* Private functions prototype -----------------------------------------------*/
static void on_move_done_isr(void);
static void stepper_task(void *pv_parameters);

/** @brief The application entry point. */
int main(void) {
  if (mx_system_init() != SYSTEM_OK) {
    app_console_print("[ERROR] System init failed.\r\n");
    while (1);
  }

#if defined(USE_TRACE) && USE_TRACE != 0
  mx_basic_stdio_init();
#endif

  app_console_init();
  app_console_commands_register();
  mb_regs_init();

  BaseType_t task_ret = xTaskCreate(stepper_task,
                                    "StepperTask",
                                    APP_TASK_STACK_SIZE,
                                    NULL,
                                    APP_TASK_PRIORITY,
                                    NULL);
  configASSERT(task_ret == pdPASS);

  vTaskStartScheduler();

  return 0;
}

void HardFault_Handler(void) {
  app_console_print("[ERROR] HardFault occurred.\r\n");
  while (1);
}

/**
 * @brief Stepper done callback — called from ISR context when a move completes.
 */
static void on_move_done_isr(void) {
  stepper_status_enum status = STEPPER_OK;
  BaseType_t woken = pdFALSE;
  (void)xQueueSendFromISR(s_move_done_q, &status, &woken);
  portYIELD_FROM_ISR(woken);
}

/**
 * @brief Stepper task state machine.
 *
 *  Sits idle after init. CLI commands drive all movement:
 *    auto 1  — continuous CW->3s pause->CCW->3s pause loop until auto 0
 *    cw  1   — one CW move then back to idle
 *    ccw 1   — one CCW move then back to idle
 *    auto 0  — immediate stop from any state
 *
 *  RPM and step count are snapshotted when a command is accepted so that
 *  CLI changes during a run take effect on the next command.
 */
static void stepper_task(void *pv_parameters) {
  (void)pv_parameters;

  task_state_enum     state         = ST_INIT;
  hal_i2c_handle_t   *hi2c          = mx_i2c1_i2c_gethandle();
  hal_tim_handle_t   *penc          = NULL;
  stepper_status_enum move_status   = STEPPER_OK;
  stepper_cmd_enum    pending_cmd   = STEPPER_CMD_AUTO_START;
  uint32_t            counter       = 0U;
  uint32_t            last_counter  = 0U;
  uint32_t            pause_ticks   = 0U;
  uint32_t            current_rpm   = 0U;
  uint32_t            current_steps = 0U;
  uint8_t             auto_mode     = 0U;

  configASSERT(hi2c != NULL);

  s_move_done_q = xQueueCreate(1U, sizeof(stepper_status_enum));
  configASSERT(s_move_done_q != NULL);
  stepper_ctrl_init();

  while (1) {
    switch (state) {
      /* ------------------------------------------------------------------ */
      case ST_INIT: {
        hal_status_t io_ret;
        hal_status_t enc_ret;
        uint8_t expected_out = (IO_EXPANDER_M0 | IO_EXPANDER_M1);
        uint8_t readback_out = 0x00U;

        penc = m1_encoder_timer_init();
        if (penc == NULL) {
          app_console_print("[ERROR] Encoder timer init failed.\r\n");
          state = ST_ERROR;
          break;
        }

        enc_ret = HAL_TIM_IC_StartChannel(penc, HAL_TIM_CHANNEL_1);
        if (enc_ret != HAL_OK) {
          state = ST_ERROR;
          break;
        }
        enc_ret = HAL_TIM_IC_StartChannel(penc, HAL_TIM_CHANNEL_2);
        if (enc_ret != HAL_OK) {
          state = ST_ERROR;
          break;
        }
        HAL_TIM_SetCounter(penc, 10000U);
        enc_ret = HAL_TIM_Start(penc);
        if (enc_ret != HAL_OK) {
          state = ST_ERROR;
          break;
        }

        io_ret = pca9538a_init(hi2c, 0x00U);
        if (io_ret != HAL_OK) {
          app_console_print("[ERROR] PCA9538A init failed.\r\n");
          state = ST_ERROR;
          break;
        }

        io_ret = pca9538a_write_output(hi2c, expected_out);
        if (io_ret != HAL_OK) {
          app_console_print("[ERROR] PCA9538A write failed.\r\n");
          state = ST_ERROR;
          break;
        }

        io_ret = pca9538a_read_output(hi2c, &readback_out);
        if (io_ret != HAL_OK) {
          app_console_print("[ERROR] PCA9538A read failed.\r\n");
          state = ST_ERROR;
          break;
        }

        if (readback_out == expected_out) {
          app_console_print("PCA9538A OK: 0x%02X\r\n", readback_out);
          stepper_init();
          stepper_register_done_cb(on_move_done_isr);
          state = ST_IDLE;
        }
        else {
          app_console_print("PCA9538A FAIL: wrote 0x%02X, read 0x%02X\r\n",
                            expected_out, readback_out);
          state = ST_ERROR;
        }
        break;
      }

      /* ------------------------------------------------------------------ */
      case ST_IDLE: {
        stepper_ctrl_notify_idle();
        app_console_print("[STEPPER] Idle.\r\n");

        if (stepper_ctrl_recv_cmd(&pending_cmd, STEPPER_CTRL_WAIT_FOREVER) == 1U) {
          current_rpm   = stepper_ctrl_get_rpm();
          current_steps = stepper_ctrl_get_steps();

          switch (pending_cmd) {
            case STEPPER_CMD_AUTO_START:
              auto_mode = 1U;
              state     = ST_RUN_CW;
              break;
            case STEPPER_CMD_RUN_CW:
              auto_mode = 0U;
              state     = ST_RUN_CW;
              break;
            case STEPPER_CMD_RUN_CCW:
              auto_mode = 0U;
              state     = ST_RUN_CCW;
              break;
            default:
              break;
          }
        }
        break;
      }

      /* ------------------------------------------------------------------ */
      case ST_RUN_CW: {
        if (stepper_ctrl_is_stop_requested() != 0U) {
          state = ST_IDLE;
          break;
        }
        app_console_print("[INFO] MOVE CW — steps=%lu rpm=%lu\r\n", current_steps, current_rpm);
        if (stepper_move_start(current_steps, current_rpm, STEPPER_DIR_CW) == STEPPER_OK) {
          last_counter = HAL_TIM_GetCounter(penc);
          state        = ST_WAIT_CW;
        }
        break;
      }

      /* ------------------------------------------------------------------ */
      case ST_WAIT_CW: {
        if (stepper_ctrl_is_stop_requested() != 0U) {
          state = ST_IDLE;
          break;
        }
        if (xQueueReceive(s_move_done_q, &move_status, pdMS_TO_TICKS(250U)) == pdTRUE) {
          counter = HAL_TIM_GetCounter(penc);
          app_console_print("[INFO] CW done. Encoder = %lu\r\n", counter);
          last_counter = counter;

          if ((auto_mode != 0U) && (stepper_ctrl_is_stop_requested() == 0U)) {
            pause_ticks = PAUSE_TICK_COUNT;
            state       = ST_PAUSE_POST_CW;
          }
          else {
            state = ST_IDLE;
          }
        }
        else {
          counter = HAL_TIM_GetCounter(penc);
          if (counter != last_counter) {
            app_console_print("[INFO] Moving CW... Encoder = %lu\r\n", counter);
            last_counter = counter;
          }
        }
        break;
      }

      /* ------------------------------------------------------------------ */
      case ST_PAUSE_POST_CW: {
        if (stepper_ctrl_is_stop_requested() != 0U) {
          state = ST_IDLE;
          break;
        }
        if (pause_ticks == 0U) {
          state = ST_RUN_CCW;
          break;
        }
        vTaskDelay(pdMS_TO_TICKS(250U));
        pause_ticks--;
        break;
      }

      /* ------------------------------------------------------------------ */
      case ST_RUN_CCW: {
        if (stepper_ctrl_is_stop_requested() != 0U) {
          state = ST_IDLE;
          break;
        }
        app_console_print("[INFO] MOVE CCW — steps=%lu rpm=%lu\r\n", current_steps, current_rpm);
        if (stepper_move_start(current_steps, current_rpm, STEPPER_DIR_CCW) == STEPPER_OK) {
          last_counter = HAL_TIM_GetCounter(penc);
          state        = ST_WAIT_CCW;
        }
        break;
      }

      /* ------------------------------------------------------------------ */
      case ST_WAIT_CCW: {
        if (stepper_ctrl_is_stop_requested() != 0U) {
          state = ST_IDLE;
          break;
        }
        if (xQueueReceive(s_move_done_q, &move_status, pdMS_TO_TICKS(250U)) == pdTRUE) {
          counter = HAL_TIM_GetCounter(penc);
          app_console_print("[INFO] CCW done. Encoder = %lu\r\n\r\n", counter);
          last_counter = counter;

          if ((auto_mode != 0U) && (stepper_ctrl_is_stop_requested() == 0U)) {
            pause_ticks = PAUSE_TICK_COUNT;
            state       = ST_PAUSE_POST_CCW;
          }
          else {
            state = ST_IDLE;
          }
        }
        else {
          counter = HAL_TIM_GetCounter(penc);
          if (counter != last_counter) {
            app_console_print("[INFO] Moving CCW... Encoder = %lu\r\n", counter);
            last_counter = counter;
          }
        }
        break;
      }

      /* ------------------------------------------------------------------ */
      case ST_PAUSE_POST_CCW: {
        if (stepper_ctrl_is_stop_requested() != 0U) {
          state = ST_IDLE;
          break;
        }
        if (pause_ticks == 0U) {
          /* Auto loop — go straight back to CW without re-entering IDLE */
          state = ST_RUN_CW;
          break;
        }
        vTaskDelay(pdMS_TO_TICKS(250U));
        pause_ticks--;
        break;
      }

      /* ------------------------------------------------------------------ */
      case ST_ERROR: {
        vTaskDelay(pdMS_TO_TICKS(1000U));
        break;
      }

      default:
        state = ST_IDLE;
        break;
    }
  }
}
