#pragma once

#include <stdint.h>

#include "errors.hpp"

namespace deloop {
namespace UartStream {

deloop::Error Init();
deloop::Error Write(const uint8_t *data, uint16_t size);

} // namespace UartStream
} // namespace deloop
