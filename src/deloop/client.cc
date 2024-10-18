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

#include <cstring>
#include <atomic>
#include <unordered_map>

#include "deloop/logging.h"
// #include "deloop/track.h"

#include "deloop/client.h"

#define DEFAULT_NUM_CHANNELS 2
#define MAX_NUM_PORTS 32
#define MAX_PORT_NAME_LENGTH 64
#define MAX_NUM_TRACKS 32

static int initialize();
static int process(jack_nframes_t nframes, void *arg);

static struct app_state_t {
  bool initialized;

  jack_client_t *client;
  jack_port_t *output_FL;
  jack_port_t *output_FR;
  jack_port_t *input_FL;
  jack_port_t *input_FR;
  jack_port_t *control_port;

  // unsigned int focus_track;
  // std::vector<std::unique_ptr<deloop::Track>> tracks;
  std::atomic<bool> input_queued;
} state_;

static int initialize() {
  const std::string client_name = "deloop";
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
  state_.client = jack_client_open(client_name.c_str(), options, &status, server_name);
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

  /* tell the JACK server to call `process()' whenever
     there is work to be done.
  */

  jack_set_process_callback(state_.client, process, NULL);

  /* tell the JACK server to call `jack_shutdown()' if
     it ever shuts down, either entirely, or if it
     just decides to stop calling us.
  */

  jack_on_shutdown(state_.client, (JackShutdownCallback) NULL, 0);

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

static int process(jack_nframes_t nframes, void *arg) {
  jack_midi_data_t *ctrl_buffer;
  jack_midi_event_t ctrl_event;
  uint8_t *ctrl_event_data;
  jack_nframes_t ctrl_event_count;
  // deloop_track_t *track;

//if (!state_.initialized || state_.focus_track >= state_.tracks.size()) {
//  return 0;
//}
//track = state_.track_list[state_.focus_track];

  ctrl_buffer = (jack_midi_data_t *) jack_port_get_buffer(state_.control_port, nframes);
  ctrl_event_count = (unsigned int) jack_midi_get_event_count(ctrl_buffer);

  // Handle queued user input
  if (state_.input_queued) {
    // track->handleUserInput();
    state_.input_queued = false;
  }

  // Handle MIDI control events
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
      // track->handleUserInput();
    }
  }

//track->stereoWrite(
//    (float *) jack_port_get_buffer(state_.input_FL, nframes),
//    (float *) jack_port_get_buffer(state_.input_FR, nframes),
//    nframes);

//memset(jack_port_get_buffer(state_.output_FL, nframes), 0,
//       nframes * sizeof(float));
//memset(jack_port_get_buffer(state_.output_FR, nframes), 0,
//       nframes * sizeof(float));

//for (auto &track : state_.tracks) {
//  track->stereoRead(
//      (float *) jack_port_get_buffer(state_.output_FL, nframes),
//      (float *) jack_port_get_buffer(state_.output_FR, nframes), nframes);
//}
  return 0;
}

namespace deloop {

Client::Client() {
  if (state_.initialized) {
    return;
  }

  int error = initialize();
  if (error != 0) {
    DELOOP_LOG_ERROR("Failed to initialize client: %d", error);
    return;
  }
}

Client::~Client() { }

std::vector<Device> findAudioDevices(bool allow_input, bool allow_output) {
  std::vector<Device> devices;

  unsigned long port_flags = 0;
  if (allow_input) port_flags |= JackPortIsInput;
  if (allow_output) port_flags |= JackPortIsOutput;

  const char **port_names = jack_get_ports(
    state_.client, NULL, JACK_DEFAULT_AUDIO_TYPE, port_flags);
  if (port_names == NULL) {
    return devices;
  }

  const std::unordered_map<std::string, DeviceType> kDeviceTypeMap = {
    { "output", DeviceType::kAudioOutput },
    { "playback", DeviceType::kAudioInput },
    { "capture", DeviceType::kAudioInput },
  };

  const std::unordered_map<std::string, DevicePortType> kDevicePortTypeMap = {
    { "FL", DevicePortType::kStereoFL },
    { "FR", DevicePortType::kStereoFR },
    { "MONO", DevicePortType::kMono },
  };

  unsigned int num_ports = 0;
  while (port_names[num_ports] != NULL) {
    // Split port name into client and port via ":"
    const char *client_split = strpbrk(port_names[num_ports], ":");
    if (client_split == NULL) {
      num_ports += 1;
      continue;
    }

    // Split port name into device and port type via "_"
    const char *port_split = strpbrk(port_names[num_ports], "_");
    if (port_split == NULL) {
      num_ports += 1;
      continue;
    }

    // Check if device type and port type are valid
    if (kDeviceTypeMap.find(port_split + 1) == kDeviceTypeMap.end() ||
        kDevicePortTypeMap.find(port_split + 1) == kDevicePortTypeMap.end()) {
      num_ports += 1;
      continue;
    } 

    jack_port_t *port = jack_port_by_name(state_.client, port_names[num_ports]);
    if (port == NULL) {
      num_ports += 1;
      continue;
    }

    Device device = {
      .type = kDeviceTypeMap.at(port_split + 1),
      .port_type = kDevicePortTypeMap.at(port_split + 1),
      .port_name = std::string(port_names[num_ports]),
      .client_name = std::string(
        port_names[num_ports],
        client_split - port_names[num_ports]),
    };
    devices.push_back(device);
    num_ports += 1;
  }

  jack_free(port_names);
  return devices;
}

std::vector<Device> findMidiDevices() {
  std::vector<Device> devices;

  const char **port_names = jack_get_ports(
    state_.client, NULL, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput);
  if (port_names == NULL) {
    return devices;
  }

  unsigned int num_ports = 0;
  while (port_names[num_ports] != NULL) {
    Device device = {
      .type = DeviceType::kMidiOutput,
      .port_type = DevicePortType::kMono,
      .port_name = std::string(port_names[num_ports]),
      .client_name = std::string(port_names[num_ports]),
    };
    devices.push_back(device);
    num_ports += 1;
  }
  
  jack_free(port_names);
  return devices;
}

//std::uint32_t addTrack() {

//}

//Track *getTrack(uint32_t track_num) {

//}

//std::uint32_t getFocusedTrack() {

//}

//void setFocusedTrack(std::uint32_t track_num) {

//}

void handleInput() {

}

void addSource(const char &source_FL, const char &source_FR) {

}

void removeSource(const char &source_FL, const char &source_FR) {

}

void configureSink(const char &sink_FL, const char &sink_FR) {

}

void disableSink() {

}

void configureControl(const char &control) {

}

void disableControl() {

}

} // namespace deloop

/*
deloop::Track *deloop::client_get_track(deloop::client_track_id_t track_num) {
  if (track_num >= state_.tracks.size()) {
    DELOOP_LOG_ERROR("Invalid track number: %u", track_num);
    return NULL;
  }

  return state_.tracks[track_num].get();
}

deloop::client_track_id_t deloop::client_add_track() {
  if (state_.num_tracks >= MAX_NUM_TRACKS) {
    DELOOP_LOG_ERROR("Max number of tracks reached: %u", MAX_NUM_TRACKS);
    return UINT32_MAX;
  }

  auto track = std::make_unique<deloop::Track>(
      DELOOP_TRACK_TYPE_STEREO, jack_get_sample_rate(state_.client));
  state_.tracks.push_back(track.move());
  return state_.tracks.size() - 1; 
}

void deloop::client_set_focused_track(deloop::client_track_id_t track_num) {
  if (track_num >= state_.num_tracks) {
    DELOOP_LOG_ERROR("Invalid track number: %u", track_num);
    return;
  }

  state_.focus_track = track_num;
}

deloop::client_track_id_t deloop::client_get_focused_track() {
  return state_.focus_track;
}

void deloop::client_handle_input() {
  state_.input_queued = true;
}

void deloop::client_add_sour

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

  if (sink_FL != NULL) {
    jack_connect(state_.client, jack_port_name(state_.output_FL), sink_FL);
  }

  if (sink_FR != NULL) {
    jack_connect(state_.client, jack_port_name(state_.output_FR), sink_FR);
  }
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
*/

