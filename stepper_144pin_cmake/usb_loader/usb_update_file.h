/**
 * @file usb_update_file.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief Locates and validates a firmware-update file on a mounted FileX volume, and compares
 *        its version against the running application's.
 *
 *        The update file is named v<major>_<minor>_<build>_stepper_144pin.bin and holds the same
 *        header+payload blob the USART3 loader streams: a 16-byte ede_update_file_header_t
 *        (usart3_loader/update_image.h) followed immediately by the raw application binary. That
 *        means a validated file can later be copied byte-for-byte into the SPI-flash staging
 *        slot at STAGED_HEADER_OFFSET with no repackaging.
 *
 *        This module only reads the 16-byte header -- it never reads the payload and never
 *        touches SPI flash. Staging is a later phase.
 * @version 0.1
 * @date 2026-07-28
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef USB_UPDATE_FILE_H_
#define USB_UPDATE_FILE_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include <stdbool.h>
#include <stdint.h>

#include "fx_api.h"
#include "update_image.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
/** Fixed tail of a conforming update filename, following the v<major>_<minor>_<build> prefix.
 * Matched case-insensitively -- FAT long names preserve whatever case the file was created
 * with, so a technician's rename must not be able to make a valid file invisible. */
#define USB_UPDATE_FILE_NAME_SUFFIX "_stepper_144pin.bin"

/** Size of the stored filename buffer. FileX hands back names of up to FX_MAX_LONG_NAME_LEN
 * (256) bytes, but only names that already matched the pattern above are ever copied in here,
 * and the longest of those ("v255_255_255" + the suffix) is 32 bytes including its terminator. */
#define USB_UPDATE_FILE_NAME_MAX 64U

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/
/** A found, fully validated update file. Versions in the filename and in the header have been
 * confirmed equal by the time this is populated, so either may be used. */
typedef struct {
  CHAR name[USB_UPDATE_FILE_NAME_MAX]; /* filename exactly as it appears on the drive */
  uint8_t version_major;               /* parsed from the filename */
  uint8_t version_minor;
  uint8_t version_build;
  uint32_t file_size;              /* total size on disk, header included */
  ede_update_file_header_t header; /* the file's first 16 bytes */
} usb_update_file_t;

/** Outcome of usb_update_file_find(). Anything other than USB_UPDATE_FILE_OK leaves the caller's
 * usb_update_file_t untouched and means no update should be attempted. */
typedef enum {
  USB_UPDATE_FILE_OK = 0,           /* a single best candidate was found and fully validated */
  USB_UPDATE_FILE_NONE_FOUND,       /* no filename on the volume matched the update pattern */
  USB_UPDATE_FILE_DIR_READ_FAILED,  /* the directory walk aborted before it finished */
  USB_UPDATE_FILE_OPEN_FAILED,      /* the matched name would not open for reading */
  USB_UPDATE_FILE_HEADER_TRUNCATED, /* file too short to hold a 16-byte header, or a short read */
  USB_UPDATE_FILE_TOO_LARGE,        /* larger than the SPI-flash staging slot can hold */
  USB_UPDATE_FILE_SIZE_MISMATCH,    /* header binary_size disagrees with the file's actual size */
  USB_UPDATE_FILE_VERSION_MISMATCH  /* header version disagrees with the filename's version */
} usb_update_file_status_enum;

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 *Function Prototypes
 *******************************************************************************/
/**
 * @brief scans the root directory of a mounted volume for update files, picks the
 *        highest-versioned one, and validates its 16-byte header
 * @note reports each candidate it finds over the app console, including which ones it passed
 *       over, so an operator can see why a particular file was or was not chosen. Walks the
 *       directory with the FileX entry-find calls, so it must not be called from inside another
 *       directory walk on the same media -- FileX keeps that search state in the FX_MEDIA.
 *
 *       Validation is structural only: it confirms the file is large enough to contain a header,
 *       fits the staging slot, that the header's binary_size accounts for exactly the bytes
 *       following the header, and that the header's version agrees with the filename's. The
 *       payload CRC is deliberately not checked here -- that requires reading the whole payload
 *       and is done against the staged copy in SPI flash instead (see
 *       usart3_stream_validate_staged_crc()).
 * @param media mounted FileX media to scan; must not be NULL
 * @param out_file destination for the chosen file's details; must not be NULL. Only written when
 *        the return value is USB_UPDATE_FILE_OK.
 * @return usb_update_file_status_enum USB_UPDATE_FILE_OK when out_file holds a usable update
 *         file, otherwise the reason no file was accepted
 */
usb_update_file_status_enum usb_update_file_find(FX_MEDIA *media, usb_update_file_t *out_file);

/**
 * @brief maps a usb_update_file_find() result to a short human-readable phrase for the console
 * @param status status to describe
 * @return const CHAR* a static, never-NULL description
 */
const CHAR *usb_update_file_status_string(usb_update_file_status_enum status);

/**
 * @brief reports whether a found update file's version differs from the running application's
 * @note "differs", not "is newer" -- the update decision is deliberately a plain inequality, so
 *       a deliberate downgrade to an older release works the same way an upgrade does.
 *
 *       The comparison is numeric major.minor.build only. The running version's git-describe
 *       suffix is not considered, so a development build 10 commits past v0.0.1 with local
 *       changes still compares equal to a v0_0_1 update file.
 * @param file file returned by usb_update_file_find(); must not be NULL
 * @return bool true if an update should be applied. Also true when the running version is
 *         unknown (see usb_update_file_running_version_get()), since nothing can then rule the
 *         file out, and false if file is NULL.
 */
bool usb_update_file_version_differs(const usb_update_file_t *file);

/**
 * @brief reports the running application's numeric version, as baked in at build time
 * @note derived from the repo's vX.Y.Z git tag by cmake/gen_version.cmake. A build with no
 *       reachable tag has no numeric version at all; *out_valid is false in that case and the
 *       three version outputs are left at 0.
 * @param out_major destination for the major field; must not be NULL
 * @param out_minor destination for the minor field; must not be NULL
 * @param out_build destination for the build field; must not be NULL
 * @param out_valid destination for whether the version is known; must not be NULL
 */
void usb_update_file_running_version_get(uint8_t *out_major, uint8_t *out_minor, uint8_t *out_build, bool *out_valid);

#endif /* USB_UPDATE_FILE_H_ */
