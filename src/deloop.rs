use log::{info, warn};
use snafu::prelude::*;
use std::collections::HashSet;
use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::{Arc, Mutex};

mod track;
use track::Track;

#[derive(Debug, Snafu)]
pub enum Error {
    #[snafu(display("{msg}: {source}"))]
    JackError { source: jack::Error, msg: String },

    #[snafu(display("Failed to find port by name '{name}'"))]
    PortNotFound { name: String },

    #[snafu(display("No ports found for source '{source_name}'"))]
    SourceNotFound { source_name: String },

    #[snafu(display("No ports found for sink '{sink_name}'"))]
    SinkNotFound { sink_name: String },

    #[snafu(display("Ports don't match expected format on '{client_name}'"))]
    UnexpectedPortFormat {
        client_name: String,
        labeled_ports: LabeledPorts,
    },
}

struct ClientState {
    output_fl: Mutex<jack::Port<jack::AudioOut>>,
    output_fr: Mutex<jack::Port<jack::AudioOut>>,
    input_fl: jack::Port<jack::AudioIn>,
    input_fr: jack::Port<jack::AudioIn>,
    control: jack::Port<jack::MidiIn>,
    tracks: Mutex<Vec<Track>>,
    focused_track: AtomicUsize,
}

// Implementation of Jack processing callbacks.
struct ClientHandler {
    state: Arc<ClientState>,
}

impl ClientHandler {
    // Setup handler from an existing Jack client.
    fn new(client: &jack::Client) -> Result<Self, Error> {
        let output_fl = client
            .register_port("output_FL", jack::AudioOut::default())
            .context(JackSnafu {
                msg: "Failed to register output_FL port",
            })?;
        let output_fr = client
            .register_port("output_FR", jack::AudioOut::default())
            .context(JackSnafu {
                msg: "Failed to register output_FR port",
            })?;
        let input_fl = client
            .register_port("input_FL", jack::AudioIn::default())
            .context(JackSnafu {
                msg: "Failed to register input_FL port",
            })?;
        let input_fr = client
            .register_port("input_FR", jack::AudioIn::default())
            .context(JackSnafu {
                msg: "Failed to register input_FR port",
            })?;
        let control = client
            .register_port("control", jack::MidiIn::default())
            .context(JackSnafu {
                msg: "Failed to register control port",
            })?;
        let state = ClientState {
            output_fl: Mutex::new(output_fl),
            output_fr: Mutex::new(output_fr),
            input_fl,
            input_fr,
            control,
            tracks: Mutex::new(Vec::new()),
            focused_track: AtomicUsize::new(0),
        };

        Ok(ClientHandler {
            state: Arc::new(state),
        })
    }
}

impl jack::ProcessHandler for ClientHandler {
    // Carries out a single iteration of the audio processing loop.
    //
    // In essence, forwards incoming data to the respective tracks and mixes
    // their output to a single port.
    fn process(&mut self, _: &jack::Client, ps: &jack::ProcessScope) -> jack::Control {
        let mut tracks = self.state.tracks.lock().unwrap();
        if tracks.is_empty() {
            return jack::Control::Continue;
        }

        let focused_track = &mut tracks[self.state.focused_track.load(Ordering::Relaxed)];
        focused_track.read_from(
            self.state.input_fl.as_slice(ps),
            self.state.input_fr.as_slice(ps),
        );
        focused_track.write_to(
            self.state.output_fl.lock().unwrap().as_mut_slice(ps),
            self.state.output_fr.lock().unwrap().as_mut_slice(ps),
        );

        for midi_event in self.state.control.iter(ps) {
            focused_track.handle_midi_event(midi_event.bytes);
        }
        jack::Control::Continue
    }

    // Handles changes in the buffer size.
    fn buffer_size(&mut self, _: &jack::Client, _len: jack::Frames) -> jack::Control {
        jack::Control::Continue
    }
}

#[derive(Debug)]
pub struct LabeledPorts {
    fl: Option<String>,
    fr: Option<String>,
    mono: Option<String>,
}

impl LabeledPorts {
    // Identifies port types by name.
    fn from_ports_names(port_names: &Vec<String>) -> Self {
        let mut fl = None;
        let mut fr = None;
        let mut mono = None;
        for port_name in port_names {
            match port_name.as_str() {
                _ if port_name.ends_with("_FL") => fl = Some(port_name.clone()),
                _ if port_name.ends_with("_FR") => fr = Some(port_name.clone()),
                _ if port_name.ends_with("_MONO") => mono = Some(port_name.clone()),
                _ => {}
            }
        }

        LabeledPorts { fl, fr, mono }
    }
}

/// Deloop's Jack client
///
/// Bridges the gap between user commands and audio processing.
pub struct Client {
    jack_session: jack::AsyncClient<(), ClientHandler>,
    state: Arc<ClientState>,
}

impl Client {
    /// Creates a new Deloop client.
    ///
    /// Opens a connection to the jack server and begins processing audio.
    pub fn new() -> Result<Self, Error> {
        let (jack_client, _status) =
            jack::Client::new("deloop", jack::ClientOptions::default()).unwrap();
        let handler = ClientHandler::new(&jack_client)?;
        let state = handler.state.clone();

        Ok(Client {
            jack_session: jack_client.activate_async((), handler).unwrap(),
            state,
        })
    }

    pub fn add_track(&self) {
        self.state.tracks.lock().unwrap().push(Track::new());
    }

    /// Returns a set of available audio sources.
    ///
    /// Can be passed to `subscribe_to` to begin processing audio from a source.
    pub fn audio_sources(&self) -> HashSet<String> {
        self.get_clients_from_ports(self.jack_session.as_client().ports(
            None,
            Some(jack::jack_sys::FLOAT_MONO_AUDIO),
            jack::PortFlags::IS_OUTPUT,
        ))
    }

    /// Returns a set of available audio sinks.
    ///
    /// Can be passed to `publish_to` to begin sending audio to a sink.
    pub fn audio_sinks(&self) -> HashSet<String> {
        self.get_clients_from_ports(self.jack_session.as_client().ports(
            None,
            Some(jack::jack_sys::FLOAT_MONO_AUDIO),
            jack::PortFlags::IS_INPUT,
        ))
    }

    /// Returns a set of available MIDI sources.
    ///
    /// Can be passed to `subscribe_to` to begin processing MIDI from a source.
    pub fn midi_sources(&self) -> HashSet<String> {
        let mut sources: HashSet<String> = HashSet::new();
        let ports = self.jack_session.as_client().ports(
            None,
            Some(jack::jack_sys::RAW_MIDI_TYPE),
            jack::PortFlags::IS_OUTPUT,
        );
        for port in ports {
            let Some(channel_name) = port.split(':').nth(1) else {
                continue;
            };
            sources.insert(channel_name.to_string());
        }

        sources
    }

    /// Starts reading audio/MIDI data from the specified source.
    pub fn subscribe_to(&mut self, source_name: &str) -> Result<(), Error> {
        let jack_client = self.jack_session.as_client();
        let port_names = jack_client.ports(Some(source_name), None, jack::PortFlags::IS_OUTPUT);
        ensure!(!port_names.is_empty(), SourceNotFoundSnafu { source_name });

        // If both FL and FR ports are available, use them.
        // TODO(hmcty): Handle failure mode where only one port connects.
        let labeled_ports = LabeledPorts::from_ports_names(&port_names);
        if let (Some(fl), Some(fr)) = (&labeled_ports.fl, &labeled_ports.fr) {
            info!("Subscribing to '{} [STEREO]'", source_name);
            self.subscribe_port_to(&self.state.input_fl, fl)?;
            self.subscribe_port_to(&self.state.input_fr, fr)?;
        } else if let Some(mono) = &labeled_ports.mono {
            info!("Subscribing to '{} [MONO]'", source_name);
            self.subscribe_port_to(&self.state.input_fl, mono)?;
            self.subscribe_port_to(&self.state.input_fr, mono)?;
        } else if port_names.len() == 1 && self.midi_sources().contains(source_name) {
            info!("Subscribing to '{} [MIDI]'", source_name);
            self.subscribe_port_to(&self.state.control, &port_names[0])?;
        } else {
            return UnexpectedPortFormatSnafu {
                client_name: source_name.to_string(),
                labeled_ports,
            }
            .fail();
        }

        Ok(())
    }

    /// Stops reading audio/MIDI data from the specified source.
    pub fn unsubscribe_from(&mut self, source_name: &str) -> Result<(), Error> {
        for port in self.state.input_fl.get_connections() {
            if port.starts_with(source_name) {
                self.unsubscribe_port_from(&self.state.input_fl, &port)?;
            }
        }

        for port in self.state.input_fr.get_connections() {
            if port.starts_with(source_name) {
                self.unsubscribe_port_from(&self.state.input_fr, &port)?;
            }
        }

        for port in self.state.control.get_connections() {
            if port.starts_with(source_name) {
                self.unsubscribe_port_from(&self.state.control, &port)?;
            }
        }

        Ok(())
    }

    /// Creates an outgoing audio stream to the specified sink.
    pub fn publish_to(&mut self, sink_name: &str) -> Result<(), Error> {
        let jack_client = self.jack_session.as_client();
        let port_names = jack_client.ports(Some(sink_name), None, jack::PortFlags::IS_INPUT);
        ensure!(!port_names.is_empty(), SinkNotFoundSnafu { sink_name });

        // If both FL and FR ports are available, use them.
        let labeled_ports = LabeledPorts::from_ports_names(&port_names);
        if let (Some(fl), Some(fr)) = (&labeled_ports.fl, &labeled_ports.fr) {
            info!("Publishing at '{} [STEREO]'", sink_name);
            self.publish_port_at(&self.state.output_fl.lock().unwrap(), fl)?;
            self.publish_port_at(&self.state.output_fr.lock().unwrap(), fr)?;
        } else if let Some(mono) = &labeled_ports.mono {
            info!("Publishing at '{} [MONO]'", sink_name);
            self.publish_port_at(&self.state.output_fl.lock().unwrap(), mono)?;
            self.publish_port_at(&self.state.output_fl.lock().unwrap(), mono)?;
        } else {
            return UnexpectedPortFormatSnafu {
                client_name: sink_name.to_string(),
                labeled_ports,
            }
            .fail();
        }

        Ok(())
    }

    /// Ends any outgoing audio streams.
    pub fn stop_publishing(&mut self) -> Result<(), Error> {
        let output_fl = self.state.output_fl.lock().unwrap();
        for port in output_fl.get_connections() {
            self.stop_publishing_port_at(&output_fl, &port)?;
        }

        let output_fr = self.state.output_fr.lock().unwrap();
        for port in output_fr.get_connections() {
            self.stop_publishing_port_at(&output_fr, &port)?;
        }

        Ok(())
    }

    // Parses a list of ports into a set of client names.
    //
    // Assumes port names are formatted as "client:port".
    fn get_clients_from_ports(&self, ports: Vec<String>) -> HashSet<String> {
        let mut devices: HashSet<String> = HashSet::new();
        for port in ports {
            if let Some(device) = port.split(":").next() {
                devices.insert(device.to_string());
            }
        }

        devices
    }

    // Connects a destination jack port to another by name.
    fn subscribe_port_to<T: jack::PortSpec>(
        &self,
        dest_port: &jack::Port<T>,
        source_port_name: &str,
    ) -> Result<(), Error> {
        let source_port = self
            .jack_session
            .as_client()
            .port_by_name(source_port_name)
            .context(PortNotFoundSnafu {
                name: source_port_name,
            })?;
        self.connect_ports(&source_port, dest_port)
    }

    // Connects a source jack port to another by name.
    fn publish_port_at(
        &self,
        source_port: &jack::Port<jack::AudioOut>,
        dest_port_name: &str,
    ) -> Result<(), Error> {
        let dest_port = self
            .jack_session
            .as_client()
            .port_by_name(dest_port_name)
            .context(PortNotFoundSnafu {
                name: dest_port_name,
            })?;
        self.connect_ports(source_port, &dest_port)
    }

    // Wrapper for `jack::Client::connect_ports` with additional error handling.
    fn connect_ports<A: jack::PortSpec, B: jack::PortSpec>(
        &self,
        source_port: &jack::Port<A>,
        dest_port: &jack::Port<B>,
    ) -> Result<(), Error> {
        let dest_port_name = dest_port.name().context(JackSnafu {
            msg: "Failed to get port name",
        })?;
        let already_connected = source_port
            .is_connected_to(dest_port_name.as_str())
            .context(JackSnafu {
                msg: "Failed to check connection status",
            })?;
        if already_connected {
            warn!("Port already connected to '{}'", dest_port_name);
            return Ok(());
        }

        let jack_client = self.jack_session.as_client();
        jack_client
            .connect_ports(source_port, dest_port)
            .context(JackSnafu {
                msg: "Failed to connnect to port",
            })?;

        Ok(())
    }

    // Disconnects a destination jack port from another by name.
    fn unsubscribe_port_from<T: jack::PortSpec>(
        &self,
        dest_port: &jack::Port<T>,
        source_port_name: &str,
    ) -> Result<(), Error> {
        let source_port = self
            .jack_session
            .as_client()
            .port_by_name(source_port_name)
            .context(PortNotFoundSnafu {
                name: source_port_name,
            })?;
        self.disconnect_ports(&source_port, dest_port)
    }

    // Disconnects a source jack port from another by name.
    fn stop_publishing_port_at(
        &self,
        source_port: &jack::Port<jack::AudioOut>,
        dest_port_name: &str,
    ) -> Result<(), Error> {
        let dest_port = self
            .jack_session
            .as_client()
            .port_by_name(dest_port_name)
            .context(PortNotFoundSnafu {
                name: dest_port_name,
            })?;
        self.disconnect_ports(source_port, &dest_port)
    }

    // Wrapper for `jack::Client::disconnect_ports` with additional error handling.
    fn disconnect_ports<A: jack::PortSpec, B: jack::PortSpec>(
        &self,
        source_port: &jack::Port<A>,
        dest_port: &jack::Port<B>,
    ) -> Result<(), Error> {
        let dest_port_name = dest_port.name().context(JackSnafu {
            msg: "Failed to get port name",
        })?;
        let still_connected = source_port
            .is_connected_to(dest_port_name.as_str())
            .context(JackSnafu {
                msg: "Failed to check connection status",
            })?;
        if !still_connected {
            warn!("Port already disconnected from '{}'", dest_port_name);
            return Ok(());
        }

        let jack_client = self.jack_session.as_client();
        jack_client
            .disconnect_ports(source_port, dest_port)
            .context(JackSnafu {
                msg: "Failed to disconnect port",
            })?;

        Ok(())
    }
}
