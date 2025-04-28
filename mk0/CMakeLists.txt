cmake_minimum_required(VERSION 3.15)
project(deloop_mk0
  VERSION 0.2.0
  DESCRIPTION "Deloop Mk0"
)

# LANGUAGE SETTINGS
enable_language(C CXX ASM)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# COMPILER OPTIONS
set(MCU_OPTIONS
  -mcpu=cortex-m4
  -mthumb
  -mfloat-abi=hard
  -mfpu=fpv4-sp-d16
)
set(EXTRA_OPTIONS
  -fdata-sections
  -ffunction-sections
  -fno-exceptions
  -finput-charset=UTF-8
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
)

# LINKER OPTIONS
add_link_options(
  ${MCU_OPTIONS}
  -specs=nano.specs
  -T${CMAKE_SOURCE_DIR}/src/linker/STM32F446RE_FLASH.ld
  -Wl,-Map=${PROJECT_NAME}.map,--cref
  -Wl,--gc-sections
  -Wl,--print-memory-usage
  -Wl,--wrap=malloc  # Wrap to throw an error if malloc is called.
)

# FreeRTOS
set(FREERTOS_PORT GCC_ARM_CM4F)
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

file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/python/deloop_mk0)
add_custom_command(TARGET proto
POST_BUILD
COMMAND
  protoc -I=${CMAKE_SOURCE_DIR}/proto --python_out=${CMAKE_SOURCE_DIR}/python/
  ${PROTO_SOURCES}
)

# Application
set(EXECUTABLE ${PROJECT_NAME}.elf)
set(SOURCES
  src/main.cpp
  src/rtos_hooks.cpp
  src/uart_stream.cpp
  src/drv/wm8960_stm32f4xx.cpp
  # src/tasks/play_sine_wave.cpp
  src/startup/startup_stm32f446xx.s
  src/board/stm32f4xx_it.h
  src/board/stm32f4xx_it.c
  src/board/stm32f4xx_hal_msp.c
  src/board/system_stm32f4xx.c
)
add_executable(${EXECUTABLE} ${SOURCES})
target_include_directories(${EXECUTABLE}
PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/src/config
)
target_compile_options(${EXECUTABLE}
PRIVATE
  -Wall
  -Wextra
  -Wshadow
  -Wconversion
  -Wdouble-promotion
)
target_link_libraries(${EXECUTABLE}
PRIVATE
  proto
  nanopb
  freertos_kernel
  freertos_config
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
  --output ${CMAKE_SOURCE_DIR}/python/deloop_mk0/log_table.json
  # NOTE: This must match the logging macros described in `src/logging.hpp`.
  --macro ${LOGGING_MACROS}
  --source_files ${SOURCES}
  --source_version ${PROJECT_VERSION}
WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
