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
  bool is_running;

  jack_client_t *client;
  jack_port_t *output_ports[DEFAULT_NUM_CHANNELS];
  jack_port_t *input_ports[DEFAULT_NUM_CHANNELS];
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
    (float *) jack_port_get_buffer(state_.input_ports[0], nframes),
    (float *) jack_port_get_buffer(state_.input_ports[1], nframes),
    nframes);  

  // If playback not active, exit early
  if (!state_.track1->is_playing) return 0;
  bizzy_track_stereo_read(
    state_.track1,
    (float *) jack_port_get_buffer(state_.output_ports[0], nframes),
    (float *) jack_port_get_buffer(state_.output_ports[1], nframes),
    nframes);

	return 0;      
}

int bizzy_init() {
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

	jack_on_shutdown(state_.client, bizzy_cleanup, 0);

	/* create ports */

  state_.output_ports[0] = jack_port_register(
    state_.client, "output_FL", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  if (state_.output_ports[0] == NULL) {
    fprintf(stderr, "no more JACK ports available\n");
    return 1;
  }

  state_.output_ports[1] = jack_port_register(
    state_.client, "output_FR", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  if (state_.output_ports[1] == NULL) {
    fprintf(stderr, "no more JACK ports available\n");
    return 1;
  }

  state_.input_ports[0] = jack_port_register(
    state_.client, "input_FL", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
  if (state_.input_ports[0] == NULL) {
    fprintf(stderr, "no more JACK ports available\n");
    return 1;
  }

  state_.input_ports[1] = jack_port_register(
    state_.client, "input_FR", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
  if (state_.input_ports[1] == NULL) {
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

  char **output_audio_ports = bizzy_get_output_audio_ports();
  i = 0;
  while (output_audio_ports[i] != NULL) {
    bizzy_log_info("%s", output_audio_ports[i]);
    i += 1;
  }
 	
  state_.track1 = bizzy_track_create(
    BIZZY_TRACK_TYPE_STEREO,
    jack_get_sample_rate(state_.client));

  state_.initialized = true;
  return 0;
}

void bizzy_start() {
  state_.is_running = true;
}

char **bizzy_get_output_audio_ports() {
  // Update the list of output ports
  return jack_get_ports(
    state_.client, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);

  //int i = 0;
  //while (port_names[i] != NULL && i < MAX_NUM_PORTS) {
  //  strncpy(state_.output_audio_ports[i], port_names[i], MAX_PORT_NAME_LENGTH);
  //  i++;
  //}
  //for (; i < MAX_NUM_PORTS; i++) {
  //  state_.output_audio_ports[i][0] = '\0';
  //}
  // jack_free(port_names);
}

void bizzy_port_list_free(char **port_list) {
  jack_free(port_list);
}

void bizzy_stop() {
  state_.is_running = false;
}

bizzy_track_t *bizzy_get_track() {
  return state_.track1;
}

void bizzy_cleanup() {
  jack_client_close(state_.client);
  bizzy_track_free(state_.track1);
  state_.initialized = false;
}

