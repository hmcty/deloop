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
  uint8_t queue_buffer[kQueueSize * StreamPacket_size];
  QueueHandle_t queue_handle;

  StaticTask_t task_info;
  StackType_t tack_stack[kTaskStackSize];
} _state;

static void StreamTask(void *pvParameters);

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

void deloop::SubmitLog(deloop::LogLevel level, const uint64_t hash,
                       const std::array<LogArg, 4> &args) {
  LogRecord record = LogRecord_init_zero;
  record.hash = hash;
  switch (level) {
  case deloop::LogLevel::INFO:
    record.level = LogLevel_INFO;
    break;
  case deloop::LogLevel::WARNING:
    record.level = LogLevel_WARNING;
    break;
  case deloop::LogLevel::ERROR:
    record.level = LogLevel_ERROR;
    break;
  default:
    record.level = LogLevel_INFO;
    break;
  }

  for (size_t i = 0; i < args.size(); i++) {
    LogRecord_Arg arg = LogRecord_Arg_init_zero;
    if (args[i].type == deloop::LogArg::Type::kUnset) {
      break;
    }

    switch (args[i].type) {
    case deloop::LogArg::Type::kU32:
      arg.which_value = LogRecord_Arg_u32_tag;
      arg.value.u32 = args[i].value.u32;
      break;
    case deloop::LogArg::Type::kI32:
      arg.which_value = LogRecord_Arg_i32_tag;
      arg.value.i32 = args[i].value.i32;
      break;
    case deloop::LogArg::Type::kF32:
      arg.which_value = LogRecord_Arg_f32_tag;
      arg.value.f32 = args[i].value.f32;
      break;
    default:
      break;
    }

    record.args[i] = arg;
    record.args_count++;
  }

  StreamPacket packet = StreamPacket_init_zero;
  packet.which_payload = StreamPacket_log_tag;
  packet.payload.log = record;

  xQueueSend(_state.queue_handle, (void *)&packet, 1000);
}

static void StreamTask(void *pvParameters) {
  (void)pvParameters;

  while (true) {
    if (_state.initialized == false) {
      vTaskDelay(1000);
      continue;
    }

    StreamPacket packet = StreamPacket_init_zero;
    if (xQueueReceive(_state.queue_handle, &packet, portMAX_DELAY) == pdTRUE) {
      uint8_t buffer[StreamPacket_size + 2];
      buffer[0] = 0xEB;
      pb_ostream_t stream =
          pb_ostream_from_buffer(buffer + 2, sizeof(buffer) - 2);
      pb_encode(&stream, StreamPacket_fields, &packet);
      buffer[1] = (uint8_t)stream.bytes_written;

      // Terminate the stream with a null character.
      HAL_UART_Transmit(&_state.uart_handle, buffer,
                        (uint16_t)stream.bytes_written + 2, 1000);
    }
  }
}
