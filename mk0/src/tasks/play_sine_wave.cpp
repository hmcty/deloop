#include <array>
#include <cmath>
#include <cstdint>

#include "FreeRTOS.h"
#include "task.h"

#include "drv/wm8960.hpp"

const float PI = 3.14159265358979323846f;
const int16_t n_samples = 256;
const auto sine_table = []() {
  std::array<uint16_t, n_samples> table;
  for (int i = 0; i < n_samples; i++) {
    table[i] = (uint16_t) (32767 * std::sin(2.0f * PI * i / n_samples));
  }
  return table;
}();

const size_t task_stack_size = 16 * 1024;
static StaticTask_t task_buffer;
static StackType_t task_stack[task_stack_size];

void PlaySineWave(void *pvParameters) {
  (void)pvParameters;

  WM8960::Play();
  while(1) {
    WM8960::Write(sine_table.data(), n_samples);
    vTaskDelay(5000);
  }
}

void AddSineWaveTask() {
  xTaskCreateStatic(
    PlaySineWave,
    "Play Sine Wave",
    task_stack_size,
    NULL,
    1,
    &(task_stack[0]),
    &task_buffer
  );
}


