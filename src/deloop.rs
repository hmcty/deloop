use std::sync::Arc;

pub struct Track {
}

struct ClientState {
    output_FL: jack::Port<jack::AudioOut>,
    output_FR: jack::Port<jack::AudioOut>,
    input_FL: jack::Port<jack::AudioIn>,
    input_FR: jack::Port<jack::AudioIn>,
    control: jack::Port<jack::MidiIn>,
    tracks: Vec<Track>,
}

struct ClientHandler {
    state: Arc<ClientState>,
}

impl ClientHandler {
    
    fn from_jack_client(client: &jack::Client) -> Self {
        let output_FL = client
            .register_port("output_FL", jack::AudioOut::default())
            .unwrap();
        let output_FR = client
            .register_port("output_FR", jack::AudioOut::default())
            .unwrap();
        let input_FL = client
            .register_port("input_FL", jack::AudioIn::default())
            .unwrap();
        let input_FR = client
            .register_port("input_FR", jack::AudioIn::default())
            .unwrap();
        let control = client
            .register_port("control", jack::MidiIn::default())
            .unwrap();
        let state = Arc::new(ClientState {
            output_FL,
            output_FR,
            input_FL,
            input_FR,
            control,
            tracks: Vec::new(),
        });

        ClientHandler { state }
    }
}

impl jack::ProcessHandler for ClientHandler {
    
    fn process(&mut self, _: &jack::Client, ps: &jack::ProcessScope) -> jack::Control {
        jack::Control::Continue
    }

    fn buffer_size(&mut self, _: &jack::Client, len: jack::Frames) -> jack::Control {
        jack::Control::Continue
    }

}

pub struct Client { 
    jack_client: jack::AsyncClient<(), ClientHandler>,
    state: Arc<ClientState>,
}

impl Client {
    pub fn new() -> Self {
        let (jack_client, _status) =
            jack::Client::new("deloop", jack::ClientOptions::default()).unwrap();
        let handler = ClientHandler::from_jack_client(&jack_client);
        let state = handler.state.clone();

        Client {
            jack_client: jack_client.activate_async((), handler).unwrap(),
            state: state,
        }
    }
}
