#include "audio/stream.hpp"

#include <stm32f4xx_hal_dma.h>
#include <stm32f4xx_hal_dma_ex.h>
#include <stm32f4xx_hal_sai.h>

#include "errors.hpp"
#include "logging.hpp"
#include "stm32f4xx_hal_def.h"

using namespace deloop;

const uint16_t kFrameSize = 3;
static struct {
  bool initialized;
  SAI_HandleTypeDef sai_rx_handle;
  SAI_HandleTypeDef sai_tx_handle;
  // TODO: Is this already stored in a struct?
  bool is_tx_A_active;
  bool is_rx_A_active;
  uint32_t rx_buf_A[kFrameSize];
  uint32_t rx_buf_B[kFrameSize];
  uint32_t tx_buf_A[kFrameSize];
  uint32_t tx_buf_B[kFrameSize];
} state_ = {0};

static Error convertStatus(HAL_StatusTypeDef status);
static HAL_StatusTypeDef enableRxDMA(SAI_HandleTypeDef *sai_handle,
                                     uint8_t *dst_A, uint8_t *dst_B,
                                     uint16_t size);
static HAL_StatusTypeDef enableTxDMA(SAI_HandleTypeDef *sai_handle,
                                     uint8_t *src_A, uint8_t *src_B,
                                     uint16_t size);
static void rxXferComplete(DMA_HandleTypeDef *dma_handle);
static void txXferComplete(DMA_HandleTypeDef *dma_handle);

Error audio_stream::init(SAI_Block_TypeDef *sai_rx, SAI_Block_TypeDef *sai_tx) {
  // Initialize SAI Rx.
  __HAL_SAI_RESET_HANDLE_STATE(&state_.sai_rx_handle);
  state_.sai_rx_handle.Instance = sai_rx;
  __HAL_SAI_DISABLE(&state_.sai_rx_handle);

  state_.sai_rx_handle.Init.AudioMode = SAI_MODESLAVE_RX;
  state_.sai_rx_handle.Init.Synchro = SAI_SYNCHRONOUS;
  state_.sai_rx_handle.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE;
  state_.sai_rx_handle.Init.NoDivider = SAI_MASTERDIVIDER_DISABLE;
  state_.sai_rx_handle.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_1QF;
  // The following settings are applied by HAL_SAI_InitProtocol below:
  // - Protocol, DataSize, FirstBit, ClockStrobing, etc.
  state_.sai_rx_handle.Init.ClockSource = SAI_CLKSOURCE_PLLSAI;
  state_.sai_rx_handle.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_48K;

  if (HAL_SAI_InitProtocol(&state_.sai_rx_handle, SAI_I2S_STANDARD,
                           SAI_PROTOCOL_DATASIZE_24BIT, 2) != HAL_OK) {
    DELOOP_LOG_ERROR("[AUDIO_STREAM] Failed to initialize SAI peripheral: %d",
                     (uint32_t)HAL_SAI_GetError(&state_.sai_rx_handle));
    return Error::kFailedToInitializePeripheral;
  }

  // Initialize SAI Tx.
  __HAL_SAI_RESET_HANDLE_STATE(&state_.sai_tx_handle);
  state_.sai_tx_handle.Instance = sai_tx;
  __HAL_SAI_DISABLE(&state_.sai_tx_handle);

  state_.sai_tx_handle.Init.AudioMode = SAI_MODEMASTER_TX;
  state_.sai_tx_handle.Init.Synchro = SAI_ASYNCHRONOUS;
  state_.sai_tx_handle.Init.OutputDrive = SAI_OUTPUTDRIVE_ENABLE;
  state_.sai_tx_handle.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;
  state_.sai_tx_handle.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_1QF;
  // The following settings are applied by HAL_SAI_InitProtocol below:
  // - Protocol, DataSize, FirstBit, ClockStrobing, etc.
  state_.sai_tx_handle.Init.ClockSource = SAI_CLKSOURCE_PLLSAI;
  state_.sai_tx_handle.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_48K;

  if (HAL_SAI_InitProtocol(&state_.sai_tx_handle, SAI_I2S_STANDARD,
                           SAI_PROTOCOL_DATASIZE_24BIT, 2) != HAL_OK) {
    DELOOP_LOG_ERROR("[AUDIO_STREAM] Failed to initialize SAI peripheral: %d",
                     (uint32_t)HAL_SAI_GetError(&state_.sai_tx_handle));
    return Error::kFailedToInitializePeripheral;
  }

  state_.tx_buf_A[0] = 0x000100;
  state_.tx_buf_A[1] = 0x000200;
  state_.tx_buf_A[2] = 0x000300;

  state_.tx_buf_B[0] = 0x000000;
  state_.tx_buf_B[1] = 0x000000;
  state_.tx_buf_B[2] = 0x000000;

  state_.initialized = true;
  return Error::kOk;
}

Error audio_stream::start(void) {
  if (!state_.initialized) {
    return Error::kNotInitialized;
  }

  auto status = enableRxDMA(&state_.sai_rx_handle, (uint8_t *)state_.rx_buf_A,
                            (uint8_t *)state_.rx_buf_B, kFrameSize);
  auto err = convertStatus(status);
  if (err != Error::kOk) {
    DELOOP_LOG_ERROR("[AUDIO_STREAM] Failed to enable Rx DMA: %d", err);
    return err;
  }

  status = enableTxDMA(&state_.sai_tx_handle, (uint8_t *)state_.tx_buf_A,
                       (uint8_t *)state_.tx_buf_B, kFrameSize);
  err = convertStatus(status);
  if (err != Error::kOk) {
    DELOOP_LOG_ERROR("[AUDIO_STREAM] Failed to enable Tx DMA: %d", err);
    return err;
  }

  return Error::kOk;
}

Error audio_stream::stop(void) {
  if (!state_.initialized) {
    return Error::kNotInitialized;
  }

  return Error::kOk;
}

static Error convertStatus(HAL_StatusTypeDef status) {
  switch (status) {
  case HAL_OK:
    return Error::kOk;
  case HAL_BUSY:
    return Error::kSaiBusy;
  case HAL_ERROR:
    return Error::kSaiHalError;
  case HAL_TIMEOUT:
    return Error::kSaiTimeout;
  default:
    return Error::kSaiHalError;
  }
}

static HAL_StatusTypeDef enableRxDMA(SAI_HandleTypeDef *sai_handle,
                                     uint8_t *dst_A, uint8_t *dst_B,
                                     uint16_t size) {
  if (sai_handle == nullptr) {
    return HAL_ERROR;
  } else if (sai_handle->State != HAL_SAI_STATE_READY) {
    return HAL_BUSY;
  }

  sai_handle->hdmarx->XferCpltCallback = rxXferComplete;
  sai_handle->hdmarx->XferM1CpltCallback = rxXferComplete;
  sai_handle->hdmarx->XferErrorCallback = rxXferComplete;

  __HAL_LOCK(sai_handle);
  sai_handle->ErrorCode = HAL_SAI_ERROR_NONE;
  sai_handle->State = HAL_SAI_STATE_BUSY_RX;

  uint32_t src_addr = (uint32_t)&sai_handle->Instance->DR;
  uint32_t dst_A_addr = (uint32_t)dst_A;
  uint32_t dst_B_addr = (uint32_t)dst_B;
  auto status = HAL_DMAEx_MultiBufferStart_IT(sai_handle->hdmarx, src_addr,
                                              dst_A_addr, dst_B_addr, size);
  if (status != HAL_OK) {
    __HAL_UNLOCK(sai_handle);
    return status;
  }

  sai_handle->Instance->CR1 |= SAI_xCR1_DMAEN;
  if ((sai_handle->Instance->CR1 & SAI_xCR1_SAIEN) == RESET) {
    __HAL_SAI_ENABLE(sai_handle);
  }

  __HAL_UNLOCK(sai_handle);
  return HAL_OK;
}

static HAL_StatusTypeDef enableTxDMA(SAI_HandleTypeDef *sai_handle,
                                     uint8_t *src_A, uint8_t *src_B,
                                     uint16_t size) {
  if (sai_handle == nullptr) {
    return HAL_ERROR;
  } else if (sai_handle->State != HAL_SAI_STATE_READY) {
    return HAL_BUSY;
  }

  sai_handle->hdmatx->XferCpltCallback = txXferComplete;
  sai_handle->hdmatx->XferM1CpltCallback = txXferComplete;
  sai_handle->hdmatx->XferErrorCallback = txXferComplete;

  __HAL_LOCK(sai_handle);
  sai_handle->ErrorCode = HAL_SAI_ERROR_NONE;
  sai_handle->State = HAL_SAI_STATE_BUSY_TX;

  uint32_t src_A_addr = (uint32_t)src_A;
  uint32_t src_B_addr = (uint32_t)src_B;
  uint32_t dst_addr = (uint32_t)&sai_handle->Instance->DR;
  auto status = HAL_DMAEx_MultiBufferStart_IT(sai_handle->hdmatx, src_A_addr,
                                              dst_addr, src_B_addr, size);
  if (status != HAL_OK) {
    __HAL_UNLOCK(sai_handle);
    return status;
  }

  sai_handle->Instance->CR1 |= SAI_xCR1_DMAEN;
  if ((sai_handle->Instance->CR1 & SAI_xCR1_SAIEN) == RESET) {
    __HAL_SAI_ENABLE(sai_handle);
  }

  __HAL_UNLOCK(sai_handle);
  return HAL_OK;
}

static void rxXferComplete(DMA_HandleTypeDef *dma_handle) {
  if (!state_.initialized) {
    // TODO: Throw an error
    return;
  }

  state_.is_rx_A_active = !state_.is_rx_A_active;
}

static void txXferComplete(DMA_HandleTypeDef *dma_handle) {
  if (!state_.initialized) {
    // TODO: Throw an error
    return;
  }

  state_.is_tx_A_active = !state_.is_tx_A_active;
}
