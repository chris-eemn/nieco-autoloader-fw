/**
 * @file cal_data_save.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief Save-progress status enum shared by the queued cal-data save paths.
 *
 *        Both the general parameter section (cal_data.c) and the per-pair position sections
 *        (cal_data_position.c) save asynchronously through the w25q command queue and report
 *        progress with the same three states, so the enum lives here rather than in either
 *        module's header.
 *
 * @version 0.1
 * @date 2026-09-04
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef CAL_DATA_SAVE_H_
#define CAL_DATA_SAVE_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/
/**
 * Progress of the most recent queued cal-data save (general section or one stepper pair).
 */
typedef enum {
  CAL_DATA_SAVE_IDLE = 0, /**< No save outstanding -- either none was ever issued, or the last
                           *   one finished. */
  CAL_DATA_SAVE_PENDING,  /**< Erase and/or write still queued or in progress. */
  CAL_DATA_SAVE_ERROR,    /**< The w25q driver reported a transfer error. The stored data must
                           *   be assumed lost; the section was already erased. */
} cal_data_save_status_enum;

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 *Function Prototypes
 *******************************************************************************/

#endif /* CAL_DATA_SAVE_H_ */
