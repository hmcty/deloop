#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>
#include <jack/jack.h>

#include "bizzy.h"

#define DEFAULT_NUM_CHANNELS 2

static jack_client_t *client;
static jack_port_t *output_ports[DEFAULT_NUM_CHANNELS];
static jack_port_t *input_ports[DEFAULT_NUM_CHANNELS];

typedef struct {
 /* Empty */
} app_data_t;

/**
 * The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 *
 * This client follows a simple rule: when the JACK transport is
 * running, write nframes sine wave samples to the two outputs ports.
 * When it stops, exit.
 */

int process(jack_nframes_t nframes, void *arg) {
  app_data_t *data = (app_data_t*) arg;
  int i;

  for (i = 0; i < DEFAULT_NUM_CHANNELS; i++) {
    float *out = (float *) jack_port_get_buffer(output_ports[i], nframes);
    float *in = (float *) jack_port_get_buffer(input_ports[i], nframes);

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
	app_data_t data;
	int i;

	/* open a client connection to the JACK server */
	client = jack_client_open(client_name, options, &status, server_name);
	if (client == NULL) {
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
		client_name = jack_get_client_name(client);
		fprintf (stderr, "unique name `%s' assigned\n", client_name);
	}

	/* tell the JACK server to call `process()' whenever
	   there is work to be done.
	*/

	jack_set_process_callback(client, process, &data);

	/* tell the JACK server to call `jack_shutdown()' if
	   it ever shuts down, either entirely, or if it
	   just decides to stop calling us.
	*/

	jack_on_shutdown(client, bizzy_cleanup, 0);

	/* create ports */

  for (i = 0; i < DEFAULT_NUM_CHANNELS; i++) {
    output_ports[i] = jack_port_register (
      client, "output",
      JACK_DEFAULT_AUDIO_TYPE,
      JackPortIsOutput, 0);

    input_ports[i] = jack_port_register (
      client, "input",
      JACK_DEFAULT_AUDIO_TYPE,
      JackPortIsInput, 0);

    if ((output_ports[i] == NULL) || (input_ports[i] == NULL)) {
      fprintf(stderr, "no more JACK ports available\n");
      return 1;
    }
  }

	/* Tell the JACK server that we are ready to roll.  Our
	 * process() callback will start running now. */

	if (jack_activate(client)) {
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
 	
	ports = jack_get_ports(client, NULL, NULL, JackPortIsPhysical | JackPortIsInput);
	if (ports == NULL) {
		fprintf(stderr, "no physical playback ports\n");
    return 1;
	}

//if (jack_connect(client, jack_port_name(output_port1), ports[0])) {
//	fprintf (stderr, "cannot connect output ports\n");
//}

//if (jack_connect(client, jack_port_name(output_port1), ports[0])) {
//	fprintf (stderr, "cannot connect output ports\n");
//}

	jack_free(ports);
    
  /* install a signal handler to properly quits jack client */
  // signal(SIGQUIT, signal_handler);
	// signal(SIGTERM, signal_handler);
	// signal(SIGHUP, signal_handler);
	// signal(SIGINT, signal_handler);

	/* keep running until the Ctrl+C */

 // while (1) {
 //		sleep (1);
 // 	}

	// jack_client_close(client);
  return 0;
}

void bizzy_cleanup() {
  jack_client_close(client);
}

