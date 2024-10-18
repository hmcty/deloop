#[cxx::bridge(namespace = "deloop")]
mod ffi {

    enum DeviceType {
        kInvalid,
        kMidiOutput,
        kMidiInput,
        kAudioOutput,
    } 

    enum DevicePortType {
        kInvalid,
        kMono,
        kStereoFL,
        kStereoFR,
    } 

    struct Device {
        device_type: DeviceType,
        port_type: DevicePortType,
        port_name: String,
        client_name: String,
    } 

    unsafe extern "C++" {
        include!("deloop/client.h");
        type Client;
        fn findAudioDevices(self: &Client, allow_input: bool, allow_output: bool) -> Vec<Device>;
    }
}
