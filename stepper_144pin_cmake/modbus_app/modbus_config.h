/**
 * @file modbus_config.h
 * @author
 * @brief
 * @version 0.1
 * @date 2024-11-19
 *
 * @copyright Copyright (c) 2024 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef MODBUS_CONFIG_H_
#define MODBUS_CONFIG_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include "FreeRTOS.h"
#include "task.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
#define MB_BAUD_RATE 57600
#define MB_RESPONSE_DELAY_US 500
#define MB_REG_ARRAY_SIZE (128)
#define MB_SLAVE_ADDR (15)

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/* Customer register ids. Values are 1-based register numbers: the dispatcher
 * adds 1 to the wire address before lookup, so enum value N is reached at
 * wire address N-1 (existing convention, inherited by all ids below).
 * Sheet references are to MODBUS_Register_Map_AsSpecified.md. */
typedef enum {
  /* ID registers */
  REG_SLAVE_FW_VERSION = 1,
  REG_SLAVE_SERIAL_NUMBER = 2,

  REG_PATTY1_ADD_TO_QUEUE = 3,  /* write count of Patty 1 patties to dispense */
  REG_PATTY2_ADD_TO_QUEUE = 4,  /* write count of Patty 2 patties to dispense */
  REG_RELOAD_REQUEST = 5,
  REG_STATUS = 6,               /* machine status (see app_sm_status_enum; enum
                                 * values do not match the sheet's 1-5 codes yet) */
  REG_LTO_PAUSE = 7,            /* 1 = pause LTO dispense logic, 0 = resume
                                 * (future: patty_handler_set_lto_pause via event post) */
  REG_HOME_THRESHOLD = 8,       /* home error threshold (cal_data.home_error_counts) */
  REG_LOAD_OFFSET = 9,          /* load offset (cal_data.load_offset) */
  REG_STALL_THRESHOLD = 10,     /* stall error threshold (cal_data.stall_error_counts) */
  REG_PATTY1_THICKNESS = 11,    /* per-patty travel for Patty 1 (cal_data.patty_thickness_counts) */
  REG_PATTY2_THICKNESS = 12,    /* per-patty travel for Patty 2 (cal_data.patty2_thickness_counts) */
  REG_CARTRIDGE1_STATUS = 13,   /* cartridge 1 type (future: cartridge_read_type_sensors) */
  REG_CARTRIDGE2_STATUS = 14,   /* cartridge 2 type (future: cartridge_read_type_sensors) */
  REG_CARTRIDGE3_STATUS = 15,   /* cartridge 3 type (future: cartridge_read_type_sensors) */
  REG_CARTRIDGE4_STATUS = 16,   /* cartridge 4 type (future: cartridge_read_type_sensors) */
  REG_CARTRIDGE1_REMAINING = 17, /* cartridge 1 patty remaining (future: cartridge_t.remaining) */
  REG_CARTRIDGE2_REMAINING = 18, /* cartridge 2 patty remaining (future: cartridge_t.remaining) */
  REG_CARTRIDGE3_REMAINING = 19, /* cartridge 3 patty remaining (future: cartridge_t.remaining) */
  REG_CARTRIDGE4_REMAINING = 20, /* cartridge 4 patty remaining (future: cartridge_t.remaining) */
  REG_CARTRIDGE1_QUEUE = 21,    /* cartridge 1 dispense queue depth (future: cartridge_t.pending) */
  REG_CARTRIDGE2_QUEUE = 22,    /* cartridge 2 dispense queue depth (future: cartridge_t.pending) */
  REG_CARTRIDGE3_QUEUE = 23,    /* cartridge 3 dispense queue depth (future: cartridge_t.pending) */
  REG_CARTRIDGE4_QUEUE = 24,    /* cartridge 4 dispense queue depth (future: cartridge_t.pending) */
  REG_DOOR_SWITCH = 25,         /* 0 = open, 1 = closed (future: input_get_door_closed) */
  REG_DOOR_LOCK = 26,           /* 0 = unlocked, 1 = locked (future: input_get_lock_confirmed) */
  REG_TEMP_SENSOR1 = 27,        /* 0.1 deg F signed (no sensor hardware/driver exists) */
  REG_TEMP_SENSOR2 = 28,        /* 0.1 deg F signed (no sensor hardware/driver exists) */
  REG_ERROR_BITMASK = 29,       /* error bits (future: synthesize from app_sm_t.fault_code,
                                 * cartridge_t.faulted, cartridge_validate_lane_pairs) */

  // Cal-data backed registers.
  REG_CAL_DATA_PUSH_RETRACT_TIMEOUT_MS = 75,
  REG_CAL_DATA_LIFT_TIMEOUT_MS = 76,
  REG_CAL_DATA_PUSHER_RPM = 77,            /* cal_data.pusher_rpm */
  REG_CAL_DATA_LIFTER_RPM = 78,            /* cal_data.lifter_rpm */
  REG_CAL_DATA_HOME_RPM = 79,              /* cal_data.home_rpm */
  REG_CAL_DATA_HOME_BACKOFF_STEPS = 80,    /* cal_data.home_backoff_steps (microsteps) */
  REG_CAL_DATA_HOME_SETTLE_DELAY_MS = 81,  /* cal_data.home_settle_delay_ms */
  REG_CAL_DATA_HOME_MAX_STEPS = 82,        /* cal_data.home_max_steps (microsteps) */
  REG_CAL_DATA_SUPERVISOR_PERIOD_MS = 83,  /* cal_data.supervisor_period_ms */
  REG_CAL_DATA_DEFAULT_MOVE_RPM = 84,      /* cal_data.default_move_rpm */
  REG_CAL_DATA_DEFAULT_MOVE_STEPS = 85,    /* cal_data.default_move_steps (microsteps) */
  REG_CAL_DATA_RECOUNT_TIMEOUT_MS = 86,

  REG_CAL_DATA_LOCK_TIMEOUT_MS = 100,           /* cal_data.lock_timeout_ms */
  REG_CAL_DATA_MOTION_TIMEOUT_MS = 101,         /* cal_data.motion_timeout_ms */
  REG_CAL_DATA_DOOR_TIMEOUT_MS = 102,           /* cal_data.door_timeout_ms -- default 65535 is the u16 ceiling */
  REG_CAL_DATA_STARTUP_SETTLE_DELAY_MS = 103,   /* cal_data.startup_settle_delay_ms */
  /* cal_data.door_debounce_ms -- input.c samples it once at boot; runtime writes take
   * effect on the next boot */
  REG_CAL_DATA_DOOR_DEBOUNCE_MS = 104,

} app_slave_regs_enum;

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 *Function Prototypes
 *******************************************************************************/

/**
 * @brief Returns the FreeRTOS tick count recorded when the last Modbus frame arrived.
 * @return TickType_t tick count.
 */
TickType_t modbus_portable_get_last_frame_time(void);

#endif /* MODBUS_CONFIG_H_ */
