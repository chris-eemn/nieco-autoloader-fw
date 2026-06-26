/**
 * @file    pca9538a.c
 * @brief   Driver for the PCA9538A 8-bit I2C GPIO expander.
 */

#include "pca9538a.h"

#define I2C_TIMEOUT_MS 10U

hal_status_t pca9538a_init(hal_i2c_handle_t *hi2c, uint8_t output_mask) {
  if (hi2c == NULL) {
    return HAL_ERROR;
  }

  hal_status_t ret;
  uint8_t val = 0x00U;

  /* Drive outputs low before enabling them */
  ret = HAL_I2C_MASTER_MemWrite(hi2c, PCA9538A_ADDR, PCA9538A_REG_OUTPUT,
                                HAL_I2C_MEM_ADDR_8BIT, &val, 1U, I2C_TIMEOUT_MS);
  if (ret != HAL_OK) {
    return ret;
  }

  /* Configure direction: 0 = output, 1 = input */
  val = output_mask;
  ret = HAL_I2C_MASTER_MemWrite(hi2c, PCA9538A_ADDR, PCA9538A_REG_CONFIG,
                                HAL_I2C_MEM_ADDR_8BIT, &val, 1U, I2C_TIMEOUT_MS);
  return ret;
}

hal_status_t pca9538a_write_output(hal_i2c_handle_t *hi2c, uint8_t value) {
  if (hi2c == NULL) {
    return HAL_ERROR;
  }

  return HAL_I2C_MASTER_MemWrite(hi2c, PCA9538A_ADDR, PCA9538A_REG_OUTPUT,
                                 HAL_I2C_MEM_ADDR_8BIT, &value, 1U, I2C_TIMEOUT_MS);
}

hal_status_t pca9538a_read_output(hal_i2c_handle_t *hi2c, uint8_t *value) {
  if ((hi2c == NULL) || (value == NULL)) {
    return HAL_ERROR;
  }

  return HAL_I2C_MASTER_MemRead(hi2c, PCA9538A_ADDR, PCA9538A_REG_OUTPUT,
                                HAL_I2C_MEM_ADDR_8BIT, value, 1U, I2C_TIMEOUT_MS);
}

hal_status_t pca9538a_read_input(hal_i2c_handle_t *hi2c, uint8_t *value) {
  if ((hi2c == NULL) || (value == NULL)) {
    return HAL_ERROR;
  }

  return HAL_I2C_MASTER_MemRead(hi2c, PCA9538A_ADDR, PCA9538A_REG_INPUT,
                                HAL_I2C_MEM_ADDR_8BIT, value, 1U, I2C_TIMEOUT_MS);
}

hal_status_t pca9538a_read_config(hal_i2c_handle_t *hi2c, uint8_t *value) {
  if ((hi2c == NULL) || (value == NULL)) {
    return HAL_ERROR;
  }

  return HAL_I2C_MASTER_MemRead(hi2c, PCA9538A_ADDR, PCA9538A_REG_CONFIG,
                                HAL_I2C_MEM_ADDR_8BIT, value, 1U, I2C_TIMEOUT_MS);
}
