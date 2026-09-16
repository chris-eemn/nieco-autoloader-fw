/**
 * @file cli_test_example.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief Example test runner module with CLI handler functions.
 *        Tests: motor_speed, rs232_loopback, spi_flash.
 * @version 0.1
 * @date 2026-06-23
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include "cli_test_example.h"
#include "app_console.h"
#include <string.h>

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

typedef struct {
  const char* name;
  int32_t last_result;
} test_entry_t;

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

static test_entry_t s_tests[TEST_NUM_PARAMS] = {
    [TEST_MOTOR_SPEED] = {"motor_speed", 0},
    [TEST_RS232_LOOPBACK] = {"rs232_loopback", 0},
    [TEST_SPI_FLASH] = {"spi_flash", 0},
};

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

static test_param_enum lookup_test(const char* name);
static void run_motor_speed(int32_t speed);
static void run_rs232_loopback(void);
static void run_spi_flash(void);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

void clitest_set_handler(char* param, int32_t val) {
  switch (lookup_test(param)) {
    case TEST_MOTOR_SPEED:
      run_motor_speed(val);
      break;
    case TEST_RS232_LOOPBACK:
      run_rs232_loopback();
      break;
    case TEST_SPI_FLASH:
      run_spi_flash();
      break;
    default:
      app_console_print("Unknown test: %s\r\n", param);
      break;
  }
}

void clitest_get_handler(char* param) {
  test_param_enum t = lookup_test(param);

  if (t >= TEST_NUM_PARAMS) {
    app_console_print("Unknown test: %s\r\n", param);
    return;
  }

  app_console_print("test get: %s last_result = %d\r\n", s_tests[t].name, s_tests[t].last_result);
}

void clitest_list_handler(void) {
  app_console_print("Available tests:\r\n");
  for (int32_t i = 0; i < TEST_NUM_PARAMS; i++) {
    app_console_print("  %d: %s\r\n", i, s_tests[i].name);
  }
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/

/**
 * @brief Look up a test by name string.
 * @param name Test name string.
 * @return The matching enum value, or TEST_NUM_PARAMS if not found.
 */
static test_param_enum lookup_test(const char* name) {
  for (int32_t i = 0; i < TEST_NUM_PARAMS; i++) {
    if (strcmp(s_tests[i].name, name) == 0)
      return (test_param_enum)i;
  }
  return TEST_NUM_PARAMS;
}

/**
 * @brief Run the motor speed test at the given speed.
 * @param speed Commanded speed value.
 */
static void run_motor_speed(int32_t speed) {
  /* TODO: call BSP/HAL to command motor speed */
  s_tests[TEST_MOTOR_SPEED].last_result = speed;
  app_console_print("motor_speed: running at %d\r\n", speed);
}

/**
 * @brief Run the RS-232 loopback test.
 */
static void run_rs232_loopback(void) {
  /* TODO: transmit test pattern and verify echo */
  s_tests[TEST_RS232_LOOPBACK].last_result = 0; /* 0 = PASS placeholder */
  app_console_print("rs232_loopback: PASS (stub)\r\n");
}

/**
 * @brief Run the SPI flash read/write verification test.
 */
static void run_spi_flash(void) {
  /* TODO: write/read/verify a sector */
  s_tests[TEST_SPI_FLASH].last_result = 0; /* 0 = PASS placeholder */
  app_console_print("spi_flash: PASS (stub)\r\n");
}
