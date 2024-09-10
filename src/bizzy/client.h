#pragma once

#include <stdint.h>

#include <bizzy/track.h>

typedef enum {
  BIZZY_DEVICE_TYPE_INVALID,
  BIZZY_DEVICE_TYPE_MIDI_OUTPUT,
  BIZZY_DEVICE_TYPE_AUDIO_OUTPUT,
  BIZZY_DEVICE_TYPE_AUDIO_INPUT,
} bizzy_device_type_t;

typedef enum {
  BIZZY_DEVICE_PORT_TYPE_INVALID,
  BIZZY_DEVICE_PORT_TYPE_MONO,
  BIZZY_DEVICE_PORT_TYPE_STEREO_FL,
  BIZZY_DEVICE_PORT_TYPE_STEREO_FR,
} bizzy_device_port_t;

typedef struct bizzy_device {
  bizzy_device_type_t type;
  bizzy_device_port_t port_type;
  char *port_name;
  char *client_name;
  struct bizzy_device *last;
} bizzy_device_t;

int bizzy_client_init();
void bizzy_client_cleanup();

bizzy_track_t *bizzy_client_get_track();
void bizzy_client_configure_source(const char *source_FL, const char *source_FR);
void bizzy_client_configure_sink(const char *output_FL, const char *output_FR);

// bizzy_device_t *bizzy_client_find_control_devices();
// bizzy_device_t *bizzy_client_find_input_devices();
bizzy_device_t *bizzy_client_find_output_devices();
void bizzy_client_device_list_free(bizzy_device_t *devices);

