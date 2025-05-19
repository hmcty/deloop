#include "mk0/audio/plugins.h"

#include <cstdint>

#include "mk0/errors.h"

deloop::Error deloop::audio::plugins::echo(uint32_t num_frames, int32_t *tx,
                                           int32_t *rx) {
  if (num_frames == 0 || tx == nullptr || rx == nullptr) {
    return deloop::Error::kInvalidArgument;
  }

  // Fill the tx buffer with echo samples
  for (uint32_t i = 0; i < num_frames; i++) {
    tx[i] = rx[i];
  }

  return deloop::Error::kOk;
}
