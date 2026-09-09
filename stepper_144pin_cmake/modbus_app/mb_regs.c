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
#include "input.h"
#include "stepper_system.h"

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
static bool reg_cal_u16_write(uint32_t* field, uint16_t val);
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
static int reg_cartridge_type_read(uint16_t reg, uint16_t* val_ptr, uint8_t slot_index);
static int reg_cartridge_remaining_read(uint16_t reg, uint16_t* val_ptr, uint8_t slot_index);
static int reg_cartridge_queue_read(uint16_t reg, uint16_t* val_ptr, uint8_t slot_index);
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
static int reg_pusher_rpm_read(uint16_t reg, uint16_t* val_ptr);
static int reg_pusher_rpm_write(uint16_t reg, uint16_t val);
static int reg_lifter_rpm_read(uint16_t reg, uint16_t* val_ptr);
static int reg_lifter_rpm_write(uint16_t reg, uint16_t val);
static int reg_home_rpm_read(uint16_t reg, uint16_t* val_ptr);
static int reg_home_rpm_write(uint16_t reg, uint16_t val);
static int reg_home_backoff_steps_read(uint16_t reg, uint16_t* val_ptr);
static int reg_home_backoff_steps_write(uint16_t reg, uint16_t val);
static int reg_home_settle_delay_ms_read(uint16_t reg, uint16_t* val_ptr);
static int reg_home_settle_delay_ms_write(uint16_t reg, uint16_t val);
static int reg_home_max_steps_read(uint16_t reg, uint16_t* val_ptr);
static int reg_home_max_steps_write(uint16_t reg, uint16_t val);
static int reg_supervisor_period_ms_read(uint16_t reg, uint16_t* val_ptr);
static int reg_supervisor_period_ms_write(uint16_t reg, uint16_t val);
static int reg_default_move_rpm_read(uint16_t reg, uint16_t* val_ptr);
static int reg_default_move_rpm_write(uint16_t reg, uint16_t val);
static int reg_default_move_steps_read(uint16_t reg, uint16_t* val_ptr);
static int reg_default_move_steps_write(uint16_t reg, uint16_t val);
static int reg_recount_timeout_ms_read(uint16_t reg, uint16_t* val_ptr);
static int reg_recount_timeout_ms_write(uint16_t reg, uint16_t val);
static int reg_lock_timeout_ms_read(uint16_t reg, uint16_t* val_ptr);
static int reg_lock_timeout_ms_write(uint16_t reg, uint16_t val);
static int reg_motion_timeout_ms_read(uint16_t reg, uint16_t* val_ptr);
static int reg_motion_timeout_ms_write(uint16_t reg, uint16_t val);
static int reg_door_timeout_ms_read(uint16_t reg, uint16_t* val_ptr);
static int reg_door_timeout_ms_write(uint16_t reg, uint16_t val);
static int reg_startup_settle_delay_ms_read(uint16_t reg, uint16_t* val_ptr);
static int reg_startup_settle_delay_ms_write(uint16_t reg, uint16_t val);
static int reg_door_debounce_ms_read(uint16_t reg, uint16_t* val_ptr);
static int reg_door_debounce_ms_write(uint16_t reg, uint16_t val);
static int reg_clear_faults_read(uint16_t reg, uint16_t* val_ptr);
static int reg_clear_faults_write(uint16_t reg, uint16_t val);

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
    {.reg_id = REG_CLEAR_FAULTS,
     .reg_amount = 1,
     .read_callback = reg_clear_faults_read,
     .write_callback = reg_clear_faults_write,
     .name = "clear_faults"},
    {.reg_id = REG_CAL_DATA_PUSHER_RPM,
     .reg_amount = 1,
     .read_callback = reg_pusher_rpm_read,
     .write_callback = reg_pusher_rpm_write,
     .name = "pusher_rpm"},
    {.reg_id = REG_CAL_DATA_LIFTER_RPM,
     .reg_amount = 1,
     .read_callback = reg_lifter_rpm_read,
     .write_callback = reg_lifter_rpm_write,
     .name = "lifter_rpm"},
    {.reg_id = REG_CAL_DATA_HOME_RPM,
     .reg_amount = 1,
     .read_callback = reg_home_rpm_read,
     .write_callback = reg_home_rpm_write,
     .name = "home_rpm"},
    {.reg_id = REG_CAL_DATA_HOME_BACKOFF_STEPS,
     .reg_amount = 1,
     .read_callback = reg_home_backoff_steps_read,
     .write_callback = reg_home_backoff_steps_write,
     .name = "home_backoff_steps"},
    {.reg_id = REG_CAL_DATA_HOME_SETTLE_DELAY_MS,
     .reg_amount = 1,
     .read_callback = reg_home_settle_delay_ms_read,
     .write_callback = reg_home_settle_delay_ms_write,
     .name = "home_settle_delay_ms"},
    {.reg_id = REG_CAL_DATA_HOME_MAX_STEPS,
     .reg_amount = 1,
     .read_callback = reg_home_max_steps_read,
     .write_callback = reg_home_max_steps_write,
     .name = "home_max_steps"},
    {.reg_id = REG_CAL_DATA_SUPERVISOR_PERIOD_MS,
     .reg_amount = 1,
     .read_callback = reg_supervisor_period_ms_read,
     .write_callback = reg_supervisor_period_ms_write,
     .name = "supervisor_period_ms"},
    {.reg_id = REG_CAL_DATA_DEFAULT_MOVE_RPM,
     .reg_amount = 1,
     .read_callback = reg_default_move_rpm_read,
     .write_callback = reg_default_move_rpm_write,
     .name = "default_move_rpm"},
    {.reg_id = REG_CAL_DATA_DEFAULT_MOVE_STEPS,
     .reg_amount = 1,
     .read_callback = reg_default_move_steps_read,
     .write_callback = reg_default_move_steps_write,
     .name = "default_move_steps"},
    {.reg_id = REG_CAL_DATA_RECOUNT_TIMEOUT_MS,
     .reg_amount = 1,
     .read_callback = reg_recount_timeout_ms_read,
     .write_callback = reg_recount_timeout_ms_write,
     .name = "recount_timeout_ms"},
    {.reg_id = REG_CAL_DATA_LOCK_TIMEOUT_MS,
     .reg_amount = 1,
     .read_callback = reg_lock_timeout_ms_read,
     .write_callback = reg_lock_timeout_ms_write,
     .name = "lock_timeout_ms"},
    {.reg_id = REG_CAL_DATA_MOTION_TIMEOUT_MS,
     .reg_amount = 1,
     .read_callback = reg_motion_timeout_ms_read,
     .write_callback = reg_motion_timeout_ms_write,
     .name = "motion_timeout_ms"},
    {.reg_id = REG_CAL_DATA_DOOR_TIMEOUT_MS,
     .reg_amount = 1,
     .read_callback = reg_door_timeout_ms_read,
     .write_callback = reg_door_timeout_ms_write,
     .name = "door_timeout_ms"},
    {.reg_id = REG_CAL_DATA_STARTUP_SETTLE_DELAY_MS,
     .reg_amount = 1,
     .read_callback = reg_startup_settle_delay_ms_read,
     .write_callback = reg_startup_settle_delay_ms_write,
     .name = "startup_settle_delay_ms"},
    {.reg_id = REG_CAL_DATA_DOOR_DEBOUNCE_MS,
     .reg_amount = 1,
     .read_callback = reg_door_debounce_ms_read,
     .write_callback = reg_door_debounce_ms_write,
     .name = "door_debounce_ms"},
};

modbus_slave_t ui_slave = {0};
modbus_slave_params_t slave_params = {};
mb_holding_reg_def_t* regs[REG_ARRAY_LENGTH];
mb_holding_reg_array_t reg_array = {
    .regs = &regs[0],
    .length = 0,
    .max_length = REG_ARRAY_LENGTH,
};

// file scope to keep it off freertos task stack
static cal_data_params_t* params = NULL;
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
/**
 * @brief writes one cal_data u32 field from a u16 Modbus value and persists the whole block
 * @note stepper_system_update_configs() applies the new value to the axes without a power cycle.
 * @note a zero is repaired to its factory default by cal_data_sanitize_zeros() BEFORE the
 *       block is staged, so a rejected zero is never persisted and the axes never run with it.
 * @param field pointer to the cal_data_params_t field; must not be NULL
 * @param val value received over Modbus
 * @return bool true if the save was accepted into the w25q queue
 */
static bool reg_cal_u16_write(uint32_t* field, uint16_t val) {
  bool queued = false;

  if (field != NULL) {
    *field = (uint32_t)val;
    cal_data_sanitize_zeros();
    stepper_system_update_configs();
    queued = cal_data_save();
  }

  return queued;
}

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
  params = cal_data_get();

  (void)reg;

  if (val_ptr == NULL) {
    return -1;
  }

  *val_ptr = (uint16_t)params->push_retract_timeout_ms;
  return 0;
}

static int reg_push_retract_timeout_ms_write(uint16_t reg, uint16_t val) {
  params = cal_data_get();

  (void)reg;

  return reg_cal_u16_write(&params->push_retract_timeout_ms, val) ? 0 : -1;
}

static int reg_lift_timeout_ms_read(uint16_t reg, uint16_t* val_ptr) {
  params = cal_data_get();

  (void)reg;

  if (val_ptr == NULL) {
    return -1;
  }

  *val_ptr = (uint16_t)params->lift_timeout_ms;
  return 0;
}

static int reg_lift_timeout_ms_write(uint16_t reg, uint16_t val) {
  params = cal_data_get();

  (void)reg;

  return reg_cal_u16_write(&params->lift_timeout_ms, val) ? 0 : -1;
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
  params = cal_data_get();

  (void)reg;

  if (val_ptr == NULL) {
    return -1;
  }

  *val_ptr = (uint16_t)params->home_error_counts;
  return 0;
}

static int reg_home_threshold_write(uint16_t reg, uint16_t val) {
  params = cal_data_get();

  (void)reg;

  return reg_cal_u16_write(&params->home_error_counts, val) ? 0 : -1;
}

static int reg_load_offset_read(uint16_t reg, uint16_t* val_ptr) {
  params = cal_data_get();

  (void)reg;

  if (val_ptr == NULL) {
    return -1;
  }

  *val_ptr = (uint16_t)params->load_offset;
  return 0;
}

static int reg_load_offset_write(uint16_t reg, uint16_t val) {
  params = cal_data_get();

  (void)reg;

  return reg_cal_u16_write(&params->load_offset, val) ? 0 : -1;
}

static int reg_stall_threshold_read(uint16_t reg, uint16_t* val_ptr) {
  params = cal_data_get();

  (void)reg;

  if (val_ptr == NULL) {
    return -1;
  }

  *val_ptr = (uint16_t)params->stall_error_counts;
  return 0;
}

static int reg_stall_threshold_write(uint16_t reg, uint16_t val) {
  params = cal_data_get();

  (void)reg;

  return reg_cal_u16_write(&params->stall_error_counts, val) ? 0 : -1;
}

static int reg_patty1_thickness_read(uint16_t reg, uint16_t* val_ptr) {
  params = cal_data_get();

  (void)reg;

  if (val_ptr == NULL) {
    return -1;
  }

  *val_ptr = (uint16_t)params->patty_thickness_counts;
  return 0;
}

static int reg_patty1_thickness_write(uint16_t reg, uint16_t val) {
  params = cal_data_get();

  (void)reg;

  return reg_cal_u16_write(&params->patty_thickness_counts, val) ? 0 : -1;
}

static int reg_patty2_thickness_read(uint16_t reg, uint16_t* val_ptr) {
  params = cal_data_get();

  (void)reg;

  if (val_ptr == NULL) {
    return -1;
  }

  *val_ptr = (uint16_t)params->patty2_thickness_counts;
  return 0;
}

static int reg_patty2_thickness_write(uint16_t reg, uint16_t val) {
  params = cal_data_get();

  (void)reg;

  return reg_cal_u16_write(&params->patty2_thickness_counts, val) ? 0 : -1;
}

/**
 * @brief Read one cartridge's type as a read-only snapshot from the Modbus context.
 * @param reg Register id (unused — the slot is fixed by the per-register wrapper).
 * @param val_ptr Output: cartridge_type_t value (0 = empty, 1 = whopper, 2 = whopper JR).
 * @param slot_index Cartridge index into the app_task_get_cartridges() array (0-based).
 * @return 0 on success, -1 on invalid output pointer or out-of-range slot.
 */
static int reg_cartridge_type_read(uint16_t reg, uint16_t* val_ptr, uint8_t slot_index) {
  const cartridge_t* cartridges = NULL;

  (void)reg;

  if (val_ptr == NULL) {
    return -1;
  }

  cartridges = app_task_get_cartridges();
  if ((cartridges == NULL) || (slot_index >= APP_SLOT_COUNT)) {
    return -1;
  }

  *val_ptr = (uint16_t)cartridges[slot_index].type;
  return 0;
}

/**
 * @brief Read one cartridge's remaining patty count as a read-only snapshot.
 * @param reg Register id (unused — the slot is fixed by the per-register wrapper).
 * @param val_ptr Output: cartridge_t.remaining.
 * @param slot_index Cartridge index into the app_task_get_cartridges() array (0-based).
 * @return 0 on success, -1 on invalid output pointer or out-of-range slot.
 */
static int reg_cartridge_remaining_read(uint16_t reg, uint16_t* val_ptr, uint8_t slot_index) {
  const cartridge_t* cartridges = NULL;

  (void)reg;

  if (val_ptr == NULL) {
    return -1;
  }

  cartridges = app_task_get_cartridges();
  if ((cartridges == NULL) || (slot_index >= APP_SLOT_COUNT)) {
    return -1;
  }

  *val_ptr = cartridges[slot_index].remaining;
  return 0;
}

/**
 * @brief Read one cartridge's dispense queue depth as a read-only snapshot.
 * @param reg Register id (unused — the slot is fixed by the per-register wrapper).
 * @param val_ptr Output: cartridge_t.pending.
 * @param slot_index Cartridge index into the app_task_get_cartridges() array (0-based).
 * @return 0 on success, -1 on invalid output pointer or out-of-range slot.
 */
static int reg_cartridge_queue_read(uint16_t reg, uint16_t* val_ptr, uint8_t slot_index) {
  const cartridge_t* cartridges = NULL;

  (void)reg;

  if (val_ptr == NULL) {
    return -1;
  }

  cartridges = app_task_get_cartridges();
  if ((cartridges == NULL) || (slot_index >= APP_SLOT_COUNT)) {
    return -1;
  }

  *val_ptr = cartridges[slot_index].pending;
  return 0;
}

static int reg_cartridge1_status_read(uint16_t reg, uint16_t* val_ptr) {
  return reg_cartridge_type_read(reg, val_ptr, 0U);
}

static int reg_cartridge2_status_read(uint16_t reg, uint16_t* val_ptr) {
  return reg_cartridge_type_read(reg, val_ptr, 1U);
}

static int reg_cartridge3_status_read(uint16_t reg, uint16_t* val_ptr) {
  return reg_cartridge_type_read(reg, val_ptr, 2U);
}

static int reg_cartridge4_status_read(uint16_t reg, uint16_t* val_ptr) {
  return reg_cartridge_type_read(reg, val_ptr, 3U);
}

static int reg_cartridge1_remaining_read(uint16_t reg, uint16_t* val_ptr) {
  return reg_cartridge_remaining_read(reg, val_ptr, 0U);
}

static int reg_cartridge2_remaining_read(uint16_t reg, uint16_t* val_ptr) {
  return reg_cartridge_remaining_read(reg, val_ptr, 1U);
}

static int reg_cartridge3_remaining_read(uint16_t reg, uint16_t* val_ptr) {
  return reg_cartridge_remaining_read(reg, val_ptr, 2U);
}

static int reg_cartridge4_remaining_read(uint16_t reg, uint16_t* val_ptr) {
  return reg_cartridge_remaining_read(reg, val_ptr, 3U);
}

static int reg_cartridge1_queue_read(uint16_t reg, uint16_t* val_ptr) {
  return reg_cartridge_queue_read(reg, val_ptr, 0U);
}

static int reg_cartridge2_queue_read(uint16_t reg, uint16_t* val_ptr) {
  return reg_cartridge_queue_read(reg, val_ptr, 1U);
}

static int reg_cartridge3_queue_read(uint16_t reg, uint16_t* val_ptr) {
  return reg_cartridge_queue_read(reg, val_ptr, 2U);
}

static int reg_cartridge4_queue_read(uint16_t reg, uint16_t* val_ptr) {
  return reg_cartridge_queue_read(reg, val_ptr, 3U);
}

static int reg_door_switch_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  if (val_ptr == NULL) {
    return -1;
  }
  /* Live door state (raw pin level captured on the last EXTI edge): 0 = open, 1 = closed. */
  *val_ptr = (input_get_door_closed() == true) ? 1U : 0U;
  return 0;
}

static int reg_door_lock_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  if (val_ptr == NULL) {
    return -1;
  }
  /* Live lock state (raw pin level captured on the last poll): 0 = unlocked, 1 = locked. */
  *val_ptr = (input_get_lock_confirmed() == true) ? 1U : 0U;
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

static int reg_pusher_rpm_read(uint16_t reg, uint16_t* val_ptr) {
  params = cal_data_get();

  (void)reg;

  if (val_ptr == NULL) {
    return -1;
  }

  *val_ptr = (uint16_t)params->pusher_rpm;
  return 0;
}

static int reg_pusher_rpm_write(uint16_t reg, uint16_t val) {
  params = cal_data_get();

  (void)reg;

  return reg_cal_u16_write(&params->pusher_rpm, val) ? 0 : -1;
}

static int reg_lifter_rpm_read(uint16_t reg, uint16_t* val_ptr) {
  params = cal_data_get();

  (void)reg;

  if (val_ptr == NULL) {
    return -1;
  }

  *val_ptr = (uint16_t)params->lifter_rpm;
  return 0;
}

static int reg_lifter_rpm_write(uint16_t reg, uint16_t val) {
  params = cal_data_get();

  (void)reg;

  return reg_cal_u16_write(&params->lifter_rpm, val) ? 0 : -1;
}

static int reg_home_rpm_read(uint16_t reg, uint16_t* val_ptr) {
  params = cal_data_get();

  (void)reg;

  if (val_ptr == NULL) {
    return -1;
  }

  *val_ptr = (uint16_t)params->home_rpm;
  return 0;
}

static int reg_home_rpm_write(uint16_t reg, uint16_t val) {
  params = cal_data_get();

  (void)reg;

  return reg_cal_u16_write(&params->home_rpm, val) ? 0 : -1;
}

static int reg_home_backoff_steps_read(uint16_t reg, uint16_t* val_ptr) {
  params = cal_data_get();

  (void)reg;

  if (val_ptr == NULL) {
    return -1;
  }

  *val_ptr = (uint16_t)params->home_backoff_steps;
  return 0;
}

static int reg_home_backoff_steps_write(uint16_t reg, uint16_t val) {
  params = cal_data_get();

  (void)reg;

  return reg_cal_u16_write(&params->home_backoff_steps, val) ? 0 : -1;
}

static int reg_home_settle_delay_ms_read(uint16_t reg, uint16_t* val_ptr) {
  params = cal_data_get();

  (void)reg;

  if (val_ptr == NULL) {
    return -1;
  }

  *val_ptr = (uint16_t)params->home_settle_delay_ms;
  return 0;
}

static int reg_home_settle_delay_ms_write(uint16_t reg, uint16_t val) {
  params = cal_data_get();

  (void)reg;

  return reg_cal_u16_write(&params->home_settle_delay_ms, val) ? 0 : -1;
}

static int reg_home_max_steps_read(uint16_t reg, uint16_t* val_ptr) {
  params = cal_data_get();

  (void)reg;

  if (val_ptr == NULL) {
    return -1;
  }

  *val_ptr = (uint16_t)params->home_max_steps;
  return 0;
}

static int reg_home_max_steps_write(uint16_t reg, uint16_t val) {
  params = cal_data_get();

  (void)reg;

  return reg_cal_u16_write(&params->home_max_steps, val) ? 0 : -1;
}

static int reg_supervisor_period_ms_read(uint16_t reg, uint16_t* val_ptr) {
  params = cal_data_get();

  (void)reg;

  if (val_ptr == NULL) {
    return -1;
  }

  *val_ptr = (uint16_t)params->supervisor_period_ms;
  return 0;
}

static int reg_supervisor_period_ms_write(uint16_t reg, uint16_t val) {
  params = cal_data_get();

  (void)reg;

  return reg_cal_u16_write(&params->supervisor_period_ms, val) ? 0 : -1;
}

static int reg_default_move_rpm_read(uint16_t reg, uint16_t* val_ptr) {
  params = cal_data_get();

  (void)reg;

  if (val_ptr == NULL) {
    return -1;
  }

  *val_ptr = (uint16_t)params->default_move_rpm;
  return 0;
}

static int reg_default_move_rpm_write(uint16_t reg, uint16_t val) {
  params = cal_data_get();

  (void)reg;

  return reg_cal_u16_write(&params->default_move_rpm, val) ? 0 : -1;
}

static int reg_default_move_steps_read(uint16_t reg, uint16_t* val_ptr) {
  params = cal_data_get();

  (void)reg;

  if (val_ptr == NULL) {
    return -1;
  }

  *val_ptr = (uint16_t)params->default_move_steps;
  return 0;
}

static int reg_default_move_steps_write(uint16_t reg, uint16_t val) {
  params = cal_data_get();

  (void)reg;

  return reg_cal_u16_write(&params->default_move_steps, val) ? 0 : -1;
}

static int reg_recount_timeout_ms_read(uint16_t reg, uint16_t* val_ptr) {
  params = cal_data_get();

  (void)reg;

  if (val_ptr == NULL) {
    return -1;
  }

  /* Default 120000 ms exceeds u16: reads back truncated until unit scaling lands. */
  *val_ptr = (uint16_t)params->recount_timeout_ms;
  return 0;
}

static int reg_recount_timeout_ms_write(uint16_t reg, uint16_t val) {
  params = cal_data_get();

  (void)reg;

  return reg_cal_u16_write(&params->recount_timeout_ms, val) ? 0 : -1;
}

static int reg_lock_timeout_ms_read(uint16_t reg, uint16_t* val_ptr) {
  params = cal_data_get();

  (void)reg;

  if (val_ptr == NULL) {
    return -1;
  }

  *val_ptr = (uint16_t)params->lock_timeout_ms;
  return 0;
}

static int reg_lock_timeout_ms_write(uint16_t reg, uint16_t val) {
  params = cal_data_get();

  (void)reg;

  return reg_cal_u16_write(&params->lock_timeout_ms, val) ? 0 : -1;
}

static int reg_motion_timeout_ms_read(uint16_t reg, uint16_t* val_ptr) {
  params = cal_data_get();

  (void)reg;

  if (val_ptr == NULL) {
    return -1;
  }

  *val_ptr = (uint16_t)params->motion_timeout_ms;
  return 0;
}

static int reg_motion_timeout_ms_write(uint16_t reg, uint16_t val) {
  params = cal_data_get();

  (void)reg;

  return reg_cal_u16_write(&params->motion_timeout_ms, val) ? 0 : -1;
}

static int reg_door_timeout_ms_read(uint16_t reg, uint16_t* val_ptr) {
  params = cal_data_get();

  (void)reg;

  if (val_ptr == NULL) {
    return -1;
  }

  *val_ptr = (uint16_t)params->door_timeout_ms;
  return 0;
}

static int reg_door_timeout_ms_write(uint16_t reg, uint16_t val) {
  params = cal_data_get();

  (void)reg;

  return reg_cal_u16_write(&params->door_timeout_ms, val) ? 0 : -1;
}

static int reg_startup_settle_delay_ms_read(uint16_t reg, uint16_t* val_ptr) {
  params = cal_data_get();

  (void)reg;

  if (val_ptr == NULL) {
    return -1;
  }

  *val_ptr = (uint16_t)params->startup_settle_delay_ms;
  return 0;
}

static int reg_startup_settle_delay_ms_write(uint16_t reg, uint16_t val) {
  params = cal_data_get();

  (void)reg;

  return reg_cal_u16_write(&params->startup_settle_delay_ms, val) ? 0 : -1;
}

static int reg_door_debounce_ms_read(uint16_t reg, uint16_t* val_ptr) {
  params = cal_data_get();

  (void)reg;

  if (val_ptr == NULL) {
    return -1;
  }

  *val_ptr = (uint16_t)params->door_debounce_ms;
  return 0;
}

static int reg_door_debounce_ms_write(uint16_t reg, uint16_t val) {
  params = cal_data_get();

  (void)reg;

  /* input.c samples this once in input_init(); the new value takes effect on the next boot. */
  return reg_cal_u16_write(&params->door_debounce_ms, val) ? 0 : -1;
}

/**
 * @brief Read the error bitmask (sheet Register 18).
 *
 * Bits are LIVE, not latched: each bit reflects the current condition at read
 * time. The sheet leaves latching unspecified and there is no acknowledge/clear
 * register, so no bit is sticky.
 *
 * Bit layout (sheet bits b0-b3 kept as specified; undefined bits b4+ repurposed
 * for app faults):
 *   b0  Temp 1 Error          — 0: no sensor hardware/driver exists
 *   b1  Temp 2 Error          — 0: same
 *   b2  Cartridge 1&2 mismatch — lane type compare (cartridge_validate_lane_pairs logic)
 *   b3  Cartridge 3&4 mismatch — same
 *   b4  App fault active      — published app_sm_t.fault_code != 0
 *   b5  Cartridge 1 faulted   — cartridge_t.faulted
 *   b6  Cartridge 2 faulted   — same
 *   b7  Cartridge 3 faulted   — same
 *   b8  Cartridge 4 faulted   — same
 *   b9  Door open             — !input_get_door_closed()
 *   b10 Lock not confirmed    — !input_get_lock_confirmed()
 *   b11-b15 Reserved          — read 0
 *
 * @param reg Register id (unused).
 * @param val_ptr Output bitmask.
 * @return 0 on success, -1 on invalid output pointer.
 */
static int reg_error_bitmask_read(uint16_t reg, uint16_t* val_ptr) {
  const cartridge_t* cartridges = NULL;
  uint16_t mask = 0U;

  (void)reg;

  if (val_ptr == NULL) {
    return -1;
  }

  /* b0/b1: no temperature sensor hardware or driver exists — always 0. */

  /* b2/b3: lane type mismatch, same rule as cartridge_validate_lane_pairs():
   * two non-empty cartridges in a lane with different types. Read-only here —
   * the state machine owns faulting. The APP_SLOT_COUNT guards are compile-time:
   * with the current single-slot build (APP_SLOT_COUNT == 1) the comparisons
   * fold to false and both branches are optimized out; b3 additionally requires
   * all four slots to exist. */
  cartridges = app_task_get_cartridges();
  if ((cartridges != NULL) && (APP_SLOT_COUNT >= 2U)) {
    if ((cartridges[0].type != CARTRIDGE_TYPE_EMPTY) && (cartridges[1].type != CARTRIDGE_TYPE_EMPTY) &&
        (cartridges[0].type != cartridges[1].type)) {
      mask |= (1U << 2);
    }
    if ((APP_SLOT_COUNT >= 4U) && (cartridges[2].type != CARTRIDGE_TYPE_EMPTY) && (cartridges[3].type != CARTRIDGE_TYPE_EMPTY) &&
        (cartridges[2].type != cartridges[3].type)) {
      mask |= (1U << 3);
    }
  }

  /* b4: app fault active — published fault code (cache updated by
   * app_sm_port_publish_state on every state transition and dispatch). */
  if (app_sm_port_get_fault_code() != (uint16_t)APP_FAULT_CODE_NONE) {
    mask |= (1U << 4);
  }

  /* b5-b8: per-cartridge faulted flags. */
  if (cartridges != NULL) {
    for (uint8_t slot = 0U; slot < APP_SLOT_COUNT; slot++) {
      if (cartridges[slot].faulted == true) {
        mask |= (uint16_t)(1U << (5U + (uint16_t)slot));
      }
    }
  }

  /* b9/b10: live door and lock inputs. */
  if (input_get_door_closed() == false) {
    mask |= (1U << 9);
  }
  if (input_get_lock_confirmed() == false) {
    mask |= (1U << 10);
  }

  /* b11-b15: reserved, read 0. */

  *val_ptr = mask;
  return 0;
}

static int reg_clear_faults_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  /* Write-trigger register: reads always return 0. */
  if (val_ptr == NULL) {
    return -1;
  }
  *val_ptr = 0U;
  return 0;
}

static int reg_clear_faults_write(uint16_t reg, uint16_t val) {
  app_event_t clear_event;
  bool queued;

  (void)reg;

  if (val == 0U) {
    return 0;
  }

  /* Same clear as CLI 'test set clear 1': the state machine owns the axis
   * reset and cartridge flag reset. Callbacks run from the Modbus timer ISR,
   * so the ISR-safe post is required and no console logging is allowed.
   * APP_EV_FAULT_CLEARED is handled only in APP_FAULT; a write while healthy
   * is consumed and ignored by the other states, same as the CLI today. */
  clear_event.id = APP_EV_FAULT_CLEARED;
  clear_event.slot = APP_NO_SLOT;
  clear_event.product_type = 0U;
  clear_event.value = 0U;
  clear_event.axis_num = 0U;

  queued = app_task_post_from_isr(&clear_event);

  if (queued == false) {
    /* Event queue full or not yet created: the request was dropped. */
    return -1;
  }

  return 0;
}
