#pragma once

#define DELOOP_LOG_INFO(...) \
  deloop::LogInternal(deloop::LogLevel::INFO, __VA_ARGS__)
#define DELOOP_LOG_WARN(...) \
  deloop::LogInternal(deloop::LogLevel::WARN, __VA_ARGS__)
#define DELOOP_LOG_ERROR(...) \
  deloop::LogInternal(deloop::LogLevel::ERROR, __VA_ARGS__)

namespace deloop {

enum class LogLevel {
  INFO,
  WARN,
  ERROR,
};

void LogInternal(LogLevel level, const char* format, ...);

}  // namespace deloop
