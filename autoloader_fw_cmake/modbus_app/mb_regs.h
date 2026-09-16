/**
 * @file mb_regs.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief
 * @version 0.1
 * @date 2025-06-19
 *
 * @copyright Copyright (c) 2025 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef MB_REGS_H_
#define MB_REGS_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include "modbus_slave.h"

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
 * @brief initialize modbus library
 * @note initializes most registers and the overall modbus library
 * @return none
 */
void mb_regs_init(void);

/**
 * @brief deinitialize modbus slave library
 */
void mb_regs_deinit(void);

/**
 * @brief get the ui slave modbus instance
 * @note
 * @return pointer to ui modbus slave instance
 */
modbus_slave_t* get_ui_slave(void);

/**
 * @brief get the ui slave modbus instance
 * @note
 * @return pointer to ui modbus slave instance
 */
modbus_slave_t* get_ui_slave(void);

/**
 * @brief gets the modbus register array
 * @note used by other modules that want to add a modbus register
 * @return pointer to modbus register array
 */
mb_holding_reg_array_t* get_reg_array(void);

#endif /* MB_REGS_H_ */
