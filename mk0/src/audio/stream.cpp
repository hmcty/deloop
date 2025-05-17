#include "audio/stream.hpp"

#include <stm32f4xx_hal.h>
#include <stm32f4xx_hal_dma.h>
#include <stm32f4xx_hal_dma_ex.h>
#include <stm32f4xx_hal_sai.h>

#include <FreeRTOS.h> // Must appear before other FreeRTOS includes
#include <task.h>

#include "audio/scheduler.hpp"
#include "board/stm32f4xx_it.h"
#include "errors.hpp"
#include "logging.hpp"
#include "portmacro.h"
#include "stm32f4xx_hal_def.h"

using namespace deloop;

const size_t kTaskStackSize = configMINIMAL_STACK_SIZE * 5;
const uint16_t kFrameSize = 64;

const UBaseType_t kRxNotifIndex = 0;
const UBaseType_t kTxNotifIndex = 1;

static struct {
  bool initialized;
  SAI_HandleTypeDef sai_rx_handle;
  SAI_HandleTypeDef sai_tx_handle;
  int32_t rx_buf[2][kFrameSize];
  int32_t tx_buf[2][kFrameSize];
  TaskHandle_t audio_stream_task;
  StaticTask_t task_buffer;
  StackType_t task_stack[kTaskStackSize];
} state_ = {0};

static Error convertStatus(HAL_StatusTypeDef status);
static HAL_StatusTypeDef enableRxDMA(SAI_HandleTypeDef *sai_handle,
                                     uint8_t *dst_A, uint8_t *dst_B,
                                     uint16_t size);
static HAL_StatusTypeDef enableTxDMA(SAI_HandleTypeDef *sai_handle,
                                     uint8_t *src_A, uint8_t *src_B,
                                     uint16_t size);
static void audioStreamLoop(void *args);

static void emptyCallback(DMA_HandleTypeDef *sai_handle);
static void rxMem0XferComplete(DMA_HandleTypeDef *dma_handle);
static void rxMem1XferComplete(DMA_HandleTypeDef *dma_handle);
static void txMem0XferComplete(DMA_HandleTypeDef *dma_handle);
static void txMem1XferComplete(DMA_HandleTypeDef *dma_handle);

void Audio_DMA_Rx_IRQHandler(void) {
  if (!state_.initialized) {
    // TODO: Handle error.
    return;
  }

  HAL_DMA_IRQHandler(state_.sai_rx_handle.hdmarx);
}

void Audio_DMA_Tx_IRQHandler(void) {
  if (!state_.initialized) {
    // TODO: Handle error.
    return;
  }

  HAL_DMA_IRQHandler(state_.sai_tx_handle.hdmatx);
}

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

  state_.initialized = true;
  return Error::kOk;
}

Error audio_stream::start(void) {
  if (!state_.initialized) {
    return Error::kNotInitialized;
  }

  // Create the audio stream task.
  state_.audio_stream_task =
      xTaskCreateStatic(audioStreamLoop, "AudioStream", kTaskStackSize, nullptr,
                        1, state_.task_stack, &state_.task_buffer);

  auto status = enableRxDMA(&state_.sai_rx_handle, (uint8_t *)state_.rx_buf[0],
                            (uint8_t *)state_.rx_buf[1], kFrameSize);
  auto err = convertStatus(status);
  if (err != Error::kOk) {
    DELOOP_LOG_ERROR("[AUDIO_STREAM] Failed to enable Rx DMA: %d", err);
    return err;
  }

  status = enableTxDMA(&state_.sai_tx_handle, (uint8_t *)state_.tx_buf[0],
                       (uint8_t *)state_.tx_buf[1], kFrameSize);
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

  sai_handle->hdmarx->XferCpltCallback = rxMem0XferComplete;
  sai_handle->hdmarx->XferM1CpltCallback = rxMem1XferComplete;
  sai_handle->hdmarx->XferErrorCallback = emptyCallback;

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

  sai_handle->hdmatx->XferCpltCallback = txMem0XferComplete;
  sai_handle->hdmatx->XferM1CpltCallback = txMem1XferComplete;
  sai_handle->hdmatx->XferErrorCallback = emptyCallback;

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

static void audioStreamLoop(void *args) {
  (void)args;

  while (true) {
    // TODO: Implement timeouts
    uint32_t tx_flag =
        ulTaskNotifyTakeIndexed(kTxNotifIndex, pdTRUE, portMAX_DELAY);
    uint32_t rx_flag =
        ulTaskNotifyTakeIndexed(kRxNotifIndex, pdTRUE, portMAX_DELAY);
    if (tx_flag != rx_flag) {
      DELOOP_LOG_ERROR("[AUDIO_STREAM] Tx and Rx notification mismatch: %d",
                       tx_flag);
      continue;
    }

    uint32_t indx = rx_flag - 1;
    if (indx > 2) {
      DELOOP_LOG_ERROR("[AUDIO_STREAM] Invalid notification: %d", indx);
      continue;
    }
    deloop::audio_scheduler::process(kFrameSize, state_.tx_buf[indx],
                                     state_.rx_buf[indx]);
  }
}

static void rxMem0XferComplete(DMA_HandleTypeDef *dma_handle) {
  if (!state_.initialized) {
    // TODO: Throw an error
    return;
  }

  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xTaskNotifyIndexedFromISR(state_.audio_stream_task, kRxNotifIndex, 1,
                            eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void rxMem1XferComplete(DMA_HandleTypeDef *dma_handle) {
  if (!state_.initialized) {
    // TODO: Throw an error
    return;
  }

  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xTaskNotifyIndexedFromISR(state_.audio_stream_task, kRxNotifIndex, 2,
                            eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void txMem0XferComplete(DMA_HandleTypeDef *dma_handle) {
  if (!state_.initialized) {
    // TODO: Throw an error
    return;
  }

  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xTaskNotifyIndexedFromISR(state_.audio_stream_task, kTxNotifIndex, 1,
                            eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void txMem1XferComplete(DMA_HandleTypeDef *dma_handle) {
  if (!state_.initialized) {
    // TODO: Throw an error
    return;
  }

  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xTaskNotifyIndexedFromISR(state_.audio_stream_task, kTxNotifIndex, 2,
                            eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void emptyCallback(DMA_HandleTypeDef *dma_handle) { (void)dma_handle; }
