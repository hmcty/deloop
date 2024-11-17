use std::sync::mpsc::{Receiver, Sender};
use std::time::{Duration, Instant};

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

/// Unique identifiers for each track.
///
/// NOTE: We fix the number of tracks to simplify implementation.
#[repr(usize)]
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum TrackId {
    A = 0,
    B = 1,
    C = 2,
    D = 3,
}

impl TrackId {
    pub const NUM_TRACKS: usize = 4;
    pub const ALL_TRACKS: [Self; Self::NUM_TRACKS] =
        [TrackId::A, TrackId::B, TrackId::C, TrackId::D];
}

// TODO: Add unique indentifiers for commands
pub enum TrackCommand {
    AdvanceTrackState,
    FocusOnTrack(TrackId),
    ConfigureTrack(TrackId, track::Settings),
}

pub enum TrackResponse {
    #[allow(dead_code)]
    CommandFailed,
    CommandSucceeded,
}

pub enum TrackInfo {
    FocusedTrackChanged(TrackId),
    StatusUpdate(TrackId, track::Status),
    CounterUpdate(counter::GlobalCounter),
    WaveformUpdate(TrackId, track::Status, Vec<f32>, Vec<f32>),
    ProcessingLatency(f32),
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

    /// The tracks being managed.
    tracks: [track::Track; TrackId::NUM_TRACKS],

    /// The track in focus, where incoming data is routed.
    focused_track_id: TrackId,

    prev_processing_latency: Duration,
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
            tracks: [
                track::Track::new(TrackId::A),
                track::Track::new(TrackId::B),
                track::Track::new(TrackId::C),
                track::Track::new(TrackId::D),
            ],
            focused_track_id: TrackId::A,
            prev_processing_latency: Duration::from_secs(0),
        }
    }

    pub fn get_ports(&self) -> UnownedPorts {
        self.ports.clone_unowned()
    }
}

impl jack::ProcessHandler for TrackManager {
    /// Carries out a single iteration of the audio processing loop.
    ///
    /// In essence, forwards incoming data to the respective tracks and mixes
    /// their output to a single port.
    fn process(&mut self, _: &jack::Client, ps: &jack::ProcessScope) -> jack::Control {
        let start = Instant::now();

        // Only process one command per iteration.
        match self.command_rx.try_recv() {
            Ok(TrackCommand::AdvanceTrackState) => {
                let track_idx = self.focused_track_id as usize;
                self.tracks[track_idx].advance_state(&mut self.global_ctr);
                self.response_tx
                    .send(TrackResponse::CommandSucceeded)
                    .unwrap();
            }
            Ok(TrackCommand::ConfigureTrack(track_id, settings)) => {
                self.tracks[track_id as usize].configure(settings);
                self.response_tx
                    .send(TrackResponse::CommandSucceeded)
                    .unwrap();
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

        // Process MIDI events first, as they impact control flow.
        let focused_track = &mut self.tracks[self.focused_track_id as usize];
        for midi_event in self.ports.control.iter(ps) {
            focused_track.handle_midi_event(&mut self.global_ctr, midi_event.bytes);
        }

        // Read audio data from stereo input ports.
        focused_track.read_from(
            &mut self.global_ctr,
            self.ports.input_fl.as_slice(ps),
            self.ports.input_fr.as_slice(ps),
        );

        // Clear output buffers.
        let output_fl = self.ports.output_fl.as_mut_slice(ps);
        let output_fr = self.ports.output_fr.as_mut_slice(ps);
        for i in 0..output_fl.len() {
            output_fl[i] = 0.0;
            output_fr[i] = 0.0;
        }

        // Mix all tracks.
        for track in self.tracks.iter_mut() {
            track.write_to(&mut self.global_ctr, output_fl, output_fr);

            // Post a status update.
            // TODO: Post only as needed.
            let status = track.get_status();
            let status_update = if status.state.is_being_modified() {
                let (fl, fr) = track.get_raw_buffers();
                TrackInfo::WaveformUpdate(track.id(), status, fl.to_vec(), fr.to_vec())
            } else {
                TrackInfo::StatusUpdate(track.id(), status)
            };

            self.info_tx.send(status_update).unwrap();
        }

        self.global_ctr.advance_all(ps.n_frames() as u64);
        self.info_tx
            .send(TrackInfo::CounterUpdate(self.global_ctr))
            .unwrap();
        self.info_tx
            .send(TrackInfo::ProcessingLatency(
                self.prev_processing_latency.as_secs_f32(),
            ))
            .unwrap();

        self.prev_processing_latency = start.elapsed();
        jack::Control::Continue
    }

    // Handles changes in the buffer size.
    fn buffer_size(&mut self, _: &jack::Client, _len: jack::Frames) -> jack::Control {
        jack::Control::Continue
    }
}
