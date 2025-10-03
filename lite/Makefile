# Project Name
TARGET = deloop_lite

# LVGL
LVGL_PATH ?= ${shell pwd}/external/lvgl
ASM_SOURCES += $(shell find $(LVGL_PATH)/src -type f -name '*.S')
C_SOURCES += $(shell find $(LVGL_PATH)/src -type f -name '*.c')
CPP_SOURCES += $(shell find $(LVGL_PATH)/src -type f -name '*.cpp')

AS_INCLUDES += "-I$(LVGL_PATH)"
C_INCLUDES += "-I$(LVGL_PATH)"
C_INCLUDES += -I.

C_DEFS += -DLV_CONF_INCLUDE_SIMPLE

# Sources
APP_TYPE = BOOT_QSPI
CPP_SOURCES += main.cpp \
  GC9A01.cpp

# Library Locations
LIBDAISY_DIR = external/libDaisy
# DAISYSP_DIR = @DAISYSP_DIR@

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
