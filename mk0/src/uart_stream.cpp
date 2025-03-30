#include "uart_stream.hpp"

#include <cstring>

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"

#include "FreeRTOS.h"
#include "queue.h"

#include "log.pb.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "stream.pb.h"

#include "errors.hpp"
#include "logging.hpp"

const size_t kQueueSize = 8;
const size_t kTaskStackSize = configMINIMAL_STACK_SIZE * 2;

static struct {
  bool initialized;
  UART_HandleTypeDef uart_handle;

  StaticQueue_t queue_info;
  uint8_t queue_buffer[kQueueSize * sizeof(StreamPacket)];
  QueueHandle_t queue_handle;

  StaticTask_t task_info;
  StackType_t tack_stack[kTaskStackSize];
} _state = {0};

static void StreamTask(void *pvParameters) {
  while (true) {
    if (_state.initialized == false) {
      vTaskDelay(1000);
      continue;
    }

    StreamPacket packet = StreamPacket_init_zero;
    if (xQueueReceive(_state.queue_handle, &packet, portMAX_DELAY) == pdTRUE) {
      uint8_t buffer[StreamPacket_size + 1];
      pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
      pb_encode(&stream, StreamPacket_fields, &packet);

      // Terminate the stream with a null character.
      buffer[stream.bytes_written] = '\0';
      HAL_UART_Transmit(&_state.uart_handle, buffer, stream.bytes_written + 1,
                        1000);
    }
  }
}

deloop::Error deloop::UartStream::Init() {
  if (_state.initialized) {
    return deloop::Error::kAlreadyInitialized;
  }

  memset(&_state, 0, sizeof(_state));

  // Initialize UART.
  _state.uart_handle.Instance = USART2;
  _state.uart_handle.Init.BaudRate = 115200;
  _state.uart_handle.Init.WordLength = UART_WORDLENGTH_8B;
  _state.uart_handle.Init.StopBits = UART_STOPBITS_1;
  _state.uart_handle.Init.Parity = UART_PARITY_NONE;
  _state.uart_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  _state.uart_handle.Init.Mode = UART_MODE_TX_RX;
  if (HAL_UART_Init(&_state.uart_handle) != HAL_OK) {
    return deloop::Error::kFailedToInitializePeripheral;
  }

  // Initialize queue.
  _state.queue_handle =
      xQueueCreateStatic(kQueueSize, sizeof(StreamPacket), _state.queue_buffer,
                         &_state.queue_info);

  xTaskCreateStatic(StreamTask, "UART Stream", kTaskStackSize, NULL, 1,
                    _state.tack_stack, &_state.task_info);

  _state.initialized = true;
  return deloop::Error::kOk;
}

deloop::Error deloop::UartStream::Write(const uint8_t *data, uint16_t size) {
  return deloop::Error::kOk;
}

void deloop::SubmitLog(LogLevel level, const uint64_t id, ...) {
  LogEntry entry = LogEntry_init_zero;
  // entry.level = level;
  entry.id = id;

  StreamPacket packet = StreamPacket_init_zero;
  packet.which_payload = StreamPacket_log_tag;
  packet.payload.log = entry;

  xQueueSend(_state.queue_handle, (void *)&packet, 1000);
}
