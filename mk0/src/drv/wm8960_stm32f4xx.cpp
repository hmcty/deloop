#include "drv/wm8960.hpp"

#include <expected>

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_i2s.h"
#include "stm32f4xx_hal_i2c.h"

static I2S_HandleTypeDef i2s_handle;
static I2C_HandleTypeDef i2c_handle;

// static std::expected<uint8_t, int> ReadRegister(uint8_t reg_addr);
static int WriteRegister(uint8_t reg_addr, uint8_t reg_data);

int WM8960::Init() {
  i2s_handle.Instance = SPI2; // TODO: Allow for usage of other SPI peripherals.
  i2s_handle.Init.Mode = I2S_MODE_MASTER_TX;
  i2s_handle.Init.Standard = I2S_STANDARD_PHILIPS;
  i2s_handle.Init.DataFormat = I2S_DATAFORMAT_16B;
  i2s_handle.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
  i2s_handle.Init.AudioFreq = I2S_AUDIOFREQ_48K;
  i2s_handle.Init.CPOL = I2S_CPOL_LOW;
  i2s_handle.Init.ClockSource = I2S_CLOCK_PLL;
  i2s_handle.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;
  if (HAL_I2S_Init(&i2s_handle) != HAL_OK) {
    return -1;
  }

  i2c_handle.Instance = I2C1;
  i2c_handle.Init.ClockSpeed = 100000;
  i2c_handle.Init.DutyCycle = I2C_DUTYCYCLE_2;
  i2c_handle.Init.OwnAddress1 = 0;
  i2c_handle.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  i2c_handle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  i2c_handle.Init.OwnAddress2 = 0;
  i2c_handle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  i2c_handle.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&i2c_handle) != HAL_OK) {
    return -1;
  }

  int err_code = WriteRegister(
    WM8960_REG_ADDR_POWER_MGMT_3,
    WM8960_REG_MASK_POWER_MGMT_3_LOMIX |
    WM8960_REG_MASK_POWER_MGMT_3_ROMIX);
  if (err_code != 0) {
    return err_code;
  }

  err_code = WriteRegister(
    WM8960_REG_ADDR_LEFT_MIXER_CTL,
    WM8960_REG_MASK_LEFT_MIXER_CTL_LD2LO);
  if (err_code != 0) {
    return err_code;
  }

  err_code = WriteRegister(
    WM8960_REG_ADDR_RIGHT_MIXER_CTL,
    WM8960_REG_MASK_RIGHT_MIXER_CTL_RD2RO);
  if (err_code != 0) {
    return err_code;
  }

  err_code = WriteRegister(
    WM8960_REG_ADDR_POWER_MGMT_2,
    WM8960_REG_MASK_POWER_MGMT_2_SPKR  |
    WM8960_REG_MASK_POWER_MGMT_2_SPKL  |
    WM8960_REG_MASK_POWER_MGMT_2_ROUT1 |
    WM8960_REG_MASK_POWER_MGMT_2_LOUT1 |
    WM8960_REG_MASK_POWER_MGMT_2_DACL  |
    WM8960_REG_MASK_POWER_MGMT_2_DACR);
  if (err_code != 0) {
    return err_code;
  }

  return SetVolume(32);
}

int WM8960::SetVolume(uint8_t volume) {
  if (volume > 64) {
    volume = 64;
  }
  int err_code = WriteRegister(
    WM8960_REG_ADDR_LEFT_CHAN_VOL,
    WM8960_REG_MASK_LEFT_CHAN_VOL_DACVU |
    (WM8960_REG_MASK_LEFT_CHAN_VOL_LDACVOL & volume));
  if (err_code != 0) {
    return err_code;
  }

  return WriteRegister(
    WM8960_REG_ADDR_RIGHT_CHAN_VOL,
    WM8960_REG_MASK_RIGHT_CHAN_VOL_DACVU |
    (WM8960_REG_MASK_RIGHT_CHAN_VOL_RDACVOL & volume));
}

int WM8960::Play() {
  HAL_I2S_DMAResume(&i2s_handle);

  int err_code = WriteRegister(
    WM8960_REG_ADDR_CLASS_D_CTL_1,
    (0x01 & WM8960_REG_MASK_CLASS_D_CTL_1_SPK_OP_EN));
  if (err_code != 0) {
    return err_code;
  }

  return 0;
}

int WM8960::Pause() {
  HAL_I2S_DMAPause(&i2s_handle);
  return 0;
}

int WM8960::Stop() {
  HAL_I2S_DMAStop(&i2s_handle);
  return 0;
}

int WM8960::Write(const uint16_t *data, uint16_t size) {
  HAL_I2S_Transmit_DMA(&i2s_handle, (uint16_t *) data, size);   
  return 0;
}

//std::expected<uint8_t, int> ReadRegister(uint8_t reg_addr) {
//  uint8_t reg_data;
//  if (HAL_I2C_Mem_Read(
//    &i2c_handle,
//    WM8960_I2C_ADDR,
//    reg_addr,
//    I2C_MEMADD_SIZE_8BIT,
//    &reg_data,
//    1,
//    100,
//  ) != HAL_OK) {
//    return std::unexpected<int>(-1);
//  }
//  return reg_data;
//}

int WriteRegister(uint8_t reg_addr, uint8_t reg_data) {
  HAL_I2C_Mem_Write(
    &i2c_handle,
    WM8960_I2C_ADDR,
    reg_addr,
    I2C_MEMADD_SIZE_8BIT,
    &reg_data,
    1,
    100);
  //  return -1;
  // }
  return 0;
}

