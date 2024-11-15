use std::collections::BTreeMap;
use std::sync::mpsc::{Receiver, Sender};

use super::common;
use super::counter;
use super::track;

use crate::deloop::JackSnafu;
use snafu::prelude::*;

pub struct Ports<I, O, C> {
    pub input_fl: jack::Port<I>,
    pub input_fr: jack::Port<I>,
    pub output_fl: jack::Port<O>,
    pub output_fr: jack::Port<O>,
    pub control: jack::Port<C>,
}
pub type UnownedPorts = Ports<jack::Unowned, jack::Unowned, jack::Unowned>;

impl Ports<jack::AudioIn, jack::AudioOut, jack::MidiIn> {
    pub fn new(client: &jack::Client) -> Self {
        Ports {
            input_fl: client
                .register_port("input_FL", jack::AudioIn::default())
                .context(JackSnafu {
                    msg: "Failed to register input_FL port",
                })
                .unwrap(),
            input_fr: client
                .register_port("input_FR", jack::AudioIn::default())
                .context(JackSnafu {
                    msg: "Failed to register input_FR port",
                })
                .unwrap(),
            output_fl: client
                .register_port("output_FL", jack::AudioOut::default())
                .context(JackSnafu {
                    msg: "Failed to register output_FL port",
                })
                .unwrap(),
            output_fr: client
                .register_port("output_FR", jack::AudioOut::default())
                .context(JackSnafu {
                    msg: "Failed to register output_FR port",
                })
                .unwrap(),
            control: client
                .register_port("control", jack::MidiIn::default())
                .context(JackSnafu {
                    msg: "Failed to register control port",
                })
                .unwrap(),
        }
    }

    pub fn clone_unowned(&self) -> UnownedPorts {
        Ports {
            input_fl: self.input_fl.clone_unowned(),
            input_fr: self.input_fr.clone_unowned(),
            output_fl: self.output_fl.clone_unowned(),
            output_fr: self.output_fr.clone_unowned(),
            control: self.control.clone_unowned(),
        }
    }
}

// TODO: Add unique indentifiers for commands
pub enum TrackCommand {
    AddTrack(track::Settings),
    AdvanceTrackState,
    FocusOnTrack(usize),
    ConfigureTrack(usize, track::Settings),
    // RemoveTrack(usize),
}

pub enum TrackResponse {
    CommandFailed,
    CommandSucceeded,
    TrackAdded(usize),
    // TrackRemoved(usize),
}

pub enum TrackInfo {
    FocusedTrackChanged(usize),
    StatusUpdate(usize, track::Status),
}

/// Manages the creation, configuration, and processing of tracks.
pub struct TrackManager {
    /// Audio & MIDI I/O ports shared by all tracks.
    ports: Ports<jack::AudioIn, jack::AudioOut, jack::MidiIn>,

    /// Incoming commands from the client.
    command_rx: Receiver<TrackCommand>,

    /// Outgoing, ad hoc track info (e.g. track progress, etc.)
    info_tx: Sender<TrackInfo>,

    /// Outgoing responses to client commands.
    response_tx: Sender<TrackResponse>,

    /// A frame index to synchronize all tracks.
    global_ctr: counter::GlobalCounter,

    /// Tracks indexed by unique identifiers.
    tracks: BTreeMap<usize, track::Track>,

    /// The track in focus, where incoming data is routed.
    focused_track_id: usize,
}

impl TrackManager {
    // Setup handler from an existing Jack client.
    pub fn new(
        client: &jack::Client,
        command_rx: Receiver<TrackCommand>,
        info_tx: Sender<TrackInfo>,
        response_tx: Sender<TrackResponse>,
    ) -> Self {
        let sample_rate = client.sample_rate() as u64;
        TrackManager {
            ports: Ports::new(client),
            command_rx,
            info_tx,
            response_tx,
            global_ctr: counter::GlobalCounter::new(sample_rate),
            tracks: BTreeMap::new(),
            focused_track_id: 0,
        }
    }

    pub fn get_ports(&self) -> UnownedPorts {
        self.ports.clone_unowned()
    }

    pub fn add_track(&mut self, client: &jack::Client, settings: track::Settings) -> TrackResponse {
        let track_id = self.tracks.len();
        let mut track = track::Track::new(track_id, &mut self.global_ctr);
        track.configure(settings);
        self.tracks.insert(track_id, track);
        TrackResponse::TrackAdded(track_id)
    }
}

impl jack::ProcessHandler for TrackManager {
    /// Carries out a single iteration of the audio processing loop.
    ///
    /// In essence, forwards incoming data to the respective tracks and mixes
    /// their output to a single port.
    fn process(&mut self, client: &jack::Client, ps: &jack::ProcessScope) -> jack::Control {
        // Only process one command per iteration.
        match self.command_rx.try_recv() {
            Ok(TrackCommand::AddTrack(settings)) => {
                let resp = self.add_track(client, settings);
                self.response_tx.send(resp).unwrap();
            }
            Ok(TrackCommand::AdvanceTrackState) => {
                self.tracks
                    .get_mut(&self.focused_track_id)
                    .map(|focused_track| {
                        focused_track.advance_state(&mut self.global_ctr);
                        self.response_tx
                            .send(TrackResponse::CommandSucceeded)
                            .unwrap();
                    });
            }
            Ok(TrackCommand::ConfigureTrack(track_id, settings)) => {
                self.tracks.get_mut(&track_id).map(|track| {
                    track.configure(settings);
                    self.response_tx
                        .send(TrackResponse::CommandSucceeded)
                        .unwrap();
                });
            }
            Ok(TrackCommand::FocusOnTrack(track_id)) => {
                self.focused_track_id = track_id;
                self.response_tx
                    .send(TrackResponse::CommandSucceeded)
                    .unwrap();
                self.info_tx
                    .send(TrackInfo::FocusedTrackChanged(track_id))
                    .unwrap();
            }
            _ => {}
        }

        // Forward incoming data to the focused track.
        self.tracks
            .get_mut(&self.focused_track_id)
            .map(|focused_track| {
                // Process MIDI events first, as they impact control flow.
                for midi_event in self.ports.control.iter(ps) {
                    focused_track.handle_midi_event(&mut self.global_ctr, midi_event.bytes);
                }

                // Read audio data from stereo input ports.
                focused_track.read_from(
                    &mut self.global_ctr,
                    self.ports.input_fl.as_slice(ps),
                    self.ports.input_fr.as_slice(ps),
                );
            });

        // Clear output buffers.
        let output_fl = self.ports.output_fl.as_mut_slice(ps);
        let output_fr = self.ports.output_fr.as_mut_slice(ps);
        for i in 0..output_fl.len() {
            output_fl[i] = 0.0;
            output_fr[i] = 0.0;
        }

        // Mix all tracks.
        for (track_id, track) in self.tracks.iter_mut() {
            track.write_to(&mut self.global_ctr, output_fl, output_fr);

            // Post a status update.
            // TODO: Post only as needed.
            self.info_tx
                .send(TrackInfo::StatusUpdate(*track_id, track.get_status()))
                .unwrap();
        }

        self.global_ctr.advance_all(ps.n_frames() as u64);
        jack::Control::Continue
    }

    // Handles changes in the buffer size.
    fn buffer_size(&mut self, _: &jack::Client, _len: jack::Frames) -> jack::Control {
        jack::Control::Continue
    }
}
