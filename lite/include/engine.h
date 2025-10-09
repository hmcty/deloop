#ifndef DELOOP_ENGINE_H
#define DELOOP_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// COMMANDS -------------------------------------
typedef enum deloop_engine_command_type {
  DELOOP_ENGINE_CMD_NONE = 0,
  DELOOP_ENGINE_CMD_ADVANCE,
  DELOOP_ENGINE_CMD_SET_OVERDUB,
  DELOOP_ENGINE_CMD_SET_VOLUME,
  DELOOP_ENGINE_CMD_PAUSE,
  DELOOP_ENGINE_CMD_CLEAR,
  DELOOP_ENGINE_CMD_CONFIGURE,
} deloop_engine_command_type_t;

typedef struct deloop_engine_command {
  deloop_engine_command_type_t cmd_type;
  uint16_t cmd_id;
  deloop_engine_id_t track_id;
  union {
    bool overdub_enabled;
    float volume;
    struct {
      deloop_sync_settings_t sync;
    } configure;
  } params;
} deloop_engine_command_t;

// RESPONSES ------------------------------------
typedef enum deloop_engine_response_code {
  DELOOP_ENGINE_RESP_OK = 0,
} deloop_engine_response_code_t;

typedef struct deloop_engine_response {
  uint16_t cmd_id;
  deloop_engine_response_code_t code;
} deloop_engine_response_t;

// API ------------------------------------------
void deloop_engine_init(void);
void deloop_engine_send_command(const deloop_engine_command_t *cmd);
void deloop_engine_check_response(deloop_engine_response_t *resp);
void deloop_engine_process_audio(const float *in, float *out, size_t nframes);
void deloop_engine_tick(void);

#ifdef __cplusplus
}
#endif

#endif // DELOOP_ENGINE_H
