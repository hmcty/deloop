#pragma once

#include <vector>
#include <string>

// #include <deloop/track.h>

namespace deloop {

enum class DeviceType {
  kInvalid = 0,
  kMidiOutput,
  kAudioOutput,
  kAudioInput
};

enum class DevicePortType {
  kInvalid = 0,
  kMono,
  kStereoFL,
  kStereoFR
};

struct Device {
  DeviceType type;
  DevicePortType port_type;
  std::string port_name;
  std::string client_name;
};

class Client {
  public:
    Client();
    ~Client();

    std::vector<Device> findAudioDevices(bool allow_input, bool allow_output);
    std::vector<Device> findMidiDevices();

//  unsigned int addTrack();
//  Track *getTrack(unsigned int track_id);

//  std::uint32_t getFocusedTrack();
//  void setFocusedTrack(unsigned int track_id);

    void handleUserInput();

    void addSource(const std::string &source_FL, const std::string &source_FR); 
    void removeSource(const std::string &source_FL, const std::string &source_FR);

    void configureSink(const std::string &output_FL, const std::string &output_FR);
    void disableSink();

    void configureControl(const std::string &control);
    void disableControl();

};

}; // namespace deloop
