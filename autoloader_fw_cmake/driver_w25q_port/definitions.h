/**
 * @file definitions.h
 * @brief STM32 HAL2 compatibility shim for the vendored driver_w25q submodule.
 *
 * driver_w25q/spi_portable.h was written against Microchip Harmony3's PORT PLIB
 * (PORT_PIN / PORT_PinSet() / PORT_PinClear()) and includes "definitions.h" to get
 * them. w25q.c calls PORT_PinSet()/PORT_PinClear() directly (not just through the
 * spi_portable_*_cs_pin() wrappers), so this header must provide working
 * definitions for this project rather than modifying the submodule.
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC. All Rights Reserved.
 */

#ifndef DEFINITIONS_H_
#define DEFINITIONS_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include <stdint.h>
#include "stm32_hal.h"
#include "mx_gpio_default.h"

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/
/* Name required by driver_w25q/w25q.c; only the flash chip select pin is ever
 * stored in this type, so it only needs to carry the pin mask -- the port is
 * fixed to SPI_FLASH_CS_PORT below. */
typedef uint32_t PORT_PIN;

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/
/**
 * @brief drives the flash chip select pin high (inactive)
 * @param pin pin mask on SPI_FLASH_CS_PORT to set
 */
static inline void PORT_PinSet(PORT_PIN pin) {
  HAL_GPIO_WritePin(SPI_FLASH_CS_PORT, pin, HAL_GPIO_PIN_SET);
}

/**
 * @brief drives the flash chip select pin low (active)
 * @param pin pin mask on SPI_FLASH_CS_PORT to clear
 */
static inline void PORT_PinClear(PORT_PIN pin) {
  HAL_GPIO_WritePin(SPI_FLASH_CS_PORT, pin, HAL_GPIO_PIN_RESET);
}

#endif /* DEFINITIONS_H_ */
