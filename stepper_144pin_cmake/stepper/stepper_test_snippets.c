/* -----------------------------------------------------------------------
 * 1. In your main application file or a dedicated callbacks file, add this.
 *    If HAL_TIM_UpdateCallback already exists in the encoder example,
 *    just add the TIM12 check inside it — do NOT define it twice.
 * ----------------------------------------------------------------------- */

void HAL_TIM_UpdateCallback(hal_tim_handle_t *htim) {
  if (htim == step_timer_gethandle()) {
    stepper_tim_period_elapsed_cb(htim);
  }

  /* Add other timer update callbacks here as needed */
}


/* -----------------------------------------------------------------------
 * 2. In your FreeRTOS motor task (or default task for desk testing):
 * ----------------------------------------------------------------------- */

#include "stepper.h"

void stepper_motor_task(void *argument) {
  (void)argument;

  stepper_init();

  for (;;) {
    app_console_print("[TASK] Starting CW move: 1 revolution\r\n");
    (void)stepper_move_start(STEPPER_USTEPS_PER_REV, 60, STEPPER_DIR_CW);
    (void)stepper_wait_done(STEPPER_TIMEOUT_FOREVER);

    vTaskDelay(pdMS_TO_TICKS(1000));

    app_console_print("[TASK] Starting CCW move: 1 revolution\r\n");
    (void)stepper_move_start(STEPPER_USTEPS_PER_REV, 60, STEPPER_DIR_CCW);
    (void)stepper_wait_done(STEPPER_TIMEOUT_FOREVER);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
