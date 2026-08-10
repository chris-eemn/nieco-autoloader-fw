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

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
/** Layout version of cal_data_params_t. Bump this whenever a field is added, removed, moved or
 * changes meaning -- a stored record whose version does not match is rejected and the factory
 * defaults are loaded instead, rather than being reinterpreted under the new layout. */
#define CAL_DATA_VERSION (6U)

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
  uint32_t max_sync_error_counts; /**< Maximum encoder following error. Maps to
                                   *   axis_config_t.max_sync_error_counts. */
  uint32_t home_rpm;              /**< Speed of the homing seek and back-off moves, RPM. */
  uint32_t home_settle_delay_ms;  /**< Time to wait after hitting the endstop before starting the back-off move. */
  uint32_t home_backoff_steps;    /**< Back-off distance in microsteps after the endstop is hit. */
  uint32_t home_max_steps;        /**< Homing seek limit in microsteps before a timeout fault. */
  uint32_t supervisor_period_ms;  /**< Axis supervisor tick period. */
  uint32_t default_move_rpm;      /**< Speed used for a move when none is specified. */
  uint32_t default_move_steps;    /**< Microsteps used for a move when none is specified. */
  uint32_t load_offset;           /**< Microstep backup after lifters hit the top during startup homing. */
  uint32_t push_retract_timeout_ms; /**< Push retract timeout, milliseconds. */
  uint32_t lift_timeout_ms;       /**< Lift seek timeout, milliseconds. */
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
 * @brief writes the RAM copy of the parameters to flash
 * @note erases and rewrites the general section only; stepper position sections are untouched
 * @return bool true if the record was written and verified
 */
bool cal_data_save(void);

/**
 * @brief overwrites the RAM copy with the factory defaults, without touching flash
 * @note follow with cal_data_save() to make the reset permanent
 */
void cal_data_load_defaults(void);

/**
 * @brief reports whether the record currently in RAM came from flash or from the defaults
 * @return bool true if cal_data_init() found a valid stored record
 */
bool cal_data_is_valid(void);

#endif /* CAL_DATA_H_ */
