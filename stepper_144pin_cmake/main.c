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
#include "app_version_git.h"
#include "stepper.h"
#include "encoder.h"
#include "axis.h"
#include "stepper_ctrl.h"
#include "pca9538a.h"
#include "mx_i2c1.h"
#include <stdio.h>
#include "mb_regs.h"
#include "usart3_loader.h"
#include "w25q.h"
#include "cal_data.h"

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"

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

/** Maximum time to wait for a move to complete before declaring a timeout. */
#define MOVE_WAIT_TIMEOUT_MS 30000U

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
/* Private functions prototype -----------------------------------------------*/
static void       stepper_task(void *pv_parameters);
static encoder_t *encoder_start(hal_tim_handle_t *ptim);
static encoder_t *lptim_encoder_start(hal_lptim_handle_t *plptim);

axis_t *axis  = NULL;
axis_t *axis2 = NULL;
axis_t *axis3 = NULL;
axis_t *axis4 = NULL;
axis_t *axis5 = NULL;
axis_t *axis6 = NULL;
axis_t *axis7 = NULL;
axis_t *axis8 = NULL;
/** @brief The application entry point. */
int main(void) {
  if (mx_system_init() != SYSTEM_OK) {
    app_console_print("[ERROR] System init failed.\r\n");
    while (1);
  }


  // must be called before any freertos API calls, including task creation or we run into an interrupt priority issue
  // where the spi interrupt is never handled basically locking up this function
  w25q_initialize();

#if defined(USE_TRACE) && USE_TRACE != 0
  mx_basic_stdio_init();
#endif

  app_console_init();
  app_console_commands_register();

  /* USART3 serves one role per build (see usart3_loader.h / USART3_MODE): the modbus slave,
   * or the binary image loader that stages an incoming firmware image into SPI flash for the
   * bootloader to apply. Default build is the loader; modbus is opt-in. */
#if (USART3_MODE == USART3_MODE_MODBUS)
  mb_regs_init();
#else
  usart3_loader_start();
#endif


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
 * @brief Start an encoder timer and initialise the encoder instance.
 *
 * @param ptim  Timer handle already configured for encoder mode.
 * @return      Pointer to the zeroed encoder instance, or NULL on any failure.
 */
static encoder_t *encoder_start(hal_tim_handle_t *ptim) {
  if (ptim == NULL) {
    return NULL;
  }
  if (HAL_TIM_IC_StartChannel(ptim, HAL_TIM_CHANNEL_1) != HAL_OK) {
    return NULL;
  }
  if (HAL_TIM_IC_StartChannel(ptim, HAL_TIM_CHANNEL_2) != HAL_OK) {
    return NULL;
  }
  if (HAL_TIM_Start(ptim) != HAL_OK) {
    return NULL;
  }
  encoder_t *enc = encoder_init(ptim);
  if (enc == NULL) {
    return NULL;
  }
  encoder_zero(enc);
  return enc;
}

/**
 * @brief Start an LPTIM-backed encoder timer and initialise the encoder instance.
 *
 *        LPTIM encoder mode is started differently than TIM encoder mode: it has
 *        no per-channel HAL_TIM_IC_StartChannel() equivalent — a single HAL_LPTIM_Start()
 *        enables both quadrature inputs configured via HAL_LPTIM_SetConfigEncoder()
 *        (done in m8_encoder_timer_init()).
 *
 * @param plptim  LPTIM handle already configured for encoder mode.
 * @return        Pointer to the zeroed encoder instance, or NULL on any failure.
 */
static encoder_t *lptim_encoder_start(hal_lptim_handle_t *plptim) {
  if (plptim == NULL) {
    return NULL;
  }
  if (HAL_LPTIM_Start(plptim) != HAL_OK) {
    return NULL;
  }
  encoder_t *enc = encoder_init_lptim(plptim);
  if (enc == NULL) {
    return NULL;
  }
  encoder_zero(enc);
  return enc;
}

/**
 * @brief Stepper task state machine.
 *
 *  Sits idle after init, waiting for CLI commands routed via stepper_ctrl.
 *  Manual single moves (CW/CCW) are issued directly by the CLI via the axis
 *  API — the task only handles the auto-cycle (STEPPER_CMD_AUTO_START).
 *
 *  Auto cycle: CW->3s pause->CCW->3s pause->repeat until auto 0.
 *  RPM and step count are snapshotted when a command is accepted so that
 *  CLI changes during a run take effect on the next command.
 */
static void stepper_task(void *pv_parameters) {
  (void)pv_parameters;

  task_state_enum     state         = ST_INIT;
  hal_i2c_handle_t   *hi2c          = mx_i2c1_i2c_gethandle();
  stepper_t          *motor         = NULL;
  stepper_t          *motor2        = NULL;
  stepper_t          *motor3        = NULL;
  stepper_t          *motor4        = NULL;
  encoder_t          *enc           = NULL;
  encoder_t          *enc2          = NULL;
  encoder_t          *enc3          = NULL;
  encoder_t          *enc4          = NULL;
  encoder_t          *enc5          = NULL;
  encoder_t          *enc6          = NULL;
  encoder_t          *enc7          = NULL;
  encoder_t          *enc8          = NULL;
  stepper_cmd_enum    pending_cmd   = STEPPER_CMD_AUTO_START;
  uint32_t            pause_ticks   = 0U;
  uint32_t            current_rpm   = 0U;
  uint32_t            current_steps = 0U;
  uint8_t             auto_mode     = 0U;

  static const stepper_gpio_config_t k_m1_pins = {
    .step   = { M1_STEP_PORT,   M1_STEP_PIN   },
    .dir    = { M1_DIR_PORT,    M1_DIR_PIN    },
    .en     = { M1_EN_PORT,     M1_EN_PIN     },
    .nslp   = { M1_NSLP_PORT,  M1_NSLP_PIN   },
    .nfault = { M1_NFAULT_PORT, M1_NFAULT_PIN },
  };

  static const stepper_gpio_config_t k_m2_pins = {
    .step   = { M2_STEP_PORT,   M2_STEP_PIN   },
    .dir    = { M2_DIR_PORT,    M2_DIR_PIN    },
    .en     = { M2_EN_PORT,     M2_EN_PIN     },
    .nslp   = { M2_NSLP_PORT,  M2_NSLP_PIN   },
    .nfault = { M2_NFAULT_PORT, M2_NFAULT_PIN },
  };

  static const stepper_gpio_config_t k_m3_pins = {
    .step   = { M3_STEP_PORT,   M3_STEP_PIN   },
    .dir    = { M3_DIR_PORT,    M3_DIR_PIN    },
    .en     = { M3_EN_PORT,     M3_EN_PIN     },
    .nslp   = { M3_NSLP_PORT,  M3_NSLP_PIN   },
    .nfault = { M3_NFAULT_PORT, M3_NFAULT_PIN },
  };

  static const stepper_gpio_config_t k_m4_pins = {
    .step   = { M4_STEP_PORT,   M4_STEP_PIN   },
    .dir    = { M4_DIR_PORT,    M4_DIR_PIN    },
    .en     = { M4_EN_PORT,     M4_EN_PIN     },
    .nslp   = { M4_NSLP_PORT,  M4_NSLP_PIN   },
    .nfault = { M4_NFAULT_PORT, M4_NFAULT_PIN },
  };

  static const axis_config_t k_m1_axis_cfg = {
    .supervisor_period_ms = 25U,
    .stationary_window_ms = 50U,
    .stall_window_ms      = 2000U,
    .backoff_steps        = 800U,
    .home_direction       = STEPPER_DIR_CW,
    .home_rpm             = 10U,
    .home_max_steps       = 50000U,
  };
  
  /* Boot banner: printed here (not in main()) because app_console_print() only transmits once
   * the FreeRTOS scheduler is running. Version strings are injected at build time via
   * app_version_git.h (generated by cmake/gen_version.cmake) -- tag the repo to set the version
   * (e.g. `git tag v0.0.1`). */
  app_console_print("stepper_144pin %s (%s) starting\r\n", APP_VERSION_GIT_DESCRIBE, APP_VERSION_GIT_COMMIT);

  /* Cal data is loaded from here rather than from main() because both the loader and this
   * module report failures over the app console, which only transmits once the scheduler is
   * running. Nothing consumes the parameters yet -- the axis and stepper_ctrl values below are
   * still the hard-coded ones, and the cal-data defaults deliberately mirror them. */
  if (cal_data_init() == false) {
    app_console_print("[INFO] Cal data blank or from an older layout -- factory defaults written.\r\n");
  }

  stepper_ctrl_init();

  while (1) {
    switch (state) {
      /* ------------------------------------------------------------------ */
      case ST_INIT: {
        hal_status_t      io_ret;
        hal_tim_handle_t *penc1        = NULL;
        hal_tim_handle_t *penc2        = NULL;
        hal_tim_handle_t *penc3        = NULL;
        hal_tim_handle_t *penc4        = NULL;
        hal_tim_handle_t *penc5        = NULL;
        hal_tim_handle_t *penc6        = NULL;
        hal_tim_handle_t *penc7        = NULL;
        hal_lptim_handle_t *penc8        = NULL;
        uint8_t           expected_out = (IO_EXPANDER_M0 | IO_EXPANDER_M1);
        // uint8_t expected_out = 0;
        uint8_t           readback_out = 0x00U;

        penc1 = m1_encoder_timer_init();
        penc2 = m2_encoder_timer_init();
        penc3 = m3_encoder_timer_init();
        penc4 = m4_encoder_timer_init();
        penc5 = m5_encoder_timer_init();
        penc6 = m6_encoder_timer_init();
        penc7 = m7_encoder_timer_init();
        penc8 = m8_encoder_timer_init();

        hal_tim_handle_t *enc_timers[7] = {penc1, penc2, penc3, penc4, penc5, penc6, penc7};
        encoder_t       **enc_out[7]    = {&enc, &enc2, &enc3, &enc4, &enc5, &enc6, &enc7};
        uint8_t           enc_ok        = 1U;

        for (uint8_t i = 0U; i < 7U; i++) {
          *enc_out[i] = encoder_start(enc_timers[i]);
          if (*enc_out[i] == NULL) {
            app_console_print("[ERROR] Encoder %u init failed.\r\n", (unsigned)(i + 1U));
            enc_ok = 0U;
            break;
          }
        }
        if (enc_ok == 0U) {
          state = ST_ERROR;
          break;
        }

        /* Encoder 8 is LPTIM-backed (different HAL API than TIM) and so is started
         * separately rather than folded into the enc_timers[] loop above. */
        enc8 = lptim_encoder_start(penc8);
        if (enc8 == NULL) {
          app_console_print("[ERROR] Encoder 8 init failed.\r\n");
          state = ST_ERROR;
          break;
        }

        #if 1
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
          stepper_module_init(step_timer_gethandle());
          motor = stepper_init(&k_m1_pins);
          motor2 = stepper_init(&k_m2_pins);
          motor3 = stepper_init(&k_m3_pins);
          motor4 = stepper_init(&k_m4_pins);
          configASSERT(motor != NULL);
          configASSERT(motor2 != NULL);
          configASSERT(motor3 != NULL);
          configASSERT(motor4 != NULL);
          axis = axis_init(motor, enc, m1_fault_exti_gethandle(), &k_m1_axis_cfg);
          axis2 = axis_init(motor2, enc2, m2_fault_exti_gethandle(), &k_m1_axis_cfg);
          axis3 = axis_init(motor3, enc3, m3_fault_exti_gethandle(), &k_m1_axis_cfg);
          // connecting to enc6 (TIM8)
          axis4 = axis_init(motor4, enc6, m4_fault_exti_gethandle(), &k_m1_axis_cfg);
          // axis3 = axis_init(motor, enc3, m1_fault_exti_gethandle(), &k_m1_axis_cfg);
          // axis4 = axis_init(motor, enc4, m1_fault_exti_gethandle(), &k_m1_axis_cfg);
          // axis5 = axis_init(motor, enc5, m1_fault_exti_gethandle(), &k_m1_axis_cfg);
          // axis6 = axis_init(motor, enc6, m1_fault_exti_gethandle(), &k_m1_axis_cfg);
          // axis7 = axis_init(motor, enc7, m1_fault_exti_gethandle(), &k_m1_axis_cfg);
          // axis8 = axis_init(motor, enc8, m1_fault_exti_gethandle(), &k_m1_axis_cfg);
          configASSERT(axis != NULL);
          configASSERT(axis2 != NULL);
          configASSERT(axis3 != NULL);

          stepper_ctrl_set_axis(1U, axis);
          stepper_ctrl_set_axis(2U, axis2);
          stepper_ctrl_set_axis(3U, axis3);
          stepper_ctrl_set_axis(4U, axis4);
          state = ST_IDLE;
        }
        else {
          app_console_print("PCA9538A FAIL: wrote 0x%02X, read 0x%02X\r\n",
                            expected_out, readback_out);
          state = ST_ERROR;
        }
        #else
        stepper_module_init(step_timer_gethandle());
        motor = stepper_init(&k_m1_pins);
        configASSERT(motor != NULL);
        axis = axis_init(motor, enc, m1_fault_exti_gethandle(), &k_m1_axis_cfg);
        configASSERT(axis != NULL);
        stepper_ctrl_set_axis(1U, axis);
        state = ST_IDLE;
        #endif
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
        stepper_status_enum cw_ret = axis_move(axis, current_steps, current_rpm,
                                               STEPPER_DIR_CW);
        if (cw_ret == STEPPER_OK) {
          app_console_print("[INFO] MOVE CW -- steps=%lu rpm=%lu\r\n", current_steps, current_rpm);
          state = ST_WAIT_CW;
        }
        else {
          app_console_print("[ERROR] CW start failed: %d (axis=%d)\r\n",
                            (int)cw_ret, (int)axis_get_status(axis));
          state = ST_IDLE;
        }
        break;
      }

      /* ------------------------------------------------------------------ */
      case ST_WAIT_CW: {
        stepper_status_enum wait_ret = stepper_wait_done(motor, MOVE_WAIT_TIMEOUT_MS);
        int32_t             count    = encoder_get_count(enc);

        if ((wait_ret == STEPPER_FAULT) || (wait_ret == STEPPER_TIMEOUT)) {
          app_console_print("[ERROR] CW failed: %d. Encoder = %ld\r\n", (int)wait_ret, count);
          state = ST_IDLE;
          break;
        }

        if (stepper_ctrl_is_stop_requested() != 0U) {
          app_console_print("[INFO] CW stopped. Encoder = %ld\r\n", count);
          state = ST_IDLE;
          break;
        }

        if (axis_get_status(axis) == AXIS_STATUS_STALLED) {
          app_console_print("[ERROR] CW stalled. Encoder = %ld\r\n", count);
          state = ST_IDLE;
          break;
        }

        app_console_print("[INFO] CW done. Encoder = %ld\r\n", count);

        if ((auto_mode != 0U) && (stepper_ctrl_is_stop_requested() == 0U)) {
          pause_ticks = PAUSE_TICK_COUNT;
          state       = ST_PAUSE_POST_CW;
        }
        else {
          state = ST_IDLE;
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
        stepper_status_enum ccw_ret = axis_move(axis, current_steps, current_rpm,
                                                STEPPER_DIR_CCW);
        if (ccw_ret == STEPPER_OK) {
          app_console_print("[INFO] MOVE CCW -- steps=%lu rpm=%lu\r\n", current_steps, current_rpm);
          state = ST_WAIT_CCW;
        }
        else {
          app_console_print("[ERROR] CCW start failed: %d (axis=%d)\r\n",
                            (int)ccw_ret, (int)axis_get_status(axis));
          state = ST_IDLE;
        }
        break;
      }

      /* ------------------------------------------------------------------ */
      case ST_WAIT_CCW: {
        stepper_status_enum wait_ret = stepper_wait_done(motor, MOVE_WAIT_TIMEOUT_MS);
        int32_t             count    = encoder_get_count(enc);

        if ((wait_ret == STEPPER_FAULT) || (wait_ret == STEPPER_TIMEOUT)) {
          app_console_print("[ERROR] CCW failed: %d. Encoder = %ld\r\n", (int)wait_ret, count);
          state = ST_IDLE;
          break;
        }

        if (stepper_ctrl_is_stop_requested() != 0U) {
          app_console_print("[INFO] CCW stopped. Encoder = %ld\r\n", count);
          state = ST_IDLE;
          break;
        }

        if (axis_get_status(axis) == AXIS_STATUS_STALLED) {
          app_console_print("[ERROR] CCW stalled. Encoder = %ld\r\n", count);
          state = ST_IDLE;
          break;
        }

        app_console_print("[INFO] CCW done. Encoder = %ld\r\n\r\n", count);

        if ((auto_mode != 0U) && (stepper_ctrl_is_stop_requested() == 0U)) {
          pause_ticks = PAUSE_TICK_COUNT;
          state       = ST_PAUSE_POST_CCW;
        }
        else {
          state = ST_IDLE;
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
