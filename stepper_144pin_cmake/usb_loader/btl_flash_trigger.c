/**
 * @file btl_flash_trigger.c
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
#include "btl_flash_trigger.h"
#include "spi_flash_io.h"
#include "stm32_hal.h"
#include "w25q_config.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
/* These four values define the on-flash trigger layout and MUST match the bootloader's copy
 * of btl_flash_trigger.c exactly -- the bootloader reads back what this writes. */
#define BTL_TRIGGER_PATTERN 0xB007CAFEu

/** First W25Q sector, reserved solely for this flag -- STAGED_HEADER_OFFSET (see
 * update_image.h) starts right after it, so staging a new image (which only ever
 * erases/writes starting at STAGED_HEADER_OFFSET) never disturbs this flag. The region base
 * itself is defined by the chip-wide address map in driver_w25q_port/w25q_config.h. */
#define BTL_FLASH_TRIGGER_FLAG_OFFSET (W25Q_BTL_TRIGGER_BASE_ADDRESS)

#define BTL_TRIGGER_WORD_PATTERN_0 0u
#define BTL_TRIGGER_WORD_STAGED_OFFSET 1u
#define BTL_TRIGGER_WORD_PATTERN_2 2u
#define BTL_TRIGGER_WORD_PATTERN_3 3u
#define BTL_TRIGGER_WORD_COUNT 4u
#define BTL_TRIGGER_FLAG_SIZE_BYTES (BTL_TRIGGER_WORD_COUNT * sizeof(uint32_t))

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/
bool btl_flash_trigger_arm_and_reset(uint32_t staged_header_offset) {
  uint32_t flag_words[BTL_TRIGGER_WORD_COUNT];
  bool armed_ok;

  flag_words[BTL_TRIGGER_WORD_PATTERN_0] = BTL_TRIGGER_PATTERN;
  flag_words[BTL_TRIGGER_WORD_STAGED_OFFSET] = staged_header_offset;
  flag_words[BTL_TRIGGER_WORD_PATTERN_2] = BTL_TRIGGER_PATTERN;
  flag_words[BTL_TRIGGER_WORD_PATTERN_3] = BTL_TRIGGER_PATTERN;

  armed_ok = spi_flash_io_erase_range(BTL_FLASH_TRIGGER_FLAG_OFFSET, (uint32_t)BTL_TRIGGER_FLAG_SIZE_BYTES);

  if (armed_ok == true) {
    armed_ok = spi_flash_io_write(BTL_FLASH_TRIGGER_FLAG_OFFSET, (const uint8_t *)flag_words,
                                  (uint32_t)BTL_TRIGGER_FLAG_SIZE_BYTES);
  }

  if (armed_ok == true) {
    __DSB();
    __ISB();

    HAL_CORTEX_NVIC_SystemReset();
  }

  return armed_ok;
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/
