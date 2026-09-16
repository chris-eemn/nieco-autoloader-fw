target_sources(${CMAKE_PROJECT_NAME} PRIVATE
  app_bringup_task.c
  app_event_simulator.c
  app_task.c
  app_sm_port.c
  cartridge.c
  cartridge_recount.c
  dispense_sm.c
  homing.c
  main_sm.c
  patty_handler.c
  reload_sm.c
  shutdown_sm.c
  startup_sm.c
  input.c
  temp_sense/ext_adc_i2c_service.c
  temp_sense/mcp342x_portable.c
  temp_sense/thermocouple.c
  temp_sense/temp_layer.c
  ${CMAKE_SOURCE_DIR}/ede-mcp342x/mcp342x.c
)

# set to public so mb_regs.c can access temp_layer.h
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/temp_sense
  ${CMAKE_SOURCE_DIR}/ede-mcp342x
)
