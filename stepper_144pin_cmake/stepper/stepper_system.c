/**
 * @file stepper_system.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief Board-level construction and registration of the stepper axes.
 * @version 0.1
 * @date 2026-08-07
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC. All Rights Reserved.
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include "stepper_system.h"

#include <stddef.h>
#include <stdint.h>

#include "app_console.h"
#include "axis.h"
#include "cal_data.h"
#include "encoder.h"
#include "main.h"
#include "mx_i2c1.h"
#include "pca9538a.h"
#include "stepper.h"
#include "stepper_ctrl.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

#define STEPPER_SYSTEM_MOTOR_COUNT (4U)

#define IO_EXPANDER_M0 (0x01U)
#define IO_EXPANDER_M1 (0x02U)

/* Bench hardware: 2.5 encoder counts per microstep. Production gearing and
 * encoder resolution will replace these values after mechanical integration. */
#define STEPPER_SYSTEM_ENCODER_COUNTS_NUMERATOR (5U)
#define STEPPER_SYSTEM_ENCODER_COUNTS_DENOMINATOR (2U)
#define STEPPER_SYSTEM_HOMING_SETTLING_DELAY_DEFAULT_MS (100)  // ms

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/** Zero-based encoder connector IDs used to index s_encoders. */
typedef enum {
  ENC1 = 0,
  ENC2 = 1,
  ENC3 = 2,
  ENC4 = 3,
  ENC5 = 4,
  ENC6 = 5,
  ENC7 = 6,
  ENC8 = 7,
  ENCODER_COUNT = 8,
} encoder_id_enum;

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

static encoder_t* s_encoders[ENCODER_COUNT] = {NULL};
static stepper_t* s_motors[STEPPER_SYSTEM_MOTOR_COUNT] = {NULL};
static axis_t* s_axes[STEPPER_SYSTEM_MOTOR_COUNT] = {NULL};
static bool s_initialized = false;

static const stepper_gpio_config_t s_motor_pins[STEPPER_SYSTEM_MOTOR_COUNT] = {
    {
        .step = {M1_STEP_PORT, M1_STEP_PIN},
        .dir = {M1_DIR_PORT, M1_DIR_PIN},
        .en = {M1_EN_PORT, M1_EN_PIN},
        .nslp = {M1_NSLP_PORT, M1_NSLP_PIN},
        .nfault = {M1_NFAULT_PORT, M1_NFAULT_PIN},
    },
    {
        .step = {M2_STEP_PORT, M2_STEP_PIN},
        .dir = {M2_DIR_PORT, M2_DIR_PIN},
        .en = {M2_EN_PORT, M2_EN_PIN},
        .nslp = {M2_NSLP_PORT, M2_NSLP_PIN},
        .nfault = {M2_NFAULT_PORT, M2_NFAULT_PIN},
    },
    {
        .step = {M3_STEP_PORT, M3_STEP_PIN},
        .dir = {M3_DIR_PORT, M3_DIR_PIN},
        .en = {M3_EN_PORT, M3_EN_PIN},
        .nslp = {M3_NSLP_PORT, M3_NSLP_PIN},
        .nfault = {M3_NFAULT_PORT, M3_NFAULT_PIN},
    },
    {
        .step = {M4_STEP_PORT, M4_STEP_PIN},
        .dir = {M4_DIR_PORT, M4_DIR_PIN},
        .en = {M4_EN_PORT, M4_EN_PIN},
        .nslp = {M4_NSLP_PORT, M4_NSLP_PIN},
        .nfault = {M4_NFAULT_PORT, M4_NFAULT_PIN},
    },
};

static const axis_config_t s_axis_config = {
    .supervisor_period_ms = 25U,
    .encoder_counts_numerator = STEPPER_SYSTEM_ENCODER_COUNTS_NUMERATOR,
    .encoder_counts_denominator = STEPPER_SYSTEM_ENCODER_COUNTS_DENOMINATOR,
    .stall_error_counts = AXIS_DEFAULT_STALL_ERROR_COUNTS,
    .home_error_counts = AXIS_DEFAULT_HOME_ERROR_COUNTS,
    .backoff_steps = 200U,
    .home_max_steps = 50000U,
};

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

static encoder_t* stepper_system_start_tim_encoder(hal_tim_handle_t* timer);
static encoder_t* stepper_system_start_lptim_encoder(hal_lptim_handle_t* timer);
static bool stepper_system_init_encoders(void);
static bool stepper_system_init_io_expander(void);
static bool stepper_system_init_axes(void);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

bool stepper_system_init(void) {
  bool initialized = s_initialized;

  if (initialized == false) {
    initialized = stepper_system_init_encoders();
    if (initialized != false) {
      initialized = stepper_system_init_io_expander();
    }
    if (initialized != false) {
      initialized = stepper_system_init_axes();
    }

    stepper_system_update_configs();

    s_initialized = initialized;
  }

  return initialized;
}

void stepper_system_update_configs(void) {
  cal_data_params_t* cal_params_ptr = cal_data_get();
  if (cal_params_ptr == NULL) {
    app_console_print("[ERROR] Failed to get cal_data_params_t\r\n");
    return;
  }

  for (size_t i = 0U; i < STEPPER_SYSTEM_MOTOR_COUNT; i++) {
    if (s_axes[i] != NULL) {
      axis_config_t axis_config = s_axis_config;
      axis_config.backoff_steps = cal_params_ptr->home_backoff_steps;
      axis_config.home_max_steps = cal_params_ptr->home_max_steps;
      axis_config.stall_error_counts = cal_params_ptr->stall_error_counts;
      axis_config.home_error_counts = cal_params_ptr->home_error_counts;
      axis_config.supervisor_period_ms = cal_params_ptr->supervisor_period_ms;
      axis_config.settle_delay_ms = cal_params_ptr->home_settle_delay_ms;

      if (axis_config.supervisor_period_ms == 0U) {
        axis_config.supervisor_period_ms = AXIS_DEFAULT_SUPERVISOR_PERIOD_MS;
      }
      if (axis_config.stall_error_counts == 0U) {
        axis_config.stall_error_counts = AXIS_DEFAULT_STALL_ERROR_COUNTS;
      }
      if (axis_config.home_error_counts == 0U) {
        axis_config.home_error_counts = AXIS_DEFAULT_HOME_ERROR_COUNTS;
      }
      if (axis_config.backoff_steps == 0U) {
        axis_config.backoff_steps = 200U;
      }
      if (axis_config.home_max_steps == 0U) {
        axis_config.home_max_steps = 5000U;
      }
      if (axis_config.encoder_counts_numerator == 0U) {
        axis_config.encoder_counts_numerator = STEPPER_SYSTEM_ENCODER_COUNTS_NUMERATOR;
      }
      if (axis_config.encoder_counts_denominator == 0U) {
        axis_config.encoder_counts_denominator = STEPPER_SYSTEM_ENCODER_COUNTS_DENOMINATOR;
      }
      if (axis_config.settle_delay_ms == 0U) {
        axis_config.settle_delay_ms = STEPPER_SYSTEM_HOMING_SETTLING_DELAY_DEFAULT_MS;
      }

      axis_update_config(s_axes[i], &axis_config);
    }
  }
}

void stepper_system_register_axis_event_cb(axis_event_cb_t cb, void* ctx) {
  for (size_t i = 0U; i < STEPPER_SYSTEM_MOTOR_COUNT; i++) {
    if (s_axes[i] != NULL) {
      axis_register_event_cb(s_axes[i], cb, ctx);
    }
  }
}
/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/

/**
 * @brief Start a general-purpose timer encoder and construct its encoder handle.
 * @param timer Timer configured for encoder mode.
 * @return Initialised encoder handle, or NULL on failure.
 */
static encoder_t* stepper_system_start_tim_encoder(hal_tim_handle_t* timer) {
  encoder_t* encoder = NULL;

  if (timer != NULL) {
    if ((HAL_TIM_IC_StartChannel(timer, HAL_TIM_CHANNEL_1) == HAL_OK) && (HAL_TIM_IC_StartChannel(timer, HAL_TIM_CHANNEL_2) == HAL_OK) &&
        (HAL_TIM_Start(timer) == HAL_OK)) {
      encoder = encoder_init(timer);
      if (encoder != NULL) {
        encoder_zero(encoder);
      }
    }
  }

  return encoder;
}

/**
 * @brief Start a low-power timer encoder and construct its encoder handle.
 * @param timer LPTIM configured for encoder mode.
 * @return Initialised encoder handle, or NULL on failure.
 */
static encoder_t* stepper_system_start_lptim_encoder(hal_lptim_handle_t* timer) {
  encoder_t* encoder = NULL;

  if ((timer != NULL) && (HAL_LPTIM_Start(timer) == HAL_OK)) {
    encoder = encoder_init_lptim(timer);
    if (encoder != NULL) {
      encoder_zero(encoder);
    }
  }

  return encoder;
}

/**
 * @brief Initialise and start every board encoder interface.
 * @return true when all eight encoders were initialised; otherwise false.
 */
static bool stepper_system_init_encoders(void) {
  hal_tim_handle_t* timers[ENC8] = {
      m1_encoder_timer_init(), m2_encoder_timer_init(), m3_encoder_timer_init(), m4_encoder_timer_init(),
      m5_encoder_timer_init(), m6_encoder_timer_init(), m7_encoder_timer_init(),
  };
  bool initialized = true;

  for (uint8_t index = 0U; index < (uint8_t)ENC8; index++) {
    s_encoders[index] = stepper_system_start_tim_encoder(timers[index]);
    if (s_encoders[index] == NULL) {
      app_console_print("[ERROR] Encoder %u init failed.\r\n", (uint32_t)(index + 1U));
      initialized = false;
      break;
    }
  }

  if (initialized != false) {
    s_encoders[ENC8] = stepper_system_start_lptim_encoder(m8_encoder_timer_init());
    if (s_encoders[ENC8] == NULL) {
      app_console_print("[ERROR] Encoder 8 init failed.\r\n");
      initialized = false;
    }
  }

  return initialized;
}

/**
 * @brief Configure and verify the shared stepper-driver I/O expander.
 * @return true when the expected output value was read back; otherwise false.
 */
static bool stepper_system_init_io_expander(void) {
  const uint8_t expected_output = (IO_EXPANDER_M0 | IO_EXPANDER_M1);
  uint8_t readback_output = 0U;
  hal_i2c_handle_t* i2c = mx_i2c1_i2c_gethandle();
  bool initialized = false;

  if (i2c == NULL) {
    app_console_print("[ERROR] Stepper I2C handle unavailable.\r\n");
  }
  else if (pca9538a_init(i2c, 0x00U) != HAL_OK) {
    app_console_print("[ERROR] PCA9538A init failed.\r\n");
  }
  else if (pca9538a_write_output(i2c, expected_output) != HAL_OK) {
    app_console_print("[ERROR] PCA9538A write failed.\r\n");
  }
  else if (pca9538a_read_output(i2c, &readback_output) != HAL_OK) {
    app_console_print("[ERROR] PCA9538A read failed.\r\n");
  }
  else if (readback_output != expected_output) {
    app_console_print("PCA9538A FAIL: wrote 0x%02X, read 0x%02X\r\n", expected_output, readback_output);
  }
  else {
    app_console_print("PCA9538A OK: 0x%02X\r\n", readback_output);
    initialized = true;
  }

  return initialized;
}

/**
 * @brief Construct the configured motor/axis pairs and publish them to stepper_ctrl.
 * @return true when every configured axis was constructed; otherwise false.
 */
static bool stepper_system_init_axes(void) {
  hal_exti_handle_t* fault_exti[STEPPER_SYSTEM_MOTOR_COUNT] = {m1_fault_exti_gethandle(), m2_fault_exti_gethandle(), m3_fault_exti_gethandle(),
                                                               m4_fault_exti_gethandle()};
  const encoder_id_enum encoder_id[STEPPER_SYSTEM_MOTOR_COUNT] = {ENC3, ENC2, ENC4, ENC6};
  bool initialized = true;

  stepper_module_init(step_timer_gethandle());

  for (uint8_t index = 0U; index < STEPPER_SYSTEM_MOTOR_COUNT; index++) {
    s_motors[index] = stepper_init(&s_motor_pins[index]);
    if (s_motors[index] != NULL) {
      /* Motor 4 is currently wired to encoder 6 (TIM8). Keep the board mapping
       * here so neither the CLI nor application state machines need to know it. */
      s_axes[index] = axis_init(s_motors[index], s_encoders[encoder_id[index]], fault_exti[index], &s_axis_config);
    }

    if (s_axes[index] == NULL) {
      app_console_print("[ERROR] Motor/axis %u init failed.\r\n", (uint32_t)(index + 1U));
      initialized = false;
      break;
    }

    stepper_ctrl_set_axis((uint8_t)(index + 1U), s_axes[index]);
  }

  return initialized;
}
