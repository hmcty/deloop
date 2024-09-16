
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "jack/jack.h"

#include "bizzy/logging.h"

#include "bizzy/track.h"

#define MAX(a,b)              \
  ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define MIN(a,b)              \
  ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

bizzy_track_ringbuf_t *bizzy_track_ringbuf_create(size_t cnt) {
  bizzy_track_ringbuf_t *rb =
    (bizzy_track_ringbuf_t *) malloc(sizeof(bizzy_track_ringbuf_t));
  assert(rb != NULL);

  rb->buf = (float *) malloc(cnt * sizeof(float));
  assert(rb->buf != NULL);

  rb->buf_size = cnt;
  rb->size = cnt;
  rb->read = 0;
  rb->write = 0;

  return rb;
}

void bizzy_track_ringbuf_write(bizzy_track_ringbuf_t *rb, float *data, size_t cnt) {
  if ((rb == NULL) || (data == NULL)) return 0;
  if (rb->write >= rb->buf_size) {
    return 0;
  }

  size_t end = MIN(rb->write + cnt, rb->buf_size);
  cnt = end - rb->write;
  size_t nbytes = cnt * sizeof(float);
  
  assert(rb->buf != NULL);
  memcpy(rb->buf + rb->write, data, nbytes);
  rb->write += cnt;
  
  return 0;
}

void bizzy_track_ringbuf_read(bizzy_track_ringbuf_t *rb, float *data, size_t cnt) {
  if ((rb == NULL) || (data == NULL)) return 0;
  if (rb->read >= rb->size) {
    rb->read = 0;
  }

  size_t end = MIN(rb->read + cnt, rb->size);
  size_t icnt = end - rb->read;
  size_t nbytes = icnt * sizeof(float);

  assert(rb->buf != NULL);
  // memcpy(data, rb->buf + rb->read, nbytes); 
  // Mix with existing data
  for (size_t i = 0; i < icnt; i++) {
    data[i] += rb->buf[rb->read + i];
  }

  if (icnt < cnt) {
    nbytes = (cnt - icnt) * sizeof(float);

    assert(rb->buf != NULL);
    // memcpy(data + icnt, rb->buf, nbytes);
    // Mix with existing data
    for (size_t i = 0; i < (cnt - icnt); i++) {
      data[icnt + i] += rb->buf[i];
    }

    rb->read = (cnt - icnt); 
  } else {
    rb->read += icnt;
  }

  return 0;
}

void bizzy_track_ringbuf_free(bizzy_track_ringbuf_t *rb) {
  if (rb == NULL) return;
  if (rb->buf != NULL) free(rb->buf);
  free(rb);
}

bizzy_track_t *bizzy_track_create(bizzy_track_type_t type, jack_nframes_t frame_rate) {
  bizzy_track_t *track = (bizzy_track_t *) malloc(sizeof(bizzy_track_t));
  track->type = type;
  track->frame_rate = frame_rate;
  track->duration_s = BIZZY_TRACK_DURATION_DEFAULT_S;
  
  size_t buf_size = BIZZY_TRACK_DURATION_DEFAULT_S * frame_rate; 
  printf("Creating track with buffer size %lu\n", buf_size);

  track->lrb = bizzy_track_ringbuf_create(buf_size);
  switch (type) {
    case BIZZY_TRACK_TYPE_STEREO:
      track->rrb = bizzy_track_ringbuf_create(buf_size);
      break;
    case BIZZY_TRACK_TYPE_MONO:
      track->rrb = NULL;
      break;
    default:
      printf("Invalid track type\n");
      bizzy_track_free(track);
      return NULL;
  }

  return track;
}

void bizzy_track_free(bizzy_track_t *track) {
  if (track == NULL) return;
  if (track->lrb != NULL) bizzy_track_ringbuf_free(track->lrb);
  if (track->rrb != NULL) bizzy_track_ringbuf_free(track->rrb);
  free(track);
}

void bizzy_track_set_duration(bizzy_track_t *track, uint32_t duration_s) {
  if (track == NULL) return;

  bizzy_log_info("Setting track duration to %d seconds", duration_s);
  track->duration_s = duration_s;
  
  size_t buf_size = duration_s * track->frame_rate;
  assert(buf_size <= track->lrb->buf_size);
  track->lrb->size = buf_size;

  switch (track->type) {
    case BIZZY_TRACK_TYPE_STEREO:
      assert(buf_size <= track->rrb->buf_size);
      track->rrb->size = buf_size;
      break;
    case BIZZY_TRACK_TYPE_MONO:
      break;
    default:
      assert(false);
  }
}

void bizzy_track_start_playing(bizzy_track_t *track) {
  if (track == NULL) return;

  printf("Starting playing\n");
  track->is_playing = true;
}

void bizzy_track_stop_playing(bizzy_track_t *track) {
  if (track == NULL) return;

  printf("Stopping playing\n");
  track->is_playing = false;
  if (track->lrb != NULL) track->lrb->read = 0;
  if (track->rrb != NULL) track->rrb->read = 0;
}

void bizzy_track_start_recording(bizzy_track_t *track) {
  if (track == NULL) return;

  bizzy_track_stop_playing(track);

  printf("Starting recording\n");
  track->is_recording = true;
}

void bizzy_track_stop_recording(bizzy_track_t *track) {
  if (track == NULL) return;

  printf("Stopping recording\n");
  track->is_recording = false;
  bizzy_track_start_playing(track);
}

bool bizzy_track_is_recording(bizzy_track_t *track) {
  if (track == NULL) return false;

  return track->is_recording;
}

float bizzy_track_get_progress(bizzy_track_t *track) {
  if (track == NULL) return 0.0;

  float progress = (float) track->lrb->read / (float) track->lrb->size;
  return progress;
}

void bizzy_track_stereo_tick(
  bizzy_track_t *track,
  float *lin,
  float *rin,
  size_t cnt) {
  if (track == NULL) return;
  assert(track->type == BIZZY_TRACK_TYPE_STEREO);

  if (!track->is_recording) {
    if (track->lrb != NULL && track->lrb->write > 0) {
      track->lrb->size = track->lrb->write;
      track->lrb->write = 0;
    }

    if (track->rrb != NULL && track->rrb->write > 0) {
      track->rrb->size = track->rrb->write;
      track->rrb->write = 0;
    }

    return;
  }

  if (lin != NULL) {
    assert(track->lrb != NULL && track->lrb->buf != NULL);
    bizzy_track_ringbuf_write(track->lrb, lin, cnt);
  }

  if (rin != NULL) {
    assert(track->rrb != NULL && track->rrb->buf != NULL);
    bizzy_track_ringbuf_write(track->rrb, rin, cnt);
  }
}

void bizzy_track_stereo_read(
  bizzy_track_t *track,
  float *lout,
  float *rout,
  size_t cnt) {
  if ((track == NULL) || !track->is_playing) return;
  assert(track->type == BIZZY_TRACK_TYPE_STEREO);

  if (lout != NULL) {
    assert(track->lrb != NULL && track->lrb->buf != NULL);
    bizzy_track_ringbuf_read(track->lrb, lout, cnt);
  }

  if (rout != NULL) {
    assert(track->rrb != NULL && track->rrb->buf != NULL);
    bizzy_track_ringbuf_read(track->rrb, rout, cnt);
  }
}
