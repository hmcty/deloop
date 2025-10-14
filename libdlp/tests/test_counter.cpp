#include <gtest/gtest.h>

#include "counter.h"
#include "error.h"
#include "track.h"

TEST(CounterTest, HelperSanity) {
  ASSERT_EQ(dlp_counter_init(), DLP_SUCCESS);
  for (size_t i = 0; i < DLP_NUM_TRACKS; i++) {
    ASSERT_EQ(dlp_counter_absolute((dlp_track_id_t)i), 0);
  }

  dlp_counter_advance_all(100);
  for (size_t i = 0; i < DLP_NUM_TRACKS; i++) {
    ASSERT_EQ(dlp_counter_absolute((dlp_track_id_t)i), 100);
  }

  for (size_t i = 0; i < DLP_NUM_TRACKS; i++) {
    dlp_counter_set_len((dlp_track_id_t)i, 200);
    ASSERT_EQ(dlp_counter_absolute((dlp_track_id_t)i), 100);
    ASSERT_EQ(dlp_counter_next_loop((dlp_track_id_t)i), 200);
  }

  dlp_counter_advance_all(200);
  for (size_t i = 0; i < DLP_NUM_TRACKS; i++) {
    ASSERT_EQ(dlp_counter_absolute((dlp_track_id_t)i), 300);
    ASSERT_EQ(dlp_counter_next_loop((dlp_track_id_t)i), 400);
  }

  ASSERT_EQ(dlp_counter_deinit(), DLP_SUCCESS);
}
