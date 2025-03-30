#pragma once

// fmt must be converted into uin32_t at compile time using fnv1a_64

#include <stdint.h>

#define DELOOP_LOG_INFO(fmt, ...)                                              \
  deloop::SubmitLog(deloop::LogLevel::INFO, FNV1A_64(#fmt), ##__VA_ARGS__)
#define DELOOP_LOG_WARNING(fmt, ...)                                           \
  deloop::SubmitLog(deloop::LogLevel::WARNING, FNV1A_64(#fmt), ##__VA_ARGS__)
#define DELOOP_LOG_ERROR(fmt, ...)                                             \
  deloop::SubmitLog(deloop::LogLevel::ERROR, FNV1A_64(#fmt), ##__VA_ARGS__)

constexpr uint64_t FNV1A_64(const char *str) {
  if (str == nullptr) {
    return 0;
  }

  uint64_t hash = 0xcbf29ce484222325;
  while (*str != '\0') {
    hash ^= *str++;
    hash *= 0x100000001b3;
  }
  return hash;
}

namespace deloop {

enum class LogLevel { INFO, WARNING, ERROR };
void SubmitLog(LogLevel level, const uint64_t id, ...);

} // namespace deloop
