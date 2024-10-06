
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "jack/jack.h"

#include "deloop/logging.h"
#include "track.h"

#include "deloop/track.h"

#define MAX(a, b)                                                              \
  ({                                                                           \
    __typeof__(a) _a = (a);                                                    \
    __typeof__(b) _b = (b);                                                    \
    _a > _b ? _a : _b;                                                         \
  })

#define MIN(a, b)                                                              \
  ({                                                                           \
    __typeof__(a) _a = (a);                                                    \
    __typeof__(b) _b = (b);                                                    \
    _a < _b ? _a : _b;                                                         \
  })

deloop_track_ringbuf_t *deloop_track_ringbuf_create(size_t cnt) {
  deloop_track_ringbuf_t *rb =
      (deloop_track_ringbuf_t *)malloc(sizeof(deloop_track_ringbuf_t));
  assert(rb != NULL);

  rb->buf = (float *)malloc(cnt * sizeof(float));
  assert(rb->buf != NULL);
  rb->buf_size = cnt;

  deloop_track_ringbuf_reset(rb);
  return rb;
}

void deloop_track_ringbuf_reset(deloop_track_ringbuf_t *rb) {
  if (rb == NULL)
    return;

  if (rb->buf != NULL) {
    memset(rb->buf, 0, rb->buf_size * sizeof(float));
  }
  rb->size = rb->buf_size;
  rb->read = 0;
  rb->write = 0;
}

float deloop_track_ringbuf_write(deloop_track_ringbuf_t *rb, float *data,
                                 size_t cnt, bool overdub) {
  if ((rb == NULL) || (data == NULL))
    return 0.0;

  // If overdubbing, only write up to the buffer size
  size_t max_size = overdub ? rb->size : rb->buf_size;
  if (rb->write >= max_size) {
    if (overdub) {
      rb->write = 0;
    } else {
      return 0.0;
    }
  }

  float amp = 0.0;
  size_t end = MIN(rb->write + cnt, max_size);
  size_t icnt = end - rb->write;
  size_t nbytes = cnt * sizeof(float);

  assert(rb->buf != NULL);
  for (size_t i = 0; i < icnt; i++) {
    amp += data[i];
    rb->buf[rb->write + i] += data[i];
  }

  if (overdub && icnt < cnt) {
    assert(rb->buf != NULL);
    for (size_t i = 0; i < (cnt - icnt); i++) {
      amp += data[icnt + i];
      rb->buf[i] += data[icnt + i];
    }

    rb->write = (cnt - icnt);
  } else {
    rb->write += icnt;
  }
  rb->size = MAX(rb->size, rb->write);

  amp /= cnt;
  return amp;
}

float deloop_track_ringbuf_read(deloop_track_ringbuf_t *rb, float *data,
                                size_t cnt) {
  if ((rb == NULL) || (data == NULL))
    return 0;
  if (rb->read >= rb->size) {
    rb->read = 0;
  }

  float amp = 0.0;
  size_t end = MIN(rb->read + cnt, rb->size);
  size_t icnt = end - rb->read;
  size_t nbytes = icnt * sizeof(float);

  assert(rb->buf != NULL);
  // memcpy(data, rb->buf + rb->read, nbytes);
  // Mix with existing data
  for (size_t i = 0; i < icnt; i++) {
    data[i] += rb->buf[rb->read + i];
    amp += data[i];
  }

  if (icnt < cnt) {
    nbytes = (cnt - icnt) * sizeof(float);

    assert(rb->buf != NULL);
    // memcpy(data + icnt, rb->buf, nbytes);
    // Mix with existing data
    for (size_t i = 0; i < (cnt - icnt); i++) {
      data[icnt + i] += rb->buf[i];
      amp += data[icnt + i];
    }

    rb->read = (cnt - icnt);
  } else {
    rb->read += icnt;
  }

  return amp / cnt;
}

void deloop_track_ringbuf_free(deloop_track_ringbuf_t *rb) {
  if (rb == NULL)
    return;
  if (rb->buf != NULL)
    free(rb->buf);
  free(rb);
}

deloop_track_t *deloop_track_create(deloop_track_type_t type,
                                    jack_nframes_t frame_rate) {
  deloop_track_t *track = (deloop_track_t *)malloc(sizeof(deloop_track_t));
  track->type = type;
  track->state = DELOOP_TRACK_STATE_AWAITING;
  track->frame_rate = frame_rate;
  track->duration_s = DELOOP_TRACK_DURATION_DEFAULT_S;
  track->playback_speed = 1.0;

  size_t buf_size = DELOOP_TRACK_DURATION_MAX_S * frame_rate;
  DELOOP_LOG_INFO("Creating track with buffer size %lu\n", buf_size);

  track->lrb = deloop_track_ringbuf_create(buf_size);
  switch (type) {
  case DELOOP_TRACK_TYPE_STEREO:
    track->rrb = deloop_track_ringbuf_create(buf_size);
    break;
  case DELOOP_TRACK_TYPE_MONO:
    track->rrb = NULL;
    break;
  default:
    DELOOP_LOG_ERROR("Invalid track type");
    deloop_track_free(track);
    return NULL;
  }

  deloop_track_reset(track);
  return track;
}

void deloop_track_free(deloop_track_t *track) {
  if (track == NULL)
    return;

  if (track->lrb != NULL)
    deloop_track_ringbuf_free(track->lrb);
  if (track->rrb != NULL)
    deloop_track_ringbuf_free(track->rrb);
  free(track);
}

void deloop_track_reset(deloop_track_t *track) {
  if (track == NULL)
    return;

  deloop_track_ringbuf_reset(track->lrb);
  deloop_track_ringbuf_reset(track->rrb);
  track->state = DELOOP_TRACK_STATE_AWAITING;
  track->prev_state = DELOOP_TRACK_STATE_AWAITING;
}

void deloop_track_handle_action(deloop_track_t *track) {
  if (track == NULL)
    return;

  static time_t last_action_time = 0;
  if (track->prev_state != DELOOP_TRACK_STATE_RECORDING &&
      difftime(time(NULL), last_action_time) < 1.0f) {
    deloop_track_reset(track);
  } else {
    switch (track->state) {
    case DELOOP_TRACK_STATE_AWAITING:
      track->state = DELOOP_TRACK_STATE_RECORDING;
      track->prev_state = DELOOP_TRACK_STATE_AWAITING;
      break;
    case DELOOP_TRACK_STATE_PLAYING:
      track->state = DELOOP_TRACK_STATE_PAUSED;
      track->prev_state = DELOOP_TRACK_STATE_PLAYING;
      if (track->lrb != NULL)
        track->lrb->read = 0;
      if (track->rrb != NULL)
        track->rrb->read = 0;
      break;
    case DELOOP_TRACK_STATE_PAUSED:
      track->state = DELOOP_TRACK_STATE_PLAYING;
      track->prev_state = DELOOP_TRACK_STATE_PAUSED;
      break;
    case DELOOP_TRACK_STATE_RECORDING:
      track->state = DELOOP_TRACK_STATE_OVERDUBBING;
      track->prev_state = DELOOP_TRACK_STATE_RECORDING;

      if (track->lrb != NULL) {
        track->lrb->size = track->lrb->write;
        track->lrb->write = 0;
      }

      if (track->rrb != NULL) {
        track->rrb->size = track->rrb->write;
        track->rrb->write = 0;
      }
      break;
    case DELOOP_TRACK_STATE_OVERDUBBING:
      track->state = DELOOP_TRACK_STATE_PLAYING;
      track->prev_state = DELOOP_TRACK_STATE_OVERDUBBING;
      break;
    default:
      break;
    }
  }

  last_action_time = time(NULL);
}

void deloop_track_set_playback_speed(deloop_track_t *track, float speed) {
  if (track == NULL)
    return;

  track->playback_speed = speed;
}

bool deloop_track_is_recording(deloop_track_t *track) {
  if (track == NULL)
    return false;

  return track->state == DELOOP_TRACK_STATE_RECORDING ||
         track->state == DELOOP_TRACK_STATE_OVERDUBBING;
}

float deloop_track_get_progress(deloop_track_t *track) {
  if (track == NULL)
    return 0.0;

  float progress = (float)track->lrb->read / (float)track->lrb->size;
  return progress;
}

void deloop_track_stereo_tick(deloop_track_t *track, float *lin, float *rin,
                              size_t cnt) {
  if (track == NULL)
    return;

  float wamp = 0.0;
  if (deloop_track_is_recording(track)) {
    if (lin != NULL) {
      assert(track->lrb != NULL && track->lrb->buf != NULL);
      wamp += deloop_track_ringbuf_write(
          track->lrb, lin, cnt, track->state == DELOOP_TRACK_STATE_OVERDUBBING);
    }

    if (rin != NULL) {
      assert(track->rrb != NULL && track->rrb->buf != NULL);
      wamp += deloop_track_ringbuf_write(
          track->rrb, rin, cnt, track->state == DELOOP_TRACK_STATE_OVERDUBBING);
    }
  }

  track->wamp = wamp / 2.0;
}

void deloop_track_stereo_read(deloop_track_t *track, float *lout, float *rout,
                              size_t cnt) {
  if (track == NULL || track->state == DELOOP_TRACK_STATE_AWAITING ||
      track->state == DELOOP_TRACK_STATE_PAUSED ||
      track->state == DELOOP_TRACK_STATE_RECORDING) {
    return;
  }
  assert(track->type == DELOOP_TRACK_TYPE_STEREO);

  float ramp = 0.0;
  if (lout != NULL) {
    assert(track->lrb != NULL && track->lrb->buf != NULL);
    ramp += deloop_track_ringbuf_read(track->lrb, lout, cnt);
  }

  if (rout != NULL) {
    assert(track->rrb != NULL && track->rrb->buf != NULL);
    ramp += deloop_track_ringbuf_read(track->rrb, rout, cnt);
  }

  track->ramp = ramp / 2.0;
}
