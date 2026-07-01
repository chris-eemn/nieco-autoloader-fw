/* -----------------------------------------------------------------------
 * Usage example for the multi-motor stepper driver.
 *
 * 1. Call stepper_module_init() once to start the shared step timer.
 * 2. Call stepper_init() once per motor, supplying its GPIO pin config.
 * 3. Use the returned handle for all subsequent calls.
 * ----------------------------------------------------------------------- */

#include "stepper.h"
#include "mx_hal_def.h"

void stepper_motor_task(void *argument) {
  (void)argument;

  /* One-time: start the shared step timer (100kHz ISR). */
  stepper_module_init(step_timer_gethandle());

  /* Per-motor: supply GPIO pin assignments from the CubeMX aliases. */
  static const stepper_gpio_config_t k_m1_pins = {
    .step   = { M1_STEP_PORT,   M1_STEP_PIN   },
    .dir    = { M1_DIR_PORT,    M1_DIR_PIN    },
    .en     = { M1_EN_PORT,     M1_EN_PIN     },
    .nslp   = { M1_NSLP_PORT,  M1_NSLP_PIN   },
    .nfault = { M1_NFAULT_PORT, M1_NFAULT_PIN },
  };

  stepper_t *m1 = stepper_init(&k_m1_pins);
  configASSERT(m1 != NULL);

  /* Add a second motor by defining its pins and calling stepper_init() again:
   *
   *   static const stepper_gpio_config_t k_m2_pins = { .step_port = M2_STEP_PORT, ... };
   *   stepper_t *m2 = stepper_init(&k_m2_pins);
   */

  for (;;) {
    app_console_print("[TASK] CW: 1 revolution\r\n");
    (void)stepper_move_start(m1, STEPPER_USTEPS_PER_REV, 60U, STEPPER_DIR_CW);
    (void)stepper_wait_done(m1, STEPPER_TIMEOUT_FOREVER);

    vTaskDelay(pdMS_TO_TICKS(1000U));

    app_console_print("[TASK] CCW: 1 revolution\r\n");
    (void)stepper_move_start(m1, STEPPER_USTEPS_PER_REV, 60U, STEPPER_DIR_CCW);
    (void)stepper_wait_done(m1, STEPPER_TIMEOUT_FOREVER);

    vTaskDelay(pdMS_TO_TICKS(1000U));
  }
}
