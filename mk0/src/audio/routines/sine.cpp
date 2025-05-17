#include "audio/routines/sine.hpp"

#include <array>
#include <cmath>
#include <cstdint>

#include "errors.hpp"

constexpr int SINE_TABLE_SIZE = 8096; // Size of the sine table

// 24-bit sine wave table (stored as 32-bit integers)
constexpr std::array<int32_t, SINE_TABLE_SIZE> generate_sine_table(void) {
  std::array<int32_t, SINE_TABLE_SIZE> table{};
  constexpr double TWO_PI = 4.0f * 3.141592f;
  constexpr int32_t MAX_24BIT =
      0x7FFFFF; // Maximum positive value for 24-bit signed integer

  for (int i = 0; i < SINE_TABLE_SIZE; ++i) {
    double angle = (static_cast<double>(i) / SINE_TABLE_SIZE) * TWO_PI;
    double sineValue = std::sin(angle);
    table[i] = static_cast<int32_t>(sineValue * MAX_24BIT);
  }

  return table;
}

// Const 24-bit sine wave table
const std::array<int32_t, SINE_TABLE_SIZE> SINE_TABLE = generate_sine_table();

deloop::Error tx_sine(uint32_t num_frames, int32_t *tx, int32_t *rx) {
  if (num_frames == 0 || tx == nullptr || rx == nullptr) {
    return deloop::Error::kInvalidArgument;
  }

  // Fill the tx buffer with sine wave samples
  static uint32_t j = 0;
  for (int i = 0; i < (num_frames / 2); i++) {
    int32_t sample = SINE_TABLE[j++ % SINE_TABLE_SIZE];
    tx[i] = sample;
    tx[i + 1] = sample;
  }
  return deloop::Error::kOk;
}
