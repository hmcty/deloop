#pragma once

#include <array>
#include <cmath>
#include <cstdint>

constexpr int SINE_TABLE_SIZE = 1024; // Size of the sine table

// 24-bit sine wave table (stored as 32-bit integers)
constexpr std::array<int32_t, SINE_TABLE_SIZE> generate_sine_table(void) {
  std::array<int32_t, SINE_TABLE_SIZE> table{};
  constexpr double TWO_PI = 2.0 * 3.141592;
  constexpr int32_t MAX_24BIT =
      0x7FFFFF; // Maximum positive value for 24-bit signed integer

  for (int i = 0; i < SINE_TABLE_SIZE; ++i) {
    double angle = (static_cast<double>(i) / SINE_TABLE_SIZE) * TWO_PI * 2;
    double sineValue = std::sin(angle);
    table[i] = static_cast<int32_t>(sineValue * MAX_24BIT);
  }

  return table;
}

// Const 24-bit sine wave table
const std::array<int32_t, SINE_TABLE_SIZE> SINE_TABLE = generate_sine_table();
