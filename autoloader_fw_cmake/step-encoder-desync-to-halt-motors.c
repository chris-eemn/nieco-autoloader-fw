/*
at 8usteps its 2.5 encoder tick for ustep. these can be defines for now

What i need you to do is look at this and implement something similar. Basically when step count and encoder count are not in sync, we need to stop the motor and report as such.
This is useful for homing/stall.  I dont need you to connect to the error yet, but at least stop the motors and print a message to the console.  This will be a good start to implementing a stall detection and homing detection.
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
      else if (m->progress_countdown > 0U) {
        /* Down-counter rather than a modulo on steps_done: this runs on every microstep of every
         * motor at up to 100 kHz, and a divide by a runtime value is not free. */
        m->progress_countdown--;

        if (m->progress_countdown == 0U) {
          m->progress_countdown = m->progress_interval;

          if (m->progress_cb(m) == false) {
            stop_from_isr(m, &x_higher_priority_task_woken);
          }
        }
      }
      else {
        /* No progress monitor registered for this motor. */
      }
    }
  }

  portYIELD_FROM_ISR(x_higher_priority_task_woken);
}


static bool axis_progress_isr_cb(stepper_t *motor) {
  axis_t *axis          = find_axis(motor);
  bool    continue_move = true;

  if (axis != NULL) {
    uint32_t raw = encoder_get_raw(axis->encoder);

    /* Same 16-bit wrap arithmetic as encoder_get_count(), on this detector's own previous sample. */
    int32_t delta = (int32_t)(int16_t)((uint16_t)raw - (uint16_t)axis->isr_last_raw);

    axis->isr_last_raw = raw;
    accumulate_desync(axis, delta);

    if ((axis->peak_delta > 0U) && (axis->desync_units >= axis->desync_limit_units)) {
      axis->no_progress_from_isr = 1U;
      continue_move              = false;
    }
  }

  return continue_move;
}


static void accumulate_desync(axis_t *axis, int32_t delta) {
  uint32_t moved = (delta < 0) ? (uint32_t)(-delta) : (uint32_t)delta;

  /* Kept only as evidence: the ISR's dead-encoder gate, and the number the homing-timeout message
   * prints to separate a disconnected encoder from a mechanism that simply never slowed. */
  if (moved > axis->peak_delta) {
    axis->peak_delta = moved;
  }

  /* Both totals advance, then the error is recomputed from scratch. Nothing is carried between checks
   * except the totals themselves, so there is no rounding to accumulate. */
  axis->commanded_steps += axis->config.check_interval_steps;
  axis->travel_counts += delta;

  uint32_t travelled = (axis->travel_counts < 0) ? (uint32_t)(-axis->travel_counts) : (uint32_t)axis->travel_counts;
  uint32_t owed      = axis->commanded_steps * axis->units_per_step;
  uint32_t delivered = travelled * axis->units_per_count;

  /* Floored at zero rather than allowed to go negative: an axis running ahead of its commanded
   * position is not in credit against a later stall, it is simply not behind. */
  axis->desync_units = (delivered >= owed) ? 0U : (owed - delivered);

  if (axis->desync_units > axis->desync_worst_units) {
    axis->desync_worst_units = axis->desync_units;
  }
}
