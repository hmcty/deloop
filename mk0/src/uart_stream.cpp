#include "uart_stream.hpp"

#include <cstring>
#include <pb_decode.h>
#include <pb_encode.h>
#include <stm32f4xx_hal.h>
#include <stm32f4xx_hal_uart.h>

#include <FreeRTOS.h> // Must appear before other FreeRTOS includes
#include <queue.h>

#include "command.pb.h"
#include "errors.hpp"
#include "log.pb.h"
#include "logging.hpp"
#include "stream.pb.h"

const size_t kStreamQueueSize = 8;
const size_t kCmdQueueSize = 8;
const size_t kTaskStackSize = configMINIMAL_STACK_SIZE * 2;

static struct {
  bool initialized;
  UART_HandleTypeDef *uart_handle;

  StaticQueue_t stream_queue_info;
  uint8_t stream_queue_buffer[kStreamQueueSize * StreamPacket_size];
  QueueHandle_t stream_queue_handle;

  StaticQueue_t cmd_queue_info;
  uint8_t cmd_queue_buffer[kCmdQueueSize * StreamPacket_size];
  QueueHandle_t cmd_queue_handle;

  StaticTask_t task_info;
  StackType_t task_stack[kTaskStackSize];

  // UART RX state
  uint8_t rx_buffer[StreamPacket_size + 2];
  bool rx_start_byte_received;
  uint8_t rx_packet_size;
} _state;

static void StreamTask(void *pvParameters);

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart != _state.uart_handle || !_state.initialized) {
    return;
  }

  // Process received byte
  if (!_state.rx_start_byte_received) {
    if (_state.rx_buffer[0] == 0xEB) { // Start byte
      _state.rx_start_byte_received = true;
      _state.rx_packet_size = 0;
    }

    HAL_UART_Receive_IT(_state.uart_handle, &_state.rx_buffer[0], 1);
  } else if (_state.rx_packet_size == 0) {
    _state.rx_packet_size = _state.rx_buffer[0];

    // Continue receiving to get the rest of the packet
    HAL_UART_Receive_IT(_state.uart_handle, &_state.rx_buffer[0],
                        _state.rx_packet_size);
  } else {

    Command cmd = Command_init_zero;
    pb_istream_t stream =
        pb_istream_from_buffer(&_state.rx_buffer[0], _state.rx_packet_size);
    if (pb_decode(&stream, Command_fields, &cmd)) {
      xQueueSendToBackFromISR(_state.cmd_queue_handle, (void *)&cmd, NULL);
    } else {
      DELOOP_LOG_ERROR_FROM_ISR("Failed to decode command");
    }

    // Reset for next packet
    _state.rx_start_byte_received = false;
    _state.rx_packet_size = 0;

    // Continue receiving for next packet
    HAL_UART_Receive_IT(_state.uart_handle, &_state.rx_buffer[0], 1);
  }
}

deloop::Error deloop::UartStream::init(UART_HandleTypeDef *uart_handle) {
  if (_state.initialized) {
    return deloop::Error::kAlreadyInitialized;
  }

  memset(&_state, 0, sizeof(_state));

  _state.uart_handle = uart_handle;

  // Initialize queues
  _state.stream_queue_handle =
      xQueueCreateStatic(kStreamQueueSize, sizeof(StreamPacket),
                         _state.stream_queue_buffer, &_state.stream_queue_info);
  _state.cmd_queue_handle =
      xQueueCreateStatic(kCmdQueueSize, sizeof(Command),
                         _state.cmd_queue_buffer, &_state.cmd_queue_info);

  xTaskCreateStatic(StreamTask, "UART Stream", kTaskStackSize, NULL, 1,
                    _state.task_stack, &_state.task_info);

  // Initialize RX state
  _state.rx_start_byte_received = false;
  _state.rx_packet_size = 0;
  _state.initialized = true;

  // Start UART reception
  HAL_UART_Receive_IT(_state.uart_handle, &_state.rx_buffer[0], 1);

  return deloop::Error::kOk;
}

void deloop::SubmitLog(deloop::LogLevel level, const uint64_t hash,
                       const std::array<LogArg, 4> &args, bool blocking) {
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

  if (blocking) {
    xQueueSendToBack(_state.stream_queue_handle, (void *)&packet, 1000);
  } else {
    xQueueSendToBackFromISR(_state.stream_queue_handle, (void *)&packet, NULL);
  }
}

void deloop::UartStream::sendCommandResponse(const CommandResponse &resp) {
  StreamPacket packet = StreamPacket_init_zero;
  packet.which_payload = StreamPacket_cmd_response_tag;
  packet.payload.cmd_response = resp;

  xQueueSendToBack(_state.stream_queue_handle, (void *)&packet, 1000);
}

QueueHandle_t deloop::UartStream::getCmdQueue() {
  return _state.cmd_queue_handle;
}

static void StreamTask(void *pvParameters) {
  (void)pvParameters;

  StreamPacket packet = StreamPacket_init_zero;
  uint8_t tx_buffer[StreamPacket_size + 2];

  while (true) {
    if (_state.initialized == false) {
      vTaskDelay(1000);
      continue;
    }

    if (xQueueReceive(_state.stream_queue_handle, &packet, portMAX_DELAY) ==
        pdTRUE) {
      tx_buffer[0] = 0xEB; // Start byte
      // TODO: Add checksum and escape sequence for start byte
      pb_ostream_t stream =
          pb_ostream_from_buffer(tx_buffer + 2, sizeof(tx_buffer) - 2);
      pb_encode(&stream, StreamPacket_fields, &packet);
      tx_buffer[1] = (uint8_t)stream.bytes_written;

      // Transmit the packet
      HAL_UART_Transmit(_state.uart_handle, tx_buffer,
                        (uint16_t)stream.bytes_written + 2, 1000);
    }
  }
}
