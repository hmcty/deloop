#pragma once

#include <stdint.h>
#include <functional>

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"

#include "errors.hpp"
#include "command.pb.h"

namespace deloop {
namespace UartStream {

// Command handler function type
using CommandHandler = std::function<void (const Command&)>;

deloop::Error Init(UART_HandleTypeDef* uart_handle);

// Register a callback function to handle received commands
void RegisterCommandHandler(CommandHandler handler);
void SendCommandResponse(const CommandResponse& resp, bool blocking = true);

} // namespace UartStream
} // namespace deloop
