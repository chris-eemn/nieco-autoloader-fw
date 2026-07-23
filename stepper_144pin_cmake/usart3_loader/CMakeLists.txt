target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    usart3_loader.c
    usart3_stream.c
    spi_flash_io.c
    crc16_ccitt.c
    btl_flash_trigger.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR} )
