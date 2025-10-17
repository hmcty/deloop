#include "engine.h"

#include <complex.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "counter.h"
#include "error.h"
#include "ringbuf.h"
#include "track.h"

#define AUDIO_BUFFER_CAPACITY (48000 * 2 * 60) // 90 seconds at 48kHz
#define CMD_RESP_BUFFER_CAPACITY (8)

static float __attribute__((section(
    ".sdram_bss"))) audio_buffers[DLP_NUM_TRACKS][AUDIO_BUFFER_CAPACITY];
static dlp_engine_command_t cmd_buf[CMD_RESP_BUFFER_CAPACITY];
static dlp_engine_response_t resp_buf[CMD_RESP_BUFFER_CAPACITY];

static struct {
  bool initialized;
  dlp_track_t tracks[2];
  dlp_ringbuf_t cmd_rb;
  dlp_ringbuf_t resp_rb;
} state_ = {
    .initialized = false,
    .tracks = {{.id = DLP_TRACK_A,
                .buffer = &audio_buffers[DLP_TRACK_A][0],
                .capacity = AUDIO_BUFFER_CAPACITY},
               {
                   .id = DLP_TRACK_B,
                   .buffer = &audio_buffers[DLP_TRACK_B][0],
                   .capacity = AUDIO_BUFFER_CAPACITY,
               }},
    .cmd_rb = {.data = &cmd_buf,
               .item_size = sizeof(dlp_engine_command_t),
               .capacity = CMD_RESP_BUFFER_CAPACITY},
    .resp_rb = {.data = &resp_buf,
                .item_size = sizeof(dlp_engine_response_t),
                .capacity = CMD_RESP_BUFFER_CAPACITY},
};

dlp_error_t dlp_engine_init(void) {
  if (state_.initialized) {
    return DLP_ERROR_ALREADY_INITIALIZED;
  }

  for (size_t i = 0; i < DLP_NUM_TRACKS; i++) {
    if (dlp_track_init(&state_.tracks[i]) != DLP_SUCCESS) {
      return DLP_ERROR_INTERNAL;
    }
  }

  if (dlp_ringbuf_init(&state_.cmd_rb) != DLP_SUCCESS) {
    return DLP_ERROR_INTERNAL;
  }

  if (dlp_ringbuf_init(&state_.resp_rb) != DLP_SUCCESS) {
    return DLP_ERROR_INTERNAL;
  }

  state_.initialized = true;
  return DLP_SUCCESS;
}

dlp_error_t dlp_engine_send_command(const dlp_engine_command_t *const cmd) {
  if (!state_.initialized) {
    return DLP_ERROR_NOT_INITIALIZED;
  }

  return dlp_ringbuf_push(&state_.cmd_rb, cmd);
}

dlp_error_t dlp_engine_check_response(dlp_engine_response_t *const resp) {
  if (!state_.initialized) {
    return DLP_ERROR_NOT_INITIALIZED;
  }
  return dlp_ringbuf_pop(&state_.resp_rb, resp);
}

dlp_error_t dlp_engine_process_audio(const float *const in, float *const out,
                                     size_t nframes) {
  if (!state_.initialized) {
    return DLP_ERROR_NOT_INITIALIZED;
  }

  dlp_engine_command_t cmd;
  if (dlp_ringbuf_pop(&state_.cmd_rb, &cmd) == DLP_SUCCESS) {
    switch (cmd.cmd_type) {
    case DLP_ENGINE_CMD_ADVANCE:
      dlp_track_advance_state(&state_.tracks[cmd.track_id]);
      break;
    case DLP_ENGINE_CMD_SET_OVERDUB:
      state_.tracks[cmd.track_id].overdub_enabled = cmd.params.overdub_enabled;
      break;
    case DLP_ENGINE_CMD_SET_VOLUME:
      state_.tracks[cmd.track_id].volume = cmd.params.volume;
      break;
    case DLP_ENGINE_CMD_PAUSE:
      state_.tracks[cmd.track_id].state = DLP_TRACK_STATE_PAUSED;
      break;
    case DLP_ENGINE_CMD_CLEAR:
      dlp_track_clear(&state_.tracks[cmd.track_id]);
      break;
    default:
      // Unknown command
    }

    dlp_engine_response_t resp = {
        .cmd_id = cmd.cmd_id,
        .resp_type = DLP_ENGINE_RESP_OK,
    };
    dlp_ringbuf_push(&state_.resp_rb, &resp);
  }

  memcpy(out, in, nframes * sizeof(float));
  for (size_t i = 0; i < DLP_NUM_TRACKS; i++) {
    dlp_track_read(&state_.tracks[i], in, nframes);
    dlp_track_write(&state_.tracks[i], out, nframes);
  }

  dlp_counter_advance_all(nframes);
  return DLP_SUCCESS;
}
