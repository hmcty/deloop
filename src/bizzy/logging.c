
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#include "bizzy/logging.h"

void bizzy_log_format(const char* tag, const char* message, va_list args) {
  time_t now;
  time(&now);
  char * date = ctime(&now);
  date[strlen(date) - 1] = '\0';
  printf("%s [%s] ", date, tag);
  vprintf(message, args);
  printf("\n");
}

void bizzy_log_error(const char* message, ...) {
  va_list args; 
  va_start(args, message);
  bizzy_log_format("error", message, args);
  va_end(args);
}

void bizzy_log_info(const char* message, ...) {
  va_list args;
  va_start(args, message);
  bizzy_log_format("info", message, args);
  va_end(args);
}

void bizzy_log_debug(const char* message, ...) {
  va_list args;
  va_start(args, message);
  bizzy_log_format("debug", message, args);
  va_end(args);
}
