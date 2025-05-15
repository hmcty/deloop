#pragma once

#define DELOOP_RETURN_IF_ERROR(expr)                                           \
  do {                                                                         \
    deloop::Error error = (expr);                                              \
    if (error != deloop::Error::kOk) {                                         \
      return error;                                                            \
    }                                                                          \
  } while (0)

namespace deloop {

enum class Error : int32_t {
  kOk = 0,

  // General
  kAlreadyInitialized = -1,
  kNotInitialized = -2,
  kFailedToInitializePeripheral = -3,
  kInvalidArgument = -4,

  // I2C
  kI2cBusyOnWrite = -5,
  kI2cHalWriteError = -6,
  kI2cTimeoutOnWrite = -7,

  // SAI
  kSaiBusy = -8,
  kSaiHalError = -9,
  kSaiTimeout = -10,

  // Audio Scheduler
  kSchedulerCallbacksFull = -11,
  kSchedulerBusy = -12,
};

} // namespace deloop
