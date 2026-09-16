/**
 * @file cli_test_example.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief Example test runner module. Exposes CLI handler functions for use
 *        as console_command_t function pointers.
 * @version 0.1
 * @date 2026-06-23
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef CLI_TEST_EXAMPLE_H_
#define CLI_TEST_EXAMPLE_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include <stdint.h>

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

typedef enum { TEST_MOTOR_SPEED = 0, TEST_RS232_LOOPBACK, TEST_SPI_FLASH, TEST_NUM_PARAMS } test_param_enum;

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

/**
 * @brief CLI get handler — prints the last result of a test.
 * @param param Test name string.
 */
void clitest_get_handler(char* param);

/**
 * @brief CLI set handler — runs a test with the given value.
 * @param param Test name string.
 * @param val Value to pass to the test (e.g. speed for motor_speed).
 */
void clitest_set_handler(char* param, int32_t val);

/**
 * @brief CLI list handler — prints all available test names.
 */
void clitest_list_handler(void);

#endif /* CLI_TEST_EXAMPLE_H_ */
