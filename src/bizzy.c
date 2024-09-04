#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <jack/jack.h>

#include "bizzy.h"

#define DEFAULT_NUM_CHANNELS 2

static struct app_state_t {
  bool initialized;
  bool is_running;

  jack_client_t *client;
  jack_port_t *output_ports[DEFAULT_NUM_CHANNELS];
  jack_port_t *input_ports[DEFAULT_NUM_CHANNELS];
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
  if (!state_.is_running) {
    return 0;
  }

  for (int i = 0; i < DEFAULT_NUM_CHANNELS; i++) {
    float *out = (float *) jack_port_get_buffer(state_.output_ports[i], nframes);
    float *in = (float *) jack_port_get_buffer(state_.input_ports[i], nframes);

    memcpy(out, in, nframes * sizeof(float));
  }

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

  for (i = 0; i < DEFAULT_NUM_CHANNELS; i++) {
    state_.output_ports[i] = jack_port_register (
      state_.client, "output",
      JACK_DEFAULT_AUDIO_TYPE,
      JackPortIsOutput, 0);

    state_.input_ports[i] = jack_port_register (
      state_.client, "input",
      JACK_DEFAULT_AUDIO_TYPE,
      JackPortIsInput, 0);

    if ((state_.output_ports[i] == NULL) || (state_.input_ports[i] == NULL)) {
      fprintf(stderr, "no more JACK ports available\n");
      return 1;
    }
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
  state_.initialized = true;
  return 0;
}

void bizzy_start() {
  state_.is_running = true;
}

void bizzy_stop() {
  state_.is_running = false;
}

void bizzy_cleanup() {
  jack_client_close(state_.client);
  state_.initialized = false;
}

