target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    usb_loader.c
    usb_update_file.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR} )
