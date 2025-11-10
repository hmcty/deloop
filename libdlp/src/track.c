#include "track.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "counter.h"
#include "error.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static inline dlp_track_id_t sync_counter(dlp_track_t *track);
static inline void set_state(dlp_track_t *track, dlp_track_state_type_t state);
static inline bool is_recording(dlp_track_t *track);
static inline bool is_stopped(dlp_track_t *track);
static inline void record(dlp_track_t *track, const float *in, size_t nframes);
static inline void overdub(dlp_track_t *track, const float *in, size_t nframes);

dlp_error_t dlp_track_init(dlp_track_t *track) {
  if (track == NULL) {
    return DLP_ERROR_INVALID_ARGUMENT;
  }

  track->sync.mode = DLP_SYNC_MASTER;
  track->state = DLP_TRACK_STATE_IDLE;
  track->overdub_enabled = false;
  track->volume = 1.0f;
  track->read_head = 0;
  track->write_head = 0;
  track->has_status_changed = true;
  track->last_status_read = 0;
  // TODO: Once errors, check buffer null
  return DLP_SUCCESS;
}

void dlp_track_advance_state(dlp_track_t *track) {
  if (track == NULL) {
    return;
  }

  uint64_t sync_cnt;
  switch (track->sync.mode) {
  case DLP_SYNC_MASTER:
    sync_cnt = dlp_counter_absolute(track->id);
    break;
  case DLP_SYNC_SLAVE:
    sync_cnt = dlp_counter_next_loop(track->sync.track_id);
    break;
  }

  switch (track->state) {
  case DLP_TRACK_STATE_IDLE:
    set_state(track, DLP_TRACK_STATE_RECORDING_START_QUEUED);
    track->queued_tick = sync_cnt;
    break;
  case DLP_TRACK_STATE_RECORDING:
    set_state(track, DLP_TRACK_STATE_RECORDING_STOP_QUEUED);
    track->queued_tick = sync_cnt;
    break;
  case DLP_TRACK_STATE_PLAYING:
    set_state(track, DLP_TRACK_STATE_PAUSED);
    break;
  case DLP_TRACK_STATE_PAUSED:
    set_state(track, DLP_TRACK_STATE_PLAYING_START_QUEUED);
    track->queued_tick = sync_cnt;
    break;
  case DLP_TRACK_STATE_RECORDING_START_QUEUED:
  case DLP_TRACK_STATE_RECORDING_STOP_QUEUED:
  case DLP_TRACK_STATE_PLAYING_START_QUEUED:
    // No-op
    break;
  }
}

void dlp_track_read(dlp_track_t *track, const float *in, size_t nframes) {
  if (track == NULL || in == NULL) {
    return;
  }

  uint64_t ctr_id = sync_counter(track);
  uint64_t start = dlp_counter_absolute(ctr_id);
  uint64_t end = start + nframes;
  uint64_t queued_tick = track->queued_tick;

  switch (track->state) {
  case DLP_TRACK_STATE_RECORDING_START_QUEUED:
    if (end < queued_tick) {
      return;
    }

    uint64_t record_from = (start < queued_tick) ? queued_tick - start : 0;
    set_state(track, DLP_TRACK_STATE_RECORDING);
    record(track, &in[record_from], nframes - record_from);
    dlp_counter_set_cnt(track->id, track->len - record_from);
    break;
  case DLP_TRACK_STATE_RECORDING:
    record(track, in, nframes);
    break;
  case DLP_TRACK_STATE_RECORDING_STOP_QUEUED:
    if (end < queued_tick) {
      return;
    }

    uint64_t record_to = (start < queued_tick) ? queued_tick - start : 0;
    set_state(track, DLP_TRACK_STATE_PLAYING_START_QUEUED);
    record(track, in, record_to);
    if (track->overdub_enabled) {
      overdub(track, &in[record_to], nframes - record_to);
    }
    break;
  case DLP_TRACK_STATE_PLAYING_START_QUEUED:
    uint64_t overdub_from = (start < queued_tick) ? queued_tick - start : 0;
    overdub(track, &in[overdub_from], nframes - overdub_from);
    break;
  case DLP_TRACK_STATE_PLAYING:
    if (track->overdub_enabled) {
      overdub(track, in, nframes);
    }
  case DLP_TRACK_STATE_PAUSED:
  case DLP_TRACK_STATE_IDLE:
    // No-op
    break;
  }
}

void dlp_track_write(dlp_track_t *track, float *out, size_t nframes) {
  if (track == NULL || out == NULL || track->len == 0) {
    return;
  } else if (is_stopped(track) || is_recording(track)) {
    track->read_head = 0;
    return;
  }

  uint64_t ctr_id = sync_counter(track);
  uint64_t start = dlp_counter_absolute(ctr_id);
  uint64_t end = start + nframes;

  uint64_t play_from = 0;
  uint64_t queued_tick = track->queued_tick;
  if (track->state == DLP_TRACK_STATE_PLAYING_START_QUEUED &&
      end > queued_tick) {
    set_state(track, DLP_TRACK_STATE_PLAYING);
    play_from = (start < track->queued_tick) ? track->queued_tick - start : 0;
  }

  for (uint64_t i = play_from; i < nframes; i++) {
    if (track->read_head >= track->len) {
      if (track->state == DLP_TRACK_STATE_RECORDING) {
        break;
      }

      track->read_head = 0;
    }

    out[i] += track->buffer[track->read_head];
    track->read_head += 1;
  }

  track->write_head = track->read_head;
}

dlp_error_t dlp_track_sync_to(dlp_track_t *track, dlp_track_id_t master) {
  if (track == NULL || master >= DLP_NUM_TRACKS || track->id == master) {
    return DLP_ERROR_INVALID_ARGUMENT;
  }

  track->sync.mode = DLP_SYNC_SLAVE;
  track->sync.track_id = master;
  return DLP_SUCCESS;
}

void dlp_track_clear(dlp_track_t *track) {
  if (track == NULL) {
    return;
  }

  track->state = DLP_TRACK_STATE_IDLE;
  track->read_head = 0;
  track->write_head = 0;
  track->len = 0;
  dlp_counter_set_cnt(track->id, 0);
  dlp_counter_set_len(track->id, 0);
}

dlp_error_t dlp_track_get_status(dlp_track_t *track,
                                 dlp_track_status_t *status) {
  if (track == NULL) {
    return DLP_ERROR_INVALID_ARGUMENT;
  }

  uint64_t ctr_id = sync_counter(track);
  uint64_t tick = dlp_counter_absolute(ctr_id);
  if (!track->has_status_changed ||
      (tick - track->last_status_read) >= (48000 / 2)) {
    return DLP_ERROR_NO_TRACK_STATUS;
  }

  track->has_status_changed = false;
  track->last_status_read = tick;

  status->id = track->id;
  status->state = track->state;
  status->overdub_enabled = track->overdub_enabled;
  status->read_head = track->read_head;
  status->len = track->len;

  return DLP_SUCCESS;
}

static inline dlp_track_id_t sync_counter(dlp_track_t *track) {
  switch (track->sync.mode) {
  case DLP_SYNC_MASTER:
    return track->id;
  case DLP_SYNC_SLAVE:
    return track->sync.track_id;
  default:
    return track->id;
  }
}

static inline void set_state(dlp_track_t *track, dlp_track_state_type_t state) {
  track->has_status_changed = true;
  track->state = state;
}

static inline bool is_recording(dlp_track_t *track) {
  return track->state == DLP_TRACK_STATE_RECORDING ||
         track->state == DLP_TRACK_STATE_RECORDING_START_QUEUED;
}

static inline bool is_stopped(dlp_track_t *track) {
  switch (track->state) {
  case DLP_TRACK_STATE_IDLE:
  case DLP_TRACK_STATE_RECORDING_START_QUEUED:
  case DLP_TRACK_STATE_PAUSED:
    return true;
  default:
    return false;
  }
}

static inline void record(dlp_track_t *track, const float *in, size_t nframes) {
  size_t n = MIN(track->capacity - track->write_head, nframes);
  memcpy((void *)&track->buffer[track->write_head], (const void *)in,
         n * sizeof(float));
  track->write_head += n;
  track->len += n;
  dlp_counter_set_len(track->id, track->len);
}

static inline void overdub(dlp_track_t *track, const float *in,
                           size_t nframes) {
  for (size_t i = 0; i < nframes; i++) {
    if (track->write_head >= track->len) {
      track->write_head = 0;
    }

    track->buffer[track->write_head] += in[i];
    track->write_head++;
  }
}
