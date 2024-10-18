#include <stdarg.h>
#include <stdio.h>

#include "deloop/logging.h"

void deloop_log_format(const char *file, const int lineno, const char *tag,
                       const char *message, ...) {
  va_list args;
  va_start(args, message);
  printf("[%s](%s:%d) ", tag, file, lineno);
  vprintf(message, args);
  printf("\n");
  va_end(args);
}
