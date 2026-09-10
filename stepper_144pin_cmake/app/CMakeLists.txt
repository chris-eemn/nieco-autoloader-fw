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
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)
