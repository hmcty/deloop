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

static struct {
  bool initialized;
  SAI_HandleTypeDef sai_tx_handle;
  SAI_HandleTypeDef sai_rx_handle;
  I2C_HandleTypeDef i2c_handle;
} _state;

static void SAI_RxCpltCallback(SAI_HandleTypeDef *hsai) {
  DELOOP_LOG_INFO("[WM8960] SAI Rx complete callback.");
}

static void SAI_ErrorCallback(SAI_HandleTypeDef *hsai) {
  volatile uint32_t error = HAL_SAI_GetError(hsai);
  (void)error;
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

deloop::Error deloop::WM8960::Init() {
  if (_state.initialized) {
    return deloop::Error::kAlreadyInitialized;
  }

  DELOOP_LOG_INFO("[WM8960] Initializing...");
  memset(&_state, 0, sizeof(_state));

  // Initialize I2C.
  _state.i2c_handle.Instance = I2C1;
  _state.i2c_handle.Init.ClockSpeed = 100000;
  _state.i2c_handle.Init.DutyCycle = I2C_DUTYCYCLE_2;
  _state.i2c_handle.Init.OwnAddress1 = 0;
  _state.i2c_handle.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  _state.i2c_handle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  _state.i2c_handle.Init.OwnAddress2 = 0;
  _state.i2c_handle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  _state.i2c_handle.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&_state.i2c_handle) != HAL_OK) {
    DELOOP_LOG_ERROR("[WM8960] Failed to initialize I2C peripheral.");
    return deloop::Error::kFailedToInitializePeripheral;
  }

  // Initialize SAI Rx.
  __HAL_SAI_RESET_HANDLE_STATE(&_state.sai_rx_handle);
  _state.sai_rx_handle.Instance = SAI1_Block_A;
  __HAL_SAI_DISABLE(&_state.sai_rx_handle);

  _state.sai_rx_handle.Init.AudioMode = SAI_MODESLAVE_RX;
  _state.sai_rx_handle.Init.Synchro = SAI_SYNCHRONOUS;
  _state.sai_rx_handle.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE;
  _state.sai_rx_handle.Init.NoDivider = SAI_MASTERDIVIDER_DISABLE;
  _state.sai_rx_handle.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_1QF;
  // The following settings are applied by HAL_SAI_InitProtocol below:
  // - Protocol, DataSize, FirstBit, ClockStrobing, etc.
  _state.sai_rx_handle.Init.ClockSource = SAI_CLKSOURCE_PLLSAI;
  _state.sai_rx_handle.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_48K;

  if (HAL_SAI_InitProtocol(&_state.sai_rx_handle, SAI_I2S_STANDARD,
                           SAI_PROTOCOL_DATASIZE_24BIT, 2) != HAL_OK) {
    DELOOP_LOG_ERROR("[WM8960] Failed to initialize SAI peripheral: %d",
                     (uint32_t)HAL_SAI_GetError(&_state.sai_rx_handle));
    return deloop::Error::kFailedToInitializePeripheral;
  }

  // Initialize SAI Tx.
  __HAL_SAI_RESET_HANDLE_STATE(&_state.sai_tx_handle);
  _state.sai_tx_handle.Instance = SAI1_Block_B;
  __HAL_SAI_DISABLE(&_state.sai_tx_handle);

  _state.sai_tx_handle.Init.AudioMode = SAI_MODEMASTER_TX;
  _state.sai_tx_handle.Init.Synchro = SAI_ASYNCHRONOUS;
  _state.sai_tx_handle.Init.OutputDrive = SAI_OUTPUTDRIVE_ENABLE;
  _state.sai_tx_handle.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;
  _state.sai_tx_handle.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_1QF;
  // The following settings are applied by HAL_SAI_InitProtocol below:
  // - Protocol, DataSize, FirstBit, ClockStrobing, etc.
  _state.sai_tx_handle.Init.ClockSource = SAI_CLKSOURCE_PLLSAI;
  _state.sai_tx_handle.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_48K;

  if (HAL_SAI_InitProtocol(&_state.sai_tx_handle, SAI_I2S_STANDARD,
                           SAI_PROTOCOL_DATASIZE_24BIT, 2) != HAL_OK) {
    DELOOP_LOG_ERROR("[WM8960] Failed to initialize SAI peripheral: %d",
                     (uint32_t)HAL_SAI_GetError(&_state.sai_tx_handle));
    return deloop::Error::kFailedToInitializePeripheral;
  }

  // TODO: Configure callbacks.

  _state.initialized = true;
  return deloop::WM8960::ResetToDefaults();
}

deloop::Error deloop::WM8960::ResetToDefaults(void) {
  DELOOP_LOG_INFO("[WM8960] Resetting to defaults..");

  // Any write to the reset register will set all others to default.
  DELOOP_RETURN_IF_ERROR(
      deloop::WM8960::WriteRegister(WM8960_REG_ADDR_RESET, 0x01));

  DELOOP_RETURN_IF_ERROR(deloop::WM8960::WriteRegister(
      WM8960_REG_ADDR_ADC_DAC_CTL_1, WM8960_REG_FLAG_ADC_DAC_CTL_1_DACMU_OFF));

  // Configure VMID divider for playback+recording, enable VREF (required for
  // ADC/DAC), and power on L/R ADCs.
  DELOOP_RETURN_IF_ERROR(deloop::WM8960::WriteRegister(
      WM8960_REG_ADDR_POWER_MGMT_1,
      WM8960_REG_FLAG_POWER_MGMT_1_VMIDSEL_50kOhm |
          WM8960_REG_FLAG_POWER_MGMT_1_VREF_ON |
          WM8960_REG_FLAG_POWER_MGMT_1_AINL_ON |
          WM8960_REG_FLAG_POWER_MGMT_1_AINR_ON |
          WM8960_REG_FLAG_POWER_MGMT_1_ADCL_ON |
          WM8960_REG_FLAG_POWER_MGMT_1_ADCR_ON));

  DELOOP_RETURN_IF_ERROR(deloop::WM8960::WriteRegister(
      WM8960_REG_ADDR_POWER_MGMT_2, WM8960_REG_FLAG_POWER_MGMT_2_DACL_ON |
                                        WM8960_REG_FLAG_POWER_MGMT_2_DACR_ON |
                                        WM8960_REG_FLAG_POWER_MGMT_2_SPKL_ON |
                                        WM8960_REG_FLAG_POWER_MGMT_2_SPKR_ON));

  DELOOP_RETURN_IF_ERROR(deloop::WM8960::WriteRegister(
      WM8960_REG_ADDR_POWER_MGMT_3, WM8960_REG_FLAG_POWER_MGMT_3_LMIC_ON |
                                        WM8960_REG_FLAG_POWER_MGMT_3_RMIC_ON |
                                        WM8960_REG_FLAG_POWER_MGMT_3_LOMIX_ON |
                                        WM8960_REG_FLAG_POWER_MGMT_3_ROMIX_ON));

  DELOOP_RETURN_IF_ERROR(deloop::WM8960::WriteRegister(
      WM8960_REG_ADDR_LEFT_OUT_MIX, WM8960_REG_FLAG_LEFT_OUT_MIX_LD2LO_ON));

  DELOOP_RETURN_IF_ERROR(deloop::WM8960::WriteRegister(
      WM8960_REG_ADDR_RIGHT_OUT_MIX, WM8960_REG_FLAG_RIGHT_OUT_MIX_RD2RO_ON));

  DELOOP_RETURN_IF_ERROR(deloop::WM8960::WriteRegister(
      WM8960_REG_ADDR_CLASS_D_CTL_1,
      WM8960_REG_FLAG_CLASS_D_CTL_1_SPK_OP_EN_LEFT_ONLY));

  __HAL_SAI_ENABLE(&_state.sai_tx_handle);
  __HAL_SAI_ENABLE(&_state.sai_rx_handle);

  return deloop::Error::kOk;
}

deloop::Error deloop::WM8960::WriteRegister(uint8_t reg_addr, uint16_t data) {
  if (!_state.initialized) {
    return deloop::Error::kNotInitialized;
  } else if ((reg_addr > 0x7F) || (data > 0x01FF)) {
    return deloop::Error::kInvalidArgument;
  }

  uint8_t ctrl_data[2] = {
      static_cast<uint8_t>((reg_addr << 1) | ((data >> 8) & 0x01)),
      static_cast<uint8_t>(data & 0xFF)};
  int retry_attempts = 3;
  HAL_StatusTypeDef status = HAL_OK;
  while (retry_attempts--) {
    status = HAL_I2C_Master_Transmit(&_state.i2c_handle, WM8960_I2C_ADDR,
                                     ctrl_data, 2, 1000);
    if (status == HAL_OK) {
      break;
    } else if (retry_attempts == 0) {
      // DELOOP_LOG_ERROR(
      //   "[WM8960] Failed to write to register 0x%02X: 0x%02X", reg_addr,
      //   data);
      break;
    }
  }

  switch (status) {
  case HAL_OK:
    return deloop::Error::kOk;
  case HAL_BUSY:
    return deloop::Error::kI2cBusyOnWrite;
  case HAL_ERROR:
    return deloop::Error::kI2cHalWriteError;
  case HAL_TIMEOUT:
    return deloop::Error::kI2cTimeoutOnWrite;
  default:
    return deloop::Error::kI2cHalWriteError;
  }
}

deloop::Error deloop::WM8960::StartRecording(uint8_t *buf, uint16_t size) {
  if (!_state.initialized) {
    return deloop::Error::kNotInitialized;
  } else if (buf == nullptr || size == 0) {
    return deloop::Error::kInvalidArgument;
  }

  return ConvertStatus(HAL_SAI_Receive_DMA(&_state.sai_rx_handle, buf, size));
}

deloop::Error deloop::WM8960::StopRecording(void) {
  if (!_state.initialized) {
    return deloop::Error::kNotInitialized;
  }

  return ConvertStatus(HAL_SAI_DMAStop(&_state.sai_rx_handle));
}

deloop::Error deloop::WM8960::StartPlayback(uint8_t *buf, uint16_t size) {
  if (!_state.initialized) {
    return deloop::Error::kNotInitialized;
  } else if (buf == nullptr || size == 0) {
    return deloop::Error::kInvalidArgument;
  }

  return ConvertStatus(HAL_SAI_Transmit_DMA(&_state.sai_tx_handle, buf, size));
}

deloop::Error deloop::WM8960::StopPlayback(void) {
  if (!_state.initialized) {
    return deloop::Error::kNotInitialized;
  }

  return ConvertStatus(HAL_SAI_DMAStop(&_state.sai_tx_handle));
}

deloop::Error deloop::WM8960::SetVolume(float volume) {
  if (!_state.initialized) {
    return deloop::Error::kNotInitialized;
  } else if (volume < 0.0f || volume > 1.0f) {
    return deloop::Error::kInvalidArgument;
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
      WriteRegister(WM8960_REG_ADDR_LEFT_SPKR_VOL, volume_flag));

  DELOOP_RETURN_IF_ERROR(WriteRegister(
      WM8960_REG_ADDR_RIGHT_SPKR_VOL,
      WM8960_REG_FLAG_RIGHT_SPKR_VOL_SPKVU | // Required to take effect.
          volume_flag));

  // DELOOP_LOG_INFO("[WM8960] Volume set to 0x%04X (%d dB)", volume_flag,
  // volume_dB);
  return deloop::Error::kOk;
}
