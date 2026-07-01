/**
 * @file    stepper.c
 * @brief   Multi-axis stepper motor control via DRV8424 (Stepper 19 Click)
 */

#include "stepper.h"
#include "app_console.h"

#include "FreeRTOS.h"
#include "semphr.h"

/* -----------------------------------------------------------------------
 * Private constants — DRV8424 fixed signal polarities
 * ----------------------------------------------------------------------- */

#define STEP_ACTIVE    HAL_GPIO_PIN_SET    /* Rising edge triggers a microstep  */
#define STEP_INACTIVE  HAL_GPIO_PIN_RESET
#define EN_ACTIVE      HAL_GPIO_PIN_SET    /* DRV8424 EN is active-high         */
#define EN_INACTIVE    HAL_GPIO_PIN_RESET
#define NSLP_WAKE      HAL_GPIO_PIN_SET    /* nSLEEP high = driver awake        */
#define NSLP_SLEEP     HAL_GPIO_PIN_RESET
#define NFAULT_FAULT   HAL_GPIO_PIN_RESET  /* nFAULT open-drain active-low      */

/* -----------------------------------------------------------------------
 * Private types
 * ----------------------------------------------------------------------- */

struct stepper_s {
  stepper_gpio_config_t pins;             /* GPIO assignments (copied at init)  */
  volatile uint32_t     target_steps;     /* Total microsteps requested         */
  volatile uint32_t     steps_done;       /* Microsteps completed so far        */
  volatile uint32_t     tick_period;      /* Timer ticks between step edges     */
  volatile uint32_t     tick_counter;     /* Ticks since last step edge         */
  volatile uint8_t      running;          /* 1 = move in progress               */
  volatile uint8_t      step_pin_state;   /* Current state of STEP pin          */
  SemaphoreHandle_t     done_sem;         /* Signalled by ISR when move done    */
  stepper_done_cb_t     done_cb;          /* Optional ISR-context callback      */
};

/* -----------------------------------------------------------------------
 * Private state
 * ----------------------------------------------------------------------- */

static stepper_t s_motors[STEPPER_MAX_MOTORS];
static uint8_t   s_motor_count = 0U;

/* -----------------------------------------------------------------------
 * Private functions
 * ----------------------------------------------------------------------- */

/**
 * @brief ISR update callback registered against the shared step timer.
 *
 * Runs at STEPPER_TIMER_TICK_HZ. Iterates all registered motors and advances
 * each active move by one tick. Generates STEP pulses by toggling the STEP pin
 * at the rate defined per motor by tick_period. On completion: disables driver
 * outputs, fires the optional done callback, then signals the done semaphore.
 */
static void stepper_isr_cb(hal_tim_handle_t *htim) {
  (void)htim;

  BaseType_t x_higher_priority_task_woken = pdFALSE;

  for (uint8_t i = 0U; i < s_motor_count; i++) {
    stepper_t *m = &s_motors[i];

    if (m->running == 0U) {
      continue;
    }

    m->tick_counter++;

    if (m->tick_counter < m->tick_period) {
      continue;
    }

    m->tick_counter = 0U;

    if (m->step_pin_state == 0U) {
      HAL_GPIO_WritePin(m->pins.step.port, m->pins.step.pin, STEP_ACTIVE);
      m->step_pin_state = 1U;
    }
    else {
      HAL_GPIO_WritePin(m->pins.step.port, m->pins.step.pin, STEP_INACTIVE);
      m->step_pin_state = 0U;

      /* Count on the falling edge — one full pulse = one microstep */
      m->steps_done++;

      if (m->steps_done >= m->target_steps) {
        m->running = 0U;
        HAL_GPIO_WritePin(m->pins.en.port, m->pins.en.pin, EN_INACTIVE);

        if (m->done_cb != NULL) {
          m->done_cb(m);
        }

        xSemaphoreGiveFromISR(m->done_sem, &x_higher_priority_task_woken);
      }
    }
  }

  portYIELD_FROM_ISR(x_higher_priority_task_woken);
}

/* -----------------------------------------------------------------------
 * Module-level API
 * ----------------------------------------------------------------------- */

void stepper_module_init(hal_tim_handle_t *htim) {
  configASSERT(htim != NULL);

  hal_status_t ret = HAL_TIM_RegisterUpdateCallback(htim, stepper_isr_cb);
  configASSERT(ret == HAL_OK);

  ret = HAL_TIM_Start_IT_Opt(htim, HAL_TIM_OPT_IT_UPDATE);
  configASSERT(ret == HAL_OK);

  app_console_print("[STEPPER] Module init. %u usteps/rev, timer %luHz\r\n",
                    STEPPER_USTEPS_PER_REV, STEPPER_TIMER_TICK_HZ);
}

/* -----------------------------------------------------------------------
 * Per-motor API
 * ----------------------------------------------------------------------- */

stepper_t *stepper_init(const stepper_gpio_config_t *pins) {
  if (pins == NULL) {
    return NULL;
  }

  if (s_motor_count >= STEPPER_MAX_MOTORS) {
    app_console_print("[STEPPER] ERROR: motor pool full (%u/%u)\r\n",
                      (unsigned)s_motor_count, (unsigned)STEPPER_MAX_MOTORS);
    return NULL;
  }

  stepper_t *m  = &s_motors[s_motor_count];
  s_motor_count++;

  m->pins           = *pins;
  m->target_steps   = 0U;
  m->steps_done     = 0U;
  m->tick_period    = 0U;
  m->tick_counter   = 0U;
  m->running        = 0U;
  m->step_pin_state = 0U;
  m->done_cb        = NULL;

  m->done_sem = xSemaphoreCreateBinary();
  configASSERT(m->done_sem != NULL);

  stepper_disable(m);
  stepper_wake(m);


  app_console_print("[STEPPER] Motor %u init OK.\r\n", (unsigned)(s_motor_count - 1U));

  return m;
}

uint32_t stepper_rpm_to_ticks(uint32_t rpm) {
  if (rpm == 0U) {
    return UINT32_MAX;
  }

  /* Each step = one HIGH + one LOW on the STEP pin.
   * ticks_per_edge = (TIMER_TICK_HZ * 60) / (rpm * usteps_per_rev * 2) */
  return (STEPPER_TIMER_TICK_HZ * 60UL) / (rpm * STEPPER_USTEPS_PER_REV * 2UL);
}

stepper_status_enum stepper_move_start(stepper_t *motor, uint32_t steps, uint32_t rpm,
                                       uint8_t direction) {
  if (motor == NULL) {
    return STEPPER_INVALID;
  }

  if (motor->running != 0U) {
    return STEPPER_BUSY;
  }

  if (stepper_is_fault(motor) != 0U) {
    return STEPPER_FAULT;
  }

  /* Drain any stale completion signal from a previous move */
  (void)xSemaphoreTake(motor->done_sem, 0);

  /* Set direction before enabling — CW = high, CCW = low on DRV8424 */
  HAL_GPIO_WritePin(motor->pins.dir.port, motor->pins.dir.pin,
                    (direction == STEPPER_DIR_CW) ? HAL_GPIO_PIN_SET : HAL_GPIO_PIN_RESET);

  /* Preload move parameters before setting the running flag */
  motor->target_steps   = steps;
  motor->steps_done     = 0U;
  motor->tick_counter   = 0U;
  motor->tick_period    = stepper_rpm_to_ticks(rpm);
  motor->step_pin_state = 0U;

  app_console_print("[STEPPER] MoveStart: %lu steps, %lu RPM, dir=%u, tick_period=%lu\r\n",
                    steps, rpm, direction, motor->tick_period);

  stepper_enable(motor);
  motor->running = 1U;

  return STEPPER_OK;
}

stepper_status_enum stepper_wait_done(stepper_t *motor, uint32_t timeout_ms) {
  if (motor == NULL) {
    return STEPPER_INVALID;
  }

  if (motor->running == 0U) {
    return STEPPER_OK;
  }

  TickType_t ticks =
      (timeout_ms == STEPPER_TIMEOUT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);

  return (xSemaphoreTake(motor->done_sem, ticks) == pdTRUE) ? STEPPER_OK : STEPPER_TIMEOUT;
}

uint8_t stepper_is_busy(stepper_t *motor) {
  if (motor == NULL) {
    return 0U;
  }

  return motor->running;
}

void stepper_register_done_cb(stepper_t *motor, stepper_done_cb_t cb) {
  if (motor != NULL) {
    motor->done_cb = cb;
  }
}

void stepper_stop(stepper_t *motor) {
  if (motor == NULL) {
    return;
  }

  motor->running = 0U;
  HAL_GPIO_WritePin(motor->pins.step.port, motor->pins.step.pin, STEP_INACTIVE);
  stepper_disable(motor);

  app_console_print("[STEPPER] Stopped at step %lu\r\n", motor->steps_done);
}

void stepper_enable(stepper_t *motor) {
  if (motor != NULL) {
    HAL_GPIO_WritePin(motor->pins.en.port, motor->pins.en.pin, EN_ACTIVE);
  }
}

void stepper_disable(stepper_t *motor) {
  if (motor != NULL) {
    HAL_GPIO_WritePin(motor->pins.en.port, motor->pins.en.pin, EN_INACTIVE);
  }
}

void stepper_wake(stepper_t *motor) {
  if (motor != NULL) {
    HAL_GPIO_WritePin(motor->pins.nslp.port, motor->pins.nslp.pin, NSLP_WAKE);
    /* DRV8424 requires ~1ms after wake before accepting STEP pulses */
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void stepper_sleep(stepper_t *motor) {
  if (motor != NULL) {
    HAL_GPIO_WritePin(motor->pins.nslp.port, motor->pins.nslp.pin, NSLP_SLEEP);
  }
}

uint8_t stepper_is_fault(stepper_t *motor) {
  if (motor == NULL) {
    return 0U;
  }

  return (HAL_GPIO_ReadPin(motor->pins.nfault.port, motor->pins.nfault.pin) ==
          NFAULT_FAULT) ? 1U : 0U;
}
