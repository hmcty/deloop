#include "metadata.hpp"

#include <zlib.h>

#include <cstdint>
#include <cstring>

uint32_t deloop::Metadata::ComputeCRC() const {
  uint8_t buffer[sizeof(Metadata) - sizeof(crc32)];
  std::memcpy(buffer, this, sizeof(Metadata) - sizeof(crc32));
  return crc32_z(0L, buffer, sizeof(Metadata) - sizeof(crc32));
}

bool deloop::Metadata::Validate() const {
  if (magic != kExpectedMagic) {
    return false;
  }

  uint32_t computed_crc = ComputeCRC();
  return computed_crc == crc32;
}
