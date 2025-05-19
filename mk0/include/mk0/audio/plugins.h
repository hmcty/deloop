#pragma once

#include <cstdint>

#include "mk0/errors.h"

namespace deloop {
namespace audio {
namespace plugins {

Error echo(uint32_t num_frames, int32_t *tx, int32_t *rx);
Error tx_sine(uint32_t num_frames, int32_t *tx, int32_t *rx);

} // namespace plugins
} // namespace audio
} // namespace deloop
