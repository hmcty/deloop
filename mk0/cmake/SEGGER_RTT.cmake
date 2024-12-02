# SEGGER_RTT

macro(add_SEGGER_RTT_library TARGET_NAME SOURCE_DIR CONFIG_DIR)
  add_library(${TARGET_NAME} STATIC)

  target_compile_definitions(
    ${TARGET_NAME}
    PUBLIC
    USE_HAL_DRIVER
  )

  target_include_directories(
    ${TARGET_NAME}
    PUBLIC
    ${CONFIG_DIR}
    ${SOURCE_DIR}/SEGGER_RTT
  )

  target_sources(
    ${TARGET_NAME}
    PRIVATE
    ${SOURCE_DIR}/SEGGER_RTT/SEGGER_RTT.c
    ${SOURCE_DIR}/SEGGER_RTT/SEGGER_RTT_printf.c
  )
