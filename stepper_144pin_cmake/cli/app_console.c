/**
 * @file app_console.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief Drop-in debug console and CLI for STM32 FreeRTOS projects.
 *        TX path: app_console_print() -> queue -> TX task -> UART.
 *        RX path: RX task collects chars -> line buffer -> tokenise -> dispatch.
 * @version 0.1
 * @date 2026-06-23
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include "app_console.h"
#include "app_console_port.h"

#include "stm32_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

typedef struct {
  char data[CONSOLE_TX_MSG_MAX_LEN];
  uint16_t len;
} tx_message_t;

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

static void console_tx_task(void* arg);
static void console_rx_task(void* arg);
static void console_dispatch(char* line);
static void console_tokenise(char* line, int32_t* argc, char* argv[]);
static void console_cmd_help(int32_t argc, char* argv[]);
static void app_console_write_raw(const char* fmt, ...);

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

static QueueHandle_t s_txQueue;

static const console_command_t* s_commands[CONSOLE_CLI_MAX_COMMANDS];
static int32_t s_commandCount = 0;

static const console_command_t s_helpCmd = {
    .name = "help",
    .help = "List available commands",
    .handler = console_cmd_help,
};

static tx_message_t msg;
/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

void app_console_init(void) {
  console_port_init();

  s_txQueue = xQueueCreate(CONSOLE_TX_QUEUE_DEPTH, sizeof(tx_message_t));
  configASSERT(s_txQueue != NULL);

  app_console_register_command(&s_helpCmd);

  xTaskCreate(console_tx_task, "Con_TX", CONSOLE_TX_TASK_STACK, NULL, CONSOLE_TX_TASK_PRIORITY, NULL);

  xTaskCreate(console_rx_task, "Con_RX", CONSOLE_RX_TASK_STACK, NULL, CONSOLE_RX_TASK_PRIORITY, NULL);
}

void app_console_print(const char* fmt, ...) {
  va_list args;
  int32_t n;
  int32_t prefix_len;

  prefix_len = snprintf(msg.data, sizeof(msg.data), "[%lu] ", (unsigned long)xTaskGetTickCount());
  if ((prefix_len < 0) || ((size_t)prefix_len >= sizeof(msg.data))) {
    return;
  }

  va_start(args, fmt);
  n = (int32_t)vsnprintf(&msg.data[prefix_len], sizeof(msg.data) - (size_t)prefix_len, fmt, args);
  va_end(args);

  if (n <= 0)
    return;

  n += prefix_len;

  msg.len = (uint16_t)(n < CONSOLE_TX_MSG_MAX_LEN ? n : CONSOLE_TX_MSG_MAX_LEN - 1);
  int in_isr = (__get_IPSR() != 0U);
  if (in_isr) {
    BaseType_t higher_priority_task_woken = pdFALSE;

    (void)xQueueSendFromISR(s_txQueue,
                            &msg,
                            &higher_priority_task_woken);

    portYIELD_FROM_ISR(higher_priority_task_woken);
  }
  else {
    xQueueSend(s_txQueue, &msg, 0);
  }
}

void app_console_register_command(const console_command_t* cmd) {
  if (s_commandCount < CONSOLE_CLI_MAX_COMMANDS) {
    s_commands[s_commandCount++] = cmd;
  }
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/

/**
 * @brief FreeRTOS task that dequeues TX messages and transmits them over the UART.
 * @param arg Unused task parameter.
 */
static void console_tx_task(void* arg) {
  (void)arg;
  for (;;) {
    if (xQueueReceive(s_txQueue, &msg, portMAX_DELAY) == pdTRUE) {
      console_port_transmit((uint8_t*)msg.data, msg.len, 1000);
    }
  }
}

/**
 * @brief FreeRTOS task that collects incoming characters and dispatches complete lines.
 * @param arg Unused task parameter.
 */
static void console_rx_task(void* arg) {
  (void)arg;
  static char line[CONSOLE_CLI_MAX_LINE_LEN];
  uint16_t pos = 0;
  uint8_t ch;

  app_console_write_raw("\r\n> ");

  for (;;) {
    if (console_port_receive(&ch, portMAX_DELAY) != 0) {
      continue;
    }

    if (ch == '\r' || ch == '\n') {
      app_console_write_raw("\r\n");

      if (pos > 0) {
        line[pos] = '\0';
        console_dispatch(line);
        pos = 0;
      }

      app_console_write_raw("> ");
    }
    else if (ch == 0x7F || ch == '\b') {
      if (pos > 0) {
        pos--;
        app_console_write_raw("\b \b");
      }
    }
    else if (pos < (CONSOLE_CLI_MAX_LINE_LEN - 1)) {
      line[pos++] = (char)ch;
      app_console_write_raw("%c", ch);
    }
  }
}

/**
 * @brief Tokenise a line and dispatch it to the matching registered command handler.
 * @param line Null-terminated input line; modified in place during tokenisation.
 */
static void console_dispatch(char* line) {
  char* argv[CONSOLE_CLI_MAX_ARGS];
  int32_t argc = 0;

  console_tokenise(line, &argc, argv);

  if (argc == 0)
    return;

  for (int32_t i = 0; i < s_commandCount; i++) {
    if (strcmp(argv[0], s_commands[i]->name) == 0) {
      s_commands[i]->handler(argc, argv);
      return;
    }
  }

  app_console_print("Unknown command: '%s'  (type 'help')\r\n", argv[0]);
}

/**
 * @brief Split a whitespace-delimited line into tokens in place.
 * @param line  Input string; spaces/tabs are replaced with '\0'.
 * @param argc  Output: number of tokens found.
 * @param argv  Output: array of pointers to each token.
 */
static void console_tokenise(char* line, int32_t* argc, char* argv[]) {
  *argc = 0;
  char* p = line;

  while ((*p != '\0') && (*argc < CONSOLE_CLI_MAX_ARGS)) {
    while ((*p == ' ') || (*p == '\t'))
      p++;
    if (*p == '\0')
      break;

    argv[(*argc)++] = p;

    while ((*p != '\0') && (*p != ' ') && (*p != '\t'))
      p++;
    if (*p != '\0')
      *p++ = '\0';
  }
}

/**
 * @brief Built-in "help" command — prints all registered commands and their help text.
 * @param argc Unused.
 * @param argv Unused.
 */
static void console_cmd_help(int32_t argc, char* argv[]) {
  (void)argc;
  (void)argv;
  app_console_print("Available commands:\r\n");
  for (int32_t i = 0; i < s_commandCount; i++) {
    app_console_print("  %-16s %s\r\n", s_commands[i]->name, s_commands[i]->help);
  }
}

static void app_console_write_raw(const char* fmt, ...) {
  tx_message_t msg;
  va_list args;
  int32_t n;

  va_start(args, fmt);
  n = (int32_t)vsnprintf(msg.data, sizeof(msg.data), fmt, args);
  va_end(args);

  if (n <= 0)
    return;

  msg.len = (uint16_t)(n < CONSOLE_TX_MSG_MAX_LEN ? n : CONSOLE_TX_MSG_MAX_LEN - 1);

  xQueueSend(s_txQueue, &msg, 0);
}
