#ifndef DLP_TRACK_H
#define DLP_TRACK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "error.h"

typedef enum dlp_track_id {
  DLP_TRACK_A = 0,
  DLP_TRACK_B,
  DLP_NUM_TRACKS,
} dlp_track_id_t;

// SETTINGS --------------------------------------
typedef enum dlp_sync_mode {
  DLP_SYNC_MASTER = 0,
  DLP_SYNC_SLAVE,
} dlp_sync_mode_t;

typedef struct dlp_sync_settings {
  dlp_sync_mode_t mode;
  dlp_track_id_t track_id; // Only valid if mode is SLAVE
} dlp_sync_settings_t;

// STATUS ---------------------------------------
typedef enum dlp_track_state_type {
  DLP_TRACK_STATE_IDLE = 0,
  DLP_TRACK_STATE_RECORDING_START_QUEUED,
  DLP_TRACK_STATE_RECORDING,
  DLP_TRACK_STATE_RECORDING_STOP_QUEUED,
  DLP_TRACK_STATE_PLAYING_START_QUEUED,
  DLP_TRACK_STATE_PLAYING,
  DLP_TRACK_STATE_PAUSED,
} dlp_track_state_type_t;

// API ------------------------------------------
typedef struct dlp_track {
  const dlp_track_id_t id;
  dlp_sync_settings_t sync;

  // State machine
  dlp_track_state_type_t state;
  uint64_t queued_tick; // Only valid if state is `*_QUEUED`
  bool overdub_enabled;
  float volume; // 0.0 to 1.0

  // Audio buffer
  size_t read_head;
  size_t write_head;
  size_t len;
  float *const buffer;
  const size_t capacity;
} dlp_track_t;

dlp_error_t dlp_track_init(dlp_track_t *track);
void dlp_track_advance_state(dlp_track_t *track);
void dlp_track_read(dlp_track_t *track, const float *in, size_t nframes);
void dlp_track_write(dlp_track_t *track, float *out, size_t nframes);
dlp_error_t dlp_track_sync_to(dlp_track_t *track, dlp_track_id_t master);
void dlp_track_clear(dlp_track_t *track);

#ifdef __cplusplus
}
#endif

#endif // DLP_TRACK_H
