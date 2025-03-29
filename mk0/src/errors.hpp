#pragma once

namespace deloop {

enum class Error {
  kOk = 0,
  kAlreadyInitialized,
  kFailedToInitializePeripheral,
};

} // namespace deloop
