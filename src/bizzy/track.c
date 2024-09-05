
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "jack/jack.h"
#include "jack/ringbuffer.h"

#include "bizzy/track.h"

bizzy_track_t *bizzy_track_create(jack_nframes_t frame_rate) {
  bizzy_track_t *track = (bizzy_track_t *) malloc(sizeof(bizzy_track_t));
  track->frame_rate = frame_rate;
  track->duration_s = BIZZY_TRACK_DURATION_DEFAULT_S;

  size_t ch_size = BIZZY_TRACK_DURATION_DEFAULT_S * frame_rate * sizeof(float);
  track->rb = jack_ringbuffer_create(ch_size * BIZZY_TRACK_NUM_CHANNELS);
  return track;
}

void bizzy_track_free(bizzy_track_t *track) {
  if (track == NULL) return;

  jack_ringbuffer_free(track->rb);
  free(track);
}

void bizzy_track_set_duration(bizzy_track_t *track, uint32_t duration_s) {
  if (track == NULL) return;

  track->duration_s = duration_s;
}

void bizzy_track_start_recording(bizzy_track_t *track) {
  if (track == NULL) return;

  printf("Starting recording\n");
  track->is_recording = true;
}

void bizzy_track_stop_recording(bizzy_track_t *track) {
  if (track == NULL) return;

  printf("Stopping recording\n");
  track->is_recording = false;
}

bool bizzy_track_is_recording(bizzy_track_t *track) {
  if (track == NULL) return false;

  return track->is_recording;
}

void bizzy_track_tick(bizzy_track_t *track, float *frames, size_t cnt) {
  if (track == NULL) return;

  /* TODO: Implement */
}
