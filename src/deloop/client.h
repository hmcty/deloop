#pragma once

#include <stdint.h>

#include <deloop/track.h>

typedef enum {
  deloop_DEVICE_TYPE_INVALID,
  deloop_DEVICE_TYPE_MIDI_OUTPUT,
  deloop_DEVICE_TYPE_AUDIO_OUTPUT,
  deloop_DEVICE_TYPE_AUDIO_INPUT,
} deloop_device_type_t;

typedef enum {
  deloop_DEVICE_PORT_TYPE_INVALID,
  deloop_DEVICE_PORT_TYPE_MONO,
  deloop_DEVICE_PORT_TYPE_STEREO_FL,
  deloop_DEVICE_PORT_TYPE_STEREO_FR,
} deloop_device_port_type_t;

typedef struct deloop_device {
  deloop_device_type_t type;
  deloop_device_port_type_t port_type;
  char *port_name;
  char *client_name;
  struct deloop_device *last;
} deloop_device_t;

typedef uint32_t deloop_client_track_id_t;

int deloop_client_init();
void deloop_client_cleanup();

deloop_client_track_id_t deloop_client_add_track();
deloop_client_track_id_t deloop_client_get_num_tracks();
deloop_track_t *deloop_client_get_track(deloop_client_track_id_t);
void deloop_client_set_focused_track(deloop_client_track_id_t);
deloop_client_track_id_t deloop_client_get_focused_track();

void deloop_client_add_source(const char *source_FL, const char *source_FR);
void deloop_client_remove_source(const char *source_FL, const char *source_FR);
void deloop_client_configure_sink(const char *output_FL, const char *output_FR);
void deloop_client_configure_control(const char *control);

deloop_device_t *deloop_client_find_midi_devices();
deloop_device_t *deloop_client_find_audio_devices(bool is_input, bool is_output);
void deloop_client_device_list_free(deloop_device_t *devices);
