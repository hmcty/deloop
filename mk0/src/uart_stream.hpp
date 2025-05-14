#pragma once

#include <cstdint>
#include <functional>
#include <stm32f4xx_hal.h>
#include <stm32f4xx_hal_uart.h>

#include <FreeRTOS.h> // Must appear before other FreeRTOS includes
#include <queue.h>

#include "command.pb.h"
#include "errors.hpp"

namespace deloop {
namespace UartStream {

deloop::Error init(UART_HandleTypeDef *uart_handle);
QueueHandle_t getCmdQueue();
void sendCommandResponse(const CommandResponse &resp);

} // namespace UartStream
} // namespace deloop
