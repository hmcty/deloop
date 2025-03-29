#include "uart_stream.hpp"

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"

#include "errors.hpp"

const size_t kQueueSize = 8;

static struct {
  bool initialized;
  UART_HandleTypeDef uart_handle;
  StaticQueue_t queue_info;
  uint8_t queue_buffer[kQueueSize * sizeof(uint8_t)];
  QueueHandle_t queue_handle;
} _state = {0};

static void StreamTask(void *pvParameters) {
  while (true) {
    if (_state.initialized == false) {
      vTaskDelay(1000);
      continue;
    }

    if (xQueueReceive(_state.queue_handle, &_state.queue_buffer,
                      portMAX_DELAY) == pdTRUE) {
      HAL_UART_Transmit(&_state.uart_handle, &_state.queue_buffer, 1, 1000);
    }
  }
}

deloop::Error deloop::UartStream::Init() {
  if (_state.initialized) {
    return deloop::Error::kAlreadyInitialized;
  }

  memset(&_state, 0, sizeof(_state));

  // Initialize UART.
  UART_handle.Instance = USART2;
  UART_handle.Init.BaudRate = 115200;
  UART_handle.Init.WordLength = UART_WORDLENGTH_8B;
  UART_handle.Init.StopBits = UART_STOPBITS_1;
  UART_handle.Init.Parity = UART_PARITY_NONE;
  UART_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  UART_handle.Init.Mode = UART_MODE_TX_RX;
  if (HAL_UART_Init(&UART_handle) != HAL_OK) {
    return deloop::Error::kFailedToInitializePeripheral;
  }

  // Initialize queue.
  _state.queue_handle = xQueueCreateStatic(
      kQueueSize, sizeof(uint8_t), _state.queue_buffer, &_state.queue_info);

  return deloop::Error::kOk;
}

deloop::Error deloop::UartStream::Write(const uint8_t *data, uint16_t size) {
  return deloop::Error::kOk;
}
