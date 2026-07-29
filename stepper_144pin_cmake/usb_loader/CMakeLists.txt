target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    usb_loader.c
    usb_update_file.c
    usb_update_stage.c
    crc16_ccitt.c
    btl_flash_trigger.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR} )
