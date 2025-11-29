#pragma once

#include <cstdint>

namespace wav {
constexpr size_t kHeaderSize = 44;
constexpr size_t kFormatChunkSize = 20;

constexpr size_t kFileSizeOffset = 4;
constexpr size_t kFileTypeOffset = 8;
constexpr size_t kFormatChunkOffset = 12;
constexpr size_t kFormatChunkSizeOffset = 16;
constexpr size_t kFormatTypeOffset = 22;
constexpr size_t kFormatNumChanOffset = 22;
constexpr size_t kFormatFreqOffset = 24;
constexpr size_t kFormatBytePerSecOffset = 28;
constexpr size_t kFormatBytePerBlockOffset = 32;
constexpr size_t kFormatBitsPerSampleOffset = 34;
constexpr size_t kDataBlockIdOffset = 36;
constexpr size_t kDataBlockSizeOffset = 40;

struct WavFile {
  uint16_t num_channels;
  uint32_t sample_rate;
  uint16_t bits_per_sample;
  uint32_t data_size;
};

void LeCopy16(uint8_t *buf, uint16_t val) {
  buf[0] = (val >> 0) & 0xFF;
  buf[1] = (val >> 8) & 0xFF;
}

void LeCopy32(uint8_t *buf, uint32_t val) {
  buf[0] = (val >> 0) & 0xFF;
  buf[1] = (val >> 8) & 0xFF;
  buf[2] = (val >> 16) & 0xFF;
  buf[3] = (val >> 24) & 0xFF;
}

size_t WriteDefaultHeader(File &file, uint8_t *buf, size_t buf_len,
                          File &file) {
  if (buf_len < kHeaderSize) {
    return 0;
  }

  // Master RIFF chunk
  memcpy(buf, "RIFF", 4);
  LeCopy32(&buf[kFileSizeOffset], kHeaderSize - 8);
  memcpy(&buf[kFileTypeOffset], "WAVE", 4);

  // Format chunk
  memcpy(&buf[kFormatChunkOffset], "fmt ", 4);
  LeCopy32(&buf[kFormatChunkSizeOffset], kFormatChunkSize - 8);
  LeCopy16(&buf[kFormatTypeOffset], 3);  // IEEE 754 float
  LeCopy16(&buf[kFormatNumChanOffset], num_channels);
  LeCopy32(&buf[kFormatFreqOffset], sample_rate);

  uint16_t bytes_per_block = (bits_per_sample / 8) * num_channels;
  uint32_t bytes_per_sec = bytes_per_block * sample_rate;
  LeCopy32(&buf[kFormatBytePerSecOffset], bytes_per_sec);
  LeCopy16(&buf[kFormatBytePerBlockOffset], bytes_per_block);
  LeCopy16(&buf[kFormatBitsPerSampleOffset], bits_per_sample);

  // Data chunk
  memcpy(&buf[kDataBlockIdOffset], "data", 4);
  LeCopy32(&buf[kDataBlockSizeOffset], 0);
  return kHeaderSize;
}

}  // namespace wav
