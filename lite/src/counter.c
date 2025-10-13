#include "counter.h"

#include "error.h"
#include "track.h"

static struct {
  bool initialized;
  dlp_counter_t track_cntrs[DLP_NUM_TRACKS];
} state_;

dlp_error_t dlp_counter_init(uint64_t rate) {
  if (state_.initialized) {
    return DLP_ERROR_ALREADY_INITIALIZED;
  }

  for (size_t i = 0; i < DLP_NUM_TRACKS; i++) {
    state_.track_cntrs[i].cnt = 0;
    state_.track_cntrs[i].len = 0;
  }

  state_.initialized = true;
  return DLP_SUCCESS;
}

void dlp_counter_advance_all(uint64_t step) {
  if (!state_.initialized) {
    return;
  }

  for (size_t i = 0; i < DLP_NUM_TRACKS; i++) {
    state_.track_cntrs[i].cnt += step;
  }
}

void dlp_counter_advance(dlp_track_id_t id, uint64_t step) {
  if (!state_.initialized || id >= DLP_NUM_TRACKS) {
    return;
  }

  state_.track_cntrs[id].cnt += step;
}

uint64_t dlp_counter_absolute(dlp_track_id_t id) {
  if (!state_.initialized || id >= DLP_NUM_TRACKS) {
    return 0;
  }

  return state_.track_cntrs[id].cnt;
}

uint64_t dlp_counter_next_loop(dlp_track_id_t id) {
  if (!state_.initialized || id >= DLP_NUM_TRACKS) {
    return 0;
  }

  uint64_t cnt = state_.track_cntrs[id].cnt;
  uint64_t len = state_.track_cntrs[id].len;
  if (len == 0) {
    return cnt;
  }

  return ((cnt / len) + 1) * len;
  // return (len - (cnt % len)) + cnt;
}

void dlp_counter_set_cnt(dlp_track_id_t id, uint64_t val) {
  if (!state_.initialized || id >= DLP_NUM_TRACKS) {
    return;
  }

  state_.track_cntrs[id].cnt = val;
}

void dlp_counter_set_len(dlp_track_id_t id, uint64_t val) {
  if (!state_.initialized || id >= DLP_NUM_TRACKS) {
    return;
  }

  state_.track_cntrs[id].len = val;
}
