target_sources(${CMAKE_PROJECT_NAME} PRIVATE
  app_bringup_task.c
  app_event_simulator.c
  app_task.c
  app_sm_port.c
  cartridge.c
  dispense_sm.c
  main_sm.c
  reload_sm.c
  startup_sm.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)
