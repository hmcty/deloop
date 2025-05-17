#pragma once

#include <cstdint>
#include <functional>

#include "errors.hpp"

namespace deloop {
namespace audio_scheduler {

using ProccessCallback =
    std::function<Error(uint32_t num_frames, int32_t *tx, int32_t *rx)>;

// TODO: Make channels configurable
Error init(void);
Error registerCallback(ProccessCallback callback);
// TODO: Register
Error process(uint32_t num_frames, int32_t *tx, int32_t *rx);

} // namespace audio_scheduler
} // namespace deloop
