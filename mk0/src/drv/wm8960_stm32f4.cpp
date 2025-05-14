#include <cstdlib>
#include <stm32f4xx_hal.h>
#include <stm32f4xx_hal_i2c.h>
#include <stm32f4xx_hal_i2s.h>
#include <stm32f4xx_hal_i2s_ex.h>
#include <stm32f4xx_hal_sai.h>

#include <cstdint>
#include <cstring>

#include "drv/wm8960.hpp"
#include "errors.hpp"
#include "logging.hpp"
#include "stm32f446xx.h"

static void SAI_RxCpltCallback(SAI_HandleTypeDef *hsai) {
  DELOOP_LOG_INFO("[WM8960] SAI Rx complete callback.");
}

static inline deloop::Error ConvertStatus(HAL_StatusTypeDef status) {
  switch (status) {
  case HAL_OK:
    return deloop::Error::kOk;
  case HAL_BUSY:
    return deloop::Error::kSaiBusy;
  case HAL_ERROR:
    return deloop::Error::kSaiHalError;
  case HAL_TIMEOUT:
    return deloop::Error::kSaiTimeout;
  default:
    return deloop::Error::kSaiHalError;
  }
}

namespace deloop {

Error WM8960::init(I2C_TypeDef *i2c, SAI_Block_TypeDef *sai_rx,
                   SAI_Block_TypeDef *sai_tx) {
  if (initialized_) {
    return Error::kAlreadyInitialized;
  }

  // TODO: Make instances configurable.
  // Initialize I2C.
  i2c_handle_.Instance = i2c;
  i2c_handle_.Init.ClockSpeed = 100000;
  i2c_handle_.Init.DutyCycle = I2C_DUTYCYCLE_2;
  i2c_handle_.Init.OwnAddress1 = 0;
  i2c_handle_.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  i2c_handle_.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  i2c_handle_.Init.OwnAddress2 = 0;
  i2c_handle_.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  i2c_handle_.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&i2c_handle_) != HAL_OK) {
    DELOOP_LOG_ERROR("[WM8960] Failed to initialize I2C peripheral.");
    return Error::kFailedToInitializePeripheral;
  }

  // Initialize SAI Rx.
  __HAL_SAI_RESET_HANDLE_STATE(&sai_rx_handle_);
  sai_rx_handle_.Instance = sai_rx;
  __HAL_SAI_DISABLE(&sai_rx_handle_);

  sai_rx_handle_.Init.AudioMode = SAI_MODESLAVE_RX;
  sai_rx_handle_.Init.Synchro = SAI_SYNCHRONOUS;
  sai_rx_handle_.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE;
  sai_rx_handle_.Init.NoDivider = SAI_MASTERDIVIDER_DISABLE;
  sai_rx_handle_.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_1QF;
  // The following settings are applied by HAL_SAI_InitProtocol below:
  // - Protocol, DataSize, FirstBit, ClockStrobing, etc.
  sai_rx_handle_.Init.ClockSource = SAI_CLKSOURCE_PLLSAI;
  sai_rx_handle_.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_48K;

  if (HAL_SAI_InitProtocol(&sai_rx_handle_, SAI_I2S_STANDARD,
                           SAI_PROTOCOL_DATASIZE_24BIT, 2) != HAL_OK) {
    DELOOP_LOG_ERROR("[WM8960] Failed to initialize SAI peripheral: %d",
                     (uint32_t)HAL_SAI_GetError(&sai_rx_handle_));
    return Error::kFailedToInitializePeripheral;
  }

  // Initialize SAI Tx.
  __HAL_SAI_RESET_HANDLE_STATE(&sai_tx_handle_);
  sai_tx_handle_.Instance = sai_tx;
  __HAL_SAI_DISABLE(&sai_tx_handle_);

  sai_tx_handle_.Init.AudioMode = SAI_MODEMASTER_TX;
  sai_tx_handle_.Init.Synchro = SAI_ASYNCHRONOUS;
  sai_tx_handle_.Init.OutputDrive = SAI_OUTPUTDRIVE_ENABLE;
  sai_tx_handle_.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;
  sai_tx_handle_.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_1QF;
  // The following settings are applied by HAL_SAI_InitProtocol below:
  // - Protocol, DataSize, FirstBit, ClockStrobing, etc.
  sai_tx_handle_.Init.ClockSource = SAI_CLKSOURCE_PLLSAI;
  sai_tx_handle_.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_48K;

  if (HAL_SAI_InitProtocol(&sai_tx_handle_, SAI_I2S_STANDARD,
                           SAI_PROTOCOL_DATASIZE_24BIT, 2) != HAL_OK) {
    DELOOP_LOG_ERROR("[WM8960] Failed to initialize SAI peripheral: %d",
                     (uint32_t)HAL_SAI_GetError(&sai_tx_handle_));
    return Error::kFailedToInitializePeripheral;
  }

  // TODO: Configure callbacks.
  initialized_ = true;
  return resetToDefaults();
}

Error WM8960::resetToDefaults(void) {
  if (!initialized_) {
    return Error::kNotInitialized;
  }

  DELOOP_LOG_INFO("[WM8960] Resetting to defaults..");

  // Any write to the reset register will set all others to default.
  auto error = writeRegister(WM8960_REG_ADDR_RESET, 0x01);
  if (error != Error::kOk) {
    DELOOP_LOG_ERROR("[WM8960] Failed to reset to defaults: %d", error);
    return error;
  }

  error = writeRegister(WM8960_REG_ADDR_ADC_DAC_CTL_1,
                        WM8960_REG_FLAG_ADC_DAC_CTL_1_DACMU_OFF);
  if (error != Error::kOk) {
    DELOOP_LOG_ERROR("[WM8960] Failed to set ADC/DAC control: %d", error);
    return error;
  }

  // Configure VMID divider for playback+recording, enable VREF (required for
  // ADC/DAC), and power on L/R ADCs.
  error = writeRegister(WM8960_REG_ADDR_POWER_MGMT_1,
                        WM8960_REG_FLAG_POWER_MGMT_1_VMIDSEL_50kOhm |
                            WM8960_REG_FLAG_POWER_MGMT_1_VREF_ON |
                            WM8960_REG_FLAG_POWER_MGMT_1_AINL_ON |
                            WM8960_REG_FLAG_POWER_MGMT_1_AINR_ON |
                            WM8960_REG_FLAG_POWER_MGMT_1_ADCL_ON |
                            WM8960_REG_FLAG_POWER_MGMT_1_ADCR_ON);
  if (error != Error::kOk) {
    DELOOP_LOG_ERROR("[WM8960] Failed to configure power mgmt 1: %d", error);
    return error;
  }

  error = writeRegister(WM8960_REG_ADDR_POWER_MGMT_2,
                        WM8960_REG_FLAG_POWER_MGMT_2_DACL_ON |
                            WM8960_REG_FLAG_POWER_MGMT_2_DACR_ON |
                            WM8960_REG_FLAG_POWER_MGMT_2_SPKL_ON |
                            WM8960_REG_FLAG_POWER_MGMT_2_SPKR_ON);
  if (error != Error::kOk) {
    DELOOP_LOG_ERROR("[WM8960] Failed to configure power mgmt 2: %d", error);
    return error;
  }

  error = writeRegister(WM8960_REG_ADDR_POWER_MGMT_3,
                        WM8960_REG_FLAG_POWER_MGMT_3_LMIC_ON |
                            WM8960_REG_FLAG_POWER_MGMT_3_RMIC_ON |
                            WM8960_REG_FLAG_POWER_MGMT_3_LOMIX_ON |
                            WM8960_REG_FLAG_POWER_MGMT_3_ROMIX_ON);
  if (error != Error::kOk) {
    DELOOP_LOG_ERROR("[WM8960] Failed to configure power mgmt 3: %d", error);
    return error;
  }

  error = writeRegister(WM8960_REG_ADDR_LEFT_OUT_MIX,
                        WM8960_REG_FLAG_LEFT_OUT_MIX_LD2LO_ON);
  if (error != Error::kOk) {
    DELOOP_LOG_ERROR("[WM8960] Failed to configure left out mix: %d", error);
    return error;
  }

  error = writeRegister(WM8960_REG_ADDR_RIGHT_OUT_MIX,
                        WM8960_REG_FLAG_RIGHT_OUT_MIX_RD2RO_ON);
  if (error != Error::kOk) {
    DELOOP_LOG_ERROR("[WM8960] Failed to configure right out mix: %d", error);
    return error;
  }

  error = writeRegister(WM8960_REG_ADDR_CLASS_D_CTL_1,
                        WM8960_REG_FLAG_CLASS_D_CTL_1_SPK_OP_EN_BOTH);
  if (error != Error::kOk) {
    DELOOP_LOG_ERROR("[WM8960] Failed to configure class D control 1: %d",
                     error);
    return error;
  }

  __HAL_SAI_ENABLE(&sai_tx_handle_);
  __HAL_SAI_ENABLE(&sai_rx_handle_);

  return Error::kOk;
}

Error WM8960::writeRegister(uint8_t reg_addr, uint16_t data) {
  if (!initialized_) {
    return Error::kNotInitialized;
  } else if ((reg_addr > 0x7F) || (data > 0x01FF)) {
    return Error::kInvalidArgument;
  }

  uint8_t ctrl_data[2] = {
      static_cast<uint8_t>((reg_addr << 1) | ((data >> 8) & 0x01)),
      static_cast<uint8_t>(data & 0xFF)};
  int retry_attempts = 3;
  HAL_StatusTypeDef status = HAL_OK;
  while (retry_attempts--) {
    status = HAL_I2C_Master_Transmit(&i2c_handle_, WM8960_I2C_ADDR, ctrl_data,
                                     2, 1000);
    if (status == HAL_OK) {
      break;
    } else if (retry_attempts == 0) {
      // DELOOP_LOG_ERROR("[WM8960] Failed to write to register 0x%02X:
      // 0x%02X",
      //                  reg_addr, data);
      break;
    }
  }

  switch (status) {
  case HAL_OK:
    return Error::kOk;
  case HAL_BUSY:
    return Error::kI2cBusyOnWrite;
  case HAL_ERROR:
    return Error::kI2cHalWriteError;
  case HAL_TIMEOUT:
    return Error::kI2cTimeoutOnWrite;
  default:
    return Error::kI2cHalWriteError;
  }
}

Error WM8960::startRecording(uint8_t *buf, uint16_t size) {
  if (!initialized_) {
    return Error::kNotInitialized;
  } else if (buf == nullptr || size == 0) {
    return Error::kInvalidArgument;
  }

  return ConvertStatus(HAL_SAI_Receive_DMA(&sai_rx_handle_, buf, size));
}

Error WM8960::stopRecording(void) {
  if (!initialized_) {
    return Error::kNotInitialized;
  }

  return ConvertStatus(HAL_SAI_DMAStop(&sai_rx_handle_));
}

Error WM8960::startPlayback(uint8_t *buf, uint16_t size) {
  if (!initialized_) {
    return Error::kNotInitialized;
  } else if (buf == nullptr || size == 0) {
    return Error::kInvalidArgument;
  }

  return ConvertStatus(HAL_SAI_Transmit_DMA(&sai_tx_handle_, buf, size));
}

Error WM8960::stopPlayback(void) {
  if (!initialized_) {
    return Error::kNotInitialized;
  }

  return ConvertStatus(HAL_SAI_DMAStop(&sai_tx_handle_));
}

Error WM8960::setVolume(float volume) {
  if (!initialized_) {

  } else if (volume < 0.0f || volume > 1.0f) {
    return Error::kInvalidArgument;
  }

  const int kMaxVolume_dB = WM8960_REG_FLAG_RIGHT_SPKR_VOL_SPKRVOL_MAX_dB;
  const int kMinVolume_dB = WM8960_REG_FLAG_RIGHT_SPKR_VOL_SPKRVOL_MIN_dB;
  const int kVolumeRange_dB = kMaxVolume_dB - kMinVolume_dB;

  // Convert volume percentage to dB.
  int volume_dB = static_cast<int>(kMinVolume_dB + (kVolumeRange_dB * volume));
  volume_dB = std::max(kMinVolume_dB, std::min(kMaxVolume_dB, volume_dB));

  uint16_t volume_flag = WM8960_REG_FLAG_RIGHT_SPKR_VOL_SPKRVOL(volume_dB);
  // uint16_t volume_flag = 0b01110100;
  DELOOP_RETURN_IF_ERROR(
      writeRegister(WM8960_REG_ADDR_LEFT_SPKR_VOL, volume_flag));

  DELOOP_RETURN_IF_ERROR(writeRegister(
      WM8960_REG_ADDR_RIGHT_SPKR_VOL,
      WM8960_REG_FLAG_RIGHT_SPKR_VOL_SPKVU | // Required to take effect.
          volume_flag));

  // DELOOP_LOG_INFO("[WM8960] Volume set to 0x%04X (%d dB)", volume_flag,
  // volume_dB);
  return Error::kOk;
}

} // namespace deloop
