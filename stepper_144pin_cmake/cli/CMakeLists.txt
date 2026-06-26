# file-format: 1.0.0
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/app_console.c
  ${CMAKE_CURRENT_LIST_DIR}/app_console_commands.c
  ${CMAKE_CURRENT_LIST_DIR}/app_console_port_stm32.c
  ${CMAKE_CURRENT_LIST_DIR}/cal_data_example.c
  ${CMAKE_CURRENT_LIST_DIR}/cli_test_example.c
  ${CMAKE_CURRENT_LIST_DIR}/stepper_cli.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}
)
