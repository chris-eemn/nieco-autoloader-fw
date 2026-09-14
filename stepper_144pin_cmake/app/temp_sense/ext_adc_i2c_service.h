/**
 * @file ext_adc_i2c_service.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief MCP342x thermocouple temperature sensing service on I2C1.
 * @version 0.1
 * @date 2026-09-10
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC. All Rights Reserved.
 *
 */

#ifndef EXT_ADC_I2C_SERVICE_H_
#define EXT_ADC_I2C_SERVICE_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 *Function Prototypes
 *******************************************************************************/

/**
 * @brief Create the TempSense task that polls the MCP342x thermocouple channels.
 *
 *        The task latches the MCP342x address pins, configures the ADC for
 *        one-shot 18-bit conversions on CH1 and CH2, then polls in a loop.
 *        When a channel temperature exceeds the hard-coded limit, it posts
 *        APP_EV_FAULT with APP_FAULT_CODE_TEMP_HIGH to the app task queue.
 */
void ext_adc_i2c_init(void);

#endif /* EXT_ADC_I2C_SERVICE_H_ */
