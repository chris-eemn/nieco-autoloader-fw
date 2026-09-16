target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    spi_flash_io.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR} )
