#include <fff.h>
#include <gtest/gtest.h>
#include <stm32f4xx_hal.h>
#include <stm32f4xx_hal_i2c.h>
#include <stm32f4xx_hal_i2s.h>
#include <stm32f4xx_hal_i2s_ex.h>
#include <stm32f4xx_hal_sai.h>

#include "drv/wm8960.hpp"
DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(HAL_StatusTypeDef, HAL_I2C_Init, I2C_HandleTypeDef *);
FAKE_VALUE_FUNC(HAL_StatusTypeDef, HAL_SAI_InitProtocol, SAI_HandleTypeDef *,
                uint32_t, uint32_t, uint32_t);
FAKE_VALUE_FUNC(HAL_StatusTypeDef, HAL_I2C_Master_Transmit, I2C_HandleTypeDef *,
                uint16_t, uint8_t *, uint16_t, uint32_t);
FAKE_VALUE_FUNC(uint32_t, HAL_SAI_GetError, const SAI_HandleTypeDef *);

TEST(WM8960Tests, init_successful) {
  // ASSERT_EQ(deloop::WM8960::Init(), deloop::Error::kOk);
}
