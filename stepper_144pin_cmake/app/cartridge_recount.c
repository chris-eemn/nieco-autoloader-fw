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
void cartridge_recount_start(cartridge_t* slot) {
  if (slot != NULL) {
    app_sm_port_home_lift(slot, DIR_LIFTER_UP);
  }
}

bool cartridge_recount_apply(cartridge_t* slot, uint32_t* fault_out) {
  if ((slot == NULL) || (fault_out == NULL)) {
    if (fault_out != NULL) {
      *fault_out = APP_FAULT_CODE_INVALID_STATE;
    }
    return false;
  }

  uint32_t thickness = cal_data_get()->patty_thickness_counts;

  if (thickness == 0U) {
    app_console_print("[Recount] Slot %d: patty thickness unconfigured, cannot recount\r\n", slot->num);
    *fault_out = APP_FAULT_CODE_CAL_MISSING;
    return false;
  }

  int32_t travel = axis_get_home_travel_counts(stepper_ctrl_get_axis(cartridge_get_axis_num(slot, LIFTER)));
  if (travel < 0) {
    travel = -travel;
  }

  slot->remaining = (uint16_t)((uint32_t)travel / thickness);

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
