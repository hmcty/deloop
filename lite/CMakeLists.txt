cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0135 NEW)
project(deloop_lite
  VERSION 1.0.0
  DESCRIPTION "Firmware for the Deloop Lite"
)

include(external/libDaisy/cmake/toolchains/ArmGNUToolchain.cmake)

# TFT eSPI
add_library(TFT_eSPI STATIC
  external/TFT_eSPI/TFT_eSPI.cpp
)

target_include_directories(TFT_eSPI PUBLIC
  .
  external/TFT_eSPI
)

set_target_properties(TFT_eSPI PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED YES
)

# Daisy Application
set(FIRMWARE_NAME "DELOOP_LITE")
set(FIRMWARE_SOURCES
  main.cpp
)
set(FIRMWARE_EXTRA_LIBS
  TFT_eSPI
)

add_subdirectory(external/libDaisy)
include(DaisyProject)
