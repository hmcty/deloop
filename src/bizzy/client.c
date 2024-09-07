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

#include "bizzy/logging.h"
#include "bizzy/track.h"

#include "bizzy/client.h"

#define DEFAULT_NUM_CHANNELS 2

static struct app_state_t {
  bool initialized;
  bool is_running;

  jack_client_t *client;
  jack_port_t *output_ports[DEFAULT_NUM_CHANNELS];
  jack_port_t *input_ports[DEFAULT_NUM_CHANNELS];

  bizzy_track_t *track1;
} state_;

/**
 * The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 *
 * This client follows a simple rule: when the JACK transport is
 * running, write nframes sine wave samples to the two outputs ports.
 * When it stops, exit.
 */

int process(jack_nframes_t nframes, void *arg) {
  assert(DEFAULT_NUM_CHANNELS == 2);
  bizzy_track_stereo_tick(
    state_.track1,
    (float *) jack_port_get_buffer(state_.input_ports[0], nframes),
    (float *) jack_port_get_buffer(state_.input_ports[1], nframes),
    nframes);

  /*
  for (int i = 0; i < DEFAULT_NUM_CHANNELS; i++) {
    float *out = (float *) jack_port_get_buffer(state_.output_ports[i], nframes);
    float *in = (float *) jack_port_get_buffer(state_.input_ports[i], nframes);

    memcpy(out, in, nframes * sizeof(float));
  }
  */

  if (!state_.track1->is_playing) {
    return 0;
  }
  float *out_FL = (float *) jack_port_get_buffer(state_.output_ports[0], nframes);
  float *out_FR = (float *) jack_port_get_buffer(state_.output_ports[1], nframes);
  bizzy_track_stereo_read(state_.track1, out_FL, out_FR, nframes);

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
 	
	ports = jack_get_ports(
    state_.client, NULL, NULL, JackPortIsPhysical | JackPortIsInput);
	if (ports == NULL) {
		fprintf(stderr, "no physical playback ports\n");
    return 1;
	}

	jack_free(ports);

  jack_position_t pos;
  jack_transport_query(state_.client, &pos);
  state_.track1 = bizzy_track_create(BIZZY_TRACK_TYPE_STEREO, pos.frame_rate);

  state_.initialized = true;
  return 0;
}

void bizzy_start() {
  state_.is_running = true;
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

