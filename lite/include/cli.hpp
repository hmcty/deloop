#pragma once

#include <daisy_seed.h>

namespace cli {
void Init();
void UsbCallback(uint8_t *buff, uint32_t *length);
void Step(uint32_t now);
}  // namespace cli
