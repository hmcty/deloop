#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <jack/jack.h>
#include <jack/midiport.h>

#include "bizzy/logging.h"
#include "bizzy/track.h"

#include "bizzy/client.h"

#define DEFAULT_NUM_CHANNELS 2
#define MAX_NUM_PORTS 32
#define MAX_PORT_NAME_LENGTH 64

static struct app_state_t {
  bool initialized;

  jack_client_t *client;
  jack_port_t *output_FL;
  jack_port_t *output_FR;
  jack_port_t *input_FL;
  jack_port_t *input_FR;
  jack_port_t *control_port;

  bizzy_track_t *track1;
} state_;

int process(jack_nframes_t nframes, void *arg) {
  jack_midi_data_t *ctrl_buffer;
  jack_midi_event_t ctrl_event;
  uint8_t *ctrl_event_data;
  jack_nframes_t ctrl_event_count;

  ctrl_buffer = jack_port_get_buffer(state_.control_port, nframes);
  ctrl_event_count = jack_midi_get_event_count(ctrl_buffer); 
  for (int i = 0; i < ctrl_event_count; i++) {
    jack_midi_event_get(&ctrl_event, ctrl_buffer, i);
    ctrl_event_data = ctrl_event.buffer;
    if (ctrl_event.size != 3) continue;
    if ((ctrl_event_data[0] & 0xB0) != 0xB0) continue;
    if (ctrl_event_data[1] != 0x40) continue;
    if (ctrl_event_data[2] == 0x7F) {
      bizzy_track_start_recording(state_.track1);
    } else {
      bizzy_track_stop_recording(state_.track1);
    }
  }


  bizzy_track_stereo_tick(
    state_.track1,
    (float *) jack_port_get_buffer(state_.input_FL, nframes),
    (float *) jack_port_get_buffer(state_.input_FR, nframes),
    nframes);  

  // If playback not active, exit early
  if (!state_.track1->is_playing) return 0;
  bizzy_track_stereo_read(
    state_.track1,
    (float *) jack_port_get_buffer(state_.output_FL, nframes),
    (float *) jack_port_get_buffer(state_.output_FR, nframes),
    nframes);

	return 0;      
}

int bizzy_client_init() {
  const char *client_name = "bizzy";
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
		fprintf (stderr, "jack_client_open() failed, "
			 "status = 0x%2.0x\n", status);
		if (status & JackServerFailed) {
			fprintf (stderr, "Unable to connect to JACK server\n");
		}
    return 1;
	}
	if (status & JackServerStarted) {
		fprintf (stderr, "JACK server started\n");
	}
	if (status & JackNameNotUnique) {
		client_name = jack_get_client_name(state_.client);
		fprintf (stderr, "unique name `%s' assigned\n", client_name);
	}

	/* tell the JACK server to call `process()' whenever
	   there is work to be done.
	*/

	jack_set_process_callback(state_.client, process, NULL);

	/* tell the JACK server to call `jack_shutdown()' if
	   it ever shuts down, either entirely, or if it
	   just decides to stop calling us.
	*/

	jack_on_shutdown(state_.client, bizzy_client_cleanup, 0);

	/* create ports */

  state_.output_FL = jack_port_register(
    state_.client, "output_FL", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  if (state_.output_FL == NULL) {
    fprintf(stderr, "no more JACK ports available\n");
    return 1;
  }

  state_.output_FR = jack_port_register(
    state_.client, "output_FR", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  if (state_.output_FR == NULL) {
    fprintf(stderr, "no more JACK ports available\n");
    return 1;
  }

  state_.input_FL = jack_port_register(
    state_.client, "input_FL", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
  if (state_.input_FL == NULL) {
    fprintf(stderr, "no more JACK ports available\n");
    return 1;
  }

  state_.input_FR = jack_port_register(
    state_.client, "input_FR", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
  if (state_.input_FR == NULL) {
    fprintf(stderr, "no more JACK ports available\n");
    return 1;
  }

  state_.control_port = jack_port_register(
    state_.client, "control", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
  if (state_.control_port == NULL) {
    fprintf(stderr, "no more JACK ports available\n");
    return 1;
  }

	/* Tell the JACK server that we are ready to roll.  Our
	 * process() callback will start running now. */

	if (jack_activate(state_.client)) {
		fprintf (stderr, "cannot activate client");
    return 1;
	}

	/* Connect the ports.  You can't do this before the client is
	 * activated, because we can't make connections to clients
	 * that aren't running.  Note the confusing (but necessary)
	 * orientation of the driver backend ports: playback ports are
	 * "input" to the backend, and capture ports are "output" from
	 * it.
	 */

  state_.track1 = bizzy_track_create(
    BIZZY_TRACK_TYPE_STEREO,
    jack_get_sample_rate(state_.client));

  state_.initialized = true;
  return 0;
}

bizzy_device_t *bizzy_client_find_output_devices(size_t *num_devices) {
  bizzy_device_t *devices = NULL;
  char **port_names = NULL;
  int num_ports = 0;

  port_names = jack_get_ports(
    state_.client, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);
  if (port_names == NULL) {
    return NULL;
  }
  while (port_names[num_ports] != NULL) {
    // Split port name into client and port via ":"
    char *pch = strpbrk(port_names[num_ports], ":");
    if (pch == NULL) {
      num_ports += 1;
      continue;
    }

    bool is_left_output = strcmp(pch + 1, "output_FL") == 0;
    bool is_right_output = strcmp(pch + 1, "output_FR") == 0;
    jack_port_t *port = jack_port_by_name(state_.client, port_names[num_ports]);
    if ((!is_left_output && !is_right_output) || port == NULL) {
      num_ports += 1;
      continue;
    }

    bizzy_device_t *device = malloc(sizeof(bizzy_device_t)); 
    device->type = BIZZY_DEVICE_TYPE_AUDIO_OUTPUT;
    if (is_left_output) {
      device->port_type = BIZZY_DEVICE_PORT_TYPE_STEREO_FL;
    } else {
      device->port_type = BIZZY_DEVICE_PORT_TYPE_STEREO_FR;
    }
    device->port_name = strdup(port_names[num_ports]);
    device->client_name = strndup(port_names[num_ports], pch - port_names[num_ports]);
    device->last = devices;
    devices = device;
    num_ports += 1;
  }
  jack_free(port_names);

  return devices;
}

void bizzy_client_device_list_free(bizzy_device_t *devices) {
  while (devices != NULL) {
    bizzy_device_t *next = devices->last;
    free(devices->client_name);
    free(devices->port_name);
    free(devices);
    devices = next;
  }
}

bizzy_track_t *bizzy_client_get_track() {
  return state_.track1;
}

void bizzy_client_configure_source(const char *source_FL, const char *source_FR) {
  // @todo(hmcty): Check return values
  jack_port_disconnect(state_.client, state_.input_FL);
  jack_port_disconnect(state_.client, state_.input_FR);
  jack_connect(state_.client, source_FL, jack_port_name(state_.input_FL));
  jack_connect(state_.client, source_FR, jack_port_name(state_.input_FR));
}

void bizzy_client_configure_sink(const char *sink_FL, const char *sink_FR) {
  // @todo(hmcty): Check return values
  jack_port_disconnect(state_.client, state_.output_FL);
  jack_port_disconnect(state_.client, state_.output_FR);
  jack_connect(state_.client, jack_port_name(state_.output_FL), sink_FL);
  jack_connect(state_.client, jack_port_name(state_.output_FR), sink_FR);
}

void bizzy_client_cleanup() {
  jack_client_close(state_.client);
  bizzy_track_free(state_.track1);
  state_.initialized = false;
}

