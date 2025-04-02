#pragma once

#include "stm32f4xx_hal.h"
#include <stdint.h>

#include "errors.hpp"

#define WM8960_I2C_ADDR (0x34) // Pg 62 of datasheet

// WM8960 Register Addresses
#define WM8960_REG_ADDR_LEFT_CHAN_VOL (uint8_t)(0x0A)
#define WM8960_REG_ADDR_RIGHT_CHAN_VOL (uint8_t)(0x0B)
#define WM8960_REG_ADDR_POWER_MGMT_2 (uint8_t)(0x1A)
#define WM8960_REG_ADDR_LEFT_MIXER_CTL (uint8_t)(0x22)
#define WM8960_REG_ADDR_RIGHT_MIXER_CTL (uint8_t)(0x2D)
#define WM8960_REG_ADDR_POWER_MGMT_3 (uint8_t)(0x2F)
#define WM8960_REG_ADDR_CLASS_D_CTL_1 (uint8_t)(0x31)

// WM8960 Register Masks
#define WM8960_REG_MASK_POWER_MGMT_2_OUT3 (uint8_t)(0x01 << 0)  // 0=OFF, 1=ON
#define WM8960_REG_MASK_POWER_MGMT_2_SPKR (uint8_t)(0x01 << 3)  // 0=OFF, 1=ON
#define WM8960_REG_MASK_POWER_MGMT_2_SPKL (uint8_t)(0x01 << 4)  // 0=OFF, 1=ON
#define WM8960_REG_MASK_POWER_MGMT_2_ROUT1 (uint8_t)(0x01 << 5) // 0=OFF, 1=ON
#define WM8960_REG_MASK_POWER_MGMT_2_LOUT1 (uint8_t)(0x01 << 6) // 0=OFF, 1=ON
#define WM8960_REG_MASK_POWER_MGMT_2_DACR (uint8_t)(0x01 << 7)  // 0=OFF, 1=ON
#define WM8960_REG_MASK_POWER_MGMT_2_DACL (uint8_t)(0x01 << 8)  // 0=OFF, 1=ON

#define WM8960_REG_MASK_POWER_MGMT_3_LOMIX (uint8_t)(0x01 << 3) // 0=OFF, 1=ON
#define WM8960_REG_MASK_POWER_MGMT_3_ROMIX (uint8_t)(0x01 << 4) // 0=OFF, 1=ON

#define WM8960_REG_MASK_LEFT_CHAN_VOL_DACVU (uint8_t)(0x01 << 8) // 0=OFF, 1=ON
#define WM8960_REG_MASK_LEFT_CHAN_VOL_LDACVOL                                  \
  (uint8_t)(0x7F) // 0=MUTE, 0x7F=0dB

#define WM8960_REG_MASK_RIGHT_CHAN_VOL_DACVU (uint8_t)(0x01 << 8) // 0=OFF, 1=ON
#define WM8960_REG_MASK_RIGHT_CHAN_VOL_RDACVOL                                 \
  (uint8_t)(0x7F) // 0=MUTE, 0x7F=0dB

#define WM8960_REG_MASK_LEFT_MIXER_CTL_LD2LO (uint8_t)(0x01 << 8) // 0=OFF, 1=ON
#define WM8960_REG_MASK_RIGHT_MIXER_CTL_RD2RO                                  \
  (uint8_t)(0x01 << 8) // 0=OFF, 1=ON

#define WM8960_REG_MASK_CLASS_D_CTL_1_SPK_OP_EN                                \
  (uint8_t)(0x02 << 6) // 0x00=OFF, 0x01=L, 0x10=R, 0x11=L+R

namespace deloop {
namespace WM8960 {

deloop::Error Init(void);
deloop::Error ResetToDefaults(void);

// NOTE: WM8960 does not support reading from registers.
deloop::Error WriteRegister(uint8_t reg_addr, uint16_t data);

// int SetVolume(uint8_t volume);

// int Play();
// int Pause();
// int Stop();

// int Write(const uint16_t *data, uint16_t size);

} // namespace WM8960
} // namespace deloop
