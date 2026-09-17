/**
 * @file mcp342x_portable.c
 * @brief STM32C5 MCP342x transport on the PCA9538A's I2C1 bus.
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC. All Rights Reserved.
 *
 * Call from task context after CubeMX I2C1 initialization. Callers must serialize
 * access to the shared bus and the MCP342x driver's shared buffers.
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include "mcp342x_portable.h"

#include <stddef.h>
#include <string.h>

#include "mx_gpio_default.h"
#include "mx_i2c1.h"
#include "FreeRTOS.h"
#include "task.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
/* Matches the PCA9538A transport timeout. */
#define MCP342X_I2C_TIMEOUT_MS 10U

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/
static bool mcp342x_portable_recover_bus(void);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/
void mcp342x_portable_init(void *i2c_ref) {
  /* The driver passes its device pointer here, not a HAL handle. CubeMX owns
   * initialization of the shared bus; this port always uses I2C1. */
  (void)i2c_ref;
}

void mcp342x_portable_read(uint8_t address, uint8_t *rx_buf, uint32_t size) {
  if ((rx_buf == NULL) || (size == 0U)) {
    return;
  }

  hal_i2c_handle_t *hi2c = mx_i2c1_i2c_gethandle();
  hal_status_t status = HAL_ERROR;
  if (hi2c != NULL) {
    /* HAL expects the seven-bit device address shifted left by one. */
    status = HAL_I2C_MASTER_Receive(hi2c, ((uint32_t)address << 1U), rx_buf, size, MCP342X_I2C_TIMEOUT_MS);
    if ((status != HAL_OK) && (status != HAL_BUSY) && (mcp342x_portable_recover_bus() == true)) {
      status = HAL_I2C_MASTER_Receive(hi2c, ((uint32_t)address << 1U), rx_buf, size, MCP342X_I2C_TIMEOUT_MS);
    }
  }

  if (status != HAL_OK) {
    /* The void transport API cannot return errors. Set NRDY in every byte so
     * either configuration-byte position rejects a failed or partial read. */
    (void)memset(rx_buf, 0x80, size);
  }
}

void mcp342x_portable_write(uint8_t address, uint8_t *tx_buf, uint32_t size) {
  if ((tx_buf == NULL) || (size == 0U)) {
    return;
  }

  hal_i2c_handle_t *hi2c = mx_i2c1_i2c_gethandle();
  if (hi2c != NULL) {
    hal_status_t status = HAL_I2C_MASTER_Transmit(hi2c, ((uint32_t)address << 1U), tx_buf, size, MCP342X_I2C_TIMEOUT_MS);
    if ((status != HAL_OK) && (status != HAL_BUSY) && (mcp342x_portable_recover_bus() == true)) {
      status = HAL_I2C_MASTER_Transmit(hi2c, ((uint32_t)address << 1U), tx_buf, size, MCP342X_I2C_TIMEOUT_MS);
    }
    if (status != HAL_OK) {
      /* The driver's void API cannot report a failed configuration write. */
      return;
    }
  }
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/
/**
 * @brief Release a stuck slave with up to nine SCL pulses, then generate STOP.
 * @return True if both bus lines are released and I2C1 is reinitialized.
 * @note Caller owns the shared bus throughout recovery. Open-drain high releases
 *       the line; it must never drive against a slave holding SDA or SCL low.
 *       Recovery and the single transfer retry are bounded, including a stuck SCL.
 */
static bool mcp342x_portable_recover_bus(void) {
  /* Reset the peripheral and HAL state before taking GPIO ownership. */
  mx_i2c1_i2c_deinit();
  HAL_RCC_GPIOB_EnableClock();
  HAL_GPIO_WritePin(HAL_GPIOB, THERMO_I2C_SCL_PIN | THERMO_I2C_SDA_PIN, HAL_GPIO_PIN_SET);

  const hal_gpio_config_t gpio_config = {
      .mode = HAL_GPIO_MODE_OUTPUT,
      .output_type = HAL_GPIO_OUTPUT_OPENDRAIN,
      .pull = HAL_GPIO_PULL_NO,
      .speed = HAL_GPIO_SPEED_FREQ_LOW,
      .alternate = HAL_GPIO_AF_4,
  };
  bool released = (HAL_GPIO_Init(HAL_GPIOB, THERMO_I2C_SCL_PIN | THERMO_I2C_SDA_PIN, &gpio_config) == HAL_OK);

  if (released == true) {
    vTaskDelay(1U);
    released = (HAL_GPIO_ReadPin(HAL_GPIOB, THERMO_I2C_SCL_PIN) == HAL_GPIO_PIN_SET);
    for (uint8_t clock_count = 0U;
         (released == true) && (clock_count < 9U) && (HAL_GPIO_ReadPin(HAL_GPIOB, THERMO_I2C_SDA_PIN) == HAL_GPIO_PIN_RESET);
         clock_count++) {
      HAL_GPIO_WritePin(HAL_GPIOB, THERMO_I2C_SCL_PIN, HAL_GPIO_PIN_RESET);
      vTaskDelay(1U);
      HAL_GPIO_WritePin(HAL_GPIOB, THERMO_I2C_SCL_PIN, HAL_GPIO_PIN_SET);
      vTaskDelay(1U);
      released = (HAL_GPIO_ReadPin(HAL_GPIOB, THERMO_I2C_SCL_PIN) == HAL_GPIO_PIN_SET);
    }

    if (released == true) {
      /* Pull SDA low while SCL is low, then release SCL before SDA for STOP. */
      HAL_GPIO_WritePin(HAL_GPIOB, THERMO_I2C_SCL_PIN, HAL_GPIO_PIN_RESET);
      HAL_GPIO_WritePin(HAL_GPIOB, THERMO_I2C_SDA_PIN, HAL_GPIO_PIN_RESET);
      vTaskDelay(1U);
      HAL_GPIO_WritePin(HAL_GPIOB, THERMO_I2C_SCL_PIN, HAL_GPIO_PIN_SET);
      vTaskDelay(1U);
      released = (HAL_GPIO_ReadPin(HAL_GPIOB, THERMO_I2C_SCL_PIN) == HAL_GPIO_PIN_SET);
      HAL_GPIO_WritePin(HAL_GPIOB, THERMO_I2C_SDA_PIN, HAL_GPIO_PIN_SET);
      vTaskDelay(1U);
      released = (released == true) && (HAL_GPIO_ReadPin(HAL_GPIOB, THERMO_I2C_SDA_PIN) == HAL_GPIO_PIN_SET);
    }
  }

  /* Restore alternate-function pins, timing, filters and IRQs even on failure. */
  HAL_GPIO_WritePin(HAL_GPIOB, THERMO_I2C_SCL_PIN | THERMO_I2C_SDA_PIN, HAL_GPIO_PIN_SET);
  if (mx_i2c1_i2c_init() == NULL) {
    released = false;
  }
  return released;
}
