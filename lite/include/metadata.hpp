#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "logging.hpp"

namespace deloop {

struct alignas(4) Metadata {
  static constexpr uint32_t kExpectedMagic = 0x444C504B;  // 'DLPK'
  static constexpr uint32_t kVersion = 1;

  uint32_t magic = kExpectedMagic;
  uint32_t version = kVersion;
  uint32_t active_buffer = 0;
  uint32_t crc32 = 0;

  uint32_t ComputeCRC() const;
  bool Validate() const;
};

typedef std::array<uint8_t, sizeof(Metadata)> MetadataBuffer;
static bool ReadMetadata(Metadata& md, const MetadataBuffer& buffer) {
  uint32_t magic, version;
  std::memcpy(&magic, buffer.data() + offsetof(Metadata, magic),
              sizeof(uint32_t));
  if (magic != Metadata::kExpectedMagic) {
    return false;
  }

  std::memcpy(&version, buffer.data() + offsetof(Metadata, version),
              sizeof(uint32_t));
  if (version != Metadata::kVersion) {
    // TODO: Handle version migration
    DELOOP_LOG_ERROR("Unsupported metadata version: %d", version);
    return false;
  }

  std::memcpy(&md, buffer.data(), sizeof(Metadata) - sizeof(uint32_t));
  md.crc32 = md.ComputeCRC();
  return md.Validate();
}

static void WriteMetadata(Metadata& md, MetadataBuffer& buffer) {
  md.crc32 = md.ComputeCRC();
  std::memcpy(buffer.data(), &md, sizeof(Metadata));
}

}  // namespace deloop
