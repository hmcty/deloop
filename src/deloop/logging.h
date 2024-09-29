#pragma once

#define DELOOP_LOG_ERROR(msg, ...)                                             \
  do {                                                                         \
    deloop_log_format(__FILE__, __LINE__, "ERROR", msg, ##__VA_ARGS__);        \
  } while (0)

#define DELOOP_LOG_WARN(msg, ...)                                              \
  do {                                                                         \
    deloop_log_format(__FILE__, __LINE__, "WARNING", msg, ##__VA_ARGS__);      \
  } while (0)

#define DELOOP_LOG_INFO(msg, ...)                                              \
  do {                                                                         \
    deloop_log_format(__FILE__, __LINE__, "INFO", msg, ##__VA_ARGS__);         \
  } while (0)

void deloop_log_format(const char *file, const int lineno, const char *tag,
                       const char *message, ...);
