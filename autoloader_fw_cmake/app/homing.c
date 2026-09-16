/**
 * @file homing.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief Shared homing-phase event handling for the application state machines.
 * @version 0.1
 * @date 2026-08-29
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include "homing.h"

#include <stdbool.h>
#include <stddef.h>

#include "app_console.h"

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

bool homing_all_homed(const cartridge_t cartridges[APP_SLOT_COUNT], homing_target_enum target) {
  bool all_homed = false;

  if (cartridges != NULL) {
    all_homed = true;
    for (uint8_t i = 0U; (i < APP_SLOT_COUNT) && (all_homed == true); i++) {
      switch (target) {
        case HOMING_TARGET_PUSHER:
          all_homed = cartridges[i].pusher_homed;
          break;
        case HOMING_TARGET_LIFTER_DOWN:
          all_homed = cartridges[i].lifter_homed_down;
          break;
        case HOMING_TARGET_LIFTER_UP:
          all_homed = cartridges[i].lifter_homed_up;
          break;
        default:
          all_homed = false;
          break;
      }
    }
  }

  return all_homed;
}

void homing_mark_complete(cartridge_t* cartridge, homing_target_enum target) {
  if (cartridge != NULL) {
    switch (target) {
      case HOMING_TARGET_PUSHER:
        cartridge->pusher_homed = true;
        break;
      case HOMING_TARGET_LIFTER_DOWN:
        cartridge->lifter_homed_down = true;
        break;
      case HOMING_TARGET_LIFTER_UP:
        cartridge->lifter_homed_up = true;
        break;
      default:
        break;
    }
  }
}

homing_event_result_enum homing_handle_event(cartridge_t cartridges[APP_SLOT_COUNT], const app_event_t* event,
                                             uint32_t* fault_code_out, const homing_phase_cfg_t* cfg) {
  if ((cartridges == NULL) || (event == NULL) || (fault_code_out == NULL) || (cfg == NULL)) {
    return HOMING_EVENT_FAILED;
  }

  if (event->id == APP_EV_MOTION_FAILED) {
    app_console_print("[Homing] %s homing failed: axis_num=%d\r\n", cfg->name, event->axis_num);
    *fault_code_out = cfg->fault_code;
    return HOMING_EVENT_FAILED;
  }

  if ((event->id == APP_EV_TIMEOUT) && (event->value == APP_SM_TIMEOUT_MOTION)) {
    app_console_print("%s", cfg->timeout_log);
    *fault_code_out = cfg->fault_code;
    return HOMING_EVENT_FAILED;
  }

  if (event->id == APP_EV_MOTION_DONE) {
    // Mark the axis as homed for this phase.
    uint8_t cartridge_idx = cartridge_get_slot_from_axis_num(event->axis_num) - 1;
    homing_mark_complete(&cartridges[cartridge_idx], cfg->target);

    // Check if all cartridges are homed.
    if (homing_all_homed(cartridges, cfg->target) == true) {
      return HOMING_EVENT_COMPLETE;
    }

    return HOMING_EVENT_WAITING;
  }

  // Unexpected event — fail
  app_console_print("[Homing] Unexpected event in homing: id=%d, value=%d\r\n", event->id, event->value);
  *fault_code_out = cfg->fault_code;
  return HOMING_EVENT_FAILED;
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/
