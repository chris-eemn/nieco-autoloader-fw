target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    modbus_port.c
    mb_regs.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR} )
