cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0135 NEW)
project(deloop_lite
  VERSION 1.0.0
  DESCRIPTION "Firmware for the Deloop Lite"
)

# GoogleTest
if (BUILD_TESTS)
  set(BUILD_GMOCK OFF)
  add_subdirectory(${CMAKE_SOURCE_DIR}/external/googletest)
  enable_testing()
endif()

# LVGL
if (NOT DLP_HEADLESS)
  set(LV_BUILD_CONF_DIR
    ${PROJECT_SOURCE_DIR}/include
    CACHE PATH "Path to LVGL build configuration directory")
  add_subdirectory(external/lvgl)
endif()

# libdlp
add_subdirectory(../libdlp libdlp_build)

# Daisy Application
set(FIRMWARE_NAME "${PROJECT_NAME}")
set(FIRMWARE_SOURCES
  src/main.cpp
  src/WS2812B.cpp
)
if (NOT DLP_HEADLESS)
  list(APPEND FIRMWARE_SOURCES
    src/GC9A01.cpp
    src/display.cpp
  )
endif()

if (DLP_DAISYSP)
  set(DAISYSP_DIR ${CMAKE_CURRENT_LIST_DIR}/external/DaisySP)
  add_subdirectory(${DAISYSP_DIR} DaisySP)
  set(DAISYSP_LIB DaisySP)
endif()

set(DAISY_STORAGE qspi)
set(DAISY_GENERATE_BIN ON)
set(LIBDAISY_DIR ${CMAKE_CURRENT_LIST_DIR}/external/libDaisy)
add_compile_definitions(FILEIO_ENABLE_FATFS_READER)
add_subdirectory(${LIBDAISY_DIR})
include(DaisyProject)

target_include_directories(${FIRMWARE_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(${FIRMWARE_NAME} PRIVATE dlp)
if (NOT DLP_HEADLESS)
  target_link_libraries(${FIRMWARE_NAME} PRIVATE lvgl)
else()
  target_compile_definitions(${FIRMWARE_NAME} PRIVATE DLP_HEADLESS)
endif()

if (DLP_DAISYSP)
  target_compile_definitions(${FIRMWARE_NAME} PRIVATE DLP_DAISYSP)
endif()

# Custom targets to flash via dfu-util
add_custom_target(dfu_app
  COMMAND dfu-util -a 0 -s 0x90040000:leave -D ${FIRMWARE_NAME}.bin -d ,0483:df11
  DEPENDS ${FIRMWARE_NAME}
  COMMENT "Flashing external storage via dfu-util"
)

set(DAISY_QSPI_BOOTLOADER ${LIBDAISY_DIR}/core/dsy_bootloader_v6_3-intdfu-2000ms.bin)
add_custom_target(dfu_bootloader
  COMMAND dfu-util -a 0 -s 0x08000000:leave -D ${DAISY_QSPI_BOOTLOADER} -d ,0483:df11
  COMMENT "Flashing bootloader to internal storage via dfu-util"
)
