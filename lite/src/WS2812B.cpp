#include "WS2812B.hpp"

#include <daisy_seed.h>

using namespace daisy;

static uint8_t scale8(uint8_t i, uint8_t scale) {
  return (uint8_t)(((uint16_t)i * (uint16_t)(scale)) >> 8);
}

template <uint16_t N> void WS2812B<N>::Init(PWMHandle::Channel *pwm_channel) {
  pwm_channel_ = pwm_channel;
}

template <uint16_t N>
void WS2812B<N>::SetPixelColor(size_t index, uint8_t red, uint8_t green,
                               uint8_t blue) {
  if (index >= N) {
    return;
  }

  size_t offset = index * kNumBytesPerPixel * 8;
  for (uint8_t i = 0; i < 8; i++) {
    data_buf_[offset + i] = kBitLow
                            << (((scale8(green, 0xB0) << i) & 0x80) > 0);
    data_buf_[offset + 8 + i] = kBitLow << (((red << i) & 0x80) > 0);
    data_buf_[offset + 16 + i] = kBitLow
                                 << (((scale8(blue, 0xF0) << i) & 0x80) > 0);
  }
}

template <uint16_t N> void WS2812B<N>::Render() {
  if (pwm_channel_ == nullptr) {
    return;
  }

  SCB_CleanDCache_by_Addr((uint32_t *)data_buf_,
                          kBufferSize * sizeof(uint32_t));
  // dsy_dma_clear_cache_for_buffer((uint8_t *)data_buf_,
  //                                kBufferSize * sizeof(uint32_t));
  pwm_channel_->DmaTransmit(data_buf_, kBufferSize, nullptr, nullptr, nullptr);
}

template class WS2812B<2>;
template class WS2812B<12>;
template class WS2812B<16>;
