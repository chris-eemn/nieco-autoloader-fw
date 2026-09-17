/**
 * @file stepper.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief Multi-axis stepper motor control via DRV8424 (Stepper 19 Click).
 * @version 0.1
 * @date 2026-07-02
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include "stepper.h"
#include "app_console.h"

#include "FreeRTOS.h"
#include "task.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

#define STEP_ACTIVE HAL_GPIO_PIN_SET /* Rising edge triggers a microstep  */
#define STEP_INACTIVE HAL_GPIO_PIN_RESET
#define EN_ACTIVE HAL_GPIO_PIN_RESET /* DRV8424 EN is active-high but this pin routes through an inverting circuit        */
#define EN_INACTIVE HAL_GPIO_PIN_SET
#define NSLP_WAKE HAL_GPIO_PIN_SET /* nSLEEP high = driver awake        */
#define NSLP_SLEEP HAL_GPIO_PIN_RESET
#define NFAULT_FAULT HAL_GPIO_PIN_RESET /* nFAULT open-drain active-low      */

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

struct stepper_s {
  stepper_gpio_config_t pins;      /* GPIO assignments (copied at init)        */
  volatile uint32_t target_steps;  /* Total microsteps requested               */
  volatile uint32_t steps_done;    /* Microsteps completed so far              */
  volatile uint32_t tick_period;   /* Timer ticks between step edges           */
  volatile uint32_t tick_counter;  /* Ticks since last step edge               */
  volatile uint8_t running;        /* 1 = move in progress                     */
  volatile uint8_t step_pin_state; /* Current state of STEP pin                */
  volatile uint8_t fault_latched;  /* 1 if a fault has been detected and latched */
  stepper_done_cb_t done_cb;       /* Optional ISR-context done callback       */
  stepper_fault_cb_t fault_cb;     /* Optional ISR-context fault callback      */

  /* Per-step encoder following-error monitor. The ISR owns the running totals;
   * task code only configures them between moves and takes completed
   * events. */
  encoder_t* sync_encoder;
  stepper_sync_config_t sync_config;
  uint32_t sync_last_raw;
  int32_t sync_travel_counts;
  uint32_t sync_expected_units;
  volatile uint32_t sync_event_deviation_counts;
  volatile stepper_sync_event_enum sync_event;
  uint8_t sync_enabled;
  uint8_t sync_lead_reported;
};

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

static stepper_t s_motors[STEPPER_MAX_MOTORS];
static uint8_t s_motor_count = 0U;

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

static void stepper_isr_cb(hal_tim_handle_t* htim);
static void stepper_fault_exti_cb(hal_exti_handle_t* hexti, hal_exti_trigger_t trigger);
static uint8_t stepper_sync_update_from_isr(stepper_t* motor);
static void stepper_stop_from_isr(stepper_t* motor);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

void stepper_module_init(hal_tim_handle_t* htim) {
  configASSERT(htim != NULL);

  hal_status_t ret = HAL_TIM_RegisterUpdateCallback(htim, stepper_isr_cb);
  configASSERT(ret == HAL_OK);

  ret = HAL_TIM_Start_IT_Opt(htim, HAL_TIM_OPT_IT_UPDATE);
  configASSERT(ret == HAL_OK);

  app_console_print("[STEPPER] Module init. %u usteps/rev, timer %luHz\r\n", STEPPER_USTEPS_PER_REV, STEPPER_TIMER_TICK_HZ);
}

stepper_t* stepper_init(const stepper_gpio_config_t* pins) {
  if (pins == NULL) {
    return NULL;
  }

  if (s_motor_count >= STEPPER_MAX_MOTORS) {
    app_console_print("[STEPPER] ERROR: motor pool full (%u/%u)\r\n", (unsigned)s_motor_count, (unsigned)STEPPER_MAX_MOTORS);
    return NULL;
  }

  stepper_t* m = &s_motors[s_motor_count];
  s_motor_count++;

  m->pins = *pins;
  m->target_steps = 0U;
  m->steps_done = 0U;
  m->tick_period = 0U;
  m->tick_counter = 0U;
  m->running = 0U;
  m->step_pin_state = 0U;
  m->fault_latched = 0U;
  m->done_cb = NULL;
  m->fault_cb = NULL;
  m->sync_encoder = NULL;
  m->sync_last_raw = 0U;
  m->sync_travel_counts = 0;
  m->sync_expected_units = 0U;
  m->sync_event_deviation_counts = 0U;
  m->sync_event = STEPPER_SYNC_EVENT_NONE;
  m->sync_enabled = 0U;
  m->sync_lead_reported = 0U;

  stepper_disable(m);
  stepper_wake(m);

  app_console_print("[STEPPER] Motor %u init OK.\r\n", (unsigned)(s_motor_count - 1U));

  return m;
}

stepper_status_enum stepper_sync_configure(stepper_t* motor, encoder_t* encoder, const stepper_sync_config_t* config) {
  stepper_status_enum status = STEPPER_INVALID;

  if ((motor != NULL) && (encoder != NULL) && (config != NULL) && (config->encoder_counts_numerator > 0U) &&
      (config->encoder_counts_denominator > 0U) && (config->max_error_counts > 0U)) {
    if (motor->running != 0U) {
      status = STEPPER_BUSY;
    }
    else {
      motor->sync_encoder = encoder;
      motor->sync_config = *config;
      motor->sync_last_raw = encoder_get_raw(encoder);
      motor->sync_travel_counts = 0;
      motor->sync_expected_units = 0U;
      motor->sync_event_deviation_counts = 0U;
      motor->sync_event = STEPPER_SYNC_EVENT_NONE;
      motor->sync_enabled = 1U;
      motor->sync_lead_reported = 0U;
      status = STEPPER_OK;
    }
  }

  return status;
}

stepper_sync_event_enum stepper_sync_take_event(stepper_t* motor, uint32_t* deviation_counts) {
  stepper_sync_event_enum event = STEPPER_SYNC_EVENT_NONE;

  if (motor != NULL) {
    taskENTER_CRITICAL();
    event = motor->sync_event;
    if (deviation_counts != NULL) {
      *deviation_counts = motor->sync_event_deviation_counts;
    }
    motor->sync_event = STEPPER_SYNC_EVENT_NONE;
    taskEXIT_CRITICAL();
  }

  return event;
}

uint32_t stepper_rpm_to_ticks(uint32_t rpm) {
  if (rpm == 0U) {
    return UINT32_MAX;
  }

  /* Each step = one HIGH + one LOW on the STEP pin.
   * ticks_per_edge = (TIMER_TICK_HZ * 60) / (rpm * usteps_per_rev * 2) */
  return (STEPPER_TIMER_TICK_HZ * 60UL) / (rpm * STEPPER_USTEPS_PER_REV * 2UL);
}

stepper_status_enum stepper_move_start(stepper_t* motor, uint32_t steps, uint32_t rpm, uint8_t direction) {
  if (motor == NULL) {
    return STEPPER_INVALID;
  }

  if (motor->running != 0U) {
    return STEPPER_BUSY;
  }

  /* Sample the live pin at move-start time to catch faults that occurred before
   * the EXTI was registered or while the system was idle. */
  if (HAL_GPIO_ReadPin(motor->pins.nfault.port, motor->pins.nfault.pin) == NFAULT_FAULT) {
    motor->fault_latched = 1U;
  }

  if (stepper_is_fault(motor) != 0U) {
    return STEPPER_FAULT;
  }

  /* Set direction before enabling — CW = high, CCW = low on DRV8424 */
  HAL_GPIO_WritePin(motor->pins.dir.port, motor->pins.dir.pin, (direction == STEPPER_DIR_CW) ? HAL_GPIO_PIN_SET : HAL_GPIO_PIN_RESET);

  /* Preload move parameters before setting the running flag */
  motor->target_steps = steps;
  motor->steps_done = 0U;
  motor->tick_counter = 0U;
  motor->tick_period = stepper_rpm_to_ticks(rpm);
  motor->step_pin_state = 0U;

  if (motor->sync_enabled != 0U) {
    motor->sync_last_raw = encoder_get_raw(motor->sync_encoder);
    motor->sync_travel_counts = 0;
    motor->sync_expected_units = 0U;
    motor->sync_lead_reported = 0U;
  }

  app_console_print("[STEPPER] MoveStart: %lu steps, %lu RPM, dir=%u, tick_period=%lu\r\n", steps, rpm, direction, motor->tick_period);

  stepper_enable(motor);
  motor->running = 1U;

  return STEPPER_OK;
}

uint8_t stepper_is_busy(stepper_t* motor) {
  if (motor == NULL) {
    return 0U;
  }

  return motor->running;
}

uint32_t stepper_get_steps_done(const stepper_t* motor) {
  if (motor == NULL) {
    return 0U;
  }

  /* Single-word load of a volatile uint32_t — atomic against the step ISR. */
  return motor->steps_done;
}

void stepper_register_done_cb(stepper_t* motor, stepper_done_cb_t cb) {
  if (motor != NULL) {
    motor->done_cb = cb;
  }
}

void stepper_stop(stepper_t* motor) {
  if (motor == NULL) {
    return;
  }

  motor->running = 0U;
  HAL_GPIO_WritePin(motor->pins.step.port, motor->pins.step.pin, STEP_INACTIVE);
  stepper_disable(motor);

  app_console_print("[STEPPER] Stopped at step %lu\r\n", motor->steps_done);
}

void stepper_enable(stepper_t* motor) {
  if (motor != NULL) {
    HAL_GPIO_WritePin(motor->pins.en.port, motor->pins.en.pin, EN_ACTIVE);
  }
}

void stepper_disable(stepper_t* motor) {
  if (motor != NULL) {
    HAL_GPIO_WritePin(motor->pins.en.port, motor->pins.en.pin, EN_INACTIVE);
  }
}

void stepper_wake(stepper_t* motor) {
  if (motor != NULL) {
    HAL_GPIO_WritePin(motor->pins.nslp.port, motor->pins.nslp.pin, NSLP_WAKE);
    /* DRV8424 requires ~1ms after wake before accepting STEP pulses */
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void stepper_sleep(stepper_t* motor) {
  if (motor != NULL) {
    HAL_GPIO_WritePin(motor->pins.nslp.port, motor->pins.nslp.pin, NSLP_SLEEP);
  }
}

uint8_t stepper_is_fault(stepper_t* motor) {
  if (motor == NULL) {
    return 0U;
  }

  return motor->fault_latched;
}

void stepper_register_fault_exti(stepper_t* motor, hal_exti_handle_t* hexti) {
  if ((motor == NULL) || (hexti == NULL)) {
    return;
  }

  /* Store the motor handle in the EXTI user-data slot so the ISR callback
   * can recover it without iterating the motor pool. */
  HAL_EXTI_SetUserData(hexti, motor);

  hal_status_t ret = HAL_EXTI_RegisterTriggerCallback(hexti, stepper_fault_exti_cb);
  configASSERT(ret == HAL_OK);
}

void stepper_register_fault_cb(stepper_t* motor, stepper_fault_cb_t cb) {
  if (motor != NULL) {
    motor->fault_cb = cb;
  }
}

void stepper_clear_fault(stepper_t* motor) {
  if (motor != NULL) {
    motor->fault_latched = 0U;
  }
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/

/**
 * @brief ISR update callback registered against the shared step timer.
 *
 * Runs at STEPPER_TIMER_TICK_HZ. Iterates all registered motors and advances
 * each active move by one tick. Generates STEP pulses by toggling the STEP pin
 * at the rate defined per motor by tick_period. On completion: disables driver
 * outputs and fires the optional done callback.
 */
static void stepper_isr_cb(hal_tim_handle_t* htim) {
  (void)htim;

  for (uint8_t i = 0U; i < s_motor_count; i++) {
    stepper_t* m = &s_motors[i];

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

      if (stepper_sync_update_from_isr(m) != 0U) {
        stepper_stop_from_isr(m);
      }
      else if (m->steps_done >= m->target_steps) {
        m->running = 0U;
        HAL_GPIO_WritePin(m->pins.en.port, m->pins.en.pin, EN_INACTIVE);

        if (m->done_cb != NULL) {
          m->done_cb(m);
        }
      }
    }
  }
}

/**
 * @brief Update one motor's cumulative following error after a completed microstep.
 * @param motor Motor currently being advanced by the shared
 * step ISR.
 * @return 1 when encoder lag reached the configured limit and motion must stop; otherwise 0.
 */
static uint8_t stepper_sync_update_from_isr(stepper_t* motor) {
  uint8_t stop_motor = 0U;

  if ((motor->sync_enabled != 0U) && (motor->sync_encoder != NULL)) {
    uint32_t raw = encoder_get_raw(motor->sync_encoder);
    int32_t delta = (int32_t)(int16_t)((uint16_t)raw - (uint16_t)motor->sync_last_raw);

    motor->sync_last_raw = raw;
    motor->sync_travel_counts += delta;
    motor->sync_expected_units += motor->sync_config.encoder_counts_numerator;

    uint32_t travel_counts = (motor->sync_travel_counts < 0) ? (uint32_t)(-motor->sync_travel_counts) : (uint32_t)motor->sync_travel_counts;
    uint32_t actual_units = travel_counts * motor->sync_config.encoder_counts_denominator;
    uint32_t error_limit_units = motor->sync_config.max_error_counts * motor->sync_config.encoder_counts_denominator;

    if (motor->sync_expected_units > actual_units) {
      uint32_t lag_units = motor->sync_expected_units - actual_units;

      if (lag_units >= error_limit_units) {
        motor->sync_event_deviation_counts =
            (lag_units + motor->sync_config.encoder_counts_denominator - 1U) / motor->sync_config.encoder_counts_denominator;
        motor->sync_event = STEPPER_SYNC_EVENT_LAG;
        stop_motor = 1U;
      }
    }
    else if ((motor->sync_lead_reported == 0U) && ((actual_units - motor->sync_expected_units) >= error_limit_units)) {
      uint32_t lead_units = actual_units - motor->sync_expected_units;

      motor->sync_event_deviation_counts =
          (lead_units + motor->sync_config.encoder_counts_denominator - 1U) / motor->sync_config.encoder_counts_denominator;
      motor->sync_event = STEPPER_SYNC_EVENT_LEAD;
      motor->sync_lead_reported = 1U;
    }
    else {
      /* Following error is inside the configured window. */
    }
  }

  return stop_motor;
}

/**
 * @brief Stop and disable one motor from the shared step ISR.
 * @param motor Motor to stop.
 */
static void stepper_stop_from_isr(stepper_t* motor) {
  motor->running = 0U;
  HAL_GPIO_WritePin(motor->pins.step.port, motor->pins.step.pin, STEP_INACTIVE);
  HAL_GPIO_WritePin(motor->pins.en.port, motor->pins.en.pin, EN_INACTIVE);
}

/**
 * @brief EXTI falling-edge ISR callback for nFAULT monitoring.
 *
 * Registered per-motor via stepper_register_fault_exti(). The motor handle
 * is recovered from the EXTI user-data slot set at registration time.
 * Latches the fault, stops any in-progress move, and fires the fault callback.
 */
static void stepper_fault_exti_cb(hal_exti_handle_t* hexti, hal_exti_trigger_t trigger) {
  (void)trigger;

  stepper_t* m = (stepper_t*)HAL_EXTI_GetUserData(hexti);
  if (m == NULL) {
    return;
  }

  m->fault_latched = 1U;

  if (m->running != 0U) {
    m->running = 0U;
    HAL_GPIO_WritePin(m->pins.step.port, m->pins.step.pin, STEP_INACTIVE);
    HAL_GPIO_WritePin(m->pins.en.port, m->pins.en.pin, EN_INACTIVE);
  }

  if (m->fault_cb != NULL) {
    m->fault_cb(m);
  }
}
