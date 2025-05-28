#include <gtest/gtest.h>

#include "stm32_hal.h"

#include "mk0/drv/wm8960.h"

#include "fff.h"
#include "stm32f4xx_hal_def.h"
DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(HAL_StatusTypeDef, HAL_I2C_Init, I2C_HandleTypeDef *);
FAKE_VALUE_FUNC(HAL_StatusTypeDef, HAL_I2C_Master_Transmit, I2C_HandleTypeDef *,
                uint16_t, uint8_t *, uint16_t, uint32_t);

void SetUpTestSuite() {
  // Initialize the fake functions.
  RESET_FAKE(HAL_I2C_Init);
  RESET_FAKE(HAL_I2C_Master_Transmit);
}

TEST(WM8960Tests, init_successful) {
  HAL_I2C_Init_fake.return_val = HAL_OK;
  HAL_I2C_Master_Transmit_fake.return_val = HAL_OK;

  auto wm8960 = deloop::WM8960();
  ASSERT_EQ(wm8960.init(I2C1), deloop::Error::kOk);
  ASSERT_TRUE(HAL_I2C_Init_fake.call_count == 1);
  ASSERT_EQ(HAL_I2C_Init_fake.arg0_val->Instance, I2C1);
}
