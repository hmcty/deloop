#include "track.h"

#include <stdbool.h>

#include "counter.h"

static inline deloop_track_id_t sync_counter(deloop_track_t *track);
static inline bool is_recording(deloop_track_t *track);
static inline bool is_stopped(deloop_track_t *track);
static inline void record(deloop_track_id_t *track, const float *in,
                          size_t nframes);
static inline void overdub(deloop_track_id_t *track, const float *in,
                           size_t nframes);

void deloop_track_init(deloop_track_t *track) {
  if (track == NULL) {
    return;
  }

  track->sync.mode = DELOOP_SYNC_MASTER;
  track->state = DELOOP_TRACK_STATE_IDLE;
  track->overdub_enabled = false;
  track->volume = 1.0f;
  track->read_head = 0;
  track->write_head = 0;
  // TODO: Once errors, check buffer null
}

void deloop_track_advance_state(deloop_track_t *track) {
  if (track == NULL) {
    return;
  }

  uint64_t sync_cnt;
  switch (track->sync.mode) {
  case DELOOP_SYNC_MASTER:
    sync_cnt = deloop_counter_absolute(track->id);
    break;
  case DELOOP_SYNC_SLAVE:
    sync_cnt = deloop_counter_next_loop(track->sync.track_id);
    break;
  }

  switch (track->state) {
  case DELOOP_TRACK_STATE_IDLE:
    track->state = DELOOP_TRACK_STATE_RECORDING_START_QUEUED;
    track->queued_tick = sync_cnt;
    break;
  case DELOOP_TRACK_STATE_RECORDING:
    track->state = DELOOP_TRACK_STATE_RECORDING_STOP_QUEUED;
    track->queued_tick = sync_cnt;
    break;
  case DELOOP_TRACK_STATE_PLAYING:
    track->state = DELOOP_TRACK_STATE_PAUSED;
    break;
  case DELOOP_TRACK_STATE_PAUSED:
    track->state = DELOOP_TRACK_STATE_PLAYING_START_QUEUED;
    track->queued_tick = sync_cnt;
    break;
  case DELOOP_TRACK_STATE_RECORDING_START_QUEUED:
  case DELOOP_TRACK_STATE_RECORDING_STOP_QUEUED:
  case DELOOP_TRACK_STATE_PLAYING_START_QUEUED:
    // No-op
    break;
  }
}

void deloop_track_read(deloop_track_t *track, const float *in, size_t nframes) {
  if (track == NULL || in == NULL) {
    return;
  }

  uint64_t ctr_id = sync_counter(track);
  uint64_t start = deloop_counter_absolute(ctr_id);
  uint64_t end = start + nframes;
  uint64_t queued_tick = track->queued_tick;

  switch (track->state) {
  case DELOOP_TRACK_STATE_RECORDING_START_QUEUED:
    if (end < queued_tick) {
      return;
    }

    uint64_t record_from = (start < queued_tick) ? queued_tick - start : 0;
    track->state = DELOOP_TRACK_STATE_RECORDING;
    record(track, &in[record_from], nframes - record_from);
    deloop_counter_set_cnt(track->id, track->len - record_from);
    break;
  case DELOOP_TRACK_STATE_RECORDING:
    record(track, in, nframes);
    break;
  case DELOOP_TRACK_STATE_RECORDING_STOP_QUEUED:
    if (end < queued_tick) {
      return;
    }

    uint64_t record_to = (start < queued_tick) ? queued_tick - start : 0;
    track->state = DELOOP_TRACK_STATE_PLAYING_START_QUEUED;
    record(track, in, record_to);
    if (track->overdub_enabled) {
      overdub(track, &in[record_to], nframes - record_to);
    }
    break;
  case DELOOP_TRACK_STATE_PLAYING_START_QUEUED:
    uint64_t overdub_from = (start < queued_tick) ? queued_tick - start : 0;
    overdub(track, &in[overdub_from], nframes - overdub_from);
    break;
  case DELOOP_TRACK_STATE_PLAYING:
    if (track->overdub_enabled) {
      overdub(track, in, nframes);
    }
  case DELOOP_TRACK_STATE_PAUSED:
  case DELOOP_TRACK_STATE_IDLE:
    // No-op
    break;
  }
}

void deloop_track_write(deloop_track_t *track, float *out, size_t nframes) {
  if (track == NULL || out == NULL || track->len == 0) {
    return;
  } else if (is_stopped(track) || is_recording(track)) {
    track->read_head = 0;
    return;
  }

  uint64_t ctr_id = sync_counter(track);
  uint64_t start = deloop_counter_absolute(ctr_id);
  uint64_t end = start + nframes;

  uint64_t play_from = 0;
  uint64_t queued_tick = track->queued_tick;
  if (track->state == DELOOP_TRACK_STATE_PLAYING_START_QUEUED &&
      end > queued_tick) {
    track->state = DELOOP_TRACK_STATE_PLAYING;
    play_from = (start < track->queued_tick) ? track->queued_tick - start : 0;
  }

  for (uint64_t i = play_from; i < nframes; i++) {
    if (track->read_head >= track->len) {
      if (track->state == DELOOP_TRACK_STATE_RECORDING) {
        break;
      }

      track->read_head = 0;
    }

    out[i] += track->buffer[track->read_head];
    track->read_head += 1;
  }

  track->write_head = track->read_head;
}

void deloop_track_clear(deloop_track_t *track) {
  if (track == NULL) {
    return;
  }

  track->state = DELOOP_TRACK_STATE_IDLE;
  track->read_head = 0;
  track->write_head = 0;
  track->len = 0;
  deloop_counter_set_cnt(track->id, 0);
  deloop_counter_set_len(track->id, 0);
}

static inline deloop_track_id_t sync_counter(deloop_track_t *track) {
  switch (track->sync.mode) {
  case DELOOP_SYNC_MASTER:
    return track->id;
  case DELOOP_SYNC_SLAVE:
    return track->sync.track_id;
  }
}

static inline bool is_recording(deloop_track_t *track) {
  return track->state == DELOOP_TRACK_STATE_RECORDING ||
         track->state == DELOOP_TRACK_STATE_RECORDING_START_QUEUED;
}

static inline bool is_stopped(deloop_track_t *track) {
  switch (track->state) {
  case DELOOP_TRACK_STATE_IDLE:
  case DELOOP_TRACK_STATE_RECORDING_START_QUEUED:
  case DELOOP_TRACK_STATE_PAUSED:
    return true;
  default:
    return false;
  }
}

static inline void record(deloop_track_id_t *track, const float *in,
                          size_t nframes) {
  size_t n = MIN(track->capacity - track->write_head, nframes);
  memcpy((void *)&track->buffer[track->write_head], (const void *)in,
         n * sizeof(float));
  track->write_head += n;
  track->len += n;
  deloop_counter_set_len(track->id, track->len);
}

static inline void overdub(deloop_track_id_t *track, const float *in,
                           size_t nframes) {
  for (size_t i = 0; i < nframes; i++) {
    if (track->write_head >= track->len) {
      track->write_head = 0;
    }

    track->buffer[track->write_head] += in[i];
    track->write_head++;
  }
}
