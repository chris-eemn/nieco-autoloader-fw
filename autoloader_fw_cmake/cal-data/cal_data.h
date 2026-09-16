/**
 * @file cal_data.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief Non-volatile storage of general application parameters in the W25Q cal-data region.
 *
 *        Despite the name this is not restricted to calibration values -- it is the general
 *        store for anything the application needs to survive a power cycle, minus stepper
 *        positions, which live in their own per-pair sections (see cal_data_position.h).
 *
 *        The module keeps a RAM copy of the parameters. cal_data_init() loads it from flash at
 *        boot; callers read and modify it in place through cal_data_get() and persist it with
 *        cal_data_save(). Nothing is written to flash until cal_data_save() is called.
 *
 *        Typical call sequence:
 *          cal_data_init();
 *          cal_data_params_t *params = cal_data_get();
 *          params->pusher_rpm = 40U;
 *          cal_data_save();
 *
 * @note Not thread safe. All cal_data_* calls must come from a single task, or be serialised
 *       by the caller -- they share the SPI flash with the USB firmware-update loader.
 *
 * @version 0.1
 * @date 2026-07-27
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef CAL_DATA_H_
#define CAL_DATA_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "cal_data_save.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
/** Layout version of cal_data_params_t. Bump this whenever a field is added, removed, moved or
 * changes meaning -- a stored record whose version does not match is rejected and the factory
 * defaults are loaded instead, rather than being reinterpreted under the new layout. */
#define CAL_DATA_VERSION (14U)

/** Number of uint32_t fields in cal_data_params_t. Kept in sync with the struct by the
 *  _Static_assert in cal_data.c. */
#define CAL_DATA_PARAM_COUNT (26U)

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/
/**
 * General application parameters held in non-volatile storage.
 *
 * Placeholder set for now -- the application is not fully defined yet. Fields are all uint32_t
 * so the struct layout stays trivially predictable across compilers and so that adding a field
 * never introduces padding.
 */
typedef struct {
  uint32_t pusher_rpm;            /**< Pusher motor speed, RPM. */
  uint32_t lifter_rpm;            /**< Lifter motor speed, RPM. */
  uint32_t stall_error_counts;    /**< Encoder following-error threshold for stall detection when NOT homing. Maps to axis_config_t.stall_error_counts. */
  uint32_t home_error_counts;     /**< Encoder following-error threshold used during homing (endstop detection). Maps to axis_config_t.home_error_counts. Both in encoder ticks. */
  uint32_t home_rpm;              /**< Speed of the homing seek and back-off moves, RPM. */
  uint32_t home_settle_delay_ms;  /**< Time to wait after hitting the endstop before starting the back-off move. */
  uint32_t home_max_steps;        /**< Homing seek limit in microsteps before a timeout fault. */
  uint32_t supervisor_period_ms;  /**< Axis supervisor tick period. */
  uint32_t default_move_rpm;      /**< Speed used for a move when none is specified. */
  uint32_t default_move_steps;    /**< Microsteps used for a move when none is specified. */
  /** Back-off distance in microsteps after the endstop is hit. Wired into every axis as
   * axis_config_t.backoff_steps (stepper_system.c); the top-level name for this value is
   * "load offset" (Modbus REG_LOAD_OFFSET, CLI param "load_offset"). */
  uint32_t load_offset;
  uint32_t push_retract_timeout_ms; /**< Push retract timeout, milliseconds. */
  uint32_t lift_timeout_ms;       /**< Lift seek timeout, milliseconds. */
  uint32_t patty_thickness_counts;  /**< Encoder counts per patty, for recount. 0 = unconfigured. */
  uint32_t recount_timeout_ms;      /**< Timeout for the recount lift-to-stall move, milliseconds. */
  /** Encoder counts per Patty 2 patty; reserved for per-product thickness. Recount still uses
   * patty_thickness_counts for all slots. 0 = unconfigured. */
  uint32_t patty2_thickness_counts;
  uint32_t lock_timeout_ms; /** Lock-wait timeout, milliseconds. */
  uint32_t motion_timeout_ms; /** Motion/homing timeout, milliseconds. */
  uint32_t door_timeout_ms; /** Door-interaction timeout, milliseconds */
  uint32_t startup_settle_delay_ms; /** Post-home settle delay during startup, milliseconds. */
  uint32_t door_debounce_ms; /** Door/lock/reload input debounce, milliseconds. */
  uint32_t auto_clear_faults;   /** 1 = auto-clear faults after auto_clear_delay_ms, 0 = off. */
  uint32_t auto_clear_delay_ms; /** Auto-clear delay in milliseconds. Default 5000. */
  /** Over-temperature fault threshold, whole degrees F. A channel reading
   * above this value faults the machine. 0 is repaired to the default by sanitize_zeros. */
  uint32_t temp_max_f;
  uint32_t thickness_offset_counts; /** Encoder counts subtracted from the raw lift-seek home travel before it becomes a */
                                    /*  thickness sample. Default 0 (no offset).*/
  uint32_t use_measured_thickness;  /** 1 = measured thickness drives remaining on dispense commit, 0 = off (decrement-only). */
} cal_data_params_t;

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 *Function Prototypes
 *******************************************************************************/
/**
 * @brief loads the general parameters from flash into the RAM copy
 * @note if the stored record is missing, was written by a different layout version, or the
 *       read fails, the factory defaults are loaded into the RAM copy and written back to
 *       flash -- so a blank chip is self-provisioning on first boot
 * @note the underlying w25q driver must already be initialised (main() calls w25q_initialize()
 *       before the scheduler starts); this function does not initialise it
 * @return bool true if a valid record was loaded from flash, false if the defaults were used
 */
bool cal_data_init(void);

/**
 * @brief returns the RAM copy of the parameters, for reading or in-place modification
 * @note the returned pointer is to static storage and is never NULL. Changes made through it
 *       are not persisted until cal_data_save() is called
 * @return cal_data_params_t* pointer to the live parameter block
 */
cal_data_params_t* cal_data_get(void);

/**
 * @brief stages the RAM copy and queues the erase and write that store it
 * @note returns as soon as the commands are queued -- the data is not on the chip yet. The
 *       payload is copied into a module-owned staging buffer that the driver reads from during
 *       the write, so the RAM copy does not need to outlive the call. Poll
 *       cal_data_save_status() for completion
 * @note erases and rewrites the general section only; stepper position sections are untouched
 * @note fails while a previous save is still in flight (the staging buffer is in use), when the
 *       w25q queue cannot hold all five commands, or when the driver is already in an error
 *       state. The erases are never queued unless the write can be queued behind them, so a
 *       rejected save leaves the stored record intact
 * @return bool true if all commands were accepted into the w25q queue
 */
bool cal_data_save(void);

/**
 * @brief reports how far the most recent cal_data_save() has got
 * @note the w25q driver exposes no per-command completion, so this resolves to IDLE once the
 *       command queue has drained and the driver is no longer busy -- which also means any
 *       other module's queued traffic delays the transition to IDLE
 * @note CAL_DATA_SAVE_ERROR is sticky until the next save is queued. The driver has no reset
 *       entry point; its error state clears when the next direct (non-queued) w25q operation
 *       runs, such as the boot-path save in cal_data_init()
 * @return cal_data_save_status_enum progress of the last save issued
 */
cal_data_save_status_enum cal_data_save_status(void);

/**
 * @brief overwrites the RAM copy with the factory defaults, without touching flash
 * @note follow with cal_data_save() to make the reset permanent
 */
void cal_data_load_defaults(void);

/**
 * @brief replaces every zero-valued field of the RAM copy with its factory default
 * @note every cal_data parameter must be non-zero to be usable -- a zero timeout arms a
 *       timer that fires immediately, a zero rpm or step count makes a move fail or complete
 *       instantly. The runtime consumers read the parameters verbatim (the defaults here are
 *       the single source of truth), so a zero must never survive into the live copy.
 *       cal_data_init() calls this after loading a stored record, which is checked only for
 *       magic and version -- a record written by older firmware, or after a field was added,
 *       can hold zeros. Callers that accept external writes (CLI, Modbus) apply this after
 *       each write so the rejected zero is not persisted.
 * @note RAM only; follow with cal_data_save() to persist the repaired values
 */
void cal_data_sanitize_zeros(void);

/**
 * @brief reports whether the record currently in RAM came from flash or from the defaults
 * @return bool true if cal_data_init() found a valid stored record
 */
bool cal_data_is_valid(void);

#endif /* CAL_DATA_H_ */
