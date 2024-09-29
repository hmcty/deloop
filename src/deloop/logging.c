
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#include "deloop/logging.h"

void deloop_log_format(const char* tag, const char* message, va_list args) {
  time_t now;
  time(&now);
  char * date = ctime(&now);
  date[strlen(date) - 1] = '\0';
  printf("%s [%s] ", date, tag);
  vprintf(message, args);
  printf("\n");
}

void deloop_log_error(const char* message, ...) {
  va_list args;
  va_start(args, message);
  deloop_log_format("error", message, args);
  va_end(args);
}

void deloop_log_info(const char* message, ...) {
  va_list args;
  va_start(args, message);
  deloop_log_format("info", message, args);
  va_end(args);
}

void deloop_log_debug(const char* message, ...) {
  va_list args;
  va_start(args, message);
  deloop_log_format("debug", message, args);
  va_end(args);
}
