#ifndef DLP_ENGINE_H
#define DLP_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "error.h"
#include "track.h"

// COMMANDS -------------------------------------
typedef enum dlp_engine_command_type {
  DLP_ENGINE_CMD_NONE = 0,
  DLP_ENGINE_CMD_ADVANCE,
  DLP_ENGINE_CMD_SET_OVERDUB,
  DLP_ENGINE_CMD_SET_VOLUME,
  DLP_ENGINE_CMD_PAUSE,
  DLP_ENGINE_CMD_CLEAR,
  DLP_ENGINE_CMD_CONFIGURE,
  DLP_ENGINE_CMD_STOP_ALL,
} dlp_engine_command_type_t;

typedef struct dlp_engine_command {
  uint16_t cmd_id;
  dlp_engine_command_type_t cmd_type;
  dlp_track_id_t track_id;
  union {
    bool overdub_enabled;
    float volume;
    struct {
      dlp_sync_settings_t sync;
    } configure;
  } params;
} dlp_engine_command_t;

// RESPONSES ------------------------------------
typedef enum dlp_engine_response_type {
  DLP_ENGINE_RESP_OK = 0,
} dlp_engine_response_type_t;

typedef struct dlp_engine_response {
  uint16_t cmd_id;
  dlp_engine_response_type_t resp_type;
} dlp_engine_response_t;

// API ------------------------------------------
dlp_error_t dlp_engine_init(void);
dlp_error_t dlp_engine_send_command(const dlp_engine_command_t *const cmd);
dlp_error_t dlp_engine_check_response(dlp_engine_response_t *const resp);
dlp_error_t dlp_engine_process_audio(const float *const in, float *const out,
                                     size_t nframes);

#ifdef __cplusplus
}
#endif

#endif // DLP_ENGINE_H
