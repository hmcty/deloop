use std::sync::Arc;
use std::collections::HashSet;

pub struct Track {
}

struct ClientState {
    output_fl: jack::Port<jack::AudioOut>,
    output_fr: jack::Port<jack::AudioOut>,
    input_fl: jack::Port<jack::AudioIn>,
    input_fr: jack::Port<jack::AudioIn>,
    control: jack::Port<jack::MidiIn>,
    tracks: Vec<Track>,
}

struct ClientHandler {
    state: Arc<ClientState>,
}

impl ClientHandler {
    
    fn from_jack_client(client: &jack::Client) -> Self {
        let output_fl = client
            .register_port("output_fl", jack::AudioOut::default())
            .unwrap();
        let output_fr = client
            .register_port("output_fr", jack::AudioOut::default())
            .unwrap();
        let input_fl = client
            .register_port("input_fl", jack::AudioIn::default())
            .unwrap();
        let input_fr = client
            .register_port("input_fr", jack::AudioIn::default())
            .unwrap();
        let control = client
            .register_port("control", jack::MidiIn::default())
            .unwrap();
        let state = Arc::new(ClientState {
            output_fl,
            output_fr,
            input_fl,
            input_fr,
            control,
            tracks: Vec::new(),
        });

        ClientHandler { state }
    }
}

impl jack::ProcessHandler for ClientHandler {
    
    fn process(&mut self, _: &jack::Client, _ps: &jack::ProcessScope) -> jack::Control {
        jack::Control::Continue
    }

    fn buffer_size(&mut self, _: &jack::Client, _len: jack::Frames) -> jack::Control {
        jack::Control::Continue
    }

}

pub struct Client { 
    jack_session: jack::AsyncClient<(), ClientHandler>,
    state: Arc<ClientState>,
}

impl Client {
    pub fn new() -> Self {
        let (jack_client, _status) =
            jack::Client::new("deloop", jack::ClientOptions::default()).unwrap();
        let handler = ClientHandler::from_jack_client(&jack_client);
        let state = handler.state.clone();

        Client {
            jack_session: jack_client.activate_async((), handler).unwrap(),
            state: state,
        }
    }

    fn get_clients_from_ports(&self, ports: Vec<String>) -> HashSet<String> {
        let mut devices: HashSet<String> = HashSet::new();
        for port in ports {
            if let Some(device) = port.split(":").next() {
                devices.insert(device.to_string());
            }
        }
        return devices;
    }

    pub fn audio_sources(&self) -> HashSet<String> {
        self.get_clients_from_ports(
            self.jack_session
                .as_client()
                .ports(
                    None,
                    Some(jack::jack_sys::FLOAT_MONO_AUDIO),
                    jack::PortFlags::IS_OUTPUT,
                )
        )
    }

    pub fn audio_sinks(&self) -> HashSet<String> {
        self.get_clients_from_ports(
            self.jack_session
                .as_client()
                .ports(
                    None,
                    Some(jack::jack_sys::FLOAT_MONO_AUDIO),
                    jack::PortFlags::IS_INPUT,
                )
        )
    }

    pub fn midi_sources(&self) -> HashSet<String> {
        self.get_clients_from_ports(
            self.jack_session
                .as_client()
                .ports(
                    None,
                    Some(jack::jack_sys::RAW_MIDI_TYPE),
                    jack::PortFlags::IS_OUTPUT,
                )
        )
    }
}
