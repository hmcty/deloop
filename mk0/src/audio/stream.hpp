#pragma once

#include <cstdint>
#include <functional>
#include <stm32f4xx_hal.h>

#include "errors.hpp"

namespace deloop {
namespace audio_stream {

Error init(SAI_Block_TypeDef *sai_rx, SAI_Block_TypeDef *sai_tx);
Error start(void);
Error stop(void);

} // namespace audio_stream
} // namespace deloop
