pub mod track;

mod common;
mod counter;
mod track_manager;

use log::{info, warn};
use snafu::prelude::*;
use std::collections::HashSet;
use std::sync::mpsc::{channel, Receiver, Sender};
use std::time::Duration;

pub use counter::GlobalCounter;
use track_manager::{TrackCommand, TrackManager, TrackResponse, UnownedPorts};
pub use track_manager::{TrackId, TrackInfo};

#[derive(Debug, Snafu)]
pub enum Error {
    #[snafu(whatever, display("Encountered internal error: {message}"))]
    InternalError {
        message: String,
        #[snafu(source(from(Box<dyn std::error::Error>, Some)))]
        source: Option<Box<dyn std::error::Error>>,
    },

    #[snafu(display("{msg}: {source}"))]
    JackError { source: jack::Error, msg: String },

    #[snafu(display("Failed to find track: '{track_id}'"))]
    TrackNotFound { track_id: usize },

    #[snafu(display("Failed to find port by name '{name}'"))]
    PortNotFound { name: String },

    #[snafu(display("No ports found for source '{source_name}'"))]
    SourceNotFound { source_name: String },

    #[snafu(display("No ports found for sink '{sink_name}'"))]
    SinkNotFound { sink_name: String },

    #[snafu(display("Ports don't match expected format on '{client_name}'"))]
    UnexpectedPortFormat {
        client_name: String,
        labeled_ports: common::LabeledPorts,
    },
}

pub fn send_advance_track(
    command_tx: &Sender<TrackCommand>,
    track_id: TrackId,
) -> Result<(), Error> {
    command_tx
        .send(TrackCommand::Advance(track_id))
        .with_whatever_context(|e| e.to_string())?;
    Ok(())
}

pub fn send_clear_track(command_tx: &Sender<TrackCommand>, track_id: TrackId) -> Result<(), Error> {
    command_tx
        .send(TrackCommand::Clear(track_id))
        .with_whatever_context(|e| e.to_string())?;
    Ok(())
}

struct Notifications;

impl jack::NotificationHandler for Notifications {
    fn thread_init(&self, _: &jack::Client) {
        println!("JACK: thread init");
    }

    /// Not much we can do here, see https://man7.org/linux/man-pages/man7/signal-safety.7.html.
    unsafe fn shutdown(&mut self, _: jack::ClientStatus, _: &str) {}

    fn freewheel(&mut self, _: &jack::Client, is_enabled: bool) {
        println!(
            "JACK: freewheel mode is {}",
            if is_enabled { "on" } else { "off" }
        );
    }

    fn sample_rate(&mut self, _: &jack::Client, srate: jack::Frames) -> jack::Control {
        println!("JACK: sample rate changed to {srate}");
        jack::Control::Continue
    }

    fn client_registration(&mut self, _: &jack::Client, name: &str, is_reg: bool) {
        println!(
            "JACK: {} client with name \"{}\"",
            if is_reg { "registered" } else { "unregistered" },
            name
        );
    }

    fn port_registration(&mut self, _: &jack::Client, port_id: jack::PortId, is_reg: bool) {
        println!(
            "JACK: {} port with id {}",
            if is_reg { "registered" } else { "unregistered" },
            port_id
        );
    }

    fn port_rename(
        &mut self,
        _: &jack::Client,
        port_id: jack::PortId,
        old_name: &str,
        new_name: &str,
    ) -> jack::Control {
        println!("JACK: port with id {port_id} renamed from {old_name} to {new_name}",);
        jack::Control::Continue
    }

    fn ports_connected(
        &mut self,
        _: &jack::Client,
        port_id_a: jack::PortId,
        port_id_b: jack::PortId,
        are_connected: bool,
    ) {
        println!(
            "JACK: ports with id {} and {} are {}",
            port_id_a,
            port_id_b,
            if are_connected {
                "connected"
            } else {
                "disconnected"
            }
        );
    }

    fn graph_reorder(&mut self, _: &jack::Client) -> jack::Control {
        println!("JACK: graph reordered");
        jack::Control::Continue
    }

    fn xrun(&mut self, _: &jack::Client) -> jack::Control {
        println!("JACK: xrun occurred");
        jack::Control::Continue
    }
}

/// Deloop's Jack client
///
/// Bridges the gap between user commands and audio processing.
pub struct Client {
    jack_session: jack::AsyncClient<Notifications, TrackManager>,
    ports: UnownedPorts,
    command_tx: Sender<TrackCommand>,
    info_rx: Receiver<TrackInfo>,
    response_rx: Receiver<TrackResponse>,
}

impl Default for Client {
    /// Creates a new Deloop client.
    ///
    /// Opens a connection to the jack server and begins processing audio.
    fn default() -> Self {
        Self::new("deloop", jack::ClientOptions::default())
    }
}

impl Client {
    pub fn new(name: &str, options: jack::ClientOptions) -> Self {
        let (jack_client, _status) = jack::Client::new(name, options).unwrap();

        let (command_tx, command_rx) = channel::<TrackCommand>();
        let (info_tx, info_rx) = channel::<TrackInfo>();
        let (response_tx, response_rx) = channel::<TrackResponse>();

        let manager = TrackManager::new(&jack_client, command_rx, info_tx, response_tx);
        let ports = manager.get_ports();
        let jack_session = jack_client.activate_async(Notifications, manager).unwrap();

        Client {
            jack_session,
            ports,
            command_tx,
            info_rx,
            response_rx,
        }
    }

    /// Fetches any available track updates.
    pub fn get_track_updates(&self) -> Vec<TrackInfo> {
        let mut updates = Vec::new();
        while let Ok(update) = self.info_rx.try_recv() {
            updates.push(update);
        }

        updates
    }

    /// Returns a reference to the command sender.
    pub fn command_sender(&self) -> &Sender<TrackCommand> {
        &self.command_tx
    }

    /// Advances state of the specified track.
    pub fn advance_track(&self, track_id: TrackId) -> Result<(), Error> {
        send_advance_track(&self.command_tx, track_id)?;
        let response = self
            .response_rx
            .recv_timeout(Duration::from_secs(5))
            .with_whatever_context(|e| e.to_string())?;
        match response {
            TrackResponse::CommandFailed => Err(Error::InternalError {
                message: "Failed to advance track state".to_string(),
                source: None,
            }),
            TrackResponse::CommandSucceeded => Ok(()),
        }
    }

    /// Enable overdubbing on track.
    pub fn enable_overdub_on_track(&self, track_id: TrackId, enable: bool) -> Result<(), Error> {
        self.command_tx
            .send(TrackCommand::EnableOverdub(track_id, enable))
            .with_whatever_context(|e| e.to_string())?;
        let response = self
            .response_rx
            .recv_timeout(Duration::from_secs(5))
            .with_whatever_context(|e| e.to_string())?;
        match response {
            TrackResponse::CommandFailed => Err(Error::InternalError {
                message: "Failed to set overdub on track".to_string(),
                source: None,
            }),
            TrackResponse::CommandSucceeded => Ok(()),
        }
    }

    pub fn set_track_volume(&self, track_id: TrackId, volume: f32) -> Result<(), Error> {
        self.command_tx
            .send(TrackCommand::SetVolume(track_id, volume))
            .with_whatever_context(|e| e.to_string())?;
        let response = self
            .response_rx
            .recv_timeout(Duration::from_secs(5))
            .with_whatever_context(|e| e.to_string())?;
        match response {
            TrackResponse::CommandFailed => Err(Error::InternalError {
                message: "Failed to set track volume".to_string(),
                source: None,
            }),
            TrackResponse::CommandSucceeded => Ok(()),
        }
    }

    /// Stops any playback on track.
    pub fn pause_track(&self, track_id: TrackId) -> Result<(), Error> {
        self.command_tx
            .send(TrackCommand::Pause(track_id))
            .with_whatever_context(|e| e.to_string())?;
        let response = self
            .response_rx
            .recv_timeout(Duration::from_secs(5))
            .with_whatever_context(|e| e.to_string())?;
        match response {
            TrackResponse::CommandFailed => Err(Error::InternalError {
                message: "Failed to pause track".to_string(),
                source: None,
            }),
            TrackResponse::CommandSucceeded => Ok(()),
        }
    }

    /// Stops track and clears any existing state.
    pub fn clear_track(&self, track_id: TrackId) -> Result<(), Error> {
        self.command_tx
            .send(TrackCommand::Clear(track_id))
            .with_whatever_context(|e| e.to_string())?;
        let response = self
            .response_rx
            .recv_timeout(Duration::from_secs(5))
            .with_whatever_context(|e| e.to_string())?;
        match response {
            TrackResponse::CommandFailed => Err(Error::InternalError {
                message: "Failed to clear track".to_string(),
                source: None,
            }),
            TrackResponse::CommandSucceeded => Ok(()),
        }
    }

    /// Configures speed of a track.
    pub fn configure_track(
        &self,
        track_id: TrackId,
        settings: track::Settings,
    ) -> Result<(), Error> {
        self.command_tx
            .send(TrackCommand::Configure(track_id, settings))
            .with_whatever_context(|e| e.to_string())?;
        let response = self
            .response_rx
            .recv_timeout(Duration::from_secs(5))
            .with_whatever_context(|e| e.to_string())?;
        match response {
            TrackResponse::CommandFailed => Err(Error::InternalError {
                message: "Failed to configure track".to_string(),
                source: None,
            }),
            TrackResponse::CommandSucceeded => Ok(()),
        }
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
            jack::PortFlags::IS_INPUT | jack::PortFlags::IS_PHYSICAL,
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
        let port_names = jack_client.ports(
            Some(regex::escape(source_name).as_str()),
            None,
            jack::PortFlags::IS_OUTPUT,
        );
        ensure!(!port_names.is_empty(), SourceNotFoundSnafu { source_name });

        // If both FL and FR ports are available, use them.
        // TODO(hmcty): Handle failure mode where only one port connects.
        let labeled_ports = common::LabeledPorts::from_ports_names(&port_names);
        if let (Some(fl), Some(fr)) = (&labeled_ports.fl, &labeled_ports.fr) {
            info!("Subscribing to '{} [STEREO]'", source_name);
            self.subscribe_port_to(&self.ports.input_fl, fl)?;
            self.subscribe_port_to(&self.ports.input_fr, fr)?;
        } else if let Some(mono) = &labeled_ports.mono {
            info!("Subscribing to '{} [MONO]'", source_name);
            self.subscribe_port_to(&self.ports.input_fl, mono)?;
            self.subscribe_port_to(&self.ports.input_fr, mono)?;
        } else if port_names.len() == 1 && self.midi_sources().contains(source_name) {
            info!("Subscribing to '{} [MIDI]'", source_name);
            self.subscribe_port_to(&self.ports.control, &port_names[0])?;
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
        for port in self.ports.input_fl.get_connections() {
            if port.starts_with(source_name) {
                self.unsubscribe_port_from(&self.ports.input_fl, &port)?;
            }
        }

        for port in self.ports.input_fr.get_connections() {
            if port.starts_with(source_name) {
                self.unsubscribe_port_from(&self.ports.input_fr, &port)?;
            }
        }

        for port in self.ports.control.get_connections() {
            if port.contains(source_name) {
                self.unsubscribe_port_from(&self.ports.control, &port)?;
            }
        }

        Ok(())
    }

    /// Creates an outgoing audio stream to the specified sink.
    pub fn publish_to(&mut self, sink_name: &str) -> Result<(), Error> {
        let jack_client = self.jack_session.as_client();
        let port_names = jack_client.ports(
            Some(regex::escape(sink_name).as_str()),
            Some(jack::jack_sys::FLOAT_MONO_AUDIO),
            jack::PortFlags::IS_INPUT,
        );
        ensure!(!port_names.is_empty(), SinkNotFoundSnafu { sink_name });

        // If both FL and FR ports are available, use them.
        let labeled_ports = common::LabeledPorts::from_ports_names(&port_names);
        if let (Some(fl), Some(fr)) = (&labeled_ports.fl, &labeled_ports.fr) {
            info!("Publishing at '{} [STEREO]'", sink_name);
            self.publish_port_at(&self.ports.output_fl, fl)?;
            self.publish_port_at(&self.ports.output_fr, fr)?;
        } else if let Some(mono) = &labeled_ports.mono {
            info!("Publishing at '{} [MONO]'", sink_name);
            self.publish_port_at(&self.ports.output_fl, mono)?;
            self.publish_port_at(&self.ports.output_fr, mono)?;
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
        for port in self.ports.output_fl.get_connections() {
            self.stop_publishing_port_at(&self.ports.output_fl, &port)?;
        }

        for port in self.ports.output_fr.get_connections() {
            self.stop_publishing_port_at(&self.ports.output_fr, &port)?;
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
    fn publish_port_at<T: jack::PortSpec>(
        &self,
        source_port: &jack::Port<T>,
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
    fn stop_publishing_port_at<T: jack::PortSpec>(
        &self,
        source_port: &jack::Port<T>,
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
        let jack_client = self.jack_session.as_client();
        jack_client
            .disconnect_ports(source_port, dest_port)
            .context(JackSnafu {
                msg: "Failed to disconnect port",
            })?;

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

        Ok(())
    }
}
