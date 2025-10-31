#pragma once

#include <cstddef>
#include <cstdint>
#include <daisy_seed.h>

template <uint16_t NUM_PIXELS> class WS2812B {
public:
  WS2812B() {}
  void Init(daisy::PWMHandle::Channel *pwm_channel);
  void SetPixelColor(size_t pixel_index, uint8_t red, uint8_t green,
                     uint8_t blue);
  void Render();

private:
  static constexpr uint32_t kBitLow = 80;
  static constexpr uint32_t kBitHigh = 159;
  static constexpr size_t kNumBytesPerPixel = 3;
  static constexpr size_t kBufferSize = NUM_PIXELS * kNumBytesPerPixel * 8;
  // uint8_t rgb_buf_[NUM_PIXELS * kNumBytesPerPixel] = {0};
  uint32_t data_buf_[kBufferSize] = {0};

  daisy::PWMHandle::Channel *pwm_channel_{nullptr};
};
