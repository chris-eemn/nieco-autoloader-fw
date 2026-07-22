/**
 * @file usart3_loader.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief Owns USART3's role for the application and hosts the background task that streams an
 *        incoming firmware image into SPI flash.
 *
 *        USART3 can only serve one role per build: either the modbus slave or the binary
 *        image loader. USART3_MODE selects which, at compile time. This is a bring-up/test
 *        arrangement -- the final design delivers the image over a USB thumb drive instead --
 *        so for now the loader is the default and modbus is opt-in, letting the bootloader's
 *        update flow be exercised end to end without USB mass-storage support on the board.
 * @version 0.1
 * @date 2026-07-22
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef USART3_LOADER_H_
#define USART3_LOADER_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
/** USART3_MODE selects what USART3 is used for. Exactly one role is active per build.
 * Override USART3_MODE from the build (e.g. a target_compile_definitions entry) to switch;
 * left undefined it defaults to the image loader, per the bring-up needs described above. */
#define USART3_MODE_BIN_LOADER 0
#define USART3_MODE_MODBUS 1

#ifndef USART3_MODE
#define USART3_MODE USART3_MODE_BIN_LOADER
#endif

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
 * @brief reconfigures USART3 for the loader and starts the background task that receives an
 *        image over USART3 and stages it into SPI flash
 * @note only meaningful when USART3_MODE == USART3_MODE_BIN_LOADER. Call once from main(),
 *       in place of the modbus slave init, before the scheduler starts. w25q_initialize()
 *       must already have been called (main() does this).
 */
void usart3_loader_start(void);

#endif /* USART3_LOADER_H_ */
