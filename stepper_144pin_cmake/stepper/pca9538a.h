/**
 * @file    pca9538a.h
 * @brief   Driver for the PCA9538A 8-bit I2C GPIO expander.
 */

#ifndef PCA9538A_H
#define PCA9538A_H

#include <stdint.h>
#include "stm32_hal.h"

/* HAL_I2C_MASTER_* expects the 7-bit address left-shifted by 1 (placed in CR2.SADD[7:1]).
 * PCA9538A 7-bit address with A1=A0=0 is 0x70; pass 0xE0 to the HAL. */
#define PCA9538A_ADDR         (0x70U << 1U)

/* Register map */
#define PCA9538A_REG_INPUT    0x00U
#define PCA9538A_REG_OUTPUT   0x01U
#define PCA9538A_REG_POLARITY 0x02U
#define PCA9538A_REG_CONFIG   0x03U

/**
 * @brief  Initialise the PCA9538A: drive outputs low, then configure direction.
 * @param  hi2c        I2C peripheral handle.
 * @param  output_mask Direction register value (bit=0 → output, bit=1 → input).
 * @return HAL_OK on success, HAL_ERROR if hi2c is NULL, or a HAL error code on bus fault.
 */
hal_status_t pca9538a_init(hal_i2c_handle_t *hi2c, uint8_t output_mask);

/**
 * @brief  Write the output latch register.
 * @param  hi2c   I2C peripheral handle.
 * @param  value  Byte to write to the output register.
 * @return HAL_OK on success, HAL_ERROR if hi2c is NULL, or a HAL error code on bus fault.
 */
hal_status_t pca9538a_write_output(hal_i2c_handle_t *hi2c, uint8_t value);

/**
 * @brief  Read back the output latch register.
 * @param  hi2c   I2C peripheral handle.
 * @param  value  Receives the current output latch value.
 * @return HAL_OK on success, HAL_ERROR if hi2c or value is NULL, or a HAL error code on bus fault.
 */
hal_status_t pca9538a_read_output(hal_i2c_handle_t *hi2c, uint8_t *value);

/**
 * @brief  Read the input port register.
 * @param  hi2c   I2C peripheral handle.
 * @param  value  Receives the sampled pin states.
 * @return HAL_OK on success, HAL_ERROR if hi2c or value is NULL, or a HAL error code on bus fault.
 */
hal_status_t pca9538a_read_input(hal_i2c_handle_t *hi2c, uint8_t *value);

/**
 * @brief  Read the configuration (direction) register.
 * @param  hi2c   I2C peripheral handle.
 * @param  value  Receives the configuration register value.
 * @return HAL_OK on success, HAL_ERROR if hi2c or value is NULL, or a HAL error code on bus fault.
 */
hal_status_t pca9538a_read_config(hal_i2c_handle_t *hi2c, uint8_t *value);

#endif /* PCA9538A_H */
