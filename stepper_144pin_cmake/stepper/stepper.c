/**
 * @file    stepper.c
 * @brief   Single-axis stepper motor control via DRV8424 (Stepper 19 Click)
 */

#include "stepper.h"
#include "app_console.h"

/* CubeMX generated GPIO/timer aliases */
#include "mx_gpio_default.h"
#include "mx_tim12.h"
#include "mx_hal_def.h"

/* FreeRTOS */
#include "FreeRTOS.h"
#include "semphr.h"

/* -----------------------------------------------------------------------
 * Private types
 * ----------------------------------------------------------------------- */

typedef struct {
  volatile uint32_t target_steps;    /* Total microsteps requested      */
  volatile uint32_t steps_done;      /* Microsteps completed so far     */
  volatile uint32_t tick_period;     /* Timer ticks between step pulses */
  volatile uint32_t tick_counter;    /* Ticks since last step pulse     */
  volatile uint8_t  running;         /* 1 = move in progress            */
  volatile uint8_t  step_pin_state;  /* Current state of STEP pin       */
  SemaphoreHandle_t done_sem;        /* Signalled by ISR when move done */
  stepper_done_cb_t done_cb;         /* Optional ISR-context callback   */
} stepper_t;

/* -----------------------------------------------------------------------
 * Private state
 * ----------------------------------------------------------------------- */

static stepper_t s_motor = { 0 };

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

uint32_t stepper_rpm_to_ticks(uint32_t rpm) {
  if (rpm == 0U) {
    return UINT32_MAX;
  }

  /* Each step = one HIGH + one LOW on the STEP pin → one full pulse = 2 ISR ticks.
   * ticks_per_step = (TIMER_TICK_HZ * 60) / (rpm * usteps_per_rev * 2) */
  return (STEPPER_TIMER_TICK_HZ * 60UL) / (rpm * STEPPER_USTEPS_PER_REV * 2UL);
}

void stepper_init(void) {
  s_motor.done_sem = xSemaphoreCreateBinary();
  configASSERT(s_motor.done_sem != NULL);

  s_motor.done_cb = NULL;

  /* Driver starts disabled and awake — outputs off until stepper_move_start */
  stepper_disable();
  stepper_wake();

  /* Register update callback directly against TIM12 handle.
   * This avoids touching the global HAL_TIM_UpdateCallback which is
   * already used by the FreeRTOS HAL timebase (TIM17). */
  hal_tim_handle_t *htim = step_timer_gethandle();
  configASSERT(htim != NULL);

  hal_status_t ret = HAL_TIM_RegisterUpdateCallback(htim, stepper_tim_period_elapsed_cb);
  configASSERT(ret == HAL_OK);

  /* Start the timer with update interrupt enabled — fires at 100kHz */
  ret = HAL_TIM_Start_IT_Opt(htim, HAL_TIM_OPT_IT_UPDATE);
  configASSERT(ret == HAL_OK);

  app_console_print("[STEPPER] Initialised. %u usteps/rev, timer %luHz\r\n",
                    STEPPER_USTEPS_PER_REV,
                    STEPPER_TIMER_TICK_HZ);
}

stepper_status_enum stepper_move_start(uint32_t steps, uint32_t rpm, uint8_t direction) {
  if (s_motor.running != 0U) {
    return STEPPER_BUSY;
  }

  if (stepper_is_fault() != 0U) {
    return STEPPER_FAULT;
  }

  /* Drain any stale completion signal from a previous move */
  (void)xSemaphoreTake(s_motor.done_sem, 0);

  /* Set direction before enabling */
  HAL_GPIO_WritePin(M1_DIR_PORT, M1_DIR_PIN,
                    (direction == STEPPER_DIR_CW) ? HAL_GPIO_PIN_SET : HAL_GPIO_PIN_RESET);

  /* Preload move parameters — do this before setting running flag */
  s_motor.target_steps   = steps;
  s_motor.steps_done     = 0U;
  s_motor.tick_counter   = 0U;
  s_motor.tick_period    = stepper_rpm_to_ticks(rpm);
  s_motor.step_pin_state = 0U;

  app_console_print("[STEPPER] MoveStart: %lu steps, %lu RPM, dir=%u, tick_period=%lu\r\n",
                    steps, rpm, direction, s_motor.tick_period);

  /* Enable driver outputs then start move */
  stepper_enable();
  s_motor.running = 1U;

  return STEPPER_OK;
}

stepper_status_enum stepper_wait_done(uint32_t timeout_ms) {
  if (s_motor.running == 0U) {
    return STEPPER_OK;
  }

  TickType_t ticks =
      (timeout_ms == STEPPER_TIMEOUT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);

  if (xSemaphoreTake(s_motor.done_sem, ticks) == pdTRUE) {
    return STEPPER_OK;
  }

  return STEPPER_TIMEOUT;
}

uint8_t stepper_is_busy(void) {
  return s_motor.running;
}

void stepper_register_done_cb(stepper_done_cb_t cb) {
  s_motor.done_cb = cb;
}

void stepper_stop(void) {
  s_motor.running = 0U;
  HAL_GPIO_WritePin(M1_STEP_PORT, M1_STEP_PIN, M1_STEP_INACTIVE_STATE);
  stepper_disable();
  app_console_print("[STEPPER] Stopped at step %lu\r\n", s_motor.steps_done);
}

void stepper_enable(void) {
  HAL_GPIO_WritePin(M1_EN_PORT, M1_EN_PIN, M1_EN_ACTIVE_STATE);
}

void stepper_disable(void) {
  HAL_GPIO_WritePin(M1_EN_PORT, M1_EN_PIN, M1_EN_INACTIVE_STATE);
}

void stepper_wake(void) {
  HAL_GPIO_WritePin(M1_NSLP_PORT, M1_NSLP_PIN, M1_NSLP_ACTIVE_STATE);
  /* DRV8424 requires ~1ms after wake before accepting STEP pulses */
  vTaskDelay(pdMS_TO_TICKS(1));
}

void stepper_sleep(void) {
  HAL_GPIO_WritePin(M1_NSLP_PORT, M1_NSLP_PIN, M1_NSLP_INACTIVE_STATE);
}

uint8_t stepper_is_fault(void) {
  /* nFAULT is active low open-drain — LOW means fault asserted */
  return (HAL_GPIO_ReadPin(M1_NFAULT_PORT, M1_NFAULT_PIN) == HAL_GPIO_PIN_RESET) ? 1U : 0U;
}

/* -----------------------------------------------------------------------
 * ISR callback
 * ----------------------------------------------------------------------- */

/**
 * @brief Called from HAL_TIM_UpdateCallback when TIM12 fires.
 *
 * Runs at 100kHz. Generates STEP pulses by toggling the STEP pin at the
 * rate defined by tick_period. On completion: disables driver outputs,
 * fires the optional done callback, then signals the done semaphore.
 */
void stepper_tim_period_elapsed_cb(hal_tim_handle_t *htim) {
  (void)htim;  /* TIM12 only — handle not needed */

  if (s_motor.running == 0U) {
    return;
  }

  s_motor.tick_counter++;

  if (s_motor.tick_counter < s_motor.tick_period) {
    return;
  }

  s_motor.tick_counter = 0U;

  /* Toggle STEP pin */
  if (s_motor.step_pin_state == 0U) {
    HAL_GPIO_WritePin(M1_STEP_PORT, M1_STEP_PIN, M1_STEP_ACTIVE_STATE);
    s_motor.step_pin_state = 1U;
  }
  else {
    HAL_GPIO_WritePin(M1_STEP_PORT, M1_STEP_PIN, M1_STEP_INACTIVE_STATE);
    s_motor.step_pin_state = 0U;

    /* Count on the falling edge — one full pulse = one microstep */
    s_motor.steps_done++;

    if (s_motor.steps_done >= s_motor.target_steps) {
      s_motor.running = 0U;
      stepper_disable();

      if (s_motor.done_cb != NULL) {
        s_motor.done_cb();
      }

      /* Signal any waiting task */
      BaseType_t x_higher_priority_task_woken = pdFALSE;
      xSemaphoreGiveFromISR(s_motor.done_sem, &x_higher_priority_task_woken);
      portYIELD_FROM_ISR(x_higher_priority_task_woken);
    }
  }
}
