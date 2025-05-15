#include "audio/scheduler.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>

#include "errors.hpp"

using namespace deloop;

const size_t kMaxCallbacks = 4;
static struct {
  bool initialized;
  std::atomic_flag lock;
  std::size_t num_callbacks;
  std::array<audio_scheduler::ProccessCallback, kMaxCallbacks> callbacks;
} state_ = {0};

Error audio_scheduler::init(void) {
  if (state_.initialized) {
    return Error::kAlreadyInitialized;
  }

  state_.lock.clear();
  state_.num_callbacks = 0;
  state_.callbacks.fill(nullptr);
  state_.initialized = true;
  return Error::kOk;
}

Error audio_scheduler::registerCallback(ProccessCallback callback) {
  if (!state_.initialized) {
    return Error::kNotInitialized;
  } else if (callback == nullptr) {
    return Error::kInvalidArgument;
  } else if (state_.num_callbacks >= kMaxCallbacks) {
    return Error::kSchedulerCallbacksFull;
  }

  if (state_.lock.test_and_set(std::memory_order_acquire)) {
    return Error::kSchedulerBusy;
  }

  state_.callbacks[state_.num_callbacks++] = callback;
  state_.lock.clear(std::memory_order_release);

  return Error::kOk;
}

Error audio_scheduler::process(uint32_t num_frames, uint8_t *tx, uint8_t *rx) {
  if (!state_.initialized) {
    return Error::kNotInitialized;
  } else if (num_frames == 0 || tx == nullptr || rx == nullptr) {
    return Error::kInvalidArgument;
  }

  if (state_.lock.test_and_set(std::memory_order_acquire)) {
    return Error::kSchedulerBusy;
  }

  for (size_t i = 0; i < state_.num_callbacks; i++) {
    state_.callbacks[i](num_frames, tx, rx);
  }

  state_.lock.clear(std::memory_order_release);
  return Error::kOk;
}
