# app_console — FreeRTOS debug console + CLI

MCU-agnostic debug console and CLI for FreeRTOS projects. The core module has no hardware dependencies — all UART access is isolated behind a three-function port interface in `app_console_port.h`. An STM32 HAL reference port is included; porting to any other MCU means writing one new `.c` file.

* TX path: `app_console_print()` → queue → TX task → UART.
* RX path: interrupt-driven RX task collects chars → line buffer → tokenise → dispatch.

## Files

```
app_console.h                 Public API — include this everywhere
app_console_port.h            Port interface — three functions your port must implement
app_console.c                 Queue, TX task, RX task, CLI dispatcher (no HAL calls)
app_console_port_stm32.c      Reference port: STM32 HAL, interrupt-driven RX
app_console_commands.c        Registers command groups with the console
app_console_commands.h        Public declaration of app_console_commands_register()
cal_data_example.c/.h         Example calibration parameter store (param command)
cli_test_example.c/.h         Example test runner (test command)
```

## Dependencies

- **FreeRTOS** — tasks, queues, and a binary semaphore are used in the core module.
- **A UART peripheral** — accessed only through your port implementation.

The core module (`app_console.c`) includes only `FreeRTOS.h`, `queue.h`, `task.h`, and `semphr.h`. No vendor HAL headers appear outside the port file.

## Setup — STM32 + CubeMX

1. Copy all files from this directory into your project.
2. In CubeMX: enable USART2, set mode to Asynchronous, 115200 8N1.
   **Enable the global interrupt for USART2** — the port uses interrupt-driven RX
   (`HAL_UART_Receive_IT`) with a FreeRTOS binary semaphore.
3. In `main.c`, after all MX_ inits and before `vTaskStartScheduler()`:

```c
#include "app_console.h"

// in main(), after mx_system_init():
app_console_commands_register();   // register param, test, etc.
app_console_init();                // creates TX + RX tasks
vTaskStartScheduler();
```

### Retargeting to a different UART on STM32

Only two lines in `app_console_port_stm32.c` need to change:

```c
#include "mx_usart1.h"                              // was mx_usart2.h
#define CONSOLE_UART_GETHANDLE  mx_usart1_uart_gethandle  // was mx_usart2_uart_gethandle
```

Enable the global interrupt for the new peripheral in CubeMX. Everything else is unchanged.

## Using from application tasks

```c
#include "app_console.h"

// From any FreeRTOS task — non-blocking, safe to call anywhere:
app_console_print("sensor val: %d mV\r\n", reading);
app_console_print("state -> %s\r\n", state_name);
```

## Optional: retarget printf

Note: This has never been tested

Add this to ONE `.c` file in your project (not in this module):

```c
#include "app_console.h"
int _write(int fd, char *ptr, int len)
{
    (void)fd;
    app_console_print("%.*s", len, ptr);
    return len;
}
```

After this, `printf(...)` routes through the queue automatically.

## Porting to a new MCU

Create a new `app_console_port_<mcu>.c` that implements the three functions declared in `app_console_port.h`. Delete (or exclude from your build) the STM32 port file. No other files change.

```c
// The three functions every port must implement:

void    console_port_init(void);
int32_t console_port_transmit(const uint8_t *buf, uint16_t len, uint32_t timeout_ms);
int32_t console_port_receive(uint8_t *byte, uint32_t timeout_ms);
```

The general pattern is the same regardless of MCU:

1. **`console_port_init`** — create a FreeRTOS binary semaphore for RX synchronisation and enable the UART peripheral and its RX interrupt.
2. **`console_port_transmit`** — blocking write of `len` bytes; return `0` on success, `-1` on error or timeout.
3. **`console_port_receive`** — arm a one-byte interrupt-driven receive, block on the semaphore for up to `timeout_ms`, store the byte, return `0` on success or `-1` on timeout/error.

### Example port — ATSAMD/E (ASF4 USART async driver)

This shows the pattern for a Microchip SAM D/E series device using the Atmel Start / ASF4 `usart_async` driver. `UART_0` is the ASF4-generated descriptor for your chosen SERCOM instance.

```c
#include "app_console_port.h"
#include "driver_init.h"    /* ASF4 generated — provides UART_0 */
#include "FreeRTOS.h"
#include "semphr.h"

static SemaphoreHandle_t s_rxDoneSem;

/* Called from SERCOM IRQ context when a byte arrives in the driver ring buffer. */
static void rx_cb(const struct usart_async_descriptor *const descr)
{
    (void)descr;
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(s_rxDoneSem, &woken);
    portYIELD_FROM_ISR(woken);
}

void console_port_init(void)
{
    s_rxDoneSem = xSemaphoreCreateBinary();
    configASSERT(s_rxDoneSem != NULL);

    usart_async_register_callback(&UART_0, USART_ASYNC_RXC_CB, rx_cb);
    usart_async_enable(&UART_0);
}

int32_t console_port_transmit(const uint8_t *buf, uint16_t len, uint32_t timeout_ms)
{
    (void)timeout_ms;
    return (io_write(&UART_0.io, buf, len) == (int32_t)len) ? 0 : -1;
}

int32_t console_port_receive(uint8_t *byte, uint32_t timeout_ms)
{
    TickType_t ticks = (timeout_ms == portMAX_DELAY)
                       ? portMAX_DELAY
                       : pdMS_TO_TICKS(timeout_ms);

    if (xSemaphoreTake(s_rxDoneSem, ticks) != pdTRUE) {
        return -1;
    }
    return (io_read(&UART_0.io, byte, 1) == 1) ? 0 : -1;
}
```

The same pattern applies to any RTOS-capable MCU: arm an interrupt, give a semaphore from the ISR, take it from `console_port_receive`.

## CLI usage (terminal at 115200)

```
> help
Available commands:
  help             List available commands
  param            [set/get/list/reset] [param] [value]   get/set/list/reset calibration parameters
  test             [set/get/list] [test] [value]   run/get/list tests

> param list
CAL parameters:
  1: gain             = 100
  2: motor_speed      = 500
  3: on_time          = 200

> param get gain
param get: gain = 100

> param get 1
param get: gain = 100

> param set gain 200
param set: gain = 200

> param reset
Factory reset complete.

> test list
Available tests:
  0: motor_speed
  1: rs232_loopback
  2: spi_flash

> test set motor_speed 300
motor_speed: running at 300

> test get motor_speed
test get: motor_speed last_result = 300
```

`param get` accepts either the parameter name or its numeric id from `param list`.

## Adding a new command group

1. Create a handler module (e.g., `my_cmd.c` / `my_cmd.h`) that implements three functions
   matching the `console_command_get_fn_t`, `console_command_set_fn_t`, and
   `console_command_list_fn_t` typedefs from `app_console.h`.

2. In `app_console_commands.c`, declare a static command descriptor and register it:

```c
#include "my_cmd.h"

static const console_command_t s_myCmd = {
    .name    = "my_cmd",
    .help    = "[set/get/list] [name] [value]\t description",
    .handler = cmd_my_cmd_handler,
    .get_fn  = my_cmd_get_handler,
    .set_fn  = my_cmd_set_handler,
    .list_fn = my_cmd_list_handler,
};

void app_console_commands_register(void) {
    app_console_register_command(&s_paramCmd);
    app_console_register_command(&s_testCmd);
    app_console_register_command(&s_myCmd);   // add here
}
```

3. Write a top-level handler that delegates to `cmd_dispatch()` (see `cmd_param_handler`
   in `app_console_commands.c` as the reference pattern).

## Configuration

All tunable constants are at the top of `app_console.h`:

| Constant                   | Default | Purpose                          |
|----------------------------|---------|----------------------------------|
| `CONSOLE_TX_QUEUE_DEPTH`   | 16      | Messages buffered before drop    |
| `CONSOLE_TX_MSG_MAX_LEN`   | 128     | Max bytes per formatted message  |
| `CONSOLE_CLI_MAX_LINE_LEN` | 80      | Max chars in one CLI input line  |
| `CONSOLE_CLI_MAX_ARGS`     | 8       | Max tokens per command           |
| `CONSOLE_CLI_MAX_COMMANDS` | 32      | Max registered commands          |
| `CONSOLE_TX_TASK_STACK`    | 256     | TX task stack size (words)       |
| `CONSOLE_RX_TASK_STACK`    | 256     | RX task stack size (words)       |
| `CONSOLE_TX_TASK_PRIORITY` | 1       | TX task priority (low)           |
| `CONSOLE_RX_TASK_PRIORITY` | 2       | RX task priority                 |
