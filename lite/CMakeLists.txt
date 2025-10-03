cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0135 NEW)

project(deloop_lite
  VERSION 1.0.0
  DESCRIPTION "Firmware for the Deloop Lite"
)

# Daisy Application
set(FIRMWARE_NAME "DELOOP_LITE")
set(FIRMWARE_SOURCES
  main.cpp
  GC9A01.cpp
)

set(DAISY_STORAGE qspi)
set(DAISY_GENERATE_BIN ON)
set(LIBDAISY_DIR ${CMAKE_CURRENT_LIST_DIR}/external/libDaisy)
add_subdirectory(${LIBDAISY_DIR})
include(DaisyProject)

# Custom targets to flash via dfu-util
add_custom_target(dfu_app
  COMMAND dfu-util -a 0 -s 0x90040000:leave -D ${FIRMWARE_NAME}.bin -d ,0483:df11
  DEPENDS ${FIRMWARE_NAME}
  COMMENT "Flashing external storage via dfu-util"
)

set(DAISY_QSPI_BOOTLOADER ${LIBDAISY_DIR}/core/dsy_bootloader_v6_3_extdfu-2000ms.bin)
add_custom_target(dfu_qspi_bootloader
  COMMAND dfu-util -a 0 -s 0x08000000:leave -D ${DAISY_QSPI_BOOTLOADER} -d ,0483:df11
  DEPENDS ${FIRMWARE_NAME}
  COMMENT "Flashing bootloader to internal storage via dfu-util"
)
