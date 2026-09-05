/**
 * @file cal_data.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief General non-volatile parameter storage. See cal_data.h for the API contract and
 *        cal_data_map.h for where in the W25Q this section lives.
 * @version 0.1
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
#include "cal_data.h"
#include "cal_data_map.h"
#include "spi_flash_io.h"
#include "w25q.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
/** Marks a general-section record as having been written by this module. "CALD" in ASCII. */
#define CAL_DATA_MAGIC (0x43414C44UL)

/** Longest buffer spi_flash_io_write() accepts in one call -- see spi_flash_io.h. The boot-path
 * write is a single call, so the record has to fit; the static assert below enforces that. */
#define CAL_DATA_MAX_WRITE_LEN (512U)

/** Commands one queued save puts on the w25q queue: one erase per general-section sector, then
 * one write. All have to fit or none is queued -- queued erases with no write behind them would
 * wipe the stored record and put nothing back. */
#define CAL_DATA_QUEUE_SLOTS_PER_SAVE ((int32_t)CAL_DATA_GENERAL_SECTOR_COUNT + 1)

/** Usable depth of the w25q command queue. w25q_push_command() rejects once the queue holds
 * W25Q_COMMAND_QUEUE_SIZE - 1 entries, so that -- not the array size -- is the real capacity. */
#define CAL_DATA_QUEUE_CAPACITY ((int32_t)W25Q_COMMAND_QUEUE_SIZE - 1)

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/
/**
 * Fixed prefix stored ahead of the parameters on flash.
 *
 * Validation is magic + version, matching the reference caldata module this was modelled on.
 * That catches a blank/erased section and a record written by an older firmware layout, which
 * are the two cases that actually occur in practice. It does not catch a write that was
 * interrupted partway through -- the magic is written first, so a truncated record still leads
 * with a valid-looking magic. Adding a payload checksum is the fix for that if it ever bites;
 * it is a new header field plus a CAL_DATA_VERSION bump.
 */
typedef struct {
  uint32_t magic;
  uint32_t version;
} cal_data_header_t;

/** Exactly what gets written to and read back from the general section. */
typedef struct {
  cal_data_header_t header;
  cal_data_params_t params;
} cal_data_record_t;

_Static_assert(sizeof(cal_data_record_t) <= CAL_DATA_MAX_WRITE_LEN, "cal-data general record no longer fits in a single spi_flash_io_write() call");
_Static_assert(sizeof(cal_data_record_t) <= CAL_DATA_GENERAL_SIZE, "cal-data general record no longer fits in the sectors reserved for it");
_Static_assert(CAL_DATA_ALLOCATED_SIZE <= W25Q_CAL_DATA_SIZE, "cal-data sections overflow the cal-data region of the W25Q");
_Static_assert(CAL_DATA_QUEUE_CAPACITY >= CAL_DATA_QUEUE_SLOTS_PER_SAVE,
               "the w25q command queue cannot hold a single save's erases and write");

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/
/** Factory defaults, applied when flash holds nothing usable. Mirrors the values currently
 * configured in stepper_system.c and stepper_ctrl.h so that provisioning a blank chip
 * reproduces today's behaviour rather than changing it. */
static const cal_data_params_t default_params = {
    .pusher_rpm = 30U,
    .lifter_rpm = 30U,
    .stall_error_counts = 10U,
    .home_error_counts = 50U,
    .home_rpm = 10U,
    .home_backoff_steps = 100U,
    .home_settle_delay_ms = 100U,
    .home_max_steps = 2000U,
    .supervisor_period_ms = 25U,
    .default_move_rpm = 5U,
    .default_move_steps = 800U,
    .load_offset = 200U,
    .push_retract_timeout_ms = 3000U,
    .lift_timeout_ms = 5000U,
    .patty_thickness_counts = 50U,
    .recount_timeout_ms = 120000U,
    .patty2_thickness_counts = 50U, /* unconfigured; same as patty_thickness_counts until per-product selection lands */
    .lock_timeout_ms = 30000U, /* unified startup/reload lock wait (Phase 5 decision) */
    .motion_timeout_ms = 10000U,
    .door_timeout_ms = 65535,
    .startup_settle_delay_ms = 250U,
    .door_debounce_ms = 50U,
};

_Static_assert(sizeof(cal_data_params_t) == (CAL_DATA_PARAM_COUNT * sizeof(uint32_t)),
               "cal_data_params_t is no longer a flat array of uint32_t fields -- cal_data_sanitize_zeros()'s field walk is invalid");

/** Live RAM copy handed out by cal_data_get(). */
static cal_data_params_t cal_params;

/** Staging buffer holding the record of the last queued save. The w25q driver keeps the
 * caller's pointer rather than copying, and runs the write later from the SPI ISR, so the
 * buffer backing a queued write has to outlive the cal_data_save() call. While a save is in
 * flight this buffer is owned by the driver and must not be rewritten. */
static cal_data_record_t save_stage;

/** Whether a queued save has yet to complete. While set, save_stage is owned by the driver. */
static bool s_save_in_flight = false;

/** Set when the driver reported an error while the queued save was outstanding. Sticky until
 * the next save is queued. */
static bool s_save_errored = false;

/** Whether a record this firmware can use is on flash: either cal_data_init() read one back
 * with a matching magic + version, or the boot-path blocking save wrote one and verified it.
 * The queued save path cannot verify, so it does not set this -- cal_data_is_valid() reports
 * "a usable record is on flash", which is what the CLI's loaded-from-flash vs defaults-unsaved
 * line asks. A queued save that lands later does not flip this until the next boot reads it
 * back. Sticky once set: a later failed save cannot prove the previously stored record is
 * gone (the driver gives no read-back on the error path), so the flag is not cleared. */
static bool s_verified_on_flash = false;

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/
/**
 * @brief reads the general section into a record and checks its magic and version
 * @param record destination for the record read back from flash; must not be NULL
 * @return bool true if the read succeeded and the record is one this firmware can use
 */
static bool read_record(cal_data_record_t* record);

/**
 * @brief stages the RAM copy into a record ready for writing to flash
 * @param record destination record; must not be NULL
 */
static void stage_record(cal_data_record_t* record);

/**
 * @brief erases and rewrites the general section with the staged record, blocking until the
 *        write is verified
 * @note uses the spi_flash_io blocking wrappers, which include read-back verification and
 *       retry. Only safe before the w25q command queue carries other traffic -- the boot-path
 *       provisioning save in cal_data_init() runs before the scheduler starts, so nothing else
 *       can be queued behind it.
 * @param record record to write; must not be NULL
 * @return bool true if the record was written and verified
 */
static bool save_blocking(const cal_data_record_t* record);

/**
 * @brief reports whether the w25q driver has finished everything queued on it
 * @note the driver has no per-command completion hook, so a drained queue plus a non-busy
 *       driver is the only completion signal available. The queue empties when the last command
 *       is popped, which happens before that command finishes, hence both checks
 * @return bool true if nothing is queued and no transfer is in progress
 */
static bool flash_is_idle(void);

/**
 * @brief clears the in-flight flag once the driver has gone idle, recording an error against a
 *        save that was still outstanding when the driver faulted
 * @note called at the top of every public save function so that status is re-evaluated lazily
 *       rather than needing a periodic tick from the application
 */
static void refresh_save_state(void);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/
bool cal_data_init(void) {
  cal_data_record_t record;
  bool loaded_valid;

  loaded_valid = read_record(&record);

  if (loaded_valid == true) {
    cal_params = record.params;
    /* A record that passed the magic + version check is a usable record on flash -- the
     * same fact save_blocking() records after a verified write. */
    s_verified_on_flash = true;

    /* The record was checked for magic and version only; repair any zero field so the
     * live copy always holds usable values. Write the repaired copy back (blocking --
     * the scheduler is not running yet) so the next boot reads the same values. A failed
     * write-back is not escalated, same as the blank-flash branch below: the repaired
     * values are live in RAM, the application runs correctly, and the repair is retried
     * on the next boot. */
    cal_data_sanitize_zeros();
    if (memcmp(&cal_params, &record.params, sizeof(cal_params)) != 0) {
      stage_record(&record);
      (void)save_blocking(&record);
    }
  }
  else {
    /* Nothing usable on flash -- provision the section so the next boot reads back cleanly.
     * The blocking path is deliberate here: the scheduler is not running yet, so the queued
     * path has no task context to poll it and nothing else can be queued on the driver.
     * A failed save is not escalated here: the defaults are already live in RAM, so the
     * application runs correctly either way and simply re-provisions on the next boot. */
    cal_data_load_defaults();
    stage_record(&record);
    (void)save_blocking(&record);
  }

  return loaded_valid;
}

cal_data_params_t* cal_data_get(void) {
  return &cal_params;
}

bool cal_data_save(void) {
  w25q_command_t erase_command;
  w25q_command_t write_command;
  uint32_t sector_index;
  bool queued = false;

  refresh_save_state();

  if ((s_save_in_flight == false) && (w25q_get_transfer_status() != W25Q_TRANSFER_STATUS_ERROR) &&
      ((w25q_commands_in_queue() + CAL_DATA_QUEUE_SLOTS_PER_SAVE) <= CAL_DATA_QUEUE_CAPACITY)) {
    stage_record(&save_stage);

    erase_command.command = W25Q_SECTOR_ERASE;
    erase_command.length = 0;
    erase_command.buffer = NULL;

    /* Capacity was checked above, so the erases are only ever queued when the write can follow
     * them. The queue is FIFO and w25q_finish_command() chains the next command from the SPI
     * ISR, so every erase is guaranteed to complete before the write starts. */
    for (sector_index = 0U; sector_index < CAL_DATA_GENERAL_SECTOR_COUNT; sector_index++) {
      erase_command.address = CAL_DATA_GENERAL_BASE_ADDRESS + (sector_index * (uint32_t)W25Q_SECTOR_SIZE);

      if (w25q_queue_command(&erase_command) == false) {
        break;
      }
    }

    if (sector_index == CAL_DATA_GENERAL_SECTOR_COUNT) {
      write_command.command = W25Q_WRITE;
      write_command.address = CAL_DATA_GENERAL_BASE_ADDRESS;
      write_command.length = (int)sizeof(save_stage);
      write_command.buffer = (uint8_t*)&save_stage;

      queued = w25q_queue_command(&write_command);
    }

    if (queued == true) {
      s_save_in_flight = true;
      s_save_errored = false;
    }
  }

  return queued;
}

cal_data_save_status_enum cal_data_save_status(void) {
  cal_data_save_status_enum status;

  refresh_save_state();

  if (s_save_errored == true) {
    status = CAL_DATA_SAVE_ERROR;
  }
  else if (s_save_in_flight == true) {
    status = CAL_DATA_SAVE_PENDING;
  }
  else {
    status = CAL_DATA_SAVE_IDLE;
  }

  return status;
}

void cal_data_load_defaults(void) {
  cal_params = default_params;
}

void cal_data_sanitize_zeros(void) {
  const uint32_t* defaults = (const uint32_t*)&default_params;
  uint32_t* live = (uint32_t*)&cal_params;

  /* The struct is a flat array of uint32_t by design (see cal_data.h); the assert above
   * the defaults table guards that layout. Every parameter must be non-zero to be usable,
   * so a zero field -- from a stored record, or from a rejected external write -- takes
   * its factory default. */
  for (uint32_t i = 0U; i < CAL_DATA_PARAM_COUNT; i++) {
    if (live[i] == 0U) {
      live[i] = defaults[i];
    }
  }
}

bool cal_data_is_valid(void) {
  return s_verified_on_flash;
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/
static bool read_record(cal_data_record_t* record) {
  bool usable = false;

  if (record != NULL) {
    if (spi_flash_io_read(CAL_DATA_GENERAL_BASE_ADDRESS, (uint8_t*)record, (uint32_t)sizeof(*record)) == true) {
      usable = (record->header.magic == CAL_DATA_MAGIC) && (record->header.version == CAL_DATA_VERSION);
    }
  }

  return usable;
}

static void stage_record(cal_data_record_t* record) {
  if (record != NULL) {
    record->header.magic = CAL_DATA_MAGIC;
    record->header.version = CAL_DATA_VERSION;
    record->params = cal_params;
  }
}

static bool save_blocking(const cal_data_record_t* record) {
  bool saved = false;

  if (record != NULL) {
    saved = spi_flash_io_erase_range(CAL_DATA_GENERAL_BASE_ADDRESS, (uint32_t)sizeof(*record));

    if (saved == true) {
      saved = spi_flash_io_write(CAL_DATA_GENERAL_BASE_ADDRESS, (const uint8_t*)record, (uint32_t)sizeof(*record));
    }

    if (saved == true) {
      s_verified_on_flash = true;
    }
  }

  return saved;
}

static bool flash_is_idle(void) {
  return (w25q_commands_in_queue() == 0) && (w25q_get_transfer_status() != W25Q_TRANSFER_STATUS_BUSY);
}

static void refresh_save_state(void) {
  bool driver_errored = (w25q_get_transfer_status() == W25Q_TRANSFER_STATUS_ERROR);

  /* An error is terminal even with commands still queued. w25q_process_command() ignores the
   * return of the call it makes, so a command that fails to start leaves the driver in ERROR
   * with whatever was behind it stranded in the queue -- and the queue is only ever advanced
   * from the completion ISR, which will not run. Waiting for a drain that cannot happen would
   * pin the save at PENDING forever, so the error is taken as the end of the save.
   *
   * Clearing the in-flight flag hands the staging buffer back for reuse while a stranded write
   * may still reference it: if some later direct w25q operation completes, the ISR pops that
   * write and programs whatever the buffer holds by then, into sectors that were not erased for
   * it. The result is a rejected record (bad magic or version) on the next load, i.e. factory
   * defaults -- the same outcome as any other failed save, and preferable to a store that can
   * never save again. */
  if ((driver_errored == true) || (flash_is_idle() == true)) {
    if (s_save_in_flight == true) {
      s_save_in_flight = false;
      s_save_errored = driver_errored;
    }
  }
}
