/**
 * @file usb_update_file.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief See usb_update_file.h.
 * @version 0.1
 * @date 2026-07-28
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include "usb_update_file.h"

#include "app_console.h"
#include "app_version_git.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
/** Largest number of digits accepted in one version field of a filename. The header stores each
 * field in a uint8_t, so three is already the most that can ever be valid. */
#define USB_UPDATE_FILE_MAX_VERSION_DIGITS 3U

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/
/** Scratch buffers kept out of the caller's stack. FileX returns directory entry names of up to
 * FX_MAX_LONG_NAME_LEN (256) bytes and an FX_FILE control block is several hundred bytes more --
 * together they would dominate the USB loader task's stack.
 *
 * Both are therefore usable from one task only. usb_update_file_find() is the sole user and is
 * called only from the usb_loader task. */
static CHAR s_entry_name[FX_MAX_LONG_NAME_LEN];
static FX_FILE s_open_file;

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/
/**
 * @brief lowercases one ASCII character, leaving everything else untouched
 * @param character character to fold
 * @return CHAR the lowercased character
 */
static CHAR to_lower_char(CHAR character);

/**
 * @brief compares the tail of a filename against USB_UPDATE_FILE_NAME_SUFFIX, case-insensitively
 * @note requires an exact match through to the terminator, so trailing characters after the
 *       suffix (a second extension, for instance) reject the name.
 * @param tail remainder of the filename, positioned at the character after the version fields;
 *        must not be NULL
 * @return bool true if the tail is exactly the expected suffix
 */
static bool name_tail_matches_suffix(const CHAR *tail);

/**
 * @brief parses one decimal version field, advancing the cursor past it on success
 * @param cursor in/out pointer into the filename; must not be NULL, and neither may *cursor
 * @param out_value destination for the parsed field; must not be NULL
 * @return bool true if 1..3 digits were read, the value fits a uint8_t, and no fourth digit
 *         follows (a longer run of digits means a malformed name, not a large version)
 */
static bool parse_version_field(const CHAR **cursor, uint8_t *out_value);

/**
 * @brief tests a filename against the v<major>_<minor>_<build>_stepper_144pin.bin pattern
 * @param name filename to test; must not be NULL
 * @param out_major destination for the major field; must not be NULL
 * @param out_minor destination for the minor field; must not be NULL
 * @param out_build destination for the build field; must not be NULL
 * @return bool true if the name matched, in which case the three outputs are populated
 */
static bool parse_update_file_name(const CHAR *name, uint8_t *out_major, uint8_t *out_minor, uint8_t *out_build);

/**
 * @brief packs a version triple into one integer so versions compare with a single relation
 * @param major major field
 * @param minor minor field
 * @param build build field
 * @return uint32_t the packed, order-preserving version value
 */
static uint32_t version_pack(uint8_t major, uint8_t minor, uint8_t build);

/**
 * @brief copies a NUL-terminated name into a bounded destination buffer
 * @param dest destination buffer; must not be NULL
 * @param dest_size total size of dest in bytes, terminator included; must be non-zero
 * @param src source name; must not be NULL
 * @return bool true if the whole source name plus its terminator fit in dest
 */
static bool copy_name(CHAR *dest, uint32_t dest_size, const CHAR *src);

/**
 * @brief opens the chosen candidate, reads its 16-byte header, and checks it against the file's
 *        actual size, the staging slot's capacity, and the filename's version
 * @param media mounted FileX media holding the file; must not be NULL
 * @param name filename to open, already known to match the update pattern; must not be NULL
 * @param file_size the file's size as reported by the directory walk
 * @param out_file destination for the validated result; must not be NULL. Written only on success.
 * @return usb_update_file_status_enum USB_UPDATE_FILE_OK, or the reason the file was rejected
 */
static usb_update_file_status_enum read_and_validate_header(FX_MEDIA *media, const CHAR *name, ULONG file_size,
                                                            usb_update_file_t *out_file);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/
usb_update_file_status_enum usb_update_file_find(FX_MEDIA *media, usb_update_file_t *out_file) {
  usb_update_file_status_enum result = USB_UPDATE_FILE_NONE_FOUND;

  if ((media != NULL) && (out_file != NULL)) {
    CHAR best_name[USB_UPDATE_FILE_NAME_MAX];
    ULONG best_size = 0U;
    uint32_t best_version = 0U;
    uint32_t candidate_count = 0U;
    UINT attributes = 0U;
    ULONG size = 0U;
    UINT year = 0U;
    UINT month = 0U;
    UINT day = 0U;
    UINT hour = 0U;
    UINT minute = 0U;
    UINT second = 0U;
    UINT fx_status;

    best_name[0] = (CHAR)'\0';

    fx_status = fx_directory_first_full_entry_find(media, s_entry_name, &attributes, &size, &year, &month, &day, &hour, &minute, &second);

    while (fx_status == FX_SUCCESS) {
      uint8_t major = 0U;
      uint8_t minor = 0U;
      uint8_t build = 0U;

      if (((attributes & FX_DIRECTORY) == 0U) && (parse_update_file_name(s_entry_name, &major, &minor, &build) == true)) {
        uint32_t version = version_pack(major, minor, build);

        candidate_count++;
        app_console_print("[USB] Update candidate: %s\r\n", s_entry_name);

        /* Strictly greater, so the first of two identically-versioned names wins (e.g. a
         * v1_2_3_... alongside a zero-padded v01_2_3_...). */
        if ((version > best_version) && (copy_name(best_name, (uint32_t)sizeof(best_name), s_entry_name) == true)) {
          best_version = version;
          best_size = size;
        }
      }

      fx_status = fx_directory_next_full_entry_find(media, s_entry_name, &attributes, &size, &year, &month, &day, &hour, &minute, &second);
    }

    /* FX_NO_MORE_ENTRIES is the normal end of the walk; anything else means it aborted, so the
     * candidate we settled on may not be the best one actually present on the drive. */
    if (fx_status != FX_NO_MORE_ENTRIES) {
      result = USB_UPDATE_FILE_DIR_READ_FAILED;
    }
    else if ((candidate_count == 0U) || (best_name[0] == (CHAR)'\0')) {
      result = USB_UPDATE_FILE_NONE_FOUND;
    }
    else {
      if (candidate_count > 1U) {
        app_console_print("[USB] %lu candidates; using the highest version.\r\n", (unsigned long)candidate_count);
      }

      result = read_and_validate_header(media, best_name, best_size, out_file);
    }
  }

  return result;
}

const CHAR *usb_update_file_status_string(usb_update_file_status_enum status) {
  const CHAR *description;

  switch (status) {
    case USB_UPDATE_FILE_OK:
      description = "ok";
      break;

    case USB_UPDATE_FILE_NONE_FOUND:
      description = "no update file on the drive";
      break;

    case USB_UPDATE_FILE_DIR_READ_FAILED:
      description = "directory read failed";
      break;

    case USB_UPDATE_FILE_OPEN_FAILED:
      description = "file would not open";
      break;

    case USB_UPDATE_FILE_HEADER_TRUNCATED:
      description = "file too short to hold a header";
      break;

    case USB_UPDATE_FILE_TOO_LARGE:
      description = "file larger than the staging slot";
      break;

    case USB_UPDATE_FILE_SIZE_MISMATCH:
      description = "header size disagrees with the file size";
      break;

    case USB_UPDATE_FILE_VERSION_MISMATCH:
      description = "header version disagrees with the filename";
      break;

    default:
      description = "unknown error";
      break;
  }

  return description;
}

bool usb_update_file_version_differs(const usb_update_file_t *file) {
  bool differs = false;

  if (file != NULL) {
    uint8_t running_major = 0U;
    uint8_t running_minor = 0U;
    uint8_t running_build = 0U;
    bool running_valid = false;

    usb_update_file_running_version_get(&running_major, &running_minor, &running_build, &running_valid);

    if (running_valid == false) {
      /* Nothing to compare against, so the file cannot be ruled out. */
      differs = true;
    }
    else {
      differs = ((file->header.version_major != running_major) || (file->header.version_minor != running_minor) ||
                 (file->header.version_build != running_build));
    }
  }

  return differs;
}

void usb_update_file_running_version_get(uint8_t *out_major, uint8_t *out_minor, uint8_t *out_build, bool *out_valid) {
  if ((out_major != NULL) && (out_minor != NULL) && (out_build != NULL) && (out_valid != NULL)) {
#if (APP_VERSION_NUMERIC_VALID != 0)
    *out_major = (uint8_t)APP_VERSION_MAJOR;
    *out_minor = (uint8_t)APP_VERSION_MINOR;
    *out_build = (uint8_t)APP_VERSION_BUILD;
    *out_valid = true;
#else
    *out_major = 0U;
    *out_minor = 0U;
    *out_build = 0U;
    *out_valid = false;
#endif
  }
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/
static CHAR to_lower_char(CHAR character) {
  CHAR folded = character;

  if ((character >= (CHAR)'A') && (character <= (CHAR)'Z')) {
    folded = (CHAR)(character + ((CHAR)'a' - (CHAR)'A'));
  }

  return folded;
}

static bool name_tail_matches_suffix(const CHAR *tail) {
  static const CHAR suffix[] = USB_UPDATE_FILE_NAME_SUFFIX;
  bool matches = false;

  if (tail != NULL) {
    uint32_t index = 0U;

    matches = true;

    /* A tail shorter than the suffix terminates first and mismatches here, so this never reads
     * past either string's terminator. */
    while ((matches == true) && (suffix[index] != (CHAR)'\0')) {
      if (to_lower_char(tail[index]) != to_lower_char(suffix[index])) {
        matches = false;
      }
      else {
        index++;
      }
    }

    if ((matches == true) && (tail[index] != (CHAR)'\0')) {
      matches = false;
    }
  }

  return matches;
}

static bool parse_version_field(const CHAR **cursor, uint8_t *out_value) {
  bool parsed = false;

  if ((cursor != NULL) && (*cursor != NULL) && (out_value != NULL)) {
    const CHAR *scan = *cursor;
    uint32_t value = 0U;
    uint32_t digits = 0U;

    while ((digits < USB_UPDATE_FILE_MAX_VERSION_DIGITS) && (*scan >= (CHAR)'0') && (*scan <= (CHAR)'9')) {
      value = (value * 10U) + (uint32_t)(*scan - (CHAR)'0');
      scan++;
      digits++;
    }

    /* Rejecting a fourth digit rather than stopping at three means a name like v1234_... is
     * treated as malformed instead of quietly parsing as version 123. */
    if ((digits > 0U) && (value <= 255U) && ((*scan < (CHAR)'0') || (*scan > (CHAR)'9'))) {
      *out_value = (uint8_t)value;
      *cursor = scan;
      parsed = true;
    }
  }

  return parsed;
}

static bool parse_update_file_name(const CHAR *name, uint8_t *out_major, uint8_t *out_minor, uint8_t *out_build) {
  bool parsed = false;

  if ((name != NULL) && (out_major != NULL) && (out_minor != NULL) && (out_build != NULL)) {
    const CHAR *cursor = name;
    uint8_t major = 0U;
    uint8_t minor = 0U;
    uint8_t build = 0U;

    if (to_lower_char(*cursor) == (CHAR)'v') {
      cursor++;

      if ((parse_version_field(&cursor, &major) == true) && (*cursor == (CHAR)'_')) {
        cursor++;

        if ((parse_version_field(&cursor, &minor) == true) && (*cursor == (CHAR)'_')) {
          cursor++;

          if ((parse_version_field(&cursor, &build) == true) && (name_tail_matches_suffix(cursor) == true)) {
            parsed = true;
          }
        }
      }
    }

    if (parsed == true) {
      *out_major = major;
      *out_minor = minor;
      *out_build = build;
    }
  }

  return parsed;
}

static uint32_t version_pack(uint8_t major, uint8_t minor, uint8_t build) {
  return (((uint32_t)major << 16U) | ((uint32_t)minor << 8U) | (uint32_t)build);
}

static bool copy_name(CHAR *dest, uint32_t dest_size, const CHAR *src) {
  bool copied = false;

  if ((dest != NULL) && (src != NULL) && (dest_size > 0U)) {
    uint32_t index = 0U;

    while ((index < (dest_size - 1U)) && (src[index] != (CHAR)'\0')) {
      dest[index] = src[index];
      index++;
    }

    if (src[index] == (CHAR)'\0') {
      dest[index] = (CHAR)'\0';
      copied = true;
    }
    else {
      /* Leave nothing half-copied for a caller that ignores the return value. */
      dest[0] = (CHAR)'\0';
    }
  }

  return copied;
}

static usb_update_file_status_enum read_and_validate_header(FX_MEDIA *media, const CHAR *name, ULONG file_size,
                                                            usb_update_file_t *out_file) {
  usb_update_file_status_enum result = USB_UPDATE_FILE_OPEN_FAILED;

  if ((media != NULL) && (name != NULL) && (out_file != NULL)) {
    if (file_size <= (ULONG)sizeof(ede_update_file_header_t)) {
      /* A file exactly the size of the header carries no payload at all, so it is rejected here
       * along with anything shorter. */
      result = USB_UPDATE_FILE_HEADER_TRUNCATED;
    }
    else if (file_size > (ULONG)STAGED_IMAGE_MAX_SIZE) {
      result = USB_UPDATE_FILE_TOO_LARGE;
    }
    else if (fx_file_open(media, &s_open_file, (CHAR *)name, FX_OPEN_FOR_READ) != FX_SUCCESS) {
      result = USB_UPDATE_FILE_OPEN_FAILED;
    }
    else {
      ede_update_file_header_t header;
      ULONG bytes_read = 0U;
      UINT fx_status;

      /* Read straight into the struct: it is 16 bytes with no padding on this ABI, and the host
       * tooling writes the fields little-endian, matching the MCU. usb_update_stage.c reads the
       * staged copy out of SPI flash the same way. */
      fx_status = fx_file_read(&s_open_file, &header, (ULONG)sizeof(header), &bytes_read);

      if ((fx_status != FX_SUCCESS) || (bytes_read != (ULONG)sizeof(header))) {
        result = USB_UPDATE_FILE_HEADER_TRUNCATED;
      }
      else if ((ULONG)header.binary_size != (file_size - (ULONG)sizeof(header))) {
        result = USB_UPDATE_FILE_SIZE_MISMATCH;
      }
      else {
        uint8_t name_major = 0U;
        uint8_t name_minor = 0U;
        uint8_t name_build = 0U;

        /* Re-derived from the name rather than threaded through the call: the name already
         * matched the pattern, so this cannot fail, and it keeps the two versions being compared
         * side by side. A disagreement means the file was renamed without being rebuilt, which
         * would otherwise let the filename advertise a version the image does not contain. */
        if (parse_update_file_name(name, &name_major, &name_minor, &name_build) == false) {
          result = USB_UPDATE_FILE_VERSION_MISMATCH;
        }
        else if ((header.version_major != name_major) || (header.version_minor != name_minor) ||
                 (header.version_build != name_build)) {
          app_console_print("[USB] Filename says v%u.%u.%u but header says v%u.%u.%u\r\n", (unsigned int)name_major,
                            (unsigned int)name_minor, (unsigned int)name_build, (unsigned int)header.version_major,
                            (unsigned int)header.version_minor, (unsigned int)header.version_build);
          result = USB_UPDATE_FILE_VERSION_MISMATCH;
        }
        else if (copy_name(out_file->name, (uint32_t)sizeof(out_file->name), name) == false) {
          result = USB_UPDATE_FILE_OPEN_FAILED;
        }
        else {
          out_file->version_major = name_major;
          out_file->version_minor = name_minor;
          out_file->version_build = name_build;
          out_file->file_size = (uint32_t)file_size;
          out_file->header = header;
          result = USB_UPDATE_FILE_OK;
        }
      }

      (void)fx_file_close(&s_open_file);
    }
  }

  return result;
}
