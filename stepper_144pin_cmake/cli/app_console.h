/**
 * @file app_console.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief
 * @version 0.1
 * @date 2026-06-23
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef APP_CONSOLE_H_
#define APP_CONSOLE_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include <stdint.h>

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

#define CONSOLE_TX_QUEUE_DEPTH 16   /* number of messages in flight      */
#define CONSOLE_TX_MSG_MAX_LEN 128  /* max bytes per formatted message   */
#define CONSOLE_CLI_MAX_LINE_LEN 80 /* max chars in one CLI input line   */
#define CONSOLE_CLI_MAX_ARGS 8      /* max tokens per command            */
#define CONSOLE_CLI_MAX_COMMANDS 32 /* max registered commands           */
#define CONSOLE_TX_TASK_STACK 256   /* words                             */
#define CONSOLE_RX_TASK_STACK 256   /* words                             */
#define CONSOLE_TX_TASK_PRIORITY 1  /* low — beneath app tasks           */
#define CONSOLE_RX_TASK_PRIORITY 2

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

typedef void (*console_command_fn_t)(int32_t argc, char* argv[]);
typedef void (*console_command_get_fn_t)(char* param);
typedef void (*console_command_set_fn_t)(char* param, int32_t val);
typedef void (*console_command_list_fn_t)(void);

typedef struct {
  const char* name;                  /* first token that triggers this handler  */
  const char* help;                  /* shown by the built-in "help" command    */
  console_command_fn_t handler;      /* top-level dispatcher                    */
  console_command_get_fn_t get_fn;   /* optional: handles "get <param>"         */
  console_command_set_fn_t set_fn;   /* optional: handles "set <param> <val>"   */
  console_command_list_fn_t list_fn; /* optional: handles "list"                */
} console_command_t;

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

/**
 * @brief Initialise the console: UART port, TX queue, and RX/TX tasks.
 */
void app_console_init(void);

/**
 * @brief Format and enqueue a message for transmission on the console UART.
 * @param fmt printf-style format string.
 * @param ... Format arguments.
 */
void app_console_print(const char* fmt, ...);

/**
 * @brief Register a command handler with the CLI.
 * @param cmd Pointer to a statically allocated console_command_t descriptor.
 */
void app_console_register_command(const console_command_t* cmd);

/**
 * @brief Register all application command groups with the console.
 */
void app_console_commands_register(void);

#endif /* APP_CONSOLE_H_ */
