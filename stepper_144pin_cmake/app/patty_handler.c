/**
 * @file patty_handler.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief Validates, allocates, and routes incoming patty dispense requests.
 * @version 0.1
 * @date 2026-08-08
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights
 * Reserved.
 *
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include "patty_handler.h"

#include <stddef.h>

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

#define PATTY_HANDLER_LTO_FIRST_SLOT (2U)
#define PATTY_HANDLER_FAULT_INVENTORY (100U)

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

static bool patty_handler_port_is_valid(const patty_handler_port_t* port);
static bool patty_handler_is_ready(const patty_handler_t* handler);
static bool patty_handler_slot_is_lto_paused(const patty_handler_t* handler, uint8_t slot_index);
static bool patty_handler_slot_matches(const patty_handler_t* handler, uint8_t slot_index, uint8_t product_type);
static uint32_t patty_handler_get_total_available(const patty_handler_t* handler, uint8_t product_type);
static uint8_t patty_handler_select_allocation_slot(const patty_handler_t* handler, uint8_t product_type);
static bool patty_handler_slot_can_start(const patty_handler_t* handler, uint8_t slot_index);
static void patty_handler_publish_slot(const patty_handler_t* handler, uint8_t slot_index);
static patty_handler_result_enum patty_handler_commit_dispense(patty_handler_t* handler, uint8_t slot_index);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

patty_handler_result_enum patty_handler_init(patty_handler_t* handler, cartridge_t* cartridge, const patty_handler_port_t* port) {
  patty_handler_result_enum result = PATTY_HANDLER_RESULT_INVALID_ARGUMENT;
  uint8_t slot_index;

  if ((handler != NULL) && (cartridge != NULL) && (patty_handler_port_is_valid(port) == true)) {
    handler->cartridge = cartridge;
    handler->port = *port;
    handler->door_closed = false;
    handler->door_locked = false;
    handler->dispensing_enabled = false;
    handler->lto_pause_active = false;

    for (slot_index = 0U; slot_index < PATTY_HANDLER_SLOT_COUNT; slot_index++) {
      cart_dispense_sm_init(&handler->dispense_sm[slot_index], slot_index);
    }

    result = PATTY_HANDLER_RESULT_OK;
  }

  return result;
}

void patty_handler_set_safety_state(patty_handler_t* handler, bool door_closed, bool door_locked, bool dispensing_enabled) {
  if (handler != NULL) {
    handler->door_closed = door_closed;
    handler->door_locked = door_locked;
    handler->dispensing_enabled = dispensing_enabled;
  }
}

void patty_handler_set_lto_pause(patty_handler_t* handler, bool active) {
  if (handler != NULL) {
    handler->lto_pause_active = active;
  }
}

patty_handler_result_enum patty_handler_add_request(patty_handler_t* handler, uint8_t product_type, uint16_t requested_count,
                                                    patty_request_source_enum source) {
  patty_handler_result_enum result = PATTY_HANDLER_RESULT_INVALID_ARGUMENT;
  uint32_t total_available;
  uint16_t allocation_count;
  uint16_t allocated_per_slot[PATTY_HANDLER_SLOT_COUNT] = {0U};
  uint8_t selected_slot;
  uint8_t slot_index;

  if ((handler != NULL) && (handler->cartridge != NULL) && ((source == PATTY_REQUEST_SOURCE_QUEUE) || (source == PATTY_REQUEST_SOURCE_MANUAL))) {
    if (requested_count == 0U) {
      result = PATTY_HANDLER_RESULT_NO_ACTION;
    }
    else if (patty_handler_is_ready(handler) == false) {
      result = PATTY_HANDLER_RESULT_NOT_READY;
    }
    else {
      total_available = patty_handler_get_total_available(handler, product_type);

      if ((uint32_t)requested_count > total_available) {
        result = PATTY_HANDLER_RESULT_NOT_ENOUGH_PRODUCT;
      }
      else {
        result = PATTY_HANDLER_RESULT_OK;

        /*
         * The complete request was validated before this loop. The Control task
         * must be the only writer of pending so this allocation remains atomic.
         */
        for (allocation_count = 0U; allocation_count < requested_count; allocation_count++) {
          selected_slot = patty_handler_select_allocation_slot(handler, product_type);

          if (selected_slot == PATTY_HANDLER_NO_SLOT) {
            /* This indicates that another context modified cartridge counts
             * unexpectedly. */
            result = PATTY_HANDLER_RESULT_NOT_ENOUGH_PRODUCT;
            break;
          }

          handler->cartridge[selected_slot].pending++;
          allocated_per_slot[selected_slot]++;
        }

        if (result == PATTY_HANDLER_RESULT_OK) {
          for (slot_index = 0U; slot_index < PATTY_HANDLER_SLOT_COUNT; slot_index++) {
            patty_handler_publish_slot(handler, slot_index);
          }
        }
        else {
          for (slot_index = 0U; slot_index < PATTY_HANDLER_SLOT_COUNT; slot_index++) {
            handler->cartridge[slot_index].pending -= allocated_per_slot[slot_index];
          }
        }
      }
    }
  }

  return result;
}

patty_handler_result_enum patty_handler_process(patty_handler_t* handler) {
  patty_handler_result_enum result = PATTY_HANDLER_RESULT_INVALID_ARGUMENT;
  patty_handler_result_enum start_result;
  bool cycle_started = false;
  bool motion_rejected = false;
  uint8_t slot_index;

  if ((handler != NULL) && (handler->cartridge != NULL)) {
    result = PATTY_HANDLER_RESULT_NO_ACTION;

    if (patty_handler_is_ready(handler) == true) {
      for (slot_index = 0U; slot_index < PATTY_HANDLER_SLOT_COUNT; slot_index++) {
        if (handler->cartridge[slot_index].pending > handler->cartridge[slot_index].remaining) {
          handler->cartridge[slot_index].faulted = true;
          handler->dispense_sm[slot_index].state = CART_DISPENSE_FAILED;
          handler->dispense_sm[slot_index].fault_code = PATTY_HANDLER_FAULT_INVENTORY;
          patty_handler_publish_slot(handler, slot_index);
        }
        else if (patty_handler_slot_can_start(handler, slot_index) == true) {
          start_result = cart_dispense_sm_start(&handler->dispense_sm[slot_index], &handler->port);

          if (start_result == PATTY_HANDLER_RESULT_OK) {
            cycle_started = true;
          }
          else {
            handler->cartridge[slot_index].faulted = true;
            motion_rejected = true;
          }

          patty_handler_publish_slot(handler, slot_index);
        }
        else {
          /* This slot_index is already running, empty, faulted, paused, or has no
           * pending work. */
        }
      }

      if (motion_rejected == true) {
        result = PATTY_HANDLER_RESULT_MOTION_REJECTED;
      }
      else if (cycle_started == true) {
        result = PATTY_HANDLER_RESULT_OK;
      }
      else {
        result = PATTY_HANDLER_RESULT_NO_ACTION;
      }
    }
    else {
      result = PATTY_HANDLER_RESULT_NOT_READY;
    }
  }

  return result;
}

patty_handler_result_enum patty_handler_dispatch_event(patty_handler_t* handler, const cart_dispense_event_t* event) {
  patty_handler_result_enum result = PATTY_HANDLER_RESULT_INVALID_ARGUMENT;
  patty_handler_result_enum process_result;
  uint8_t slot_index;

  if ((handler != NULL) && (handler->cartridge != NULL) && (event != NULL) && (event->slot_index < PATTY_HANDLER_SLOT_COUNT)) {
    slot_index = event->slot_index;
    result = dispense_sm_dispatch(&handler->dispense_sm[slot_index], event, &handler->port);

    if (handler->dispense_sm[slot_index].state == CART_DISPENSE_COMPLETE) {
      result = patty_handler_commit_dispense(handler, slot_index);
    }
    else if (handler->dispense_sm[slot_index].state == CART_DISPENSE_FAILED) {
      handler->cartridge[slot_index].faulted = true;
      patty_handler_publish_slot(handler, slot_index);
      result = PATTY_HANDLER_RESULT_MOTION_REJECTED;
    }
    else {
      patty_handler_publish_slot(handler, slot_index);
    }

    /* A failed slot_index does not prevent another slot_index from starting or continuing.
     */
    process_result = patty_handler_process(handler);
    if ((result == PATTY_HANDLER_RESULT_OK) && (process_result == PATTY_HANDLER_RESULT_MOTION_REJECTED)) {
      result = process_result;
    }
  }

  return result;
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/

/**
 * @brief Checks that every callback required by the starter implementation is
 * installed.
 *
 * @param port Application callback table.
 * @return True when the callback table is valid.
 */
static bool patty_handler_port_is_valid(const patty_handler_port_t* port) {
  bool is_valid = false;

  if (port != NULL) {
    is_valid = ((port->push_extend != NULL) && (port->push_retract != NULL) && (port->lift_seek != NULL) && (port->lift_backoff != NULL) &&
                (port->halt_motion != NULL) && (port->arm_timeout != NULL) && (port->cancel_timeout != NULL) && (port->publish_status != NULL));
  }

  return is_valid;
}

/**
 * @brief Checks the parent safety conditions required to start or admit
 * dispensing.
 *
 * @param handler Handler instance.
 * @return True when new dispensing is allowed.
 */
static bool patty_handler_is_ready(const patty_handler_t* handler) {
  return ((handler->door_closed == true) && (handler->door_locked == true) && (handler->dispensing_enabled == true));
}

/**
 * @brief Checks whether the LTO control pauses the selected zero-based slot_index.
 *
 * @param handler Handler instance.
 * @param slot_index cartrigdes are numbered 1-4, but this indexes into an array that starts at 0
 * @return True when the slot_index is paused.
 */
static bool patty_handler_slot_is_lto_paused(const patty_handler_t* handler, uint8_t slot_index) {
  return ((handler->lto_pause_active == true) && (slot_index >= PATTY_HANDLER_LTO_FIRST_SLOT));
}

/**
 * @brief Checks whether a cartridge can reserve the requested product type.
 *
 * @param handler Handler instance.
 * @param slot_index cartrigdes are numbered 1-4, but this indexes into an array that starts at 0
 * @param product_type Requested application product-type value.
 * @return True when the cartridge matches and has unreserved product.
 */
static bool patty_handler_slot_matches(const patty_handler_t* handler, uint8_t slot_index, uint8_t product_type) {
  const cartridge_t* cartridge = &handler->cartridge[slot_index];
  bool type_matches;

  /* TODO: Replace this cast with the application's cartridge_type_t when
   * integrating. */
  type_matches = ((uint32_t)cartridge->type == (uint32_t)product_type);

  return ((type_matches == true) && (cartridge->faulted == false) && (patty_handler_slot_is_lto_paused(handler, slot_index) == false) &&
          (cartridge->remaining > cartridge->pending));
}

/**
 * @brief Totals unreserved product in all matching eligible cartridges.
 *
 * @param handler Handler instance.
 * @param product_type Requested application product-type value.
 * @return Number of patties that can still be reserved.
 */
static uint32_t patty_handler_get_total_available(const patty_handler_t* handler, uint8_t product_type) {
  uint32_t total_available = 0U;
  uint8_t slot_index;

  for (slot_index = 0U; slot_index < PATTY_HANDLER_SLOT_COUNT; slot_index++) {
    if (patty_handler_slot_matches(handler, slot_index, product_type) == true) {
      total_available += (uint32_t)(handler->cartridge[slot_index].remaining - handler->cartridge[slot_index].pending);
    }
  }

  return total_available;
}

/**
 * @brief Selects one slot_index using the customer queue-allocation preference.
 *
 * Lowest slot_index number resolves otherwise equal candidates. If every candidate
 * queue is empty, the cartridge with the largest remaining count is selected.
 *
 * @param handler Handler instance.
 * @param product_type Requested application product-type value.
 * @return Selected zero-based slot_index or PATTY_HANDLER_NO_SLOT.
 */
static uint8_t patty_handler_select_allocation_slot(const patty_handler_t* handler, uint8_t product_type) {
  uint8_t selected_slot = PATTY_HANDLER_NO_SLOT;
  uint8_t slot_index;
  uint16_t smallest_pending = UINT16_MAX;
  uint16_t largest_remaining = 0U;
  bool all_pending_zero = true;

  for (slot_index = 0U; slot_index < PATTY_HANDLER_SLOT_COUNT; slot_index++) {
    if ((patty_handler_slot_matches(handler, slot_index, product_type) == true) && (handler->cartridge[slot_index].pending != 0U)) {
      all_pending_zero = false;
    }
  }

  for (slot_index = 0U; slot_index < PATTY_HANDLER_SLOT_COUNT; slot_index++) {
    if (patty_handler_slot_matches(handler, slot_index, product_type) == true) {
      if (all_pending_zero == true) {
        if ((selected_slot == PATTY_HANDLER_NO_SLOT) || (handler->cartridge[slot_index].remaining > largest_remaining)) {
          selected_slot = slot_index;
          largest_remaining = handler->cartridge[slot_index].remaining;
        }
      }
      else if ((selected_slot == PATTY_HANDLER_NO_SLOT) || (handler->cartridge[slot_index].pending < smallest_pending)) {
        selected_slot = slot_index;
        smallest_pending = handler->cartridge[slot_index].pending;
      }
      else {
        /* Equal candidates keep the lower slot_index number selected. */
      }
    }
  }

  return selected_slot;
}

/**
 * @brief Checks whether one idle cartridge can begin its next reserved cycle.
 *
 * @param handler Handler instance.
 * @param slot_index cartrigdes are numbered 1-4, but this indexes into an array that starts at 0
 * @return True when the cartridge may start.
 */
static bool patty_handler_slot_can_start(const patty_handler_t* handler, uint8_t slot_index) {
  const cartridge_t* cartridge = &handler->cartridge[slot_index];
  const cart_dispense_sm_t* dispense_sm = &handler->dispense_sm[slot_index];

  return ((dispense_sm->state == CART_DISPENSE_IDLE) && (cartridge->faulted == false) && (cartridge->pending > 0U) && (cartridge->remaining > 0U) &&
          (patty_handler_slot_is_lto_paused(handler, slot_index) == false));
}

/**
 * @brief Publishes one cartridge's cached count and dispense state.
 *
 * @param handler Handler instance.
 * @param slot_index cartrigdes are numbered 1-4, but this indexes into an array that starts at 0
 */
static void patty_handler_publish_slot(const patty_handler_t* handler, uint8_t slot_index) {
  handler->port.publish_status(slot_index, &handler->cartridge[slot_index], &handler->dispense_sm[slot_index]);
}

/**
 * @brief Commits one completed physical dispense and releases the cartridge for
 * more work.
 *
 * @param handler Handler instance.
 * @param slot_index cartrigdes are numbered 1-4, but this indexes into an array that starts at 0
 * @return PATTY_HANDLER_RESULT_OK when counts are committed or a motion error
 * on inconsistency.
 */
static patty_handler_result_enum patty_handler_commit_dispense(patty_handler_t* handler, uint8_t slot_index) {
  patty_handler_result_enum result = PATTY_HANDLER_RESULT_MOTION_REJECTED;
  cartridge_t* cartridge = &handler->cartridge[slot_index];
  cart_dispense_sm_t* dispense_sm = &handler->dispense_sm[slot_index];

  if ((cartridge->remaining > 0U) && (cartridge->pending > 0U)) {
    cartridge->remaining--;
    cartridge->pending--;

    /*
     * TODO: Feed dispense_sm->measured_lift_travel_counts into the
     * application's patty-thickness average and remaining-stack recalculation.
     * TODO: Save counts to nonvolatile storage if the final persistence policy
     * requires it.
     */
    dispense_sm->state = CART_DISPENSE_IDLE;
    dispense_sm->fault_code = 0U;
    dispense_sm->product_may_have_dispensed = false;
    patty_handler_publish_slot(handler, slot_index);
    result = PATTY_HANDLER_RESULT_OK;
  }
  else {
    cartridge->faulted = true;
    dispense_sm->state = CART_DISPENSE_FAILED;
    dispense_sm->fault_code = PATTY_HANDLER_FAULT_INVENTORY;
    patty_handler_publish_slot(handler, slot_index);
  }

  return result;
}
