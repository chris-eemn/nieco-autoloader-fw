/**
 * @file cal_data_map.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief Section layout inside the cal-data region of the W25Q. Single source of truth for
 *        where each cal-data section lives; both cal_data.c and cal_data_position.c derive
 *        their addresses from here rather than hard-coding offsets.
 * @version 0.1
 * @date 2026-07-27
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef CAL_DATA_MAP_H_
#define CAL_DATA_MAP_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include <stdint.h>
#include "w25q_config.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
/* --- cal-data region layout --------------------------------------------------------------
 *
 * The cal-data region itself (base address and total size) is carved out of the chip by
 * driver_w25q_port/w25q_config.h; this header only sub-divides it.
 *
 *   0x100000 - 0x103FFF   general section              4 sectors   16 KiB
 *   0x104000 - 0x104FFF   stepper pair 0 position      1 sector     4 KiB
 *   0x105000 - 0x105FFF   stepper pair 1 position      1 sector     4 KiB
 *   0x106000 - 0x106FFF   stepper pair 2 position      1 sector     4 KiB
 *   0x107000 - 0x107FFF   stepper pair 3 position      1 sector     4 KiB
 *   0x108000 - 0x1FFFFF   unallocated                            ~992 KiB
 *
 * Why the split: saving a stepper position must not rewrite anything else. Every save costs a
 * 4 KiB sector erase (the W25Q erase granularity), and during that erase the contents of the
 * whole sector are gone -- so anything sharing a sector with a frequently-written value is
 * exposed to corruption on every one of those writes. Giving each stepper pair its own sector
 * means a position save erases and rewrites 4 KiB that holds nothing but that pair's position,
 * and leaves the general parameters untouched.
 *
 * Sections are sized in whole sectors and are deliberately over-allocated (the general section
 * currently uses well under one of its four sectors) so that growing a struct does not shift
 * the address of every section after it -- a shift would silently invalidate data already on
 * the chip in the field.
 */

/** Number of stepper pairs, each of which gets its own position section. 4 pairs = 8 motors. */
#define CAL_DATA_PAIR_COUNT (4U)

/** Motors per pair. */
#define CAL_DATA_MOTORS_PER_PAIR (2U)

/** General (non-position) parameter section. */
#define CAL_DATA_GENERAL_SECTOR_COUNT (4U)
#define CAL_DATA_GENERAL_BASE_ADDRESS (W25Q_CAL_DATA_BASE_ADDRESS)
#define CAL_DATA_GENERAL_SIZE (CAL_DATA_GENERAL_SECTOR_COUNT * W25Q_SECTOR_SIZE)

/** Per-pair stepper position sections, packed immediately after the general section. */
#define CAL_DATA_POSITION_SECTOR_COUNT (1U)
#define CAL_DATA_POSITION_SECTION_SIZE (CAL_DATA_POSITION_SECTOR_COUNT * W25Q_SECTOR_SIZE)
#define CAL_DATA_POSITION_BASE_ADDRESS (CAL_DATA_GENERAL_BASE_ADDRESS + CAL_DATA_GENERAL_SIZE)

/** Base address of one pair's position section. pair_index must be < CAL_DATA_PAIR_COUNT. */
#define CAL_DATA_POSITION_PAIR_ADDRESS(pair_index) \
  (CAL_DATA_POSITION_BASE_ADDRESS + ((uint32_t)(pair_index) * (uint32_t)CAL_DATA_POSITION_SECTION_SIZE))

/** Total of the region actually allocated above, used for the fits-in-region check. */
#define CAL_DATA_ALLOCATED_SIZE (CAL_DATA_GENERAL_SIZE + (CAL_DATA_PAIR_COUNT * CAL_DATA_POSITION_SECTION_SIZE))

/** Value every byte of the W25Q reads back as after an erase. A section holding nothing but
 * this pattern has never been written. */
#define CAL_DATA_ERASED_BYTE (0xFFU)

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 *Function Prototypes
 *******************************************************************************/

#endif /* CAL_DATA_MAP_H_ */
