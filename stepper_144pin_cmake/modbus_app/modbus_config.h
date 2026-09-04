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

typedef enum {
  /* ID registers */
  REG_SLAVE_FW_VERSION = 1,
  REG_SLAVE_SERIAL_NUMBER = 2,
  REG_PATTY1_ADD_TO_QUEUE = 3,  // 3
  REG_PATTY2_ADD_TO_QUEUE = 4,  // 4
  REG_RELOAD_REQUEST = 5,       // 5 write non-zero to request a cartridge reload
  REG_STATUS = 6,               // 6 read-only machine status (see app_sm_status_enum)

  // Cal-data backed registers
  REG_CAL_DATA_PUSH_RETRACT_TIMEOUT_MS = 75,
  REG_CAL_DATA_LIFT_TIMEOUT_MS = 76,

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
