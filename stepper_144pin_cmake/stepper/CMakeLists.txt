target_sources(${CMAKE_PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/stepper.c
  ${CMAKE_CURRENT_LIST_DIR}/stepper_ctrl.c
  ${CMAKE_CURRENT_LIST_DIR}/pca9538a.c
  ${CMAKE_CURRENT_LIST_DIR}/encoder.c
  ${CMAKE_CURRENT_LIST_DIR}/axis.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}
)
