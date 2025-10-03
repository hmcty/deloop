cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0135 NEW)
project(deloop_lite
  VERSION 1.0.0
  DESCRIPTION "Firmware for the Deloop Lite"
)
include(external/libDaisy/cmake/toolchains/ArmGNUToolchain.cmake)

# Daisy Application
set(FIRMWARE_NAME "DELOOP_LITE")
set(FIRMWARE_SOURCES
  main.cpp
  GC9A01.cpp
)

set(DAISY_GENERATE_BIN ON)
add_subdirectory(external/libDaisy)
include(DaisyProject)

add_custom_target(flash
  COMMAND dfu-util -a 0 -s 0x08000000:leave -D ${FIRMWARE_NAME}.bin -d ,0483:df11
  DEPENDS ${FIRMWARE_NAME}
  COMMENT "Flashing via dfu-util"
)
