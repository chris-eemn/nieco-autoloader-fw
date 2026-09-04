/**
 * @file mb_regs.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief Modbus slave to facilitate production testing
 * @version 0.1
 * @date 2025-06-19
 *
 * @copyright Copyright (c) 2025 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include "mb_regs.h"

#include <stdint.h>

#include "modbus_config.h"
#include "modbus_port.h"
#include "modbus_slave.h"
#include "app_console.h"
#include "app_sm_port.h"
#include "app_task.h"
#include "autoloader_types.h"
#include "cal_data.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
#define REG_ARRAY_LENGTH MB_REG_ARRAY_SIZE

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/
/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/
static int reg_patty1_add_to_queue_read(uint16_t reg, uint16_t* val_ptr);
static int reg_patty1_add_to_queue_write(uint16_t reg, uint16_t val);
static int reg_patty2_add_to_queue_read(uint16_t reg, uint16_t* val_ptr);
static int reg_patty2_add_to_queue_write(uint16_t reg, uint16_t val);
static int reload_request_reg_read(uint16_t reg, uint16_t* val_ptr);
static int reload_request_reg_write(uint16_t reg, uint16_t val);
static int status_reg_read(uint16_t reg, uint16_t* val_ptr);
static int fw_ver_reg_read(uint16_t reg, uint16_t* val_ptr);
static int serial_number_reg_read(uint16_t reg, uint16_t* val_ptr);
static int reg_push_retract_timeout_ms_read(uint16_t reg, uint16_t* val_ptr);
static int reg_push_retract_timeout_ms_write(uint16_t reg, uint16_t val);
static int reg_lift_timeout_ms_read(uint16_t reg, uint16_t* val_ptr);
static int reg_lift_timeout_ms_write(uint16_t reg, uint16_t val);

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/
static mb_holding_reg_def_t regs_defines[] = {
    {.reg_id = REG_PATTY1_ADD_TO_QUEUE,
     .reg_amount = 1,
     .read_callback = reg_patty1_add_to_queue_read,
     .write_callback = reg_patty1_add_to_queue_write,
     .name = "patty1_add_to_queue"},
    {.reg_id = REG_PATTY2_ADD_TO_QUEUE,
     .reg_amount = 1,
     .read_callback = reg_patty2_add_to_queue_read,
     .write_callback = reg_patty2_add_to_queue_write,
     .name = "patty2_add_to_queue"},
    {.reg_id = REG_RELOAD_REQUEST,
     .reg_amount = 1,
     .read_callback = reload_request_reg_read,
     .write_callback = reload_request_reg_write,
     .name = "reload_request"},
    {.reg_id = REG_STATUS,
     .reg_amount = 1,
     .read_callback = status_reg_read,
     .name = "status"},
    {.reg_id = REG_SLAVE_FW_VERSION,
     .reg_amount = 1,
     .read_callback = fw_ver_reg_read,
     .name = "fw_ver"},
    {.reg_id = REG_SLAVE_SERIAL_NUMBER,
     .reg_amount = 1,
     .read_callback = serial_number_reg_read,
     .name = "serial_number"},
    {.reg_id = REG_CAL_DATA_PUSH_RETRACT_TIMEOUT_MS,
     .reg_amount = 1,
     .read_callback = reg_push_retract_timeout_ms_read,
     .write_callback = reg_push_retract_timeout_ms_write,
     .name = "push_retract_timeout_ms"},
    {.reg_id = REG_CAL_DATA_LIFT_TIMEOUT_MS,
     .reg_amount = 1,
     .read_callback = reg_lift_timeout_ms_read,
     .write_callback = reg_lift_timeout_ms_write,
     .name = "lift_timeout_ms"},
};

modbus_slave_t ui_slave = {0};
modbus_slave_params_t slave_params = {};
mb_holding_reg_def_t* regs[REG_ARRAY_LENGTH];
mb_holding_reg_array_t reg_array = {
    .regs = &regs[0],
    .length = 0,
    .max_length = REG_ARRAY_LENGTH,
};

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/
modbus_slave_t* get_ui_slave(void) {
  return &ui_slave;
}

mb_holding_reg_array_t* get_reg_array(void) {
  return &reg_array;
}

void mb_regs_init(void) {
  slave_params.slave_address = MB_SLAVE_ADDR;
  slave_params.slave_id = "stepper-tester";
  slave_params.baud = 57600;
  slave_params.response_delay_us = 5000;
  MB_InitializeModbus(&ui_slave, get_ui_port_fns(), &slave_params);

  for (int i = 0; i < (sizeof(regs_defines) / sizeof(regs_defines[0])); i++) {
    MB_AddHoldingRegister(&reg_array, &regs_defines[i]);
  }

  MB_RegisterHoldingRegisterArray(&ui_slave, &reg_array);
}

void mb_regs_deinit(void) {
  MB_SlaveDeinit(&ui_slave);
}
/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/
static int fw_ver_reg_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  *val_ptr = 0x740a;
  return 0;
}

static int serial_number_reg_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  *val_ptr = 12345;
  return 0;
}

static int reg_patty1_add_to_queue_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  *val_ptr = 0x11;
  return 0;
}

static int reg_patty1_add_to_queue_write(uint16_t reg, uint16_t val) {
  (void)reg;
  app_console_print("[INFO] Patty1 add to queue via modbus register write. val: %d\r\n", val);
  return 0;
}

static int reg_patty2_add_to_queue_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  *val_ptr = 0x22;
  return 0;
}

static int reg_patty2_add_to_queue_write(uint16_t reg, uint16_t val) {
  (void)reg;
  app_console_print("[INFO] Patty2 add to queue via modbus register write. val: %d\r\n", val);
  return 0;
}

static int reload_request_reg_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  /* Write-trigger register: reads always return 0. */
  if (val_ptr == NULL) {
    return -1;
  }
  *val_ptr = 0U;
  return 0;
}

static int reload_request_reg_write(uint16_t reg, uint16_t val) {
  (void)reg;
  app_event_t reload_event;
  bool queued;

  if (val == 0U) {
    return 0;
  }

  /* Post through the app-task event queue; the state machine must not be
   * called directly from the Modbus context. Register callbacks run from
   * the Modbus timer ISR (TIM7, priority 5 = FreeRTOS safe ceiling), so
   * the ISR-safe post variant is required here. */
  reload_event.id = APP_EV_RELOAD_REQUEST;
  reload_event.slot = APP_NO_SLOT;
  reload_event.product_type = 0U;
  reload_event.value = 0U;
  reload_event.axis_num = 0U;

  /* No console logging here: app_console_print() is not ISR-safe.
   * A dropped request returns non-zero so the caller can detect it. */
  queued = app_task_post_from_isr(&reload_event);

  if (queued == false) {
    /* Event queue full or not yet created: the request was dropped. */
    return -1;
  }

  return 0;
}

static int status_reg_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  /* Cached status published by app_sm_port_publish_state. */
  if (val_ptr == NULL) {
    return -1;
  }
  *val_ptr = (uint16_t)app_sm_port_get_status();
  return 0;
}

static int reg_push_retract_timeout_ms_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  cal_data_params_t* params = cal_data_get();
  if (params != NULL) {
    *val_ptr = (uint16_t)params->push_retract_timeout_ms;
  }
  else {
    *val_ptr = 3000;
  }
  return 0;
}

static int reg_push_retract_timeout_ms_write(uint16_t reg, uint16_t val) {
  (void)reg;
  cal_data_params_t* params = cal_data_get();
  if (params != NULL) {
    params->push_retract_timeout_ms = (uint32_t)val;
    if (cal_data_save() == true) {
      app_console_print("[MODBUS] push_retract_timeout_ms = %lu (saved)\r\n", (unsigned long)params->push_retract_timeout_ms);
    }
    else {
      app_console_print("[MODBUS] push_retract_timeout_ms = %lu -- FLASH SAVE FAILED\r\n", (unsigned long)params->push_retract_timeout_ms);
    }
  }
  return 0;
}

static int reg_lift_timeout_ms_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  cal_data_params_t* params = cal_data_get();
  if (params != NULL) {
    *val_ptr = (uint16_t)params->lift_timeout_ms;
  }
  else {
    *val_ptr = 5000;
  }
  return 0;
}

static int reg_lift_timeout_ms_write(uint16_t reg, uint16_t val) {
  (void)reg;
  cal_data_params_t* params = cal_data_get();
  if (params != NULL) {
    params->lift_timeout_ms = (uint32_t)val;
    if (cal_data_save() == true) {
      app_console_print("[MODBUS] lift_timeout_ms = %lu (saved)\r\n", (unsigned long)params->lift_timeout_ms);
    }
    else {
      app_console_print("[MODBUS] lift_timeout_ms = %lu -- FLASH SAVE FAILED\r\n", (unsigned long)params->lift_timeout_ms);
    }
  }
  return 0;
}
