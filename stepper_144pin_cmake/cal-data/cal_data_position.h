/**
 * @file cal_data_position.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief Non-volatile storage of stepper positions, one W25Q section per stepper pair.
 *
 *        Kept separate from the general parameters in cal_data.h because positions are the
 *        one thing here that gets rewritten during normal operation. Each pair owns its own
 *        4 KiB sector, so saving a position erases and rewrites nothing but that pair's data --
 *        no other cal value is exposed to the erase window.
 *
 *        Position is stored as the encoder count, not commanded microsteps. The encoder is the
 *        only position reference the axis actually trusts: commanded steps drift away from
 *        reality on any stall or missed step, which is precisely the case where a stored
 *        position most needs to be right.
 *
 *        Saves are asynchronous. cal_data_position_save() stages the payload and hands a
 *        sector-erase plus a write to the w25q command queue, then returns; the driver runs
 *        both from its SPI interrupt, in order, without the calling task waiting on the ~45 ms
 *        sector erase. Poll cal_data_position_save_status() to find out when the data is
 *        actually on the chip. Loads are synchronous -- the caller needs the value to continue.
 *
 *        Integrity is a straight two-copy compare rather than a magic number or checksum: the
 *        payload is stored twice, back to back, and a load only succeeds when both copies are
 *        identical and the section is not blank. A save that is cut short by a power loss
 *        leaves the two copies different, which reads back as "no valid position" -- the axis
 *        must then be re-homed, which is the correct response to an unknown position anyway.
 *
 * @note Queued writes are NOT read back and verified, unlike the general section, which goes
 *       through spi_flash_io_write()'s verify-and-retry. Verification requires waiting for the
 *       write, which is the cost this path exists to avoid. A write that silently fails shows
 *       up as a failed two-copy compare on the next load, i.e. as a re-home.
 *
 * @note A save erases before it writes, so the previously stored position does not survive a
 *       power loss during the save even though it was valid beforehand. Accepted: the outcome
 *       is a re-home, same as any other unknown-position case.
 *
 * @note Not thread safe -- see the note in cal_data.h. The w25q command queue itself is pushed
 *       from task context and popped from the SPI ISR, so only one task may drive this module.
 *
 * @version 0.2
 * @date 2026-07-27
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef CAL_DATA_POSITION_H_
#define CAL_DATA_POSITION_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "cal_data_map.h"
#include "cal_data_save.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/
/**
 * Stored position state for the two motors of one stepper pair.
 *
 * Deliberately header-free -- the two-copy compare in cal_data_position_load() is the whole
 * validity check, so there is no magic number or version field to keep in step. That also
 * means the struct layout must not change without a plan for records already on the chip.
 */
typedef struct {
  int32_t encoder_count[CAL_DATA_MOTORS_PER_PAIR]; /**< Signed encoder count per motor, as
                                                    *   returned by axis_get_encoder_count(),
                                                    *   relative to the last encoder_zero(). */
  uint8_t homed[CAL_DATA_MOTORS_PER_PAIR];         /**< Non-zero if that motor's encoder was
                                                    *   zeroed by a completed home, which is
                                                    *   what makes the count above meaningful
                                                    *   as an absolute position. */
  uint8_t reserved[2];                             /**< Zeroed on save; keeps the struct a
                                                    *   multiple of 4 bytes and guarantees a
                                                    *   saved record is never all-0xFF. */
} cal_data_position_t;

/* The save-progress status enum (cal_data_save_status_enum) lives in cal_data_save.h, shared
 * with the general-section save path in cal_data.c. */

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 *Function Prototypes
 *******************************************************************************/
/**
 * @brief stages one pair's position and queues the erase and write that store it
 * @note returns as soon as the commands are queued -- the data is not on the chip yet. The
 *       payload is copied into a per-pair staging buffer that the driver reads from during the
 *       write, so position does not need to outlive the call. Poll
 *       cal_data_position_save_status() for completion
 * @note fails while a previous save for the same pair is still in flight (the staging buffer is
 *       in use), when the queue cannot hold both commands, or when the driver is already in an
 *       error state. The erase is never queued unless the write can be queued behind it, so a
 *       rejected save leaves the stored position intact
 * @param pair_index 0-based stepper pair, must be < CAL_DATA_PAIR_COUNT
 * @param position position state to store; must not be NULL. The reserved bytes are zeroed
 *        regardless of what the caller passed
 * @return bool true if both commands were accepted into the w25q queue
 */
bool cal_data_position_save(uint8_t pair_index, const cal_data_position_t* position);

/**
 * @brief reports how far the most recent save for a pair has got
 * @note the w25q driver exposes no per-command completion, so this resolves to IDLE once the
 *       command queue has drained and the driver is no longer busy -- which also means any
 *       other module's queued traffic delays the transition to IDLE
 * @note CAL_DATA_SAVE_ERROR is sticky for the pair until its next successful save is queued.
 *       The driver has no reset entry point; its error state clears when the next direct
 *       (non-queued) w25q operation runs, such as a general-section cal_data_save()
 * @param pair_index 0-based stepper pair; an out-of-range index reports CAL_DATA_SAVE_IDLE
 * @return cal_data_save_status_enum progress of the last save issued for that pair
 */
cal_data_save_status_enum cal_data_position_save_status(uint8_t pair_index);

/**
 * @brief reads one pair's position back from its flash section
 * @note blocking, unlike the save path. Fails when the section has never been written, when the
 *       two stored copies disagree (a save that did not complete), when a save for any pair is
 *       still in flight, or when the read itself fails. In every one of those cases the position
 *       is unknown and the axis needs re-homing; position is left untouched
 * @param pair_index 0-based stepper pair, must be < CAL_DATA_PAIR_COUNT
 * @param position destination for the stored position; must not be NULL
 * @return bool true if a valid position was recovered
 */
bool cal_data_position_load(uint8_t pair_index, cal_data_position_t* position);

/**
 * @brief queues an erase of one pair's position section, discarding any stored position
 * @note asynchronous, like cal_data_position_save(), and tracked by the same per-pair status.
 *       Once it completes, cal_data_position_load() for that pair reports no valid position
 * @param pair_index 0-based stepper pair, must be < CAL_DATA_PAIR_COUNT
 * @return bool true if the erase was accepted into the w25q queue
 */
bool cal_data_position_invalidate(uint8_t pair_index);

#endif /* CAL_DATA_POSITION_H_ */
