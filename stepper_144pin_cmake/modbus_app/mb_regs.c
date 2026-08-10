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

#include "modbus_config.h"
#include "modbus_port.h"
#include "modbus_slave.h"
#include "app_console.h"
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

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/
static mb_holding_reg_def_t regs_defines[] = {
    {.reg_id = REG_PATTY1_ADD_TO_QUEUE,
     .read_callback = reg_patty1_add_to_queue_read,
     .write_callback = reg_patty1_add_to_queue_write,
     .name = "patty1_add_to_queue"},
    {.reg_id = REG_PATTY2_ADD_TO_QUEUE,
     .read_callback = reg_patty2_add_to_queue_read,
     .write_callback = reg_patty2_add_to_queue_write,
     .name = "patty2_add_to_queue"},
};

static int fw_ver_reg_read(uint16_t reg, uint16_t* val_ptr);
mb_holding_reg_def_t fw_ver_reg = {
    .reg_id = REG_SLAVE_FW_VERSION,
    .reg_amount = 1,
    .read_callback = fw_ver_reg_read,
    .name = "fw_ver",
};

static int serial_number_reg_read(uint16_t reg, uint16_t* val_ptr);
mb_holding_reg_def_t serial_number_reg = {
    .reg_id = REG_SLAVE_SERIAL_NUMBER,
    .reg_amount = 1,
    .read_callback = serial_number_reg_read,
    .name = "serial_number",
};

static int self_test_mode_reg_read(uint16_t reg, uint16_t* val_ptr);
static int self_test_mode_reg_write(uint16_t reg, uint16_t val);
mb_holding_reg_def_t self_test_mode_reg = {
    .reg_id = REG_SLAVE_UI_ENTER_SELF_TEST_MODE,
    .reg_amount = 1,
    .read_callback = self_test_mode_reg_read,
    .write_callback = self_test_mode_reg_write,
    .name = "self_test_mode",
};

static int self_test_results_reg_read(uint16_t reg, uint16_t* val_ptr);
mb_holding_reg_def_t self_test_results_reg = {
    .reg_id = REG_SLAVE_UI_SELF_TEST_RESULTS,
    .reg_amount = 1,
    .read_callback = self_test_results_reg_read,
    .name = "self_test_results",
};

static int sound_buzzer_reg_write(uint16_t reg, uint16_t val);
mb_holding_reg_def_t sound_buzzer_reg = {
    .reg_id = REG_SLAVE_UI_SOUND_BUZZER,
    .reg_amount = 1,
    .write_callback = sound_buzzer_reg_write,
    .name = "sound_buzzer",
};

static int show_black_pixels_reg_write(uint16_t reg, uint16_t val);
mb_holding_reg_def_t show_black_pixels_reg = {
    .reg_id = REG_SLAVE_UI_SHOW_BLACK_PIXELS,
    .reg_amount = 1,
    .write_callback = show_black_pixels_reg_write,
    .name = "show_black_pixels",
};

static int reg_push_retract_timeout_ms_read(uint16_t reg, uint16_t* val_ptr);
static int reg_push_retract_timeout_ms_write(uint16_t reg, uint16_t val);
mb_holding_reg_def_t push_retract_timeout_ms_reg = {
    .reg_id = REG_CAL_DATA_PUSH_RETRACT_TIMEOUT_MS,
    .reg_amount = 1,
    .read_callback = reg_push_retract_timeout_ms_read,
    .write_callback = reg_push_retract_timeout_ms_write,
    .name = "push_retract_timeout_ms",
};

static int reg_lift_timeout_ms_read(uint16_t reg, uint16_t* val_ptr);
static int reg_lift_timeout_ms_write(uint16_t reg, uint16_t val);
mb_holding_reg_def_t lift_timeout_ms_reg = {
    .reg_id = REG_CAL_DATA_LIFT_TIMEOUT_MS,
    .reg_amount = 1,
    .read_callback = reg_lift_timeout_ms_read,
    .write_callback = reg_lift_timeout_ms_write,
    .name = "lift_timeout_ms",
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

  MB_AddHoldingRegister(&reg_array, &fw_ver_reg);
  MB_AddHoldingRegister(&reg_array, &serial_number_reg);
  MB_AddHoldingRegister(&reg_array, &self_test_mode_reg);
  MB_AddHoldingRegister(&reg_array, &self_test_results_reg);
  MB_AddHoldingRegister(&reg_array, &sound_buzzer_reg);
  MB_AddHoldingRegister(&reg_array, &show_black_pixels_reg);
  MB_AddHoldingRegister(&reg_array, &push_retract_timeout_ms_reg);
  MB_AddHoldingRegister(&reg_array, &lift_timeout_ms_reg);

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

static int self_test_mode_reg_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  *val_ptr = 0x77;
  return 0;
}
static int self_test_mode_reg_write(uint16_t reg, uint16_t val) {
  (void)reg;
  app_console_print("[INFO] Entering self test mode via modbus register write. val: %d\r\n", val);
  return 0;
}
static int self_test_results_reg_read(uint16_t reg, uint16_t* val_ptr) {
  (void)reg;
  *val_ptr = 0x66;
  return 0;
}

static int sound_buzzer_reg_write(uint16_t reg, uint16_t val) {
  (void)reg;
  app_console_print("[INFO] Sound buzzer via modbus register write. val: %d\r\n", val);
  return 0;
}

static int show_black_pixels_reg_write(uint16_t reg, uint16_t val) {
  (void)reg;
  app_console_print("[INFO] Show black pixels via modbus register write. val: %d\r\n", val);
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
