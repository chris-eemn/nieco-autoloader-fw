/**
 * @file modbus_port.h
 * @author
 * @brief
 * @date 2025-08-05
 *
 * @copyright Copyright (c) 2025 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef MODBUS_PORT_H_
#define MODBUS_PORT_H_

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
 * @brief Returns a pointer to the UI Modbus slave port function table.
 * @return Pointer to the statically allocated modbus_slave_port_fns_t.
 */
modbus_slave_port_fns_t* get_ui_port_fns(void);

#endif /* MODBUS_PORT_H_ */
