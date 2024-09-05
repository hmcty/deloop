#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <jack/jack.h>
#include <jack/ringbuffer.h>

#define BIZZY_TRACK_DURATION_MAX_S     ((uint32_t) 60)
#define BIZZY_TRACK_DURATION_DEFAULT_S ((uint32_t) 10)

#define BIZZY_TRACK_NUM_CHANNELS 2

typedef struct {
  jack_nframes_t frame_rate;
  uint32_t duration_s;

  bool is_recording;

  jack_ringbuffer_t *rb;
} bizzy_track_t;

bizzy_track_t *bizzy_track_create(jack_nframes_t frame_rate);
void bizzy_track_free(bizzy_track_t *track);

void bizzy_track_set_duration(bizzy_track_t *track, uint32_t duration_s);

void bizzy_track_start_recording(bizzy_track_t *track);
void bizzy_track_stop_recording(bizzy_track_t *track);
bool bizzy_track_is_recording(bizzy_track_t *track);

void bizzy_track_tick(bizzy_track_t *track, float *frames, size_t cnt);

