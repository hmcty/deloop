#pragma once

// fmt must be converted into uin32_t at compile time using fnv1a_64

#include <stdint.h>

#define DELOOP_LOG_INFO(fmt, ...)                                              \
  SubmitLog(LogLevel::INFO, FNV1A_64(#fmt), ##__VA_ARGS__)
#define DELOOP_LOG_WARNING(fmt, ...)                                           \
  SubmitLog(LogLevel::WARNING, FNV1A_64(#fmt), ##__VA_ARGS__)
#define DELOOP_LOG_ERROR(fmt, ...)                                             \
  SubmitLog(LogLevel::ERROR, FNV1A_64(#fmt), ##__VA_ARGS__)

enum class LogLevel { INFO, WARNING, ERROR };

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

void SubmitLog(LogLevel level, const uint64_t id, ...);
