#pragma once

#include <array>
#include <cstdint>
#include <source_location>
#include <string_view>

#include "errors.hpp"

#define DELOOP_LOG(fmt, level, blocking, ...)                                  \
  do {                                                                         \
    std::array<deloop::LogArg, 4> args = deloop::CreateLogArgs(__VA_ARGS__);   \
    deloop::SubmitLog(level, FNV1A_64(fmt), args, blocking);                   \
  } while (0)

#define DELOOP_LOG_INFO(fmt, ...)                                              \
  DELOOP_LOG(fmt, deloop::LogLevel::INFO, true, ##__VA_ARGS__)
#define DELOOP_LOG_WARNING(fmt, ...)                                           \
  DELOOP_LOG(fmt, deloop::LogLevel::WARNING, true, ##__VA_ARGS__)
#define DELOOP_LOG_ERROR(fmt, ...)                                             \
  DELOOP_LOG(fmt, deloop::LogLevel::ERROR, true, ##__VA_ARGS__)

#define DELOOP_LOG_INFO_FROM_ISR(fmt, ...)                                     \
  DELOOP_LOG(fmt, deloop::LogLevel::INFO, false, ##__VA_ARGS__)
#define DELOOP_LOG_WARNING_FROM_ISR(fmt, ...)                                  \
  DELOOP_LOG(fmt, deloop::LogLevel::WARNING, false, ##__VA_ARGS__)
#define DELOOP_LOG_ERROR_FROM_ISR(fmt, ...)                                    \
  DELOOP_LOG(fmt, deloop::LogLevel::ERROR, false, ##__VA_ARGS__)

constexpr uint64_t FNV1A_64(std::string_view str) {
  uint64_t hash = 0xcbf29ce484222325;
  for (char c : str) {
    hash = (hash ^ c) * 0x100000001b3;
  }

  return hash;
}

namespace deloop {

enum class LogLevel { INFO, WARNING, ERROR };

struct LogArg {
  enum class Type { kUnset, kU32, kI32, kF32 };

  Type type;
  union {
    uint32_t u32;
    int32_t i32;
    float f32;
  } value;
};

constexpr LogArg ToLogArg(uint32_t value) {
  return LogArg{LogArg::Type::kU32, {.u32 = value}};
}

constexpr LogArg ToLogArg(int value) {
  return LogArg{LogArg::Type::kI32, {.i32 = value}};
}

constexpr LogArg ToLogArg(float value) {
  return LogArg{deloop::LogArg::Type::kF32, {.f32 = value}};
}

constexpr LogArg ToLogArg(deloop::Error value) {
  return LogArg{LogArg::Type::kI32, {.i32 = static_cast<int32_t>(value)}};
}

template <typename... Values>
constexpr std::array<LogArg, 4> CreateLogArgs(Values... values) {
  static_assert(sizeof...(values) <= 4, "Only 4 arguments are supported.");

  std::array<LogArg, 4> args = {
      LogArg{LogArg::Type::kUnset, {0}}, LogArg{LogArg::Type::kUnset, {0}},
      LogArg{LogArg::Type::kUnset, {0}}, LogArg{LogArg::Type::kUnset, {0}}};

  int i = 0;
  ((args[i++] = ToLogArg(std::forward<Values>(values))), ...);
  return args;
}

void SubmitLog(LogLevel level, const uint64_t hash,
               const std::array<LogArg, 4> &args, bool blocking = true);

} // namespace deloop
