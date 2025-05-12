#include <iostream>

#include "logging.hpp"

void deloop::SubmitLog(deloop::LogLevel level, const uint64_t hash,
                       const std::array<LogArg, 4> &args, bool blocking) {
  std::cout << "RECEIVED" << std::endl;
}
