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
#include "cartridge.h"

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
static int reg_lto_pause_read(uint16_t reg, uint16_t* val_ptr);
static int reg_lto_pause_write(uint16_t reg, uint16_t val);
static int reg_home_threshold_read(uint16_t reg, uint16_t* val_ptr);
static int reg_home_threshold_write(uint16_t reg, uint16_t val);
static int reg_load_offset_read(uint16_t reg, uint16_t* val_ptr);
static int reg_load_offset_write(uint16_t reg, uint16_t val);
static int reg_stall_threshold_read(uint16_t reg, uint16_t* val_ptr);
static int reg_stall_threshold_write(uint16_t reg, uint16_t val);
static int reg_patty1_thickness_read(uint16_t reg, uint16_t* val_ptr);
static int reg_patty1_thickness_write(uint16_t reg, uint16_t val);
static int reg_patty2_thickness_read(uint16_t reg, uint16_t* val_ptr);
static int reg_patty2_thickness_write(uint16_t reg, uint16_t val);
static int reg_cartridge1_status_read(uint16_t reg, uint16_t* val_ptr);
static int reg_cartridge2_status_read(uint16_t reg, uint16_t* val_ptr);
static int reg_cartridge3_status_read(uint16_t reg, uint16_t* val_ptr);
static int reg_cartridge4_status_read(uint16_t reg, uint16_t* val_ptr);
static int reg_cartridge1_remaining_read(uint16_t reg, uint16_t* val_ptr);
static int reg_cartridge2_remaining_read(uint16_t reg, uint16_t* val_ptr);
static int reg_cartridge3_remaining_read(uint16_t reg, uint16_t* val_ptr);
static int reg_cartridge4_remaining_read(uint16_t reg, uint16_t* val_ptr);
static int reg_cartridge1_queue_read(uint16_t reg, uint16_t* val_ptr);
static int reg_cartridge2_queue_read(uint16_t reg, uint16_t* val_ptr);
static int reg_cartridge3_queue_read(uint16_t reg, uint16_t* val_ptr);
static int reg_cartridge4_queue_read(uint16_t reg, uint16_t* val_ptr);
static int reg_door_switch_read(uint16_t reg, uint16_t* val_ptr);
static int reg_door_lock_read(uint16_t reg, uint16_t* val_ptr);
static int reg_temp_sensor1_read(uint16_t reg, uint16_t* val_ptr);
static int reg_temp_sensor2_read(uint16_t reg, uint16_t* val_ptr);
static int reg_error_bitmask_read(uint16_t reg, uint16_t* val_ptr);

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
    {.reg_id = REG_LTO_PAUSE,
     .reg_amount = 1,
     .read_callback = reg_lto_pause_read,
     .write_callback = reg_lto_pause_write,
     .name = "lto_pause"},
    {.reg_id = REG_HOME_THRESHOLD,
     .reg_amount = 1,
     .read_callback = reg_home_threshold_read,
     .write_callback = reg_home_threshold_write,
     .name = "home_threshold"},
    {.reg_id = REG_LOAD_OFFSET,
     .reg_amount = 1,
     .read_callback = reg_load_offset_read,
     .write_callback = reg_load_offset_write,
     .name = "load_offset"},
    {.reg_id = REG_STALL_THRESHOLD,
     .reg_amount = 1,
     .read_callback = reg_stall_threshold_read,
     .write_callback = reg_stall_threshold_write,
     .name = "stall_threshold"},
    {.reg_id = REG_PATTY1_THICKNESS,
     .reg_amount = 1,
     .read_callback = reg_patty1_thickness_read,
     .write_callback = reg_patty1_thickness_write,
     .name = "patty1_thickness"},
    {.reg_id = REG_PATTY2_THICKNESS,
     .reg_amount = 1,
     .read_callback = reg_patty2_thickness_read,
     .write_callback = reg_patty2_thickness_write,
     .name = "patty2_thickness"},
    {.reg_id = REG_CARTRIDGE1_STATUS,
     .reg_amount = 1,
     .read_callback = reg_cartridge1_status_read,
     .name = "cartridge1_status"},
    {.reg_id = REG_CARTRIDGE2_STATUS,
     .reg_amount = 1,
     .read_callback = reg_cartridge2_status_read,
     .name = "cartridge2_status"},
    {.reg_id = REG_CARTRIDGE3_STATUS,
     .reg_amount = 1,
     .read_callback = reg_cartridge3_status_read,
     .name = "cartridge3_status"},
    {.reg_id = REG_CARTRIDGE4_STATUS,
     .reg_amount = 1,
     .read_callback = reg_cartridge4_status_read,
     .name = "cartridge4_status"},
    {.reg_id = REG_CARTRIDGE1_REMAINING,
     .reg_amount = 1,
     .read_callback = reg_cartridge1_remaining_read,
     .name = "cartridge1_remaining"},
    {.reg_id = REG_CARTRIDGE2_REMAINING,
     .reg_amount = 1,
     .read_callback = reg_cartridge2_remaining_read,
     .name = "cartridge2_remaining"},
    {.reg_id = REG_CARTRIDGE3_REMAINING,
     .reg_amount = 1,
     .read_callback = reg_cartridge3_remaining_read,
     .name = "cartridge3_remaining"},
    {.reg_id = REG_CARTRIDGE4_REMAINING,
     .reg_amount = 1,
     .read_callback = reg_cartridge4_remaining_read,
     .name = "cartridge4_remaining"},
    {.reg_id = REG_CARTRIDGE1_QUEUE,
     .reg_amount = 1,
     .read_callback = reg_cartridge1_queue_read,
     .name = "cartridge1_queue"},
    {.reg_id = REG_CARTRIDGE2_QUEUE,
     .reg_amount = 1,
     .read_callback = reg_cartridge2_queue_read,
     .name = "cartridge2_queue"},
    {.reg_id = REG_CARTRIDGE3_QUEUE,
     .reg_amount = 1,
     .read_callback = reg_cartridge3_queue_read,
     .name = "cartridge3_queue"},
    {.reg_id = REG_CARTRIDGE4_QUEUE,
     .reg_amount = 1,
     .read_callback = reg_cartridge4_queue_read,
     .name = "cartridge4_queue"},
    {.reg_id = REG_DOOR_SWITCH,
     .reg_amount = 1,
     .read_callback = reg_door_switch_read,
     .name = "door_switch"},
    {.reg_id = REG_DOOR_LOCK,
     .reg_amount = 1,
     .read_callback = reg_door_lock_read,
     .name = "door_lock"},
    {.reg_id = REG_TEMP_SENSOR1,
     .reg_amount = 1,
     .read_callback = reg_temp_sensor1_read,
     .name = "temp_sensor1"},
    {.reg_id = REG_TEMP_SENSOR2,
     .reg_amount = 1,
     .read_callback = reg_temp_sensor2_read,
     .name = "temp_sensor2"},
    {.reg_id = REG_ERROR_BITMASK,
     .reg_amount = 1,
     .read_callback = reg_error_bitmask_read,
     .name = "error_bitmask"},
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
  app_event_t dispense_event;
  bool queued;

  (void)reg;

  if (val == 0U) {
    return 0;
  }

  dispense_event.id = APP_EV_DISPENSE_REQUEST;
  dispense_event.slot = APP_NO_SLOT;
  dispense_event.product_type = (uint8_t)CARTRIDGE_TYPE_WHOPPER;
  dispense_event.value = val;
  dispense_event.axis_num = 0U;

  queued = app_task_post_from_isr(&dispense_event);

  if (queued == false) {
    /* Event queue full or not yet created: the request was dropped. */
    return -1;
  }

  return 0;
}

static int reg_patty2_add_to_queue_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  *val_ptr = 0x22;
  return 0;
}

static int reg_patty2_add_to_queue_write(uint16_t reg, uint16_t val) {
  app_event_t dispense_event;
  bool queued;

  (void)reg;

  if (val == 0U) {
    return 0;
  }

  dispense_event.id = APP_EV_DISPENSE_REQUEST;
  dispense_event.slot = APP_NO_SLOT;
  dispense_event.product_type = (uint8_t)CARTRIDGE_TYPE_JR;
  dispense_event.value = val;
  dispense_event.axis_num = 0U;

  queued = app_task_post_from_isr(&dispense_event);

  if (queued == false) {
    /* Event queue full or not yet created: the request was dropped. */
    return -1;
  }

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

static int reg_lto_pause_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  if (val_ptr == NULL) {
    return -1;
  }
  app_console_print("[MODBUS] lto_pause: not implemented\r\n");
  *val_ptr = 0U;
  return 0;
}

static int reg_lto_pause_write(uint16_t reg, uint16_t val) {
  (void)reg;
  (void)val;
  app_console_print("[MODBUS] lto_pause: not implemented\r\n");
  return 0;
}

static int reg_home_threshold_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  if (val_ptr == NULL) {
    return -1;
  }
  app_console_print("[MODBUS] home_threshold: not implemented\r\n");
  *val_ptr = 0U;
  return 0;
}

static int reg_home_threshold_write(uint16_t reg, uint16_t val) {
  (void)reg;
  (void)val;
  app_console_print("[MODBUS] home_threshold: not implemented\r\n");
  return 0;
}

static int reg_load_offset_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  if (val_ptr == NULL) {
    return -1;
  }
  app_console_print("[MODBUS] load_offset: not implemented\r\n");
  *val_ptr = 0U;
  return 0;
}

static int reg_load_offset_write(uint16_t reg, uint16_t val) {
  (void)reg;
  (void)val;
  app_console_print("[MODBUS] load_offset: not implemented\r\n");
  return 0;
}

static int reg_stall_threshold_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  if (val_ptr == NULL) {
    return -1;
  }
  app_console_print("[MODBUS] stall_threshold: not implemented\r\n");
  *val_ptr = 0U;
  return 0;
}

static int reg_stall_threshold_write(uint16_t reg, uint16_t val) {
  (void)reg;
  (void)val;
  app_console_print("[MODBUS] stall_threshold: not implemented\r\n");
  return 0;
}

static int reg_patty1_thickness_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  if (val_ptr == NULL) {
    return -1;
  }
  app_console_print("[MODBUS] patty1_thickness: not implemented\r\n");
  *val_ptr = 0U;
  return 0;
}

static int reg_patty1_thickness_write(uint16_t reg, uint16_t val) {
  (void)reg;
  (void)val;
  app_console_print("[MODBUS] patty1_thickness: not implemented\r\n");
  return 0;
}

static int reg_patty2_thickness_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  if (val_ptr == NULL) {
    return -1;
  }
  app_console_print("[MODBUS] patty2_thickness: not implemented\r\n");
  *val_ptr = 0U;
  return 0;
}

static int reg_patty2_thickness_write(uint16_t reg, uint16_t val) {
  (void)reg;
  (void)val;
  app_console_print("[MODBUS] patty2_thickness: not implemented\r\n");
  return 0;
}

static int reg_cartridge1_status_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  if (val_ptr == NULL) {
    return -1;
  }
  app_console_print("[MODBUS] cartridge1_status: not implemented\r\n");
  *val_ptr = 0U;
  return 0;
}

static int reg_cartridge2_status_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  if (val_ptr == NULL) {
    return -1;
  }
  app_console_print("[MODBUS] cartridge2_status: not implemented\r\n");
  *val_ptr = 0U;
  return 0;
}

static int reg_cartridge3_status_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  if (val_ptr == NULL) {
    return -1;
  }
  app_console_print("[MODBUS] cartridge3_status: not implemented\r\n");
  *val_ptr = 0U;
  return 0;
}

static int reg_cartridge4_status_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  if (val_ptr == NULL) {
    return -1;
  }
  app_console_print("[MODBUS] cartridge4_status: not implemented\r\n");
  *val_ptr = 0U;
  return 0;
}

static int reg_cartridge1_remaining_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  if (val_ptr == NULL) {
    return -1;
  }
  app_console_print("[MODBUS] cartridge1_remaining: not implemented\r\n");
  *val_ptr = 0U;
  return 0;
}

static int reg_cartridge2_remaining_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  if (val_ptr == NULL) {
    return -1;
  }
  app_console_print("[MODBUS] cartridge2_remaining: not implemented\r\n");
  *val_ptr = 0U;
  return 0;
}

static int reg_cartridge3_remaining_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  if (val_ptr == NULL) {
    return -1;
  }
  app_console_print("[MODBUS] cartridge3_remaining: not implemented\r\n");
  *val_ptr = 0U;
  return 0;
}

static int reg_cartridge4_remaining_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  if (val_ptr == NULL) {
    return -1;
  }
  app_console_print("[MODBUS] cartridge4_remaining: not implemented\r\n");
  *val_ptr = 0U;
  return 0;
}

static int reg_cartridge1_queue_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  if (val_ptr == NULL) {
    return -1;
  }
  app_console_print("[MODBUS] cartridge1_queue: not implemented\r\n");
  *val_ptr = 0U;
  return 0;
}

static int reg_cartridge2_queue_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  if (val_ptr == NULL) {
    return -1;
  }
  app_console_print("[MODBUS] cartridge2_queue: not implemented\r\n");
  *val_ptr = 0U;
  return 0;
}

static int reg_cartridge3_queue_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  if (val_ptr == NULL) {
    return -1;
  }
  app_console_print("[MODBUS] cartridge3_queue: not implemented\r\n");
  *val_ptr = 0U;
  return 0;
}

static int reg_cartridge4_queue_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  if (val_ptr == NULL) {
    return -1;
  }
  app_console_print("[MODBUS] cartridge4_queue: not implemented\r\n");
  *val_ptr = 0U;
  return 0;
}

static int reg_door_switch_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  if (val_ptr == NULL) {
    return -1;
  }
  app_console_print("[MODBUS] door_switch: not implemented\r\n");
  *val_ptr = 0U;
  return 0;
}

static int reg_door_lock_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  if (val_ptr == NULL) {
    return -1;
  }
  app_console_print("[MODBUS] door_lock: not implemented\r\n");
  *val_ptr = 0U;
  return 0;
}

static int reg_temp_sensor1_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  if (val_ptr == NULL) {
    return -1;
  }
  app_console_print("[MODBUS] temp_sensor1: not implemented\r\n");
  *val_ptr = 0U;
  return 0;
}

static int reg_temp_sensor2_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  if (val_ptr == NULL) {
    return -1;
  }
  app_console_print("[MODBUS] temp_sensor2: not implemented\r\n");
  *val_ptr = 0U;
  return 0;
}

static int reg_error_bitmask_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  if (val_ptr == NULL) {
    return -1;
  }
  app_console_print("[MODBUS] error_bitmask: not implemented\r\n");
  *val_ptr = 0U;
  return 0;
}
