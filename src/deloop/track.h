#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <jack/jack.h>

#define DELOOP_TRACK_DURATION_MAX_S ((uint32_t)120)
#define DELOOP_TRACK_DURATION_DEFAULT_S ((uint32_t)5)

typedef enum {
  DELOOP_TRACK_TYPE_INVALID,
  DELOOP_TRACK_TYPE_MONO,
  DELOOP_TRACK_TYPE_STEREO
} deloop_track_type_t;

typedef enum {
  DELOOP_TRACK_STATE_INVALID,
  DELOOP_TRACK_STATE_AWAITING,
  DELOOP_TRACK_STATE_PLAYING,
  DELOOP_TRACK_STATE_PAUSED,
  DELOOP_TRACK_STATE_RECORDING,
  DELOOP_TRACK_STATE_OVERDUBBING
} deloop_track_state_t;

typedef struct {
  float *buf;
  size_t buf_size;

  size_t read;
  size_t write;
  size_t size;
} deloop_track_ringbuf_t;

deloop_track_ringbuf_t *deloop_track_ringbuf_create(size_t buf_size);
void deloop_track_ringbuf_reset(deloop_track_ringbuf_t *rb);
size_t deloop_track_ringbuf_write(deloop_track_ringbuf_t *rb, float *data,
                                  size_t cnt, bool overdub);
void deloop_track_ringbuf_read(deloop_track_ringbuf_t *rb, float *data,
                               size_t cnt);
void deloop_track_ringbuf_free(deloop_track_ringbuf_t *rb);

typedef struct {
  deloop_track_type_t type;
  deloop_track_state_t state;
  jack_nframes_t frame_rate;
  uint32_t duration_s;

  deloop_track_ringbuf_t *lrb;
  deloop_track_ringbuf_t *rrb; // Stereo only
} deloop_track_t;

deloop_track_t *deloop_track_create(deloop_track_type_t type,
                                    jack_nframes_t frame_rate);
void deloop_track_free(deloop_track_t *track);

float deloop_track_get_progress(deloop_track_t *track);

void deloop_track_reset(deloop_track_t *track);
void deloop_track_handle_action(deloop_track_t *track);
void deloop_track_start_playing(deloop_track_t *track);
void deloop_track_stop_playing(deloop_track_t *track);
void deloop_track_start_recording(deloop_track_t *track);
void deloop_track_stop_recording(deloop_track_t *track);
bool deloop_track_is_recording(deloop_track_t *track);

void deloop_track_stereo_tick(deloop_track_t *track, float *lin, float *rin,
                              size_t cnt);
void deloop_track_stereo_read(deloop_track_t *track, float *lout, float *rout,
                              size_t cnt);
