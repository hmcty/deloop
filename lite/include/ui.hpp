#pragma once

#include <daisy.h>

#include "WS2812B.hpp"
#include "engine.h"
#include "track.h"

// DisplayText()
// HandleTrackState()
// Tick()

class Ui {
public:
  void Init(daisy::PWMHandle::Channel *led_chan_a,
            daisy::PWMHandle::Channel *led_chan_b);
  void DisplayText();
  void HandleTrackStatus(dlp_track_status_t &status);
  void Step(uint32_t now);

private:
  static constexpr int kNumLedsPerRing = 16;
  WS2812B<kNumLedsPerRing> led_ring_a_;
  WS2812B<kNumLedsPerRing> led_ring_b_;
  // dlp_track_status_t track_prev_status[DLP_NUM_TRACKS];

  int active_led_ = 0;
  uint32_t last_render_;
};
