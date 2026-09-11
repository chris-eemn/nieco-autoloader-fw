/**
 * @file thermocouple.h
 * @author aanderson@eemn.io
 * @brief provides functions to calculate emf and temperature based on
 *    https://its90.nist.gov/InvFunctions
 *    https://its90.nist.gov/RefFunctions
 * @version 0.1
 * @date 2025-01-30
 *
 * @copyright Copyright (c) 2025 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef thermocouple_H_
#define thermocouple_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/
typedef enum {
    THERMO_TYPE_E = 0,
    THERMO_TYPE_J = 1,
    THERMO_TYPE_K = 2,
} thermo_type_enum;

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 *Function Prototypes
 *******************************************************************************/
/**
 * @brief calculates temperature, in degC, of the given thermocouple type, at the given emf
 * @note emf is in units of mV
 * @param type of thermocouple
 * @param emf, in mV
 * @retval double
 * @return temperature, degC
 */
double thermocouple_get_temp(thermo_type_enum type, double emf);

/**
 * @brief calculates emf, in mV, of the given thermocouple type, at the given temperature
 * @note emv is in units of mV
 * @param type of thermocouple
 * @retval temperature, in degC. (T90 is the notation used by the NIST equation)
 * @return none
 */
double thermocouple_get_emf(thermo_type_enum type, double T90);

#endif /* thermocouple_H_ */
