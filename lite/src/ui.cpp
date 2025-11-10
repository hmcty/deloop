#include "ui.hpp"

#include <daisy.h>

#include "track.h"

void Ui::Init(daisy::PWMHandle::Channel *led_chan_a,
              daisy::PWMHandle::Channel *led_chan_b) {
  led_ring_a_.Init(led_chan_a);
  led_ring_b_.Init(led_chan_b);

  led_ring_a_.FillColor(0, 0, 0);
  led_ring_a_.FillColor(0, 0, 0);

  led_ring_a_.Render();
  led_ring_b_.Render();

  active_led_ = 0;
  last_render_ = 0;
}

void Ui::DisplayText() {
  // Not supported yet
}

void Ui::HandleTrackStatus(dlp_track_status_t &status) {
  // Not support yet
}

void Ui::Step(uint32_t now) {
  if (now - last_render_ > 500) {
    last_render_ = now;

    led_ring_a_.SetPixelColor(active_led_, 0, 0, 0);
    led_ring_b_.SetPixelColor(active_led_, 0, 0, 0);
    active_led_ = (active_led_ + 1) % kNumLedsPerRing;
    led_ring_a_.SetPixelColor(active_led_, 5, 0, 0);
    led_ring_b_.SetPixelColor(active_led_, 5, 0, 0);
    led_ring_a_.Render();
    led_ring_b_.Render();
  }
}
