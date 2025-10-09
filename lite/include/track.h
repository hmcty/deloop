#ifndef DELOOP_TRACK_H
#define DELOOP_TRACK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define DELOOP_MAX_TRACKS ((uint8_t)2)

typedef enum deloop_track_id {
  DELOOP_TRACK_A = 0,
  DELOOP_TRACK_B,
} deloop_track_id_t;

// SETTINGS --------------------------------------
typedef enum deloop_sync_mode {
  DELOOP_SYNC_MASTER = 0,
  DELOOP_SYNC_SLAVE,
} deloop_sync_mode_t;

typedef struct deloop_sync_settings {
  deloop_sync_mode_t mode;
  deloop_track_id_t track_id; // Only valid if mode is SLAVE
} deloop_sync_settings_t;

// STATUS ---------------------------------------
typedef enum deloop_track_state_type {
  DELOOP_TRACK_STATE_IDLE = 0,
  DELOOP_TRACK_STATE_RECORDING_START_QUEUED,
  DELOOP_TRACK_STATE_RECORDING,
  DELOOP_TRACK_STATE_RECORDING_STOP_QUEUED,
  DELOOP_TRACK_STATE_PLAYING_START_QUEUED,
  DELOOP_TRACK_STATE_PLAYING,
  DELOOP_TRACK_STATE_PAUSED,
} deloop_track_state_type_t;

// API ------------------------------------------
typedef struct deloop_track {
  const deloop_track_id_t id;
  deloop_sync_settings_t sync;

  // State machine
  deloop_track_state_type_t state;
  uint64_t queued_tick; // Only valid if state is `*_QUEUED`
  bool overdub_enabled;
  float volume; // 0.0 to 1.0

  // Audio buffer
  size_t read_head;
  size_t write_head;
  size_t len;
  const float *buffer;
  const size_t capacity;
} deloop_track_t;

void deloop_track_init(deloop_track_t *track);
void deloop_track_advance_state(deloop_track_t *track);
void deloop_track_read(deloop_track_t *track, const float *in, size_t nframes);
void deloop_track_write(deloop_track_t *track, float *out, size_t nframes);
void deloop_track_clear(deloop_track_t *track);

#ifdef __cplusplus
}
#endif

#endif // DELOOP_TRACK_H
