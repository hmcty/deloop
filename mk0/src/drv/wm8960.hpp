#pragma once

#include "stm32f4xx_hal.h"

#include <stdint.h>

#include "errors.hpp"

#define WM8960_I2C_ADDR (0x34) // Pg 62 of datasheet

// ---------------------------
// WM8960 Register Addresses
// ---------------------------

#define WM8960_REG_ADDR_ADC_DAC_CTL_1 (uint8_t)(0x05)
#define WM8960_REG_ADDR_LEFT_INPUT_VOL (uint8_t)(0x0A)
#define WM8960_REG_ADDR_RIGHT_INPUT_VOL (uint8_t)(0x0B)
#define WM8960_REG_ADDR_RESET (uint8_t)(0x0F)
#define WM8960_REG_ADDR_POWER_MGMT_1 (uint8_t)(0x19)
#define WM8960_REG_ADDR_POWER_MGMT_2 (uint8_t)(0x1A)
#define WM8960_REG_ADDR_LEFT_OUT_MIX (uint8_t)(0x22)
#define WM8960_REG_ADDR_RIGHT_OUT_MIX (uint8_t)(0x25)
#define WM8960_REG_ADDR_LEFT_SPKR_VOL (uint8_t)(0x28)
#define WM8960_REG_ADDR_RIGHT_SPKR_VOL (uint8_t)(0x29)
#define WM8960_REG_ADDR_POWER_MGMT_3 (uint8_t)(0x2F)
#define WM8960_REG_ADDR_CLASS_D_CTL_1 (uint8_t)(0x31)

// -------------------------
// ADC/DAC Control (1) Flags
// -------------------------

// DACMU: DAC mute control.
#define WM8960_REG_FLAG_ADC_DAC_CTL_1_DACMU_OFF (uint16_t)(0b0 << 3)
#define WM8960_REG_FLAG_ADC_DAC_CTL_1_DACMU_ON (uint16_t)(0b1 << 3)

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

// DACL: Enable/disable left channel DAC.
#define WM8960_REG_FLAG_POWER_MGMT_2_DACL_OFF (uint16_t)(0b0 << 8)
#define WM8960_REG_FLAG_POWER_MGMT_2_DACL_ON (uint16_t)(0b1 << 8)

// DACR: Enable/disable right channel DAC.
#define WM8960_REG_FLAG_POWER_MGMT_2_DACR_OFF (uint16_t)(0b0 << 7)
#define WM8960_REG_FLAG_POWER_MGMT_2_DACR_ON (uint16_t)(0b1 << 7)

// LOUT1: Enable/disable LOUT1 output buffer.
#define WM8960_REG_FLAG_POWER_MGMT_2_LOUT1_OFF (uint16_t)(0b0 << 6)
#define WM8960_REG_FLAG_POWER_MGMT_2_LOUT1_ON (uint16_t)(0b1 << 6)

// ROUT1: Enable/disable ROUT1 output buffer.
#define WM8960_REG_FLAG_POWER_MGMT_2_ROUT1_OFF (uint16_t)(0b0 << 5)
#define WM8960_REG_FLAG_POWER_MGMT_2_ROUT1_ON (uint16_t)(0b1 << 5)

// SPKL: Enable/disable SPK_LP/SPK_LN output buffer.
#define WM8960_REG_FLAG_POWER_MGMT_2_SPKL_OFF (uint16_t)(0b0 << 4)
#define WM8960_REG_FLAG_POWER_MGMT_2_SPKL_ON (uint16_t)(0b1 << 4)

// SPKR: Enable/disable SPK_RP/SPK_RN output buffer.
#define WM8960_REG_FLAG_POWER_MGMT_2_SPKR_OFF (uint16_t)(0b0 << 3)
#define WM8960_REG_FLAG_POWER_MGMT_2_SPKR_ON (uint16_t)(0b1 << 3)

// NOTE: Bit 2 is reserved.

// OUT3: Enable/disable mono output and mono mixer.
#define WM8960_REG_FLAG_POWER_MGMT_2_OUT3_OFF (uint16_t)(0b0 << 1)
#define WM8960_REG_FLAG_POWER_MGMT_2_OUT3_ON (uint16_t)(0b1 << 1)

// PLL_EN: Enable/disable PLL.
#define WM8960_REG_FLAG_POWER_MGMT_2_PLL_EN_OFF (uint16_t)(0b0 << 0)
#define WM8960_REG_FLAG_POWER_MGMT_2_PLL_EN_ON (uint16_t)(0b1 << 0)

// ------------------
// Left Out Mix Flags
// ------------------

// LD2LO: Connect left DAC to left output mixer.
#define WM8960_REG_FLAG_LEFT_OUT_MIX_LD2LO_OFF (uint16_t)(0b0 << 8)
#define WM8960_REG_FLAG_LEFT_OUT_MIX_LD2LO_ON (uint16_t)(0b1 << 8)

// LI2LO: Connect LINPUT3 to left output mixer.
#define WM8960_REG_FLAG_LEFT_OUT_MIX_LI2LO_OFF (uint16_t)(0b0 << 7)
#define WM8960_REG_FLAG_LEFT_OUT_MIX_LI2LO_ON (uint16_t)(0b1 << 7)

// NOTE: Bits 3:0 are reserved.

// -------------------
// Right Out Mix Flags
// -------------------

// RD2RO: Connect right DAC to right output mixer.
#define WM8960_REG_FLAG_RIGHT_OUT_MIX_RD2RO_OFF (uint16_t)(0b0 << 8)
#define WM8960_REG_FLAG_RIGHT_OUT_MIX_RD2RO_ON (uint16_t)(0b1 << 8)

// RI2RO: Connect RINPUT3 to right output mixer.
#define WM8960_REG_FLAG_RIGHT_OUT_MIX_RI2RO_OFF (uint16_t)(0b0 << 7)
#define WM8960_REG_FLAG_RIGHT_OUT_MIX_RI2RO_ON (uint16_t)(0b1 << 7)

// NOTE: Bits 3:0 are reserved.

// -------------------------
// Left Speaker Volume Flags
// -------------------------

// SPKVU: Left speaker volume update.
#define WM8960_REG_FLAG_LEFT_SPKR_VOL_SPKVU (uint16_t)(0b1 << 8)

// SPKLZC: Enable/disable left speaker zero cross.
#define WM8960_REG_FLAG_LEFT_SPKR_VOL_SPKLZC_OFF (uint16_t)(0b0 << 7)
#define WM8960_REG_FLAG_LEFT_SPKR_VOL_SPKLZC_ON (uint16_t)(0b1 << 7)

// SPKLVOL: Left speaker volume control (+6dB to -73dB in 1dB steps).
#define WM8960_REG_FLAG_LEFT_SPKR_VOL_SPKLVOL_MIN_dB (int)(-73)
#define WM8960_REG_FLAG_LEFT_SPKR_VOL_SPKLVOL_MAX_dB (int)(6)
#define WM8960_REG_FLAG_RIGHT_SPKR_VOL_SPKLVOL_MUTE (uint16_t)(0b0000000)
#define WM8960_REG_FLAG_LEFT_SPKR_VOL_SPKLVOL(X) \
  (uint16_t)(std::max((0b1111111 - (6 - (X))) & 0x7F, 0b0110000))

// --------------------------
// Right Speaker Volume Flags
// --------------------------

// SPKVU: Right speaker volume update.
#define WM8960_REG_FLAG_RIGHT_SPKR_VOL_SPKVU (uint16_t)(0b1 << 8)

// SPKRZC: Enable/disable right speaker zero cross.
#define WM8960_REG_FLAG_RIGHT_SPKR_VOL_SPKRZC_OFF (uint16_t)(0b0 << 7)
#define WM8960_REG_FLAG_RIGHT_SPKR_VOL_SPKRZC_ON (uint16_t)(0b1 << 7)

// SPKRVOL: Right speaker volume control (+6dB to -73dB in 1dB steps).
#define WM8960_REG_FLAG_RIGHT_SPKR_VOL_SPKRVOL_MIN_dB (int)(-73)
#define WM8960_REG_FLAG_RIGHT_SPKR_VOL_SPKRVOL_MAX_dB (int)(6)
#define WM8960_REG_FLAG_RIGHT_SPKR_VOL_SPKRVOL_MUTE (uint16_t)(0b0000000)
#define WM8960_REG_FLAG_RIGHT_SPKR_VOL_SPKRVOL(X) \
  (uint16_t)(std::max((0b1111111 - (6 - (X))) & 0x7F, 0b0110000))

// --------------------
// Power Mgmt (3) Flags
// --------------------

// NOTE: Bits 8:6 are reserved.

// LMIC: Enable/disable input PGA for left channel.
#define WM8960_REG_FLAG_POWER_MGMT_3_LMIC_OFF (uint16_t)(0b0 << 5)
#define WM8960_REG_FLAG_POWER_MGMT_3_LMIC_ON (uint16_t)(0b1 << 5)

// RMIC: Enable/disable input PGA for right channel.
#define WM8960_REG_FLAG_POWER_MGMT_3_RMIC_OFF (uint16_t)(0b0 << 4)
#define WM8960_REG_FLAG_POWER_MGMT_3_RMIC_ON (uint16_t)(0b1 << 4)

// LOMIX: Enable/disable left channel output mixer.
#define WM8960_REG_FLAG_POWER_MGMT_3_LOMIX_OFF (uint16_t)(0b0 << 3)
#define WM8960_REG_FLAG_POWER_MGMT_3_LOMIX_ON (uint16_t)(0b1 << 3)

// ROMIX: Enable/disable right channel output mixer.
#define WM8960_REG_FLAG_POWER_MGMT_3_ROMIX_OFF (uint16_t)(0b0 << 2)
#define WM8960_REG_FLAG_POWER_MGMT_3_ROMIX_ON (uint16_t)(0b1 << 2)

// NOTE: Bits 1:0 are reserved.

// -------------------------
// Class D Control (1) Flags
// -------------------------

// SPK_OP_EN: Enable/disable Class D output.
#define WM8960_REG_FLAG_CLASS_D_CTL_1_SPK_OP_EN_OFF (uint16_t)(0b00 << 6)
#define WM8960_REG_FLAG_CLASS_D_CTL_1_SPK_OP_EN_LEFT_ONLY (uint16_t)(0b01 << 6)
#define WM8960_REG_FLAG_CLASS_D_CTL_1_SPK_OP_EN_RIGHT_ONLY (uint16_t)(0b10 << 6)
#define WM8960_REG_FLAG_CLASS_D_CTL_1_SPK_OP_EN_BOTH (uint16_t)(0b11 << 6)

namespace deloop {
namespace WM8960 {

deloop::Error Init(void);
deloop::Error ResetToDefaults(void);

// NOTE: WM8960 does not support reading from registers.
deloop::Error WriteRegister(uint8_t reg_addr, uint16_t data);

deloop::Error StartRecording(uint8_t *buf, uint16_t size);
deloop::Error StopRecording(void);

deloop::Error StartPlayback(uint8_t *buf, uint16_t size);
deloop::Error StopPlayback(void);

deloop::Error SetVolume(float volume);

} // namespace WM8960
} // namespace deloop
