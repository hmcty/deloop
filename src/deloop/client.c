#include <assert.h>
#include <errno.h>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "deloop/logging.h"
#include "deloop/track.h"
#include "track.h"

#include "deloop/client.h"

#define DEFAULT_NUM_CHANNELS 2
#define MAX_NUM_PORTS 32
#define MAX_PORT_NAME_LENGTH 64
#define MAX_NUM_TRACKS 32

static struct app_state_t {
  bool initialized;

  jack_client_t *client;
  jack_port_t *output_FL;
  jack_port_t *output_FR;
  jack_port_t *input_FL;
  jack_port_t *input_FR;
  jack_port_t *control_port;

  uint32_t num_tracks;
  uint32_t focus_track;
  deloop_track_t *track_list[MAX_NUM_TRACKS];
} state_;

int process(jack_nframes_t nframes, void *arg) {
  jack_midi_data_t *ctrl_buffer;
  jack_midi_event_t ctrl_event;
  uint8_t *ctrl_event_data;
  jack_nframes_t ctrl_event_count;
  deloop_track_t *track;

  if (!state_.initialized || state_.num_tracks == 0 ||
      state_.focus_track >= state_.num_tracks ||
      (track = state_.track_list[state_.focus_track]) == NULL) {
    return 0;
  }
  track = state_.track_list[state_.focus_track];

  ctrl_buffer = jack_port_get_buffer(state_.control_port, nframes);
  ctrl_event_count = jack_midi_get_event_count(ctrl_buffer);
  for (int i = 0; i < ctrl_event_count; i++) {
    jack_midi_event_get(&ctrl_event, ctrl_buffer, i);
    ctrl_event_data = ctrl_event.buffer;
    if (ctrl_event.size != 3)
      continue;
    if ((ctrl_event_data[0] & 0xB0) != 0xB0)
      continue;
    if (ctrl_event_data[1] != 0x40)
      continue;
    if (ctrl_event_data[2] == 0x7F) {
      // Act on press: https://x.com/ID_AA_Carmack/status/1787850053912064005
      deloop_track_handle_action(track);
    }
  }

  deloop_track_stereo_tick(
      track, (float *)jack_port_get_buffer(state_.input_FL, nframes),
      (float *)jack_port_get_buffer(state_.input_FR, nframes), nframes);

  memset(jack_port_get_buffer(state_.output_FL, nframes), 0,
         nframes * sizeof(float));
  memset(jack_port_get_buffer(state_.output_FR, nframes), 0,
         nframes * sizeof(float));

  // If playback not active, exit early
  for (uint32_t i = 0; i < state_.num_tracks; i++) {
    if (state_.track_list[i] == NULL)
      continue;

    deloop_track_stereo_read(
        state_.track_list[i],
        (float *)jack_port_get_buffer(state_.output_FL, nframes),
        (float *)jack_port_get_buffer(state_.output_FR, nframes), nframes);
  }
  return 0;
}

int deloop_client_init() {
  const char *client_name = "deloop";
  const char **ports;
  const char *server_name = NULL;
  jack_options_t options = JackNullOption;
  jack_status_t status;
  int i;

  if (state_.initialized) {
    return 0;
  }
  memset(&state_, 0, sizeof(state_));

  /* open a client connection to the JACK server */
  state_.client = jack_client_open(client_name, options, &status, server_name);
  if (state_.client == NULL) {
    DELOOP_LOG_ERROR("jack_client_open() failed, status = 0x%2.0x", status);
    if (status & JackServerFailed) {
      DELOOP_LOG_ERROR("Unable to connect to JACK server");
    }
    return 1;
  }
  if (status & JackServerStarted) {
    DELOOP_LOG_INFO("JACK server started");
  }
  if (status & JackNameNotUnique) {
    client_name = jack_get_client_name(state_.client);
    DELOOP_LOG_WARN("Unique name `%s' assigned", client_name);
  }

  /* tell the JACK server to call `process()' whenever
     there is work to be done.
  */

  jack_set_process_callback(state_.client, process, NULL);

  /* tell the JACK server to call `jack_shutdown()' if
     it ever shuts down, either entirely, or if it
     just decides to stop calling us.
  */

  jack_on_shutdown(state_.client, deloop_client_cleanup, 0);

  /* create ports */

  state_.output_FL = jack_port_register(
      state_.client, "output_FL", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  if (state_.output_FL == NULL) {
    DELOOP_LOG_ERROR("No more JACK ports available");
    return 1;
  }

  state_.output_FR = jack_port_register(
      state_.client, "output_FR", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  if (state_.output_FR == NULL) {
    DELOOP_LOG_ERROR("No more JACK ports available");
    return 1;
  }

  state_.input_FL = jack_port_register(
      state_.client, "input_FL", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
  if (state_.input_FL == NULL) {
    DELOOP_LOG_ERROR("No more JACK ports available");
    return 1;
  }

  state_.input_FR = jack_port_register(
      state_.client, "input_FR", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
  if (state_.input_FR == NULL) {
    DELOOP_LOG_ERROR("No more JACK ports available");
    return 1;
  }

  state_.control_port = jack_port_register(
      state_.client, "control", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
  if (state_.control_port == NULL) {
    DELOOP_LOG_ERROR("No more JACK ports available");
    return 1;
  }

  /* Tell the JACK server that we are ready to roll.  Our
   * process() callback will start running now. */

  if (jack_activate(state_.client)) {
    DELOOP_LOG_ERROR("Failed to activate client");
    return 1;
  }

  /* Connect the ports.  You can't do this before the client is
   * activated, because we can't make connections to clients
   * that aren't running.  Note the confusing (but necessary)
   * orientation of the driver backend ports: playback ports are
   * "input" to the backend, and capture ports are "output" from
   * it.
   */

  // state_.track1 = deloop_track_create(
  //   DELOOP_TRACK_TYPE_STEREO,
  //   jack_get_sample_rate(state_.client));

  state_.initialized = true;
  return 0;
}

deloop_device_t *deloop_client_find_audio_devices(bool is_input,
                                                  bool is_output) {
  deloop_device_t *devices = NULL;
  char **port_names = NULL;
  unsigned long port_flags = 0;
  int num_ports = 0;

  if (is_input)
    port_flags |= JackPortIsInput;
  if (is_output)
    port_flags |= JackPortIsOutput;
  port_names =
      jack_get_ports(state_.client, NULL, JACK_DEFAULT_AUDIO_TYPE, port_flags);
  if (port_names == NULL) {
    return NULL;
  }
  while (port_names[num_ports] != NULL) {
    // Split port name into client and port via ":"
    char *ch = strpbrk(port_names[num_ports], ":");
    if (ch == NULL) {
      num_ports += 1;
      continue;
    }

    deloop_device_type_t device_type;
    deloop_device_port_type_t port_type;
    if (strcmp(ch + 1, "output_FL") == 0) {
      device_type = deloop_DEVICE_TYPE_AUDIO_OUTPUT;
      port_type = DELOOP_DEVICE_PORT_TYPE_STEREO_FL;
    } else if (strcmp(ch + 1, "output_FR") == 0) {
      device_type = deloop_DEVICE_TYPE_AUDIO_OUTPUT;
      port_type = DELOOP_DEVICE_PORT_TYPE_STEREO_FR;
    } else if (strcmp(ch + 1, "playback_FL") == 0) {
      device_type = deloop_DEVICE_TYPE_AUDIO_INPUT;
      port_type = DELOOP_DEVICE_PORT_TYPE_STEREO_FL;
    } else if (strcmp(ch + 1, "playback_FR") == 0) {
      device_type = deloop_DEVICE_TYPE_AUDIO_INPUT;
      port_type = DELOOP_DEVICE_PORT_TYPE_STEREO_FR;
    } else if (strcmp(ch + 1, "capture_MONO") == 0) {
      device_type = deloop_DEVICE_TYPE_AUDIO_INPUT;
      port_type = DELOOP_DEVICE_PORT_TYPE_MONO;
    } else {
      num_ports += 1;
      continue;
    }

    jack_port_t *port = jack_port_by_name(state_.client, port_names[num_ports]);
    if (port == NULL) {
      num_ports += 1;
      continue;
    }

    deloop_device_t *device = malloc(sizeof(deloop_device_t));
    device->type = device_type;
    device->port_type = port_type;
    device->port_name = strdup(port_names[num_ports]);
    device->client_name =
        strndup(port_names[num_ports], ch - port_names[num_ports]);
    device->last = devices;
    devices = device;
    num_ports += 1;
  }

  jack_free(port_names);
  return devices;
}

deloop_device_t *deloop_client_find_midi_devices() {
  deloop_device_t *devices = NULL;
  char **port_names = NULL;
  unsigned long port_flags = 0;
  int num_ports = 0;

  port_names = jack_get_ports(state_.client, NULL, JACK_DEFAULT_MIDI_TYPE,
                              JackPortIsOutput);
  if (port_names == NULL) {
    return NULL;
  }
  while (port_names[num_ports] != NULL) {
    deloop_device_t *device = malloc(sizeof(deloop_device_t));
    device->type = deloop_DEVICE_TYPE_MIDI_OUTPUT;
    device->port_type = DELOOP_DEVICE_PORT_TYPE_MONO;
    device->port_name = strdup(port_names[num_ports]);
    device->client_name = strdup(port_names[num_ports]);
    device->last = devices;
    devices = device;
    num_ports += 1;
  }

  jack_free(port_names);
  return devices;
}

void deloop_client_device_list_free(deloop_device_t *devices) {
  while (devices != NULL) {
    deloop_device_t *next = devices->last;
    free(devices->client_name);
    free(devices->port_name);
    free(devices);
    devices = next;
  }
}

deloop_track_t *deloop_client_get_track(deloop_client_track_id_t track_num) {
  if (track_num >= state_.num_tracks) {
    DELOOP_LOG_ERROR("Invalid track number: %u", track_num);
    return NULL;
  } else if (state_.track_list[track_num] == NULL) {
    DELOOP_LOG_ERROR("Track %u is NULL", track_num);
    return NULL;
  }

  return state_.track_list[track_num];
}

deloop_client_track_id_t deloop_client_get_num_tracks() {
  return state_.num_tracks;
}

deloop_client_track_id_t deloop_client_add_track() {
  if (state_.num_tracks >= MAX_NUM_TRACKS) {
    DELOOP_LOG_ERROR("Max number of tracks reached: %u", MAX_NUM_TRACKS);
    return UINT32_MAX;
  }

  deloop_track_t *track = deloop_track_create(
      DELOOP_TRACK_TYPE_STEREO, jack_get_sample_rate(state_.client));
  state_.track_list[state_.num_tracks] = track;
  state_.num_tracks += 1;
  return state_.num_tracks - 1;
}

void deloop_client_set_focused_track(deloop_client_track_id_t track_num) {
  if (track_num >= state_.num_tracks) {
    DELOOP_LOG_ERROR("Invalid track number: %u", track_num);
    return;
  }

  state_.focus_track = track_num;
}

deloop_client_track_id_t deloop_client_get_focused_track() {
  return state_.focus_track;
}

void deloop_client_add_source(const char *source_FL, const char *source_FR) {
  // @todo(hmcty): Check return values
  jack_connect(state_.client, source_FL, jack_port_name(state_.input_FL));
  jack_connect(state_.client, source_FR, jack_port_name(state_.input_FR));
}

void deloop_client_remove_source(const char *source_FL, const char *source_FR) {
  // @todo(hmcty): Check return values
  jack_port_disconnect(state_.client, state_.input_FL);
  jack_port_disconnect(state_.client, state_.input_FR);
}

void deloop_client_configure_sink(const char *sink_FL, const char *sink_FR) {
  // @todo(hmcty): Check return values
  jack_port_disconnect(state_.client, state_.output_FL);
  jack_port_disconnect(state_.client, state_.output_FR);
  jack_connect(state_.client, jack_port_name(state_.output_FL), sink_FL);
  jack_connect(state_.client, jack_port_name(state_.output_FR), sink_FR);
}

void deloop_client_configure_control(const char *control) {
  // @todo(hmcty): Check return values
  jack_port_disconnect(state_.client, state_.control_port);
  jack_connect(state_.client, control, jack_port_name(state_.control_port));
}

void deloop_client_cleanup() {
  jack_client_close(state_.client);
  for (uint32_t i = 0; i < state_.num_tracks; i++) {
    if (state_.track_list[i] == NULL)
      continue;
    deloop_track_free(state_.track_list[i]);
  }
  state_.initialized = false;
}
