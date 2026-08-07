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
#include "cal_data.h"
#include "cal_data_map.h"
#include "spi_flash_io.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
/** Marks a general-section record as having been written by this module. "CALD" in ASCII. */
#define CAL_DATA_MAGIC (0x43414C44UL)

/** Longest buffer spi_flash_io_write() accepts in one call -- see spi_flash_io.h. The record is
 * written in a single call, so it has to fit; the static assert below enforces that. */
#define CAL_DATA_MAX_WRITE_LEN (512U)

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

_Static_assert(sizeof(cal_data_record_t) <= CAL_DATA_MAX_WRITE_LEN,
               "cal-data general record no longer fits in a single spi_flash_io_write() call");
_Static_assert(sizeof(cal_data_record_t) <= CAL_DATA_GENERAL_SIZE,
               "cal-data general record no longer fits in the sectors reserved for it");
_Static_assert(CAL_DATA_ALLOCATED_SIZE <= W25Q_CAL_DATA_SIZE,
               "cal-data sections overflow the cal-data region of the W25Q");

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/
/** Factory defaults, applied when flash holds nothing usable. Mirrors the values currently
 * configured in stepper_system.c and stepper_ctrl.h so that provisioning a blank chip
 * reproduces today's behaviour rather than changing it. */
static const cal_data_params_t default_params = {
  .pusher_rpm            = 30U,
  .lifter_rpm            = 30U,
  .max_sync_error_counts = 10U,
  .home_rpm              = 10U,
  .home_backoff_steps    = 800U,
  .home_settle_delay_ms  = 100U,
  .home_max_steps        = 50000U,
  .supervisor_period_ms  = 25U,
  .default_move_rpm      = 5U,
  .default_move_steps    = 800U,
};

/** Live RAM copy handed out by cal_data_get(). */
static cal_data_params_t cal_params;

/** Whether cal_data_init() found a valid record on flash. */
static bool cal_data_valid = false;

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/
/**
 * @brief reads the general section into a record and checks its magic and version
 * @param record destination for the record read back from flash; must not be NULL
 * @return bool true if the read succeeded and the record is one this firmware can use
 */
static bool read_record(cal_data_record_t *record);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/
bool cal_data_init(void) {
  cal_data_record_t record;

  cal_data_valid = read_record(&record);

  if (cal_data_valid == true) {
    cal_params = record.params;
  } else {
    /* Nothing usable on flash -- provision the section so the next boot reads back cleanly.
     * A failed save is not escalated here: the defaults are already live in RAM, so the
     * application runs correctly either way and simply re-provisions on the next boot. */
    cal_data_load_defaults();
    (void)cal_data_save();
  }

  return cal_data_valid;
}

cal_data_params_t *cal_data_get(void) {
  return &cal_params;
}

bool cal_data_save(void) {
  cal_data_record_t record;
  bool saved;

  record.header.magic   = CAL_DATA_MAGIC;
  record.header.version = CAL_DATA_VERSION;
  record.params         = cal_params;

  saved = spi_flash_io_erase_range(CAL_DATA_GENERAL_BASE_ADDRESS, (uint32_t)sizeof(record));

  if (saved == true) {
    saved = spi_flash_io_write(CAL_DATA_GENERAL_BASE_ADDRESS, (const uint8_t *)&record,
                               (uint32_t)sizeof(record));
  }

  if (saved == true) {
    cal_data_valid = true;
  }

  return saved;
}

void cal_data_load_defaults(void) {
  cal_params = default_params;
}

bool cal_data_is_valid(void) {
  return cal_data_valid;
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/
static bool read_record(cal_data_record_t *record) {
  bool usable = false;

  if (record != NULL) {
    if (spi_flash_io_read(CAL_DATA_GENERAL_BASE_ADDRESS, (uint8_t *)record, (uint32_t)sizeof(*record)) == true) {
      usable = (record->header.magic == CAL_DATA_MAGIC) && (record->header.version == CAL_DATA_VERSION);
    }
  }

  return usable;
}
