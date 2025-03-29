#include "logging.hpp"

void PushLogMessage(LogLevel level, const uint64_t id, ...) {
#ifdef NO_LOGGING
  return;
#endif
}
