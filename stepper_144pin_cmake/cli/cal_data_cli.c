/**
 * @file cal_data_cli.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief CLI handlers for the "param" command group. See cal_data_cli.h for the command
 *        summary, for why a set persists to flash before the command returns (via the queued
 *        w25q save path), and for the outstanding SPI flash concurrency note that applies to it.
 * @version 0.1
 * @date 2026-07-27
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include "cal_data_cli.h"
#include "cal_data.h"
#include "app_console.h"
#include "app_task.h"
#include "cartridge.h"
#include "dispense_sm.h"
#include "patty_handler.h"
#include "stepper_system.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
/** Longest wait for a queued flash save to land before the CLI reports it as unfinished. A
 * sector erase is ~45 ms and the queue drains from the SPI ISR, so this only trips if the
 * driver has wedged or another module (e.g. a firmware upload) holds the queue. */
#define CAL_DATA_CLI_SAVE_TIMEOUT_MS (2000U)

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/** Sentinel used by action-only entries that have no cal_data_params_t field.
 *  The list handler iterates over these only; the pseudo-params beyond it are
 *  actions, not stored values. */
#define CAL_DATA_CLI_NO_VALUE_OFFSET ((size_t)-1)

typedef struct cal_data_cli_param_entry_s cal_data_cli_param_entry_t;
typedef void (*cal_data_cli_get_handler_t)(const cal_data_cli_param_entry_t* entry);
typedef void (*cal_data_cli_set_handler_t)(const cal_data_cli_param_entry_t* entry, int32_t value);

struct cal_data_cli_param_entry_s {
  const char* name;
  const char* usage;
  size_t value_offset;
  cal_data_cli_get_handler_t get_handler;
  cal_data_cli_set_handler_t set_handler;
};

static void stored_param_get_handler(const cal_data_cli_param_entry_t* entry);
static void stored_param_set_handler(const cal_data_cli_param_entry_t* entry, int32_t value);
static void thickness_get_handler(const cal_data_cli_param_entry_t* entry);
static void thickness_reset_handler(const cal_data_cli_param_entry_t* entry, int32_t value);

#define CAL_DATA_CLI_STORED_PARAM(param_name, field_name)                                                                  \
  {                                                                                                                        \
    .name = (param_name), .value_offset = offsetof(cal_data_params_t, field_name), .get_handler = stored_param_get_handler, \
    .set_handler = stored_param_set_handler                                                                                \
  }

/** Descriptor table accepted by "param get" and "param set".
 *  entries match the cal_data_params_t field names; the trailing entries are the
 *  pseudo-param action names. */
static const cal_data_cli_param_entry_t s_params[CAL_DATA_CLI_NUM_PARAMS] = {
    [CAL_DATA_CLI_PARAM_INVALID] = {.name = "invalid", .value_offset = CAL_DATA_CLI_NO_VALUE_OFFSET},
    [CAL_DATA_CLI_PUSHER_RPM] = CAL_DATA_CLI_STORED_PARAM("pusher_rpm", pusher_rpm),
    [CAL_DATA_CLI_LIFTER_RPM] = CAL_DATA_CLI_STORED_PARAM("lifter_rpm", lifter_rpm),
    [CAL_DATA_CLI_STALL_ERROR_COUNTS] = CAL_DATA_CLI_STORED_PARAM("stall_error_counts", stall_error_counts),
    [CAL_DATA_CLI_HOME_ERROR_COUNTS] = CAL_DATA_CLI_STORED_PARAM("home_error_counts", home_error_counts),
    [CAL_DATA_CLI_HOME_RPM] = CAL_DATA_CLI_STORED_PARAM("home_rpm", home_rpm),
    [CAL_DATA_CLI_HOME_BACKOFF_STEPS] = CAL_DATA_CLI_STORED_PARAM("home_backoff_steps", home_backoff_steps),
    [CAL_DATA_CLI_HOME_SETTLE_DELAY_MS] = CAL_DATA_CLI_STORED_PARAM("home_settle_delay_ms", home_settle_delay_ms),
    [CAL_DATA_CLI_HOME_MAX_STEPS] = CAL_DATA_CLI_STORED_PARAM("home_max_steps", home_max_steps),
    [CAL_DATA_CLI_SUPERVISOR_PERIOD_MS] = CAL_DATA_CLI_STORED_PARAM("supervisor_period_ms", supervisor_period_ms),
    [CAL_DATA_CLI_DEFAULT_MOVE_RPM] = CAL_DATA_CLI_STORED_PARAM("default_move_rpm", default_move_rpm),
    [CAL_DATA_CLI_DEFAULT_MOVE_STEPS] = CAL_DATA_CLI_STORED_PARAM("default_move_steps", default_move_steps),
    [CAL_DATA_CLI_LOAD_OFFSET] = CAL_DATA_CLI_STORED_PARAM("load_offset", load_offset),
    [CAL_DATA_CLI_PUSH_RETRACT_TIMEOUT_MS] = CAL_DATA_CLI_STORED_PARAM("push_retract_timeout_ms", push_retract_timeout_ms),
    [CAL_DATA_CLI_LIFT_TIMEOUT_MS] = CAL_DATA_CLI_STORED_PARAM("lift_timeout_ms", lift_timeout_ms),
    [CAL_DATA_CLI_PATTY_THICKNESS_COUNTS] = CAL_DATA_CLI_STORED_PARAM("patty_thickness_counts", patty_thickness_counts),
    [CAL_DATA_CLI_RECOUNT_TIMEOUT_MS] = CAL_DATA_CLI_STORED_PARAM("recount_timeout_ms", recount_timeout_ms),
    [CAL_DATA_CLI_PATTY2_THICKNESS_COUNTS] = CAL_DATA_CLI_STORED_PARAM("patty2_thickness_counts", patty2_thickness_counts),
    [CAL_DATA_CLI_AUTO_CLEAR_FAULTS] = CAL_DATA_CLI_STORED_PARAM("auto_clear_faults", auto_clear_faults),
    [CAL_DATA_CLI_AUTO_CLEAR_DELAY_MS] = CAL_DATA_CLI_STORED_PARAM("auto_clear_delay_ms", auto_clear_delay_ms),
    [CAL_DATA_CLI_TEMP_MAX_F] = CAL_DATA_CLI_STORED_PARAM("temp_max_f", temp_max_f),
    [CAL_DATA_CLI_THICKNESS_OFFSET_COUNTS] = CAL_DATA_CLI_STORED_PARAM("thickness_offset_counts", thickness_offset_counts),
    [CAL_DATA_CLI_USE_MEASURED_THICKNESS] = CAL_DATA_CLI_STORED_PARAM("use_measured_thickness", use_measured_thickness),
    [CAL_DATA_CLI_THICKNESS] = {.name = "thickness",
                                .usage = "param get thickness",
                                .value_offset = CAL_DATA_CLI_NO_VALUE_OFFSET,
                                .get_handler = thickness_get_handler},
    [CAL_DATA_CLI_RESET_THICKNESS] = {
        .name = "reset_thickness",
        .usage = "param set reset_thickness <slot>",
        .value_offset = CAL_DATA_CLI_NO_VALUE_OFFSET,
        .set_handler = thickness_reset_handler}};

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

static uint32_t* param_value_ptr(const cal_data_cli_param_entry_t* entry);
static const cal_data_cli_param_entry_t* lookup_param(const char* name);

/**
 * @brief queue a cal-data save and wait for it to land in flash.
 *
 *        cal_data_save() is asynchronous -- it only puts the erase and write on the w25q
 *        command queue. Poll cal_data_save_status() so "(saved)" is printed only once the
 *        data is actually on the chip. The console RX task yields while waiting.
 *
 * @return true if the save completed successfully, false if it could not be queued, ended in
 *         an error, or did not finish within CAL_DATA_CLI_SAVE_TIMEOUT_MS.
 */
static bool param_save_and_wait(void);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

void cal_data_cli_get_handler(char* param) {
  if (param != NULL) {
    const cal_data_cli_param_entry_t* entry = lookup_param(param);

    if ((entry == NULL) || (entry->get_handler == NULL)) {
      app_console_print("[PARAM] Unknown param: %s\r\n", param);
    }
    else {
      entry->get_handler(entry);
    }
  }
}

void cal_data_cli_set_handler(char* param, int32_t val) {
  if (param != NULL) {
    const cal_data_cli_param_entry_t* entry = lookup_param(param);

    if ((entry == NULL) || (entry->set_handler == NULL)) {
      app_console_print("[PARAM] Unknown param: %s\r\n", param);
    }
    else {
      entry->set_handler(entry, val);
    }
  }
}

void cal_data_cli_list_handler(void) {
  const char* source = (cal_data_is_valid() == true) ? "loaded from flash" : "defaults, unsaved";
  bool action_header_printed = false;

  app_console_print("Cal-data parameters (%s):\r\n", source);

  for (int32_t i = 1; i < (int32_t)CAL_DATA_CLI_NUM_PARAMS; i++) {
    const cal_data_cli_param_entry_t* entry = &s_params[i];

    if (entry->value_offset != CAL_DATA_CLI_NO_VALUE_OFFSET) {
      uint32_t* value = param_value_ptr(entry);

      if (value != NULL) {
        app_console_print("  %2ld %-21s %lu\r\n", (long)i, entry->name, (unsigned long)*value);
      }
      vTaskDelay(pdMS_TO_TICKS(10U)); /* Yield to the console task so it can flush the output. */
    }
    else if (entry->usage != NULL) {
      if (action_header_printed == false) {
        app_console_print("Actions (not cal-data parameters):\r\n");
        action_header_printed = true;
      }
      app_console_print("  %2ld %-21s use: %s\r\n", (long)i, entry->name, entry->usage);
      vTaskDelay(pdMS_TO_TICKS(10U));
    }
  }

  app_console_print("  a 'param set' is queued to flash and awaited before the command returns; 'param reset' for defaults\r\n");
}

void cal_data_cli_reset_handler(void) {
  cal_data_load_defaults();

  if (param_save_and_wait() == true) {
    app_console_print("[PARAM] Factory defaults loaded and saved.\r\n");
  }
  else if (cal_data_save_status() == CAL_DATA_SAVE_PENDING) {
    /* The save is queued and still running -- it may land after this message. Do not call
     * it a failure; say it has not finished. */
    app_console_print("[PARAM] Factory defaults loaded -- SAVE STILL PENDING, check 'param list' next boot\r\n");
  }
  else {
    app_console_print("[PARAM] Factory defaults loaded -- FLASH SAVE FAILED, RAM only\r\n");
  }

  /* Same as 'param set': the RAM copy changed, so the axes pick the new values up now
   * rather than keeping the previous ones until the next boot. Runs on the console RX
   * task, like the 'param set' path above -- the param commands are bench tools and
   * assume the machine is idle while they run. */
  stepper_system_update_configs();
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/

/**
 * @brief queue a cal-data save and wait for it to land in flash.
 * @return true if the save completed successfully, false if it could not be queued, ended in
 *         an error, or did not finish within CAL_DATA_CLI_SAVE_TIMEOUT_MS.
 */
/**
 * @brief Print one stored calibration parameter.
 * @param entry Parameter descriptor containing the name and field offset.
 */
static void stored_param_get_handler(const cal_data_cli_param_entry_t* entry) {
  uint32_t* value = param_value_ptr(entry);

  if ((entry != NULL) && (value != NULL)) {
    app_console_print("[PARAM] %s = %lu\r\n", entry->name, (unsigned long)*value);
  }
  else if (entry != NULL) {
    app_console_print("[PARAM] Unknown param: %s\r\n", entry->name);
  }
}

/**
 * @brief Update and persist one stored calibration parameter.
 * @param entry Parameter descriptor containing the name and field offset.
 * @param value New value supplied by the CLI.
 */
static void stored_param_set_handler(const cal_data_cli_param_entry_t* entry, int32_t value) {
  uint32_t* stored_value = param_value_ptr(entry);

  if ((entry != NULL) && (stored_value != NULL)) {
    /* No range or sign checking by design -- see the validation note in cal_data_cli.h.
     * A zero is repaired to its factory default by cal_data_sanitize_zeros() before the
     * block is staged, so a rejected zero is never persisted. */
    *stored_value = (uint32_t)value;
    cal_data_sanitize_zeros();

    /* Persisted here rather than on a separate command: a set the operator has to remember to
     * follow with a save is a set that silently reverts on the next power cycle. On failure the
     * RAM copy still holds the new value, so say so rather than implying nothing happened. */
    if (param_save_and_wait() == true) {
      app_console_print("[PARAM] %s = %lu (saved)\r\n", entry->name, (unsigned long)*stored_value);
      stepper_system_update_configs(); /* Apply the new values to the axes immediately. */
    }
    else if (cal_data_save_status() == CAL_DATA_SAVE_PENDING) {
      /* The save is queued and still running -- it may land after this message. Do not call
       * it a failure; say it has not finished. */
      app_console_print("[PARAM] %s = %lu -- SAVE STILL PENDING, check 'param list' next boot\r\n", entry->name,
                        (unsigned long)*stored_value);
    }
    else {
      app_console_print("[PARAM] %s = %lu -- FLASH SAVE FAILED, RAM only\r\n", entry->name, (unsigned long)*stored_value);
    }
  }
  else if (entry != NULL) {
    app_console_print("[PARAM] Unknown param: %s\r\n", entry->name);
  }
}

static bool param_save_and_wait(void) {
  bool saved = false;

  if (cal_data_save() == true) {
    uint32_t waited_ms = 0U;

    while (cal_data_save_status() == CAL_DATA_SAVE_PENDING) {
      if (waited_ms >= CAL_DATA_CLI_SAVE_TIMEOUT_MS) {
        break;
      }
      vTaskDelay(pdMS_TO_TICKS(10U));
      waited_ms += 10U;
    }

    saved = (cal_data_save_status() == CAL_DATA_SAVE_IDLE);
  }

  return saved;
}

/**
 * @brief Resolve a parameter id to the field it names inside the cal-data RAM copy.
 *
 *        This is the only place that maps an id to a field, so get/set/list all stay a few
 *        lines each. It hands back a pointer rather than assigning directly so that one switch
 *        serves all three handlers instead of each carrying its own copy of the list.
 *
 * @param param Parameter to resolve. CAL_DATA_CLI_PARAM_INVALID and any value outside the
 *              enum fall through to the default case and yield NULL.
 * @return Pointer to the live field in cal_data_params_t, or NULL if param names nothing.
 */
static uint32_t* param_value_ptr(const cal_data_cli_param_entry_t* entry) {
  uint32_t* value = NULL;

  if ((entry != NULL) && (entry->value_offset != CAL_DATA_CLI_NO_VALUE_OFFSET)) {
    cal_data_params_t* params = cal_data_get();

    if (params != NULL) {
      value = (uint32_t*)((uint8_t*)params + entry->value_offset);
    }
  }

  return value;
}

/**
 * @brief Look up a parameter by name, or by numeric id if the token is entirely digits.
 * @param name Token from the CLI. NULL yields CAL_DATA_CLI_PARAM_INVALID.
 * @return Matching enum value, or CAL_DATA_CLI_PARAM_INVALID if the token matches nothing.
 */
static const cal_data_cli_param_entry_t* lookup_param(const char* name) {
  const cal_data_cli_param_entry_t* found = NULL;

  if (name != NULL) {
    char* end_ptr = NULL;
    uint32_t id = (uint32_t)strtoul(name, &end_ptr, 10);

    if ((end_ptr != name) && (*end_ptr == '\0')) {
      /* Token parsed as a complete decimal number, so treat it as an id rather than a name. */
      if ((id > (uint32_t)CAL_DATA_CLI_PARAM_INVALID) && (id < (uint32_t)CAL_DATA_CLI_NUM_PARAMS)) {
        found = &s_params[id];
      }
    }
    else {
      for (int32_t i = 1; i < (int32_t)CAL_DATA_CLI_NUM_PARAMS; i++) {
        if (strcmp(s_params[i].name, name) == 0) {
          found = &s_params[i];
          break;
        }
      }
    }
  }

  return found;
}

/**
 * @brief "param get thickness" -- print every slot's thickness measurement state.
 *
 *        Reads the live cartridge array through app_task_get_cartridges() and
 *        the raw captured travel through each slot's dispense state machine.
 *        Per slot: travel = last raw lift-seek travel captured, last = most
 *        recent sample actually stored in the ring (added at the last commit,
 *        under the offset in effect then), now = the raw travel recomputed
 *        under the CURRENT cal-data offset (previews what `param set
 *        thickness_offset_counts` would produce before the next dispense),
 *        n = samples in the ring, avg = rolling average, plus the inventory
 *        counts. The values are a snapshot of Control-task state read from the
 *        console task; a mid-dispense read can straddle a commit.
 */
static void thickness_get_handler(const cal_data_cli_param_entry_t* entry) {
  const cartridge_t* cartridges = app_task_get_cartridges();

  (void)entry;

  app_console_print("[THICKNESS] offset=%lu use_measured=%lu\r\n", (unsigned long)cal_data_get()->thickness_offset_counts,
                    (unsigned long)cal_data_get()->use_measured_thickness);

  for (uint8_t slot_index = 0U; slot_index < (uint8_t)PATTY_HANDLER_SLOT_COUNT; slot_index++) {
    const cartridge_t* cartridge = &cartridges[slot_index];
    const dispense_sm_t* dispense_sm = app_task_get_dispense_sm(slot_index);

    if (dispense_sm == NULL) {
      app_console_print("[THICKNESS] Slot %d: dispense context unavailable\r\n", (int)(slot_index + 1U));
      continue;
    }

    const uint32_t travel = dispense_sm_get_measured_lift_travel_counts(dispense_sm);
    const uint32_t now = dispense_sm_last_patty_thickness_counts(dispense_sm);

    // most recent sample actually stored in the ring; 0 when no sample has been added yet
    uint32_t last = 0U;
    if (cartridge->thickness_sample_count > 0U) {
      const uint8_t newest =
          (uint8_t)((cartridge->thickness_sample_next + CARTRIDGE_THICKNESS_RING_SIZE - 1U) % CARTRIDGE_THICKNESS_RING_SIZE);
      last = cartridge->thickness_samples[newest];
    }

    app_console_print("[THICKNESS] Slot %d: travel=%lu last=%lu now=%lu n=%u/%u avg=%lu rem=%u pend=%u\r\n", (int)(slot_index + 1U),
                      (unsigned long)travel, (unsigned long)last, (unsigned long)now, (unsigned int)cartridge->thickness_sample_count,
                      (unsigned int)CARTRIDGE_THICKNESS_RING_SIZE, (unsigned long)cartridge->thickness_avg_counts,
                      (unsigned int)cartridge->remaining, (unsigned int)cartridge->pending);
  }
}

/**
 * @brief "param set reset_thickness <slot>" -- clear one slot's thickness ring and average.
 *
 *        Refuses only when a dispense is observed active at check time; the
 *        check and the clear run on the console task and are not atomic
 *        against the Control task, so a dispense starting right after the
 *        check can still add a sample to the freshly cleared ring.
 *
 * @param value 1-based slot number carried in the set command's value field.
 */
static void thickness_reset_handler(const cal_data_cli_param_entry_t* entry, int32_t value) {
  (void)entry;

  if ((value < 1) || (value > (int32_t)PATTY_HANDLER_SLOT_COUNT)) {
    app_console_print("[THICKNESS] Slot must be 1..%d\r\n", (int32_t)PATTY_HANDLER_SLOT_COUNT);
    return;
  }

  const uint8_t slot_index = (uint8_t)(value - 1);

  if (app_task_reset_thickness(slot_index) == true) {
    app_console_print("[THICKNESS] Slot %d: thickness history cleared\r\n", value);
  }
  else {
    /* The refusal only means a dispense was observed active at check time; a
     * dispense starting right after the check can still add a sample. */
    app_console_print("[THICKNESS] Slot %d: reset refused (dispense active?)\r\n", value);
  }
}
