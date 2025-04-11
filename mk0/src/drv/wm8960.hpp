#pragma once

#include "stm32f4xx_hal.h"
#include <stdint.h>

#include "errors.hpp"

#define WM8960_I2C_ADDR (0x34) // Pg 62 of datasheet

// ---------------------------
// WM8960 Register Addresses
// ---------------------------

#define WM8960_REG_ADDR_LEFT_INPUT_VOL (uint8_t)(0x0A)
#define WM8960_REG_ADDR_RIGHT_INPUT_VOL (uint8_t)(0x0B)
#define WM8960_REG_ADDR_RESET (uint8_t)(0x0F)
#define WM8960_REG_ADDR_POWER_MGMT_1 (uint8_t)(0x19)
#define WM8960_REG_ADDR_POWER_MGMT_2 (uint8_t)(0x1A)
#define WM8960_REG_ADDR_LEFT_OUT_MIX (uint8_t)(0x22)
#define WM8960_REG_ADDR_RIGHT_OUT_MIX (uint8_t)(0x2D)
#define WM8960_REG_ADDR_POWER_MGMT_3 (uint8_t)(0x2F)
#define WM8960_REG_ADDR_CLASS_D_CTL_1 (uint8_t)(0x31)

// --------------------
// Power Mgmt (1) Flags
// --------------------

// VMIDSEL: Selects the resistance of the VMID divider
//   - 00: VMID disabled
//   - 01: 50kOhm (for playback and record)
//   - 10: 250kOhm (for low power standby mode)
//   - 11: 5kOhm (for fast start-up)
#define WM8960_REG_FLAG_POWER_MGMT_1_VMIDSEL_OFF (uint16_t)(0b00 << 7)
#define WM8960_REG_FLAG_POWER_MGMT_1_VMIDSEL_50kOhm (uint16_t)(0b01 << 7)
#define WM8960_REG_FLAG_POWER_MGMT_1_VMIDSEL_250kOhm (uint16_t)(0b10 << 7)
#define WM8960_REG_FLAG_POWER_MGMT_1_VMIDSEL_5kOhm (uint16_t)(0b11 << 7)

// VREF: Enable/disable reference voltage (necessary for all functions)
#define WM8960_REG_FLAG_POWER_MGMT_1_VREF_OFF (uint16_t)(0b0 << 6)
#define WM8960_REG_FLAG_POWER_MGMT_1_VREF_ON (uint16_t)(0b1 << 6)

// AINL: Power on/off analogue in PGA left
#define WM8960_REG_FLAG_POWER_MGMT_1_AINL_OFF (uint16_t)(0b0 << 5)
#define WM8960_REG_FLAG_POWER_MGMT_1_AINL_ON (uint16_t)(0b1 << 5)

// AINR: Power on/off analogue in PGA right
#define WM8960_REG_FLAG_POWER_MGMT_1_AINR_OFF (uint16_t)(0b0 << 4)
#define WM8960_REG_FLAG_POWER_MGMT_1_AINR_ON (uint16_t)(0b1 << 4)

// ADCL: Power on/off the left channel ADC.
#define WM8960_REG_FLAG_POWER_MGMT_1_ADCL_OFF (uint16_t)(0b0 << 3)
#define WM8960_REG_FLAG_POWER_MGMT_1_ADCL_ON (uint16_t)(0b1 << 3)

// ADCR: Power on/off the right channel ADC.
#define WM8960_REG_FLAG_POWER_MGMT_1_ADCR_OFF (uint16_t)(0b0 << 2)
#define WM8960_REG_FLAG_POWER_MGMT_1_ADCR_ON (uint16_t)(0b1 << 2)

// --------------------
// Power Mgmt (2) Flags
// --------------------

// TODO

// --------------------
// Power Mgmt (3) Flags
// --------------------

// NOTE: Bits 8:6 are reserved.

// LMIC: Power on/off input PGA enable for left channel.
#define WM8960_REG_FLAG_POWER_MGMT_3_LMIC_OFF (uint16_t)(0b0 << 5)
#define WM8960_REG_FLAG_POWER_MGMT_3_LMIC_ON (uint16_t)(0b1 << 5)

// RMIC: Power on/off input PGA enable for right channel.
#define WM8960_REG_FLAG_POWER_MGMT_3_RMIC_OFF (uint16_t)(0b0 << 4)
#define WM8960_REG_FLAG_POWER_MGMT_3_RMIC_ON (uint16_t)(0b1 << 4)

// NOTE: Bits 1:0 are reserved.

namespace deloop {
namespace WM8960 {

deloop::Error Init(void);
deloop::Error ResetToDefaults(void);

// NOTE: WM8960 does not support reading from registers.
deloop::Error WriteRegister(uint8_t reg_addr, uint16_t data);

deloop::Error ExchangeData(uint16_t *tx, uint16_t *rx, uint16_t size);

// int SetVolume(uint8_t volume);

// int Play();
// int Pause();
// int Stop();

// int Write(const uint16_t *data, uint16_t size);

} // namespace WM8960
} // namespace deloop
