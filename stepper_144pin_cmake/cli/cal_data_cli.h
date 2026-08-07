/**
 * @file cal_data_cli.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief CLI handlers for the "param" command group, backed by the real cal-data store.
 *        Exposes get/set/list function pointers consumed by app_console_commands, plus
 *        save and reset actions.
 *
 *        This replaces cal_data_example.c, which held its own private table of placeholder
 *        parameters unrelated to anything the application actually uses. Every parameter here
 *        is a field of cal_data_params_t, so "param set" changes the same RAM copy that
 *        cal_data_save() writes to flash.
 *
 *          param list                    -- all parameters, with ids and current values
 *          param get  <name|id>          -- print one value
 *          param set  <name|id> <value>  -- update the value and write it to flash
 *          param reset                   -- load factory defaults and write them to flash
 *
 * @note There is deliberately no "param save". A set is persisted before the command returns,
 *       so an operator cannot leave a change that quietly reverts on the next power cycle.
 *       The cost of that choice, which matters if anything ever drives this command in bulk:
 *
 *         - Wear: every set costs one 4 KiB sector erase, the W25Q erase granularity, on the
 *           cal-data general section. Fine for occasional operator tuning; a script sweeping a
 *           parameter in a loop would be spending erase cycles on a single sector.
 *         - Latency: spi_flash_io_write() erases, writes, reads back and verifies, retrying up
 *           to 3 times, all blocking. The console RX task is stalled for that whole time, so
 *           console input is not serviced while a set is landing.
 *         - Divergence on failure: the RAM copy is updated before the write is attempted, so a
 *           failed write leaves RAM ahead of flash. The handler reports this rather than
 *           claiming success, but it does not roll the RAM value back.
 *
 * @note VALIDATION -- deliberately absent. "param set" performs no range or sign checking; the
 *       value is stored exactly as given. Two consequences worth knowing at the console:
 *
 *         - Every field is uint32_t and the CLI parses into an int32_t, so a negative value
 *           wraps: "param set pusher_rpm -1" reads back as 4294967295.
 *         - Zero is accepted everywhere, including the rpm fields. stepper_rpm_to_ticks(0)
 *           returns UINT32_MAX rather than dividing by zero, so that is a motor that never
 *           steps rather than a crash.
 *
 *       Neither can misbehave today because nothing consumes these values yet. The reason the
 *       checks are not here rather than merely absent: the CLI is one of several transports
 *       (Modbus is next), and per-transport limits would have to be written once per transport
 *       and would drift apart. Validation belongs where the value is consumed -- at the
 *       application apply step, or inside axis_move()/stepper_move_start() -- which covers
 *       every transport at once.
 *
 * @note Nothing consumes these values yet. axis_config_t in main.c and the rpm/steps held in
 *       stepper_ctrl.c are still the hard-coded originals, and the cal-data defaults
 *       deliberately mirror them. Making a "param set" actually change machine behaviour needs
 *       an application-level apply step that reads cal-data and pushes it into those modules --
 *       deliberately kept out of this change.
 *
 * @note THREAD SAFETY / SPI FLASH CONCURRENCY -- KNOWN ISSUE, NOT ADDRESSED HERE.
 *
 *       "param set" and "param reset" drive spi_flash_io_erase_range() and spi_flash_io_write()
 *       from the console RX task. spi_flash_io has no locking of its own (see its header) and
 *       sits on top of the single shared, interrupt-driven driver_w25q state machine that is
 *       polled via w25q_get_transfer_status(). Three separate tasks can now reach it:
 *
 *         - the console RX task, on every "param set" and "param reset"
 *         - the stepper task, via cal_data_init() at boot and cal_data_position_* later
 *         - the USB loader task, while staging a firmware image
 *
 *       Note that persisting inline on every set widens this window rather than narrowing it:
 *       the console task now touches flash on every parameter change, not only when an operator
 *       explicitly asked to save.
 *
 *       Two of those overlapping is real corruption of the driver's transfer state, not just a
 *       policy violation. The addresses do not overlap (staging area vs cal-data region), so an
 *       interleave damages the in-flight transfer rather than the wrong region -- but that is
 *       luck, not design.
 *
 *       A second, related gap: cal_data_save() is an erase call followed by a write call, with
 *       an unprotected window between them. Any multi-call sequence has the same shape,
 *       including the read-modify-write in cal_data_position.c.
 *
 *       Options considered, for whoever picks this up:
 *         1. A recursive FreeRTOS mutex inside spi_flash_io (configUSE_RECURSIVE_MUTEXES is
 *            already 1 in FreeRTOSConfig.h), taken per call, plus a public
 *            spi_flash_io_lock(timeout_ms)/unlock() pair so a caller can hold exclusivity
 *            across several calls. Recursion lets the outer session and the inner per-call
 *            takes compose from the same task. A firmware upload would hold the session for
 *            the whole transfer, so "param set" needs a short timeout and a "flash busy"
 *            message rather than blocking the console task for minutes.
 *         2. Defer the write: "param set" sets a flag and the stepper task performs the write,
 *            so all flash access stays on one task. The save becomes asynchronous, so the CLI
 *            cannot report the result inline.
 *         3. Leave as is for bench bring-up -- the collision needs a firmware upload and a
 *            "param set" to be in flight simultaneously.
 *
 *       Note that option 1 touches spi_flash_io.c, which its header describes as kept in
 *       parity with the bootloader's copy of the same module; the bootloader has no FreeRTOS,
 *       so the locking would need a compile-time guard to keep that file portable.
 *
 *       Separately: a firmware upload should probably also inhibit machine motion outright,
 *       which is an operating-mode concern rather than a flash-locking one and wants its own
 *       NORMAL / FIRMWARE_UPDATE state that motion commands consult.
 *
 * @version 0.1
 * @date 2026-07-27
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef CAL_DATA_CLI_H_
#define CAL_DATA_CLI_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include <stdint.h>

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/**
 * One entry per field of cal_data_params_t, in the same order as the struct.
 *
 * Numbering starts at 1 so that 0 stays available as the "not found" result, matching the
 * numeric ids the param command already accepted. These ids are printed by "param list" and
 * accepted by "param get"/"param set" in place of a name.
 */
typedef enum {
  CAL_DATA_CLI_PARAM_INVALID = 0,
  CAL_DATA_CLI_PUSHER_RPM,
  CAL_DATA_CLI_LIFTER_RPM,
  CAL_DATA_CLI_MAX_SYNC_ERROR_COUNTS,
  CAL_DATA_CLI_HOME_RPM,
  CAL_DATA_CLI_HOME_BACKOFF_STEPS,
  CAL_DATA_CLI_HOME_MAX_STEPS,
  CAL_DATA_CLI_SUPERVISOR_PERIOD_MS,
  CAL_DATA_CLI_DEFAULT_MOVE_RPM,
  CAL_DATA_CLI_DEFAULT_MOVE_STEPS,
  CAL_DATA_CLI_NUM_PARAMS,
} cal_data_cli_param_enum;

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 *Function Prototypes
 *******************************************************************************/

/**
 * @brief CLI get handler -- prints the current value of one cal-data parameter.
 * @param param Parameter name string, or its numeric id as a decimal string.
 */
void cal_data_cli_get_handler(char* param);

/**
 * @brief CLI set handler -- writes one cal-data parameter and persists it to SPI flash.
 * @note the value is NOT range checked; see the validation note in this file's header
 * @note the flash write happens inline, before this returns -- see the flash write cost and
 *       concurrency notes in this file's header
 * @param param Parameter name string, or its numeric id as a decimal string.
 * @param val   New value, stored as-is. See the validation note in this file's header for what
 *              a negative value does.
 */
void cal_data_cli_set_handler(char* param, int32_t val);

/**
 * @brief CLI list handler -- prints every parameter with its id and current value.
 */
void cal_data_cli_list_handler(void);

/**
 * @brief CLI reset handler -- loads the factory defaults and persists them to SPI flash.
 */
void cal_data_cli_reset_handler(void);

#endif /* CAL_DATA_CLI_H_ */
