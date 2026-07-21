/**
 * @file spi_portable.c
 * @brief STM32 HAL2 / SPI_FLASH_BUS implementation of driver_w25q's spi_portable.h port layer.
 * @note SPI_FLASH_BUS (SPI2) itself (clock, GPIO, NVIC, IRQHandler) is configured by the
 *       CubeMX-generated generated/hal/mx_spi2.c -- this file only supplies the non-blocking
 *       transfer calls and completion callbacks that driver_w25q/w25q.c depends on.
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC. All Rights Reserved.
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include "spi_portable.h"
#include "mx_hal_def.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
/* w25q.c only combines mismatched tx/rx lengths for the status-register read
 * (2 tx bytes / 3 rx bytes) -- this is sized with headroom above that. */
#define SPI_PORTABLE_MAX_COMBINED_LEN (8U)

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/
static spi_portable_callback registered_callback = NULL;
static uintptr_t registered_context = 0U;
static uint8_t combined_tx_buffer[SPI_PORTABLE_MAX_COMBINED_LEN];

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/
static void notify_transfer_complete(hal_spi_handle_t* hspi);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/
bool spi_portable_write_read(uint8_t* tx_data, uint32_t tx_len, uint8_t* rx_data, uint32_t rx_len) {
  bool request_accepted = false;
  uint32_t combined_len;
  uint32_t i;

  if ((tx_data != NULL) && (rx_data != NULL)) {
    combined_len = (tx_len > rx_len) ? tx_len : rx_len;

    if (combined_len <= SPI_PORTABLE_MAX_COMBINED_LEN) {
      /* HAL_SPI_TransmitReceive_IT clocks tx and rx simultaneously for the same
       * byte count, so the shorter side (usually tx) is padded with dummy bytes. */
      for (i = 0U; i < combined_len; i++) {
        combined_tx_buffer[i] = (i < tx_len) ? tx_data[i] : 0xFFU;
      }

      if (HAL_SPI_TransmitReceive_IT(spi_flash_bus_gethandle(), combined_tx_buffer, rx_data, combined_len) == HAL_OK) {
        request_accepted = true;
      }
    }
  }

  return request_accepted;
}

bool spi_portable_write(uint8_t* tx_data, uint32_t tx_len) {
  bool request_accepted = false;

  if (tx_data != NULL) {
    if (HAL_SPI_Transmit_IT(spi_flash_bus_gethandle(), tx_data, tx_len) == HAL_OK) {
      request_accepted = true;
    }
  }

  return request_accepted;
}

bool spi_portable_read(uint8_t* rx_data, uint32_t rx_len) {
  bool request_accepted = false;

  if (rx_data != NULL) {
    if (HAL_SPI_Receive_IT(spi_flash_bus_gethandle(), rx_data, rx_len) == HAL_OK) {
      request_accepted = true;
    }
  }

  return request_accepted;
}

bool spi_portable_is_busy(void) {
  hal_spi_state_t state = HAL_SPI_GetState(spi_flash_bus_gethandle());

  return ((state == HAL_SPI_STATE_TX_ACTIVE) || (state == HAL_SPI_STATE_RX_ACTIVE) ||
          (state == HAL_SPI_STATE_TX_RX_ACTIVE));
}

void spi_portable_callback_register(spi_portable_callback callBack, uintptr_t context) {
  registered_callback = callBack;
  registered_context = context;
}

void spi_portable_set_cs_pin(uint32_t pin) {
  HAL_GPIO_WritePin(SPI_FLASH_CS_PORT, pin, HAL_GPIO_PIN_SET);
}

void spi_portable_clear_cs_pin(uint32_t pin) {
  HAL_GPIO_WritePin(SPI_FLASH_CS_PORT, pin, HAL_GPIO_PIN_RESET);
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/
/**
 * @brief forwards an SPI_FLASH_BUS completion/error event to the callback w25q.c registered
 * @param hspi handle reported by the HAL callback
 */
static void notify_transfer_complete(hal_spi_handle_t* hspi) {
  if ((hspi == spi_flash_bus_gethandle()) && (registered_callback != NULL)) {
    registered_callback(registered_context);
  }
}

/*******************************************************************************
 * HAL Callback Overrides
 *******************************************************************************
 * USE_HAL_SPI_REGISTER_CALLBACKS is 0 in stm32c5xx_hal_conf.h, so these must be
 * strong overrides of the weak HAL names rather than HAL_SPI_RegisterXCallback().
 *******************************************************************************/
void HAL_SPI_TxCpltCallback(hal_spi_handle_t* hspi) {
  notify_transfer_complete(hspi);
}

void HAL_SPI_RxCpltCallback(hal_spi_handle_t* hspi) {
  notify_transfer_complete(hspi);
}

void HAL_SPI_TxRxCpltCallback(hal_spi_handle_t* hspi) {
  notify_transfer_complete(hspi);
}

void HAL_SPI_ErrorCallback(hal_spi_handle_t* hspi) {
  notify_transfer_complete(hspi);
}
