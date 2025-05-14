cmake_minimum_required(VERSION 3.15)
project(deloop_mk0
  VERSION 0.3.0
  DESCRIPTION "Deloop Mk0"
)

# LANGUAGE SETTINGS
enable_language(C CXX ASM)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# MCU OPTIONS
set(MCU_TARGET "CORTEX_M4" CACHE STRING "Target platform for compilation")
set_property(CACHE MCU_TARGET PROPERTY STRINGS
  CORTEX_M4
  HOST  # For unit tests
)

if (MCU_TARGET STREQUAL "CORTEX_M4")
  set(MCU_OPTIONS
    -mcpu=cortex-m4
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
  )
  set(MCU_LINKER_OPTIONS
    -specs=nano.specs
    -T${CMAKE_SOURCE_DIR}/src/linker/STM32F446RE_FLASH.ld
  )
  set(FREERTOS_PORT GCC_ARM_CM4F)
elseif (MCU_TARGET STREQUAL "HOST")
  set(MCU_OPTIONS "")
  set(MCU_LINKER_OPTIONS "")
  set(FREERTOS_PORT GCC_POSIX)
else()
  message(FATAL_ERROR "Unsupported MCU target: ${MCU_TARGET}")
endif()

# COMPILER OPTIONS
set(EXTRA_OPTIONS
  -fdata-sections
  -fno-exceptions
  -ffunction-sections
  -finput-charset=UTF-8
)
set(INTERNAL_OPTIONS  # Not applied to third-party libraries.
  -fno-rtti
  -Wall
  -Wextra
  -Wshadow
  -Wconversion
  -Wdouble-promotion
)
set(OPTIMIZATION_OPTIONS
  $<$<CONFIG:Debug>:-Og>
)
set(DEPENDENCY_INFO_OPTIONS
  -MMD  # Generate dependency files.
  -MP  # Add phony target for each dependency.
  -MF "$(@:%.o=%.d)"  # Dependency files use same name as object file.
)
set(DEBUG_INFO_OPTIONS
  -g3
  -gdwarf-2
)
add_compile_options(
  ${MCU_OPTIONS}
  ${EXTRA_OPTIONS}
  ${OPTIMIZATION_OPTIONS}
  ${DEPENDENCY_INFO_OPTIONS}
  ${DEBUG_INFO_OPTIONS}
  $<$<COMPILE_LANGUAGE:CXX>:${CXX_OPTIONS}>
)

# LINKER OPTIONS
add_link_options(
  ${MCU_OPTIONS}
  ${MCU_LINKER_OPTIONS}
  -Wl,-Map=${PROJECT_NAME}.map,--cref
  -Wl,--gc-sections
  -Wl,--print-memory-usage
  -Wl,--wrap=malloc  # Wrap to throw an error if malloc is called.
)

# FreeRTOS
set(FREERTOS_KERNEL_PATH ${CMAKE_SOURCE_DIR}/external/FreeRTOS/FreeRTOS/Source)
add_library(freertos_config INTERFACE)
target_include_directories(freertos_config INTERFACE ${CMAKE_SOURCE_DIR}/src/config)
add_subdirectory(${FREERTOS_KERNEL_PATH} ${CMAKE_BINARY_DIR}/FreeRTOS-Kernel)

# STM32CubeF4
include(cmake/STM32CubeF4.cmake)
add_STM32CubeF4_library(stm32f4xx_hal
  ${CMAKE_SOURCE_DIR}/external/STM32CubeF4
  ${CMAKE_SOURCE_DIR}/src/config
)

# nanopb
set(PROTO_SOURCES
  ${CMAKE_SOURCE_DIR}/proto/log.proto
  ${CMAKE_SOURCE_DIR}/proto/command.proto
  ${CMAKE_SOURCE_DIR}/proto/stream.proto
)

set(NANOPB_SRC_ROOT_FOLDER ${CMAKE_SOURCE_DIR}/external/nanopb)
set(NANOPB_GENERATOR_SOURCE_DIR ${NANOPB_SRC_ROOT_FOLDER}/generator)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${NANOPB_SRC_ROOT_FOLDER}/extra)
find_package(Nanopb REQUIRED)
nanopb_generate_cpp(TARGET proto ${PROTO_SOURCES})

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/python/deloop_mk0)
add_custom_command(TARGET proto
POST_BUILD
COMMAND
  protoc -I=${CMAKE_SOURCE_DIR}/proto
  --python_out=${CMAKE_BINARY_DIR}/python/
  ${PROTO_SOURCES}
)

# DRIVERS
add_library(wm8960_stm32f4 STATIC
  src/drv/wm8960_stm32f4.cpp
)
target_compile_options(wm8960_stm32f4 PRIVATE ${INTERNAL_OPTIONS})
target_include_directories(wm8960_stm32f4
PUBLIC
  ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(wm8960_stm32f4
PRIVATE
  stm32f4xx_hal_interface
)

if (NOT ENABLE_TESTING AND MCU_TARGET STREQUAL "CORTEX_M4")
  target_link_libraries(wm8960_stm32f4
  PRIVATE
    stm32f4xx_hal
  )
endif()
# APPLICATION
set(EXECUTABLE ${PROJECT_NAME}.elf)
set(SOURCES
  src/main.cpp
  src/rtos_hooks.cpp
  src/uart_stream.cpp
  src/startup/startup_stm32f446xx.s
  src/board/stm32f4xx_it.h
  src/board/stm32f4xx_it.c
  src/board/stm32f4xx_hal_msp.c
  src/board/system_stm32f4xx.c
)

add_executable(${EXECUTABLE} ${SOURCES})
target_include_directories(${EXECUTABLE}
PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/src/config
)
target_compile_options(wm8960_stm32f4 PRIVATE ${INTERNAL_OPTIONS})
target_link_libraries(${EXECUTABLE}
PUBLIC
  proto
  nanopb
  freertos_kernel
  freertos_config
  wm8960_stm32f4
  stm32f4xx_hal
)

# POST-BUILD COMMANDS
add_custom_command(TARGET ${EXECUTABLE}
POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -O ihex ${EXECUTABLE} ${PROJECT_NAME}.hex
  COMMAND ${CMAKE_OBJCOPY} -O binary ${EXECUTABLE} ${PROJECT_NAME}.bin
  COMMAND ${CMAKE_SIZE_UTIL} ${EXECUTABLE}
  COMMAND ${CMAKE_OBJDUMP} -d ${EXECUTABLE} > ${PROJECT_NAME}.lst
)

set(LOGGING_MACROS
  DELOOP_LOG_INFO
  DELOOP_LOG_WARNING
  DELOOP_LOG_ERROR
  DELOOP_LOG_INFO_FROM_ISR
  DELOOP_LOG_WARNING_FROM_ISR
  DELOOP_LOG_ERROR_FROM_ISR
)

add_custom_command(TARGET ${EXECUTABLE}
POST_BUILD
COMMAND
  ${CMAKE_SOURCE_DIR}/scripts/create_log_table.py
  --latest ${CMAKE_SOURCE_DIR}/python/deloop_mk0/log_table.json
  --output ${CMAKE_BINARY_DIR}/python/log_table.json
  # NOTE: This must match the logging macros described in `src/logging.hpp`.
  --macro ${LOGGING_MACROS}
  --source_files ${SOURCES} src/drv/wm8960_stm32f4.cpp
  --source_version ${PROJECT_VERSION}
WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# TESTING
if(ENABLE_TESTING)
  set(BUILD_GMOCK OFF)
  add_subdirectory(${CMAKE_SOURCE_DIR}/external/googletest)
  add_subdirectory(${CMAKE_SOURCE_DIR}/external/fff)

  enable_testing()
  add_subdirectory(${CMAKE_SOURCE_DIR}/tests)
endif()
