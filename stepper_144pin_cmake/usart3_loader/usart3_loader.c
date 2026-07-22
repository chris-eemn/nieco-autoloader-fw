/**
 * @file usart3_loader.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief
 * @version 0.1
 * @date 2026-07-22
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "usart3_loader.h"
#include "usart3_stream.h"
#include "btl_flash_trigger.h"
#include "update_image.h"
#include "app_console.h"

#include "FreeRTOS.h"
#include "task.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
#define USART3_LOADER_TASK_STACK 256U /* words */

/* Above the stepper task (idle+1) so incoming USART3 bytes are serviced promptly, but leaves
 * the console TX task (idle+1) to flush queued status lines whenever the loader is idle-
 * yielding between polls. */
#define USART3_LOADER_TASK_PRIORITY (tskIDLE_PRIORITY + 2U)

/* Idle poll cadence: usart3_stream_receive_update() returns immediately when no transfer is
 * in progress, so the task yields for this long between peeks to leave the CPU to the rest
 * of the system. Once a transfer starts, the stream receiver blocks through to completion. */
#define USART3_LOADER_IDLE_POLL_MS 1U

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/
/**
 * @brief FreeRTOS task body: reconfigures USART3 for the loader, then polls the stream
 *        receiver forever, staging any incoming image into SPI flash
 * @param pv_parameters unused
 */
static void usart3_loader_task(void *pv_parameters);

/**
 * @brief validates the freshly-staged image's CRC and, if it matches, arms the bootloader
 *        commit trigger and resets into the bootloader (does not return on success)
 * @note called only after a complete image has been staged. On a CRC mismatch or a failed
 *       arm it reports over the console and returns so the loader keeps listening.
 */
static void validate_and_commit(void);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/
void usart3_loader_start(void) {
  BaseType_t task_ret = xTaskCreate(usart3_loader_task,
                                    "Usart3Loader",
                                    USART3_LOADER_TASK_STACK,
                                    NULL,
                                    USART3_LOADER_TASK_PRIORITY,
                                    NULL);
  configASSERT(task_ret == pdPASS);
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/
static void usart3_loader_task(void *pv_parameters) {
  (void)pv_parameters;

  if (usart3_stream_init() == false) {
    app_console_print("[ERROR] USART3 loader: failed to configure USART3.\r\n");
  } else {
    app_console_print("[INFO] USART3 bin-loader active @115200. Modbus slave disabled.\r\n");
  }

  for (;;) {
    if (usart3_stream_receive_update() == true) {
      /* A complete image is now staged in SPI flash: validate its CRC, and on a match arm
       * the bootloader trigger and reset into the bootloader (does not return on success). */
      validate_and_commit();
    }

    vTaskDelay(pdMS_TO_TICKS(USART3_LOADER_IDLE_POLL_MS));
  }
}

static void validate_and_commit(void) {
  uint16_t expected_crc = 0u;
  uint16_t computed_crc = 0u;

  if (usart3_stream_validate_staged_crc(&expected_crc, &computed_crc) == false) {
    app_console_print("[ERROR] USART3 loader: could not read back staged image for CRC check; not committing.\r\n");
  } else if (expected_crc != computed_crc) {
    app_console_print("[ERROR] USART3 loader: staged image CRC mismatch (expected 0x%04X, got 0x%04X); "
                      "not committing.\r\n",
                      expected_crc, computed_crc);
  } else {
    app_console_print("[INFO] USART3 loader: staged image CRC valid (0x%04X). Committing and resetting into "
                      "bootloader.\r\n",
                      computed_crc);

    /* Arms the trigger flag in SPI flash and resets -- does not return on success. */
    if (btl_flash_trigger_arm_and_reset(STAGED_HEADER_OFFSET) == false) {
      app_console_print("[ERROR] USART3 loader: commit failed (could not arm trigger); staying in application.\r\n");
    }
  }
}
