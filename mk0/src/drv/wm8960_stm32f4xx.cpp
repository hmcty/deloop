#include "drv/wm8960.hpp"

#include <cstdint>
#include <cstring>

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_i2c.h"
#include "stm32f4xx_hal_i2s.h"
#include "stm32f4xx_hal_i2s_ex.h"
#include "stm32f4xx_hal_sai.h"

#include "errors.hpp"
#include "logging.hpp"

static struct {
  bool initialized;
  SAI_HandleTypeDef sai_tx_handle;
  SAI_HandleTypeDef sai_rx_handle;
  I2C_HandleTypeDef i2c_handle;
} _state;

static void SAI_ErrorCallback(SAI_HandleTypeDef *hsai) {
  volatile uint32_t error = HAL_SAI_GetError(hsai);
  (void)error;
}

deloop::Error deloop::WM8960::Init() {
  if (_state.initialized) {
    return deloop::Error::kAlreadyInitialized;
  }

  DELOOP_LOG_INFO("[WM8960] Initializing...");
  memset(&_state, 0, sizeof(_state));

  // Initialize I2S.
  // _state.i2s_handle.Instance = SPI2; // TODO: Allow for usage of other SPI peripherals.
  // _state.i2s_handle.Init.Mode = I2S_MODE_MASTER_TX;
  // _state.i2s_handle.Init.Standard = I2S_STANDARD_PHILIPS;
  // _state.i2s_handle.Init.DataFormat = I2S_DATAFORMAT_24B;
  // _state.i2s_handle.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
  // _state.i2s_handle.Init.AudioFreq = I2S_AUDIOFREQ_48K;
  // _state.i2s_handle.Init.CPOL = I2S_CPOL_LOW;
  // _state.i2s_handle.Init.ClockSource = I2S_CLOCK_PLL;
  // _state.i2s_handle.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_ENABLE;
  // if (HAL_I2S_Init(&_state.i2s_handle) != HAL_OK) {
  //   DELOOP_LOG_ERROR("[WM8960] Failed to initialize I2S peripheral: %d", (uint32_t) HAL_I2S_GetError(&_state.i2s_handle));
  //   return deloop::Error::kFailedToInitializePeripheral;
  // }


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

  // Initialize SAI Tx.
  __HAL_SAI_RESET_HANDLE_STATE(&_state.sai_tx_handle);
  _state.sai_tx_handle.Instance = SAI1_Block_B;
  __HAL_SAI_DISABLE(&_state.sai_tx_handle);

  _state.sai_tx_handle.Init.AudioMode = SAI_MODEMASTER_TX;
  _state.sai_tx_handle.Init.Synchro = SAI_ASYNCHRONOUS;
  _state.sai_tx_handle.Init.OutputDrive = SAI_OUTPUTDRIVE_ENABLE;
  _state.sai_tx_handle.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;
  _state.sai_tx_handle.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_1QF;
  // _state.sai_tx_handle.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_FULL;
  _state.sai_tx_handle.Init.ClockSource = SAI_CLKSOURCE_PLLSAI;
  // _state.sai_tx_handle.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_48K;
  _state.sai_tx_handle.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_22K;
  _state.sai_tx_handle.Init.Protocol = SAI_FREE_PROTOCOL;
  _state.sai_tx_handle.Init.DataSize = SAI_DATASIZE_16;
  _state.sai_tx_handle.Init.FirstBit = SAI_FIRSTBIT_MSB;
  _state.sai_tx_handle.Init.ClockStrobing = SAI_CLOCKSTROBING_RISINGEDGE;

//_state.sai_tx_handle.FrameInit.FrameLength       = 32;
//_state.sai_tx_handle.FrameInit.ActiveFrameLength = 16;
//_state.sai_tx_handle.FrameInit.FSDefinition      = SAI_FS_CHANNEL_IDENTIFICATION;
//_state.sai_tx_handle.FrameInit.FSPolarity        = SAI_FS_ACTIVE_LOW;
//_state.sai_tx_handle.FrameInit.FSOffset          = SAI_FS_BEFOREFIRSTBIT;

//_state.sai_tx_handle.SlotInit.FirstBitOffset = 1;
//_state.sai_tx_handle.SlotInit.SlotSize       = SAI_SLOTSIZE_DATASIZE;
//_state.sai_tx_handle.SlotInit.SlotNumber     = 2;
//_state.sai_tx_handle.SlotInit.SlotActive     = (SAI_SLOTACTIVE_0 | SAI_SLOTACTIVE_1);

//if (HAL_SAI_Init(&_state.sai_tx_handle) != HAL_OK) {
//  DELOOP_LOG_ERROR("[WM8960] Failed to initialize SAI peripheral: %d", (uint32_t) HAL_SAI_GetError(&_state.sai_tx_handle));
//  return deloop::Error::kFailedToInitializePeripheral;
//}
  if (HAL_SAI_InitProtocol(&_state.sai_tx_handle, SAI_I2S_STANDARD, SAI_PROTOCOL_DATASIZE_16BIT, 2) != HAL_OK) {
    DELOOP_LOG_ERROR("[WM8960] Failed to initialize SAI peripheral: %d", (uint32_t) HAL_SAI_GetError(&_state.sai_tx_handle));
    return deloop::Error::kFailedToInitializePeripheral;
  }

  HAL_SAI_RegisterCallback(&_state.sai_tx_handle, HAL_SAI_ERROR_CB_ID, SAI_ErrorCallback);

  // Initialize SAI Rx.
  // _state.sai_rx_handle.Instance = SAI1_Block_B;
  // _state.sai_rx_handle.Init.AudioMode = SAI_MODESLAVE_RX;
  // _state.sai_rx_handle.Init.Synchro = SAI_SYNCHRONOUS;
  // _state.sai_rx_handle.Init.OutputDrive = SAI_OUTPUTDRIVE_ENABLE;
  // _state.sai_rx_handle.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;
  // _state.sai_rx_handle.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_1QF;
  // _state.sai_rx_handle.Init.ClockSource = SAI_CLKSOURCE_PLLSAI;
  // _state.sai_rx_handle.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_48K;
  // _state.sai_rx_handle.Init.Protocol = I2S_STANDARD_PHILIPS;
  // _state.sai_rx_handle.Init.DataSize = SAI_DATASIZE_24;
  // _state.sai_rx_handle.Init.FirstBit = SAI_FIRSTBIT_MSB;
  // _state.sai_rx_handle.Init.ClockStrobing = SAI_CLOCKSTROBING_RISINGEDGE;
  // _state.sai_rx_handle.FrameInit.FrameLength = 32;
  // _state.sai_rx_handle.FrameInit.ActiveFrameLength = 16;
  // _state.sai_rx_handle.FrameInit.FSDefinition =
  // SAI_FS_CHANNEL_IDENTIFICATION; _state.sai_rx_handle.FrameInit.FSPolarity =
  // SAI_FS_ACTIVE_LOW; _state.sai_rx_handle.FrameInit.FSOffset =
  // SAI_FS_BEFOREFIRSTBIT; _state.sai_rx_handle.SlotInit.FirstBitOffset = 0;
  // _state.sai_rx_handle.SlotInit.SlotSize = SAI_SLOTSIZE_DATASIZE;
  // _state.sai_rx_handle.SlotInit.SlotNumber = 2;
  // _state.sai_rx_handle.SlotInit.SlotActive = (SAI_SLOTACTIVE_0 |
  // SAI_SLOTACTIVE_1); if (HAL_SAI_Init(&_state.sai_rx_handle) != HAL_OK) {
  //   DELOOP_LOG_ERROR("[WM8960] Failed to initialize SAI peripheral: %d",
  //   (uint32_t) HAL_SAI_GetError(&_state.sai_rx_handle)); return
  //   deloop::Error::kFailedToInitializePeripheral;
  // }

  _state.initialized = true;
  return deloop::Error::kOk;
  // return deloop::WM8960::ResetToDefaults();
}

deloop::Error deloop::WM8960::ResetToDefaults(void) {
  DELOOP_LOG_INFO("[WM8960] Resetting to defaults...");

  // Any write to the reset register will set all others to default.
  DELOOP_RETURN_IF_ERROR(deloop::WM8960::WriteRegister(WM8960_REG_ADDR_RESET, 0x01));

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

  // TODO: Configure POWER_MGMT_2 here for DAC support.

  DELOOP_RETURN_IF_ERROR(deloop::WM8960::WriteRegister(
    WM8960_REG_ADDR_POWER_MGMT_3,
    WM8960_REG_FLAG_POWER_MGMT_3_LMIC_ON |
    WM8960_REG_FLAG_POWER_MGMT_3_RMIC_ON));

  // DELOOP_RETURN_IF_ERROR(deloop::WM8960::WriteRegister(
  //     WM8960_REG_ADDR_POWER_MGMT_3,
  //     WM8960_REG_MASK_POWER_MGMT_3_LOMIX |
  //     WM8960_REG_MASK_POWER_MGMT_3_ROMIX));
  // DELOOP_RETURN_IF_ERROR(deloop::WM8960::WriteRegister(
  //     WM8960_REG_ADDR_LEFT_MIXER_CTL, WM8960_REG_MASK_LEFT_MIXER_CTL_LD2LO));
  // DELOOP_RETURN_IF_ERROR(deloop::WM8960::WriteRegister(
  //     WM8960_REG_ADDR_RIGHT_MIXER_CTL,
  //     WM8960_REG_MASK_RIGHT_MIXER_CTL_RD2RO));
  // DELOOP_RETURN_IF_ERROR(deloop::WM8960::WriteRegister(
  //     WM8960_REG_ADDR_POWER_MGMT_2,
  //     (WM8960_REG_MASK_POWER_MGMT_2_SPKR | WM8960_REG_MASK_POWER_MGMT_2_SPKL
  //     |
  //      WM8960_REG_MASK_POWER_MGMT_2_ROUT1 |
  //      WM8960_REG_MASK_POWER_MGMT_2_LOUT1 | WM8960_REG_MASK_POWER_MGMT_2_DACL
  //      | WM8960_REG_MASK_POWER_MGMT_2_DACR)));

  // HAL_I2S_DMAResume(&_state.i2s_handle);

  return deloop::Error::kOk;
}

deloop::Error deloop::WM8960::WriteRegister(uint8_t reg_addr, uint16_t data) {
  if (!_state.initialized) {
    return deloop::Error::kNotInitialized;
  } else if ((reg_addr > 0x7F) || (data > 0x01FF)) {
    return deloop::Error::kInvalidArgument;
  }

  uint8_t ctrl_data[2] = {(reg_addr << 1) | ((data >> 8) & 0x01),
                          (uint8_t)data & 0xFF};
  auto status = HAL_I2C_Master_Transmit(&_state.i2c_handle, WM8960_I2C_ADDR,
                                        ctrl_data, 2, 1000);
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

deloop::Error deloop::WM8960::ExchangeData(uint8_t *tx, uint8_t *rx, uint16_t size) {
  if (!_state.initialized) {
    return deloop::Error::kNotInitialized;
  }

  __HAL_SAI_ENABLE(&_state.sai_tx_handle);
  auto status = HAL_SAI_Transmit_DMA(&_state.sai_tx_handle, tx, size);
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

// int WM8960::SetVolume(uint8_t volume) {
//   if (volume > 64) {
//     volume = 64;
//   }
//   int err_code = WriteRegister(
//     WM8960_REG_ADDR_LEFT_CHAN_VOL,
//     WM8960_REG_MASK_LEFT_CHAN_VOL_DACVU |
//     (WM8960_REG_MASK_LEFT_CHAN_VOL_LDACVOL & volume));
//   if (err_code != 0) {
//     return err_code;
//   }
//
//   return WriteRegister(
//     WM8960_REG_ADDR_RIGHT_CHAN_VOL,
//     WM8960_REG_MASK_RIGHT_CHAN_VOL_DACVU |
//     (WM8960_REG_MASK_RIGHT_CHAN_VOL_RDACVOL & volume));
// }
//
// int WM8960::Play() {
//   HAL_I2S_DMAResume(&i2s_handle);
//
//   int err_code = WriteRegister(
//     WM8960_REG_ADDR_CLASS_D_CTL_1,
//     (0x01 & WM8960_REG_MASK_CLASS_D_CTL_1_SPK_OP_EN));
//   if (err_code != 0) {
//     return err_code;
//   }
//
//   return 0;
// }
//
// int WM8960::Pause() {
//   HAL_I2S_DMAPause(&i2s_handle);
//   return 0;
// }
//
// int WM8960::Stop() {
//   HAL_I2S_DMAStop(&i2s_handle);
//   return 0;
// }
//
// int WM8960::Write(const uint16_t *data, uint16_t size) {
//   HAL_I2S_Transmit_DMA(&i2s_handle, (uint16_t *) data, size);
//   return 0;
// }

// std::expected<uint8_t, int> ReadRegister(uint8_t reg_addr) {
//   uint8_t reg_data;
//   if (HAL_I2C_Mem_Read(
//     &i2c_handle,
//     WM8960_I2C_ADDR,
//     reg_addr,
//     I2C_MEMADD_SIZE_8BIT,
//     &reg_data,
//     1,
//     100,
//   ) != HAL_OK) {
//     return std::unexpected<int>(-1);
//   }
//   return reg_data;
// }

// deloop::Error WriteRegister(uint8_t reg_addr, uint8_t reg_data) {
//   HAL_I2C_Mem_Write(&_state.i2c_handle, WM8960_I2C_ADDR, reg_addr,
//                     I2C_MEMADD_SIZE_8BIT, &reg_data, 1, 100);
//   return deloop::Error::kOk;
// }
