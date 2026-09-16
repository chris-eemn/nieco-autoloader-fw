/**
 * @file cartridge_recount.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief Shared recount and lane-pair type validation for cartridge slots.
 * @version 0.1
 * @date 2026-08-29
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include "cartridge_recount.h"
#include "app_console.h"
#include "app_sm_port.h"
#include "cal_data.h"
#include "stepper_ctrl.h"
#include "axis.h"
#include <stddef.h>
#include <stdbool.h>

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/
bool cartridge_recount_start(cartridge_t* slot) {
  if (slot == NULL) {
    return false;
  }

  axis_t* lifter_axis = stepper_ctrl_get_axis(cartridge_get_axis_num(slot, LIFTER));

  if (lifter_axis == NULL) {
    app_console_print("[Recount] Slot %d: lifter axis unavailable\r\n", slot->num);
    return false;
  }

  /* A latched stall is recoverable here; a DRV fault needs axis_fault_reset(),
   * which only the APP_FAULT path (clear_all_axis_faults) performs. */
  if (axis_get_status(lifter_axis) == AXIS_STATUS_STALLED) {
    axis_clear_fault(lifter_axis);
  }

  stepper_status_enum status = app_sm_port_home_lift(slot, DIR_LIFTER_UP);
  if (status != STEPPER_OK) {
    app_console_print("[Recount] Slot %d: lifter home rejected (%d)\r\n", slot->num, (int)status);
    return false;
  }

  return true;
}

bool cartridge_recount_apply(cartridge_t* slot, uint32_t* fault_out) {
  if ((slot == NULL) || (fault_out == NULL)) {
    if (fault_out != NULL) {
      *fault_out = APP_FAULT_CODE_INVALID_STATE;
    }
    return false;
  }

  axis_t* lifter_axis = stepper_ctrl_get_axis(cartridge_get_axis_num(slot, LIFTER));

  if (lifter_axis == NULL) {
    app_console_print("[Recount] Slot %d: lifter axis unavailable\r\n", slot->num);
    *fault_out = APP_FAULT_CODE_INVALID_STATE;
    return false;
  }

  axis_status_enum axis_status = axis_get_status(lifter_axis);
  if ((axis_status == AXIS_STATUS_FAULT) || (axis_status == AXIS_STATUS_INVALID)) {
    app_console_print("[Recount] Slot %d: lifter axis faulted\r\n", slot->num);
    *fault_out = APP_FAULT_CODE_RELOAD_FAILED;
    return false;
  }

  uint32_t thickness = cal_data_get()->patty_thickness_counts;

  if (thickness == 0U) {
    app_console_print("[Recount] Slot %d: patty thickness unconfigured, cannot recount\r\n", slot->num);
    *fault_out = APP_FAULT_CODE_CAL_MISSING;
    return false;
  }

  int32_t travel = axis_get_home_travel_counts(lifter_axis);
  if (travel < 0) {
    travel = -travel;
  }

  uint32_t remaining = (uint32_t)travel / thickness;

  /* In-flight dispenses are tracked in pending; remaining must never drop below it. */
  if (remaining < (uint32_t)slot->pending) {
    app_console_print("[Recount] Slot %d: remaining %lu below pending %u, clamped\r\n", slot->num, (unsigned long)remaining, (unsigned)slot->pending);
    remaining = (uint32_t)slot->pending;
  }

  if (remaining > (uint32_t)UINT16_MAX) {
    app_console_print("[Recount] Slot %d: remaining %lu exceeds uint16, clamped\r\n", slot->num, (unsigned long)remaining);
    remaining = (uint32_t)UINT16_MAX;
  }

  slot->remaining = (uint16_t)remaining;

  // remaining was just reinitialized from scratch: drop any stale thickness history
  cartridge_reset_thickness(slot);

  app_console_print("[Recount] Slot %d: travel=%ld counts, thickness=%lu, remaining=%u\r\n", slot->num, (long)travel,
                    (unsigned long)thickness, (unsigned)slot->remaining);

  return true;
}

bool cartridge_validate_lane_pairs(cartridge_t cartridges[APP_SLOT_COUNT]) {
  bool mismatched = false;

  if (cartridges != NULL) {
    for (uint8_t base = 0U; (base + 1U) < APP_SLOT_COUNT; base += 2U) {
      cartridge_t* a = &cartridges[base];
      cartridge_t* b = &cartridges[base + 1U];

      if ((a->type == CARTRIDGE_TYPE_EMPTY) || (b->type == CARTRIDGE_TYPE_EMPTY)) {
        continue;
      }

      if (a->type != b->type) {
        a->faulted = true;
        b->faulted = true;
        app_console_print("[Recount] Lane mismatch: slot %d (%s) vs slot %d (%s) — both inhibited\r\n", a->num,
                          cartridge_type_to_string(a->type), b->num, cartridge_type_to_string(b->type));
        mismatched = true;
      }
    }
  }

  return mismatched;
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/
