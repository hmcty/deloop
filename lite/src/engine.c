#include "engine.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "track.h"

static struct {
  bool initialized;
  deloop_track_t tracks[2];
  // ringbuf for command
  // ringbuf for response
} state_;

void deloop_engine_init(void) {
  if (state_.initialized) {
    return;
  }

  memset(&state_, 0, sizeof(state_));
  state_.initialized = true;
}

void deloop_engine_send_command(const deloop_engine_command_t *cmd) {
  if (!state_.initialized) {
    return;
  }
}

void deloop_engine_get_response(deloop_engine_response_code_t *resp,
                                uint16_t *cmd_id) {
  if (!state_.initialized) {
    return;
  }
}

void deloop_engine_process_audio(const float *in, float *out, size_t nframes) {
  if (!state_.initialized) {
    return;
  }
}

void deloop_engine_tick(void) {
  if (!state_.initialized) {
    return;
  }

  // Pop from command ringbuf
  // Enqueue response
}
