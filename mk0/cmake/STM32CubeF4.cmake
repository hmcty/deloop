# STM32CubeF4

macro(add_STM32CubeF4_library TARGET_NAME SOURCE_DIR CONFIG_DIR)
  add_library(${TARGET_NAME} STATIC)

  target_compile_definitions(
    ${TARGET_NAME}
    PUBLIC
    STM32F4
    STM32F446xx
    USE_HAL_DRIVER
  )

  target_include_directories(
    ${TARGET_NAME}
    PUBLIC
    ${CONFIG_DIR}
    ${SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${SOURCE_DIR}/Drivers/CMSIS/Include
    ${SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc
    ${SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/IncLegacy
  )

  target_sources(
    ${TARGET_NAME}
    PRIVATE
    ${SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
    ${SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c
    ${SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
    ${SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c
    ${SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
    ${SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c
    ${SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c
    ${SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ramfunc.c
    ${SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
    ${SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c.c
    ${SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c_ex.c
    ${SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2s.c
    ${SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2s_ex.c
    ${SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_sai.c
    ${SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_sai_ex.c
    ${SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c
    ${SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_usart.c
  )
endmacro()
