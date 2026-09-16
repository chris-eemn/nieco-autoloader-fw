/**
 * @file cal_data_position.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief Per-stepper-pair position storage. See cal_data_position.h for the API contract and
 *        the rationale behind the two-copy integrity scheme, and cal_data_map.h for where in
 *        the W25Q these sections live.
 * @version 0.2
 * @date 2026-07-27
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include <stddef.h>
#include <string.h>
#include "cal_data_position.h"
#include "cal_data_map.h"
#include "spi_flash_io.h"
#include "w25q.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
/** Number of identical copies of the payload stored in each pair's section. */
#define CAL_DATA_POSITION_COPY_COUNT (2U)

/** Total bytes written to a pair's section by one save. */
#define CAL_DATA_POSITION_STORED_SIZE (CAL_DATA_POSITION_COPY_COUNT * sizeof(cal_data_position_t))

/** W25Q page program size. Keeping both copies inside one page means the queued write is a
 * single page program, which is what makes it the fast path it needs to be. */
#define CAL_DATA_POSITION_PAGE_SIZE (256U)

/** Commands one save puts on the w25q queue: one sector erase, then one write. Both have to fit
 * or neither is queued -- a queued erase with no write behind it would wipe the stored position
 * and put nothing back. */
#define CAL_DATA_POSITION_QUEUE_SLOTS_PER_SAVE (2)

/** Usable depth of the w25q command queue. w25q_push_command() rejects once the queue holds
 * W25Q_COMMAND_QUEUE_SIZE - 1 entries, so that -- not the array size -- is the real capacity. */
#define CAL_DATA_POSITION_QUEUE_CAPACITY ((int32_t)W25Q_COMMAND_QUEUE_SIZE - 1)

_Static_assert(CAL_DATA_POSITION_STORED_SIZE <= CAL_DATA_POSITION_PAGE_SIZE, "both position copies no longer fit in one W25Q page program");
_Static_assert(CAL_DATA_POSITION_STORED_SIZE <= CAL_DATA_POSITION_SECTION_SIZE,
               "both position copies no longer fit in the sector reserved per stepper pair");
_Static_assert(CAL_DATA_POSITION_SECTOR_COUNT == 1U, "a queued save issues exactly one W25Q_SECTOR_ERASE, so a section must be one sector");
_Static_assert(CAL_DATA_POSITION_QUEUE_CAPACITY >= CAL_DATA_POSITION_QUEUE_SLOTS_PER_SAVE,
               "the w25q command queue cannot hold a single save's erase and write");

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/
/** Staging buffers holding both copies of each pair's payload. The w25q driver keeps the
 * caller's pointer rather than copying, and runs the write later from the SPI ISR, so the
 * buffer backing a queued write has to outlive the cal_data_position_save() call. One buffer
 * per pair keeps pairs independent -- a save for pair 1 does not have to wait on pair 0. */
static uint8_t position_stage[CAL_DATA_PAIR_COUNT][CAL_DATA_POSITION_STORED_SIZE];

/** Whether a save issued for that pair has yet to complete. While set, the pair's staging
 * buffer is owned by the driver and must not be rewritten. */
static bool save_in_flight[CAL_DATA_PAIR_COUNT];

/** Set when the driver reported an error while one of our saves was outstanding. Sticky per
 * pair until that pair's next save is queued. */
static bool save_errored[CAL_DATA_PAIR_COUNT];

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/
/**
 * @brief reports whether the w25q driver has finished everything queued on it
 * @note the driver has no per-command completion hook, so a drained queue plus a non-busy
 *       driver is the only completion signal available. The queue empties when the last command
 *       is popped, which happens before that command finishes, hence both checks
 * @return bool true if nothing is queued and no transfer is in progress
 */
static bool flash_is_idle(void);

/**
 * @brief clears the in-flight flags of every pair once the driver has gone idle, recording an
 *        error against any pair that was still outstanding when the driver faulted
 * @note called at the top of every public function so that status is re-evaluated lazily
 *       rather than needing a periodic tick from the application
 */
static void refresh_save_state(void);

/**
 * @brief reports whether every byte of a buffer reads back as the post-erase value
 * @note this is how a never-written section is told apart from a stored record; without it two
 *       blank copies would compare equal and pass as valid. cal_data_position_save() zeroes the
 *       reserved bytes so that a genuinely saved record can never look erased
 * @param data buffer to inspect; must not be NULL
 * @param length number of bytes to inspect
 * @return bool true if all bytes are CAL_DATA_ERASED_BYTE
 */
static bool buffer_is_erased(const uint8_t* data, size_t length);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/
bool cal_data_position_save(uint8_t pair_index, const cal_data_position_t* position) {
  cal_data_position_t record;
  w25q_command_t erase_command;
  w25q_command_t write_command;
  uint32_t section_address;
  uint8_t copy;
  bool queued = false;

  refresh_save_state();

  if ((pair_index < CAL_DATA_PAIR_COUNT) && (position != NULL) && (save_in_flight[pair_index] == false) &&
      (w25q_get_transfer_status() != W25Q_TRANSFER_STATUS_ERROR) &&
      ((w25q_commands_in_queue() + CAL_DATA_POSITION_QUEUE_SLOTS_PER_SAVE) <= CAL_DATA_POSITION_QUEUE_CAPACITY)) {
    section_address = CAL_DATA_POSITION_PAIR_ADDRESS(pair_index);

    record = *position;
    memset(record.reserved, 0, sizeof(record.reserved));

    for (copy = 0U; copy < CAL_DATA_POSITION_COPY_COUNT; copy++) {
      memcpy(&position_stage[pair_index][(size_t)copy * sizeof(record)], &record, sizeof(record));
    }

    erase_command.command = W25Q_SECTOR_ERASE;
    erase_command.address = section_address;
    erase_command.length = 0;
    erase_command.buffer = NULL;

    write_command.command = W25Q_WRITE;
    write_command.address = section_address;
    write_command.length = (int)CAL_DATA_POSITION_STORED_SIZE;
    write_command.buffer = &position_stage[pair_index][0];

    /* Capacity was checked above, so the erase is only ever queued when the write can follow
     * it. The queue is FIFO and w25q_finish_command() chains the next command from the SPI
     * ISR, so the erase is guaranteed to complete before the write starts. */
    if (w25q_queue_command(&erase_command) == true) {
      queued = w25q_queue_command(&write_command);
    }

    if (queued == true) {
      save_in_flight[pair_index] = true;
      save_errored[pair_index] = false;
    }
  }

  return queued;
}

cal_data_save_status_enum cal_data_position_save_status(uint8_t pair_index) {
  cal_data_save_status_enum status = CAL_DATA_SAVE_IDLE;

  refresh_save_state();

  if (pair_index < CAL_DATA_PAIR_COUNT) {
    if (save_errored[pair_index] == true) {
      status = CAL_DATA_SAVE_ERROR;
    }
    else if (save_in_flight[pair_index] == true) {
      status = CAL_DATA_SAVE_PENDING;
    }
    else {
      status = CAL_DATA_SAVE_IDLE;
    }
  }

  return status;
}

bool cal_data_position_load(uint8_t pair_index, cal_data_position_t* position) {
  uint8_t stored[CAL_DATA_POSITION_STORED_SIZE];
  uint32_t section_address;
  bool loaded = false;

  refresh_save_state();

  /* The read is blocking and goes straight at the driver rather than through the queue, so it
   * would be rejected outright while a queued save is still running. Reporting "no valid
   * position" in that window would be wrong -- the position is merely not readable yet -- so
   * the busy case is refused here instead. */
  if ((pair_index < CAL_DATA_PAIR_COUNT) && (position != NULL) && (flash_is_idle() == true)) {
    section_address = CAL_DATA_POSITION_PAIR_ADDRESS(pair_index);

    if (spi_flash_io_read(section_address, stored, (uint32_t)CAL_DATA_POSITION_STORED_SIZE) == true) {
      bool copies_match = (memcmp(&stored[0], &stored[sizeof(cal_data_position_t)], sizeof(cal_data_position_t)) == 0);
      bool section_blank = buffer_is_erased(&stored[0], sizeof(cal_data_position_t));

      loaded = (copies_match == true) && (section_blank == false);
    }

    if (loaded == true) {
      memcpy(position, &stored[0], sizeof(cal_data_position_t));
    }
  }

  return loaded;
}

bool cal_data_position_invalidate(uint8_t pair_index) {
  w25q_command_t erase_command;
  bool queued = false;

  refresh_save_state();

  if ((pair_index < CAL_DATA_PAIR_COUNT) && (save_in_flight[pair_index] == false) && (w25q_get_transfer_status() != W25Q_TRANSFER_STATUS_ERROR)) {
    erase_command.command = W25Q_SECTOR_ERASE;
    erase_command.address = CAL_DATA_POSITION_PAIR_ADDRESS(pair_index);
    erase_command.length = 0;
    erase_command.buffer = NULL;

    queued = w25q_queue_command(&erase_command);

    if (queued == true) {
      save_in_flight[pair_index] = true;
      save_errored[pair_index] = false;
    }
  }

  return queued;
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/
static bool flash_is_idle(void) {
  return (w25q_commands_in_queue() == 0) && (w25q_get_transfer_status() != W25Q_TRANSFER_STATUS_BUSY);
}

static void refresh_save_state(void) {
  bool driver_errored = (w25q_get_transfer_status() == W25Q_TRANSFER_STATUS_ERROR);
  uint8_t pair;

  /* An error is terminal even with commands still queued. w25q_process_command() ignores the
   * return of the call it makes, so a command that fails to start leaves the driver in ERROR
   * with whatever was behind it stranded in the queue -- and the queue is only ever advanced
   * from the completion ISR, which will not run. Waiting for a drain that cannot happen would
   * pin the pair at PENDING forever, so the error is taken as the end of the save.
   *
   * Clearing the in-flight flag hands the pair's staging buffer back for reuse while a stranded
   * write may still reference it: if some later direct w25q operation completes, the ISR pops
   * that write and programs whatever the buffer holds by then, into a sector that was not
   * erased for it. The result is a mismatched two-copy compare on the next load, i.e. a
   * re-home -- the same outcome as any other failed save, and preferable to a pair that can
   * never save again. */
  if ((driver_errored == true) || (flash_is_idle() == true)) {
    for (pair = 0U; pair < CAL_DATA_PAIR_COUNT; pair++) {
      if (save_in_flight[pair] == true) {
        save_in_flight[pair] = false;
        save_errored[pair] = driver_errored;
      }
    }
  }
}

static bool buffer_is_erased(const uint8_t* data, size_t length) {
  bool all_erased = true;
  size_t index;

  if (data == NULL) {
    all_erased = false;
  }
  else {
    for (index = 0U; (index < length) && (all_erased == true); index++) {
      if (data[index] != (uint8_t)CAL_DATA_ERASED_BYTE) {
        all_erased = false;
      }
    }
  }

  return all_erased;
}
