# file-format: 1.0.0
cmake_minimum_required(VERSION 3.30)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)

include(cmake/target.cmake)
include(cmake/${TOOLCHAIN_FILE})

project(stepper_144pin)
add_executable(stepper_144pin)
include(cmake/files.cmake)
include(cmake/flags.cmake)
include(cmake/components.cmake)
add_subdirectory(app)
add_subdirectory(common)
add_subdirectory(cal-data)
add_subdirectory(cli)
add_subdirectory(stepper)
set(TARGET_NAME ${CMAKE_PROJECT_NAME}) # modbus cmakelists uses TARGET_NAME, so we need to set it here
add_subdirectory(modbus/modbus_slave)
add_subdirectory(modbus_app)
add_subdirectory(usb_loader)

# Stubs to be able to add compiler flag overrides, e.g. -Os
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
  # Add additional source files here
  driver_w25q/w25q.c
  driver_w25q_port/spi_portable.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
  # Add additional include directories here
  driver_w25q
  driver_w25q_port
  ${CMAKE_BINARY_DIR}/version
)

# --- Git version injection --------------------------------------------------
# The boot banner prints the git tag + short commit hash (app_version_git.h). The header is
# generated into the build tree (never checked in) by cmake/gen_version.cmake: once now so it
# exists before the first compile, and again on every build via the target below so a new
# commit/tag is picked up without reconfiguring. It lives in its own build-tree folder (not
# "generated", which is reserved for STM32CubeMX output). The banner is printed from a task
# (not main()) because app_console_print() only transmits once the FreeRTOS scheduler is running.
find_package(Git QUIET)
set(APP_VERSION_GIT_HEADER ${CMAKE_BINARY_DIR}/version/app_version_git.h)

execute_process(COMMAND ${CMAKE_COMMAND}
  -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
  -DSRC_DIR=${CMAKE_SOURCE_DIR}
  -DOUT_FILE=${APP_VERSION_GIT_HEADER}
  -P ${CMAKE_SOURCE_DIR}/cmake/gen_version.cmake
)

add_custom_target(app_version_git
  BYPRODUCTS ${APP_VERSION_GIT_HEADER}
  COMMAND ${CMAKE_COMMAND}
    -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
    -DSRC_DIR=${CMAKE_SOURCE_DIR}
    -DOUT_FILE=${APP_VERSION_GIT_HEADER}
    -P ${CMAKE_SOURCE_DIR}/cmake/gen_version.cmake
  COMMENT "Refreshing git version header (app_version_git.h)"
  VERBATIM
)
add_dependencies(${CMAKE_PROJECT_NAME} app_version_git)

target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC
  # Add additional defined symbols here
)

target_compile_options(${CMAKE_PROJECT_NAME} PUBLIC
  # Add additional compiler flags here
)

target_link_options(${CMAKE_PROJECT_NAME} PUBLIC
  # Add additional linker flags here
)

target_link_directories(${CMAKE_PROJECT_NAME} PUBLIC
  # Add additional linker search directories here
)

target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC
  # Add additional linker libraries here
)

# Produce .hex and .bin alongside the .elf on every build
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.hex
  COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.bin
  COMMENT "Generating ${CMAKE_PROJECT_NAME}.hex and ${CMAKE_PROJECT_NAME}.bin"
)

set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES
  "${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/${CMAKE_PROJECT_NAME}.hex"
  "${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/${CMAKE_PROJECT_NAME}.bin"
)
