/**
 * @file usb_loader.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief See usb_loader.h.
 * @version 0.1
 * @date 2026-07-27
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include "usb_loader.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "ux_api.h"
#include "ux_hcd_stm32.h"
#include "ux_host_class_storage.h"

#include "fx_api.h"

#include "mx_usb_drd_fs.h"
#include "mx_usbx_app.h"

#include "app_console.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
#define USB_LOADER_TASK_STACK_SIZE 512U
#define USB_LOADER_TASK_PRIORITY (tskIDLE_PRIORITY + 2U)

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/
/** USBX's own private memory pool -- all its internal allocations (device/class descriptors,
 * the mass-storage class's per-media sector-cache buffer, ...) come out of this one buffer. */
static __ALIGNED(4) UCHAR s_ux_memory_pool[APP_DEFAULT_STACK_SIZE];

static SemaphoreHandle_t s_usb_insert_sem = NULL;

/** Captured by usb_loader_host_event_callback() on device insertion, cleared on removal. */
static UX_HOST_CLASS_STORAGE* s_storage = UX_NULL;
static UX_HOST_CLASS_STORAGE_MEDIA* s_storage_media = UX_NULL;
static FX_MEDIA* s_media = UX_NULL;

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/
static UINT usb_loader_host_event_callback(ULONG event, UX_HOST_CLASS* current_class, VOID* current_instance);
static VOID usb_loader_host_error_callback(UINT system_level, UINT system_context, UINT error_code);
static void usb_loader_task(void* pv_parameters);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/
void usb_loader_start(void) {
  UINT status;

  fx_system_initialize();

  status = ux_system_initialize(s_ux_memory_pool, sizeof(s_ux_memory_pool), UX_NULL, 0U);
  configASSERT(status == UX_SUCCESS);

  s_usb_insert_sem = xSemaphoreCreateBinary();
  configASSERT(s_usb_insert_sem != NULL);

  status = ux_host_stack_initialize(usb_loader_host_event_callback);
  configASSERT(status == UX_SUCCESS);

  ux_utility_error_callback_register(usb_loader_host_error_callback);

  status = ux_host_stack_class_register(_ux_system_host_class_storage_name, ux_host_class_storage_entry);
  configASSERT(status == UX_SUCCESS);

  /* Binds the USBX host stack to the STM32 HCD peripheral driver. The peripheral itself is
   * already initialized at boot (mx_system_init() -> mx_usb_drd_fs_host_init(), see main.c), so
   * this only needs the handle, not another HAL-level init. */
  status = ux_host_stack_hcd_register(_ux_system_host_hcd_stm32_name, ux_hcd_stm32_initialize, 0U, (ULONG)mx_usb_drd_fs_host_gethandle());
  configASSERT(status == UX_SUCCESS);

  BaseType_t task_ret = xTaskCreate(usb_loader_task, "UsbLoader", USB_LOADER_TASK_STACK_SIZE, NULL, USB_LOADER_TASK_PRIORITY, NULL);
  configASSERT(task_ret == pdPASS);
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/
/**
 * @brief USBX host instance-change notification callback.
 * @note Runs in USBX's own enumeration thread context, not an ISR -- plain FreeRTOS calls
 *       (not the FromISR variants) are correct here.
 */
static UINT usb_loader_host_event_callback(ULONG event, UX_HOST_CLASS* current_class, VOID* current_instance) {
  UINT status = UX_SUCCESS;

  switch (event) {
    case UX_DEVICE_INSERTION: {
      if ((current_class != UX_NULL) && (current_class->ux_host_class_entry_function == ux_host_class_storage_entry)) {
        if (s_storage == UX_NULL) {
          s_storage = (UX_HOST_CLASS_STORAGE*)current_instance;
          s_storage_media = (UX_HOST_CLASS_STORAGE_MEDIA*)current_class->ux_host_class_media;

          app_console_print("[USB] Mass-storage device inserted. VID=0x%04X PID=0x%04X\r\n",
                            (unsigned int)s_storage->ux_host_class_storage_device->ux_device_descriptor.idVendor,
                            (unsigned int)s_storage->ux_host_class_storage_device->ux_device_descriptor.idProduct);

          if ((s_storage_media != UX_NULL) && (s_storage_media->ux_host_class_storage_media_lun == 0U)) {
            s_media = &s_storage_media->ux_host_class_storage_media;
          }
          else {
            app_console_print("[USB] No LUN-0 media on this device -- nothing to list.\r\n");
            s_storage_media = UX_NULL;
          }

          if (s_storage->ux_host_class_storage_state == UX_HOST_CLASS_INSTANCE_LIVE) {
            (void)xSemaphoreGive(s_usb_insert_sem);
          }
          else {
            app_console_print("[USB] Storage instance not live yet (state=%u).\r\n", (unsigned int)s_storage->ux_host_class_storage_state);
          }
        }
      }
      break;
    }

    case UX_DEVICE_REMOVAL: {
      if ((VOID*)s_storage == current_instance) {
        s_storage = UX_NULL;
        s_storage_media = UX_NULL;
        s_media = UX_NULL;
        app_console_print("[USB] Mass-storage device removed.\r\n");
      }
      break;
    }

    default:
      break;
  }

  return status;
}

/**
 * @brief USBX error-notification callback.
 */
static VOID usb_loader_host_error_callback(UINT system_level, UINT system_context, UINT error_code) {
  switch (error_code) {
    case UX_DEVICE_ENUMERATION_FAILURE:
      app_console_print("[USB] Device enumeration failure.\r\n");
      break;

    case UX_NO_DEVICE_CONNECTED:
      app_console_print("[USB] No device connected.\r\n");
      break;

    default:
      app_console_print("[USB] USBX error: code=0x%X level=%u context=%u\r\n", (unsigned int)error_code, (unsigned int)system_level,
                        (unsigned int)system_context);
      break;
  }
}

/**
 * @brief Waits for a USB drive to be mounted, then prints its root directory contents.
 */
static void usb_loader_task(void* pv_parameters) {
  (void)pv_parameters;

  CHAR name[FX_MAX_LONG_NAME_LEN];
  UINT attributes;
  ULONG size;
  UINT year;
  UINT month;
  UINT day;
  UINT hour;
  UINT minute;
  UINT second;
  UINT status;
  ULONG entry_count;

  for (;;) {
    (void)xSemaphoreTake(s_usb_insert_sem, portMAX_DELAY);

    if (s_media == UX_NULL) {
      app_console_print("[USB] Insertion signalled but no media captured -- cannot list files.\r\n");
      continue;
    }

    app_console_print("[USB] Drive inserted -- root directory:\r\n");

    entry_count = 0U;
    status = fx_directory_first_full_entry_find(s_media, name, &attributes, &size, &year, &month, &day, &hour, &minute, &second);
    while (status == FX_SUCCESS) {
      app_console_print("  %s (%lu bytes)\r\n", name, size);
      entry_count++;
      status = fx_directory_next_full_entry_find(s_media, name, &attributes, &size, &year, &month, &day, &hour, &minute, &second);
    }

    /* FX_NO_MORE_ENTRIES is the normal end of the walk; anything else means the walk aborted. */
    if (status != FX_NO_MORE_ENTRIES) {
      app_console_print("[USB] Directory read stopped early: fx status=0x%X (after %lu entries).\r\n", (unsigned int)status,
                        (unsigned long)entry_count);
    }
    else if (entry_count == 0U) {
      app_console_print("  (empty)\r\n");
    }
    else {
      app_console_print("[USB] %lu entries listed.\r\n", (unsigned long)entry_count);
    }
  }
}
