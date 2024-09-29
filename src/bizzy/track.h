#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <jack/jack.h>

#define BIZZY_TRACK_DURATION_MAX_S     ((uint32_t) 120)
#define BIZZY_TRACK_DURATION_DEFAULT_S ((uint32_t) 5)

typedef enum {
  BIZZY_TRACK_TYPE_INVALID,
  BIZZY_TRACK_TYPE_MONO,
  BIZZY_TRACK_TYPE_STEREO
} bizzy_track_type_t;

typedef enum {
  BIZZY_TRACK_STATE_INVALID,
  BIZZY_TRACK_STATE_STOPPED,
  BIZZY_TRACK_STATE_PLAYING,
  BIZZY_TRACK_STATE_RECORDING,
  BIZZY_TRACK_STATE_OVERDUBBING
} bizzy_track_state_t;

typedef struct {
  float *buf;
  size_t buf_size;

  size_t read;
  size_t write;
  size_t size;
} bizzy_track_ringbuf_t;

bizzy_track_ringbuf_t *bizzy_track_ringbuf_create(size_t buf_size);
void bizzy_track_ringbuf_reset(bizzy_track_ringbuf_t *rb);
size_t bizzy_track_ringbuf_write(
  bizzy_track_ringbuf_t *rb, float *data, size_t cnt, bool overdub);
void bizzy_track_ringbuf_read(
  bizzy_track_ringbuf_t *rb, float *data, size_t cnt);
void bizzy_track_ringbuf_free(bizzy_track_ringbuf_t *rb);

typedef struct {
  bizzy_track_type_t type;
  bizzy_track_state_t state;
  jack_nframes_t frame_rate;
  uint32_t duration_s;

  bizzy_track_ringbuf_t *lrb;
  bizzy_track_ringbuf_t *rrb; // Stereo only
} bizzy_track_t;

bizzy_track_t *bizzy_track_create(
  bizzy_track_type_t type, jack_nframes_t frame_rate);
void bizzy_track_free(bizzy_track_t *track);

float bizzy_track_get_progress(bizzy_track_t *track);

void bizzy_track_set_duration(bizzy_track_t *track, uint32_t duration_s);
void bizzy_track_start_playing(bizzy_track_t *track);
void bizzy_track_stop_playing(bizzy_track_t *track);
void bizzy_track_start_recording(bizzy_track_t *track);
void bizzy_track_stop_recording(bizzy_track_t *track);
bool bizzy_track_is_recording(bizzy_track_t *track);

void bizzy_track_stereo_tick(
  bizzy_track_t *track, float *lin, float *rin, size_t cnt);
void bizzy_track_stereo_read(
  bizzy_track_t *track, float *lout, float *rout, size_t cnt);

