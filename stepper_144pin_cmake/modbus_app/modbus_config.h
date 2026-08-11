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
  REG_SLAVE_SERIAL_NUMBER,      // 2
  REG_PATTY1_ADD_TO_QUEUE = 3,  // 3
  REG_PATTY2_ADD_TO_QUEUE = 4,  // 4

  // Cal-data backed registers
  REG_CAL_DATA_PUSH_RETRACT_TIMEOUT_MS = 75,
  REG_CAL_DATA_LIFT_TIMEOUT_MS = 76,
  REG_CAL_DATA_STALL_ERROR_COUNTS = 77,
  REG_CAL_DATA_HOME_ERROR_COUNTS = 78,

  REG_SLAVE_UI_ENTER_SELF_TEST_MODE = 500,  // Used to tell UI to go into self test mode on boot
  REG_SLAVE_UI_SELF_TEST_RESULTS = 501,     // used to indicate self test results. Bit0: 1 if self test mode was entered, else 0
                                            // Bit1: 1 if GPIO test passed, else 0
                                            // Bit2: 1 if spi flash test passed, else 0
                                            // bit3-bit14: reserved
                                            // bit15: 1 when test is complete
  REG_SLAVE_UI_SOUND_BUZZER =
      502,  // used to trigger buzzer sound from master. writing a non-zero value to this register will cause the UI to play a buzzer sound
  REG_SLAVE_UI_SHOW_BLACK_PIXELS = 503,  // used to trigger all black/white pixels on the LCD. non-zero = black, 0 = white

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
