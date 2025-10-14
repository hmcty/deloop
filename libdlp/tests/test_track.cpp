#include <gtest/gtest.h>

#include <numeric>
#include <vector>

#include "counter.h"
#include "error.h"
#include "track.h"

void SetUpTestSuite() {
  dlp_counter_deinit();
  ASSERT_EQ(dlp_counter_init(), DLP_SUCCESS);
}

void AssertDefaults(const dlp_track_t *track) {
  ASSERT_EQ(track->state, DLP_TRACK_STATE_IDLE);
  ASSERT_EQ(track->read_head, 0);
  ASSERT_EQ(track->write_head, 0);
  ASSERT_EQ(track->len, 0);
}

class TrackTest : public testing::Test {
protected:
  void SetUp() override { ASSERT_EQ(dlp_counter_init(), DLP_SUCCESS); }
  void TearDown() override { ASSERT_EQ(dlp_counter_deinit(), DLP_SUCCESS); }
};

TEST_F(TrackTest, HandlesClear) {
  float buffer[64] = {0};
  dlp_track_t track = {
      .id = DLP_TRACK_A,
      .buffer = buffer,
      .capacity = 64,
  };
  ASSERT_EQ(dlp_track_init(&track), DLP_SUCCESS);
  track.state = DLP_TRACK_STATE_RECORDING;

  float input[8] = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8};
  dlp_track_read(&track, input, 8);

  dlp_track_clear(&track);
  AssertDefaults(&track);
}

TEST_F(TrackTest, HandlesSyncedRecording) {
  float buffer[1024] = {0};
  dlp_track_t track = {
      .id = DLP_TRACK_A,
      .buffer = buffer,
      .capacity = 1024,
  };
  ASSERT_EQ(dlp_track_init(&track), DLP_SUCCESS);
  ASSERT_EQ(dlp_track_sync_to(&track, DLP_TRACK_B), DLP_SUCCESS);
  track.overdub_enabled = true;

  dlp_counter_set_len(DLP_TRACK_B, 1024);
  dlp_counter_advance_all(512);

  // Queue recording to occur on next track B loop
  dlp_track_advance_state(&track);
  ASSERT_EQ(track.state, DLP_TRACK_STATE_RECORDING_START_QUEUED);
  ASSERT_EQ(track.queued_tick, 1024);

  // Read from input buffer (only 512 frames should be recorded)
  float output[1024] = {0};
  std::vector<float> input(1024);
  std::iota(input.begin(), input.end(), 0.0f);
  dlp_track_read(&track, &input[0], 1024);
  dlp_track_write(&track, output, 1024);
  dlp_counter_advance_all(1024);
  ASSERT_EQ(track.state, DLP_TRACK_STATE_RECORDING);
  ASSERT_EQ(track.read_head, 0);
  ASSERT_EQ(track.write_head, 512);
  ASSERT_EQ(track.len, 512);

  // Queue overdubbing to begin on next loop
  dlp_track_advance_state(&track);
  ASSERT_EQ(track.state, DLP_TRACK_STATE_RECORDING_STOP_QUEUED);
  ASSERT_EQ(track.queued_tick, 2048);

  // Read from input buffer (only 512 samples should be captured)
  dlp_track_read(&track, &input[0], 1024);
  dlp_track_write(&track, output, 1024);
  dlp_counter_advance_all(1024);
  ASSERT_EQ(track.state, DLP_TRACK_STATE_PLAYING);
  ASSERT_EQ(track.read_head, 512);
  ASSERT_EQ(track.write_head, 512);
  ASSERT_EQ(track.len, 1024);

  // Check audio buffers (split and reversed from input)
  for (size_t i = 0; i < 512; i++) {
    ASSERT_FLOAT_EQ(buffer[i], input[512 + i] * 2.0f);
    ASSERT_FLOAT_EQ(buffer[512 + i], input[i]);
  }
}
