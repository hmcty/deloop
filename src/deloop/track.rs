use super::counter::GlobalCounter;
use super::TrackId;

#[derive(Clone, Copy, Debug, PartialEq)]
pub enum StateType {
    Idle,
    RecordingQueuedOnTick(u64),
    RecordingQueuedOnRisingEdge(f32),
    Recording,
    OverdubbingQueued(u64),
    Overdubbing,
    PlayingQueued(u64),
    Playing,
    Paused,
}

impl StateType {
    pub fn is_recording(&self) -> bool {
        matches!(self, StateType::Recording | StateType::OverdubbingQueued(_))
    }

    pub fn is_being_modified(&self) -> bool {
        self.is_recording() || matches!(self, StateType::Overdubbing)
    }

    pub fn is_stopped(&self) -> bool {
        matches!(
            self,
            StateType::Idle
                | StateType::RecordingQueuedOnTick(_)
                | StateType::RecordingQueuedOnRisingEdge(_)
                | StateType::Paused
        )
    }
}

#[derive(Clone, Copy, PartialEq)]
pub enum SyncTo {
    None,
    Track(TrackId),
    RisingEdge(f32),
}

impl std::fmt::Debug for SyncTo {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match self {
            SyncTo::None => write!(f, "None"),
            SyncTo::Track(track_id) => write!(f, "Track (id={:?})", track_id),
            SyncTo::RisingEdge(_) => write!(f, "Rising edge"),
        }
    }
}

#[derive(Clone, Debug)]
pub struct Settings {
    pub sync: SyncTo,
    pub speed: Option<f32>,
}

#[derive(Clone, Debug)]
pub struct Status {
    pub state: StateType,
    pub buf_index: usize,
    pub buf_size: usize,
    pub ctr: TrackId,
}

pub struct Track {
    id: TrackId,
    settings: Settings,
    sync_ctr_id: TrackId,
    state: StateType,
    read_head: usize,
    write_head: usize,
    last_write_head: usize,
    fl_buffer: Vec<f32>,
    fr_buffer: Vec<f32>,
    status_changed: bool,
}

impl Track {
    pub fn new(id: TrackId) -> Track {
        // Assuming a 48kHz sampling rate, 3125000 elements would be enough
        // to record roughly 65 seconds of audio before allocating more
        // memory. Assuming all tracks use these settings, total base memory
        // allocation would be 4 * 3125000 * 2 * 4 = 100 MB.
        const DEFAULT_BUFFER_SIZE: usize = 3125000;

        Track {
            id,
            settings: Settings {
                sync: SyncTo::None,
                speed: None,
            },
            sync_ctr_id: id,
            state: StateType::Idle,
            read_head: 0,
            write_head: 0,
            last_write_head: 0,
            fl_buffer: Vec::with_capacity(DEFAULT_BUFFER_SIZE),
            fr_buffer: Vec::with_capacity(DEFAULT_BUFFER_SIZE),
            status_changed: false,
        }
    }

    // Unique identifier of the track.
    pub fn id(&self) -> TrackId {
        self.id
    }

    /// Get metadata about the track.
    pub fn get_status(&mut self) -> Option<Status> {
        if !self.status_changed {
            return None;
        }

        self.status_changed = false;
        Some(Status {
            state: self.state,
            buf_index: self.read_head,
            buf_size: self.fl_buffer.len(),
            ctr: self.sync_ctr_id,
        })
    }

    /// Get slices of front-left and front-right buffers.
    pub fn get_raw_buffers(&self) -> (&[f32], &[f32]) {
        (&self.fl_buffer, &self.fr_buffer)
    }

    /// Update the track with the provided settings.
    pub fn configure(&mut self, settings: Settings) {
        log::info!("Configuring state machine: {:?}", settings);
        self.settings = settings;

        // Note: We determine the sync counter here, meaning track
        // track settings should only ever be modified here.
        self.sync_ctr_id = match self.settings.sync {
            SyncTo::Track(track_id) => track_id,
            _ => self.id,
        };
    }

    /// Forward midi events to the track.
    pub fn handle_midi_event(&mut self, global_ctr: &mut GlobalCounter, event: &[u8]) {
        if event.len() != 3 {
            return;
        }

        // CC 64 (sustain pedal) pressed
        // Act on press: https://x.com/ID_AA_Carmack/status/1787850053912064005
        if event[0] == 0xB0 && event[1] == 0x40 {
            // If pressed or released while recording, advance the state
            if event[2] > 0 || self.state == StateType::Recording {
                self.advance_state(global_ctr);
            }
        }
    }

    /// Advance the track's state machine.
    pub fn advance_state(&mut self, global_ctr: &mut GlobalCounter) {
        // Perform state transition
        self.enter_state(match self.state {
            StateType::Idle => {
                // If we're the owner of the counter, request on the next frame
                match self.settings.sync {
                    SyncTo::None => StateType::RecordingQueuedOnTick(global_ctr.absolute(self.id)),
                    SyncTo::Track(track_id) => {
                        StateType::RecordingQueuedOnTick(global_ctr.next_loop(track_id))
                    }
                    SyncTo::RisingEdge(thresh) => StateType::RecordingQueuedOnRisingEdge(thresh),
                }
            }
            StateType::RecordingQueuedOnTick(idx) => StateType::RecordingQueuedOnTick(idx),
            StateType::RecordingQueuedOnRisingEdge(thresh) => {
                StateType::RecordingQueuedOnRisingEdge(thresh)
            }
            StateType::Recording => {
                if self.sync_ctr_id == self.id {
                    StateType::OverdubbingQueued(global_ctr.absolute(self.sync_ctr_id))
                } else {
                    StateType::OverdubbingQueued(global_ctr.next_loop(self.sync_ctr_id))
                }
            }
            StateType::OverdubbingQueued(idx) => StateType::OverdubbingQueued(idx),
            StateType::Overdubbing => StateType::Playing,
            StateType::PlayingQueued(idx) => StateType::PlayingQueued(idx),
            StateType::Playing => StateType::Paused,
            StateType::Paused => {
                if self.sync_ctr_id == self.id {
                    StateType::PlayingQueued(global_ctr.absolute(self.sync_ctr_id))
                } else {
                    StateType::PlayingQueued(global_ctr.next_loop(self.sync_ctr_id))
                }
            }
        });
    }

    /// Sets current state within the FSM.
    pub fn enter_state(&mut self, state: StateType) {
        self.status_changed = true;
        self.state = state;
    }

    /// Handle incoming audio data.
    pub fn read_from(
        &mut self,
        global_ctr: &mut GlobalCounter,
        fl_input: &[f32],
        fr_input: &[f32],
    ) {
        self.last_write_head = self.write_head;

        let ctr_id = match self.settings.sync {
            SyncTo::Track(track_id) => track_id,
            _ => self.id,
        };
        let start = global_ctr.absolute(ctr_id);
        let end = start + fl_input.len() as u64;

        match self.state {
            StateType::RecordingQueuedOnTick(idx) => {
                if end < idx {
                    return;
                }

                let mut record_from = 0;
                if start < idx {
                    record_from = (idx - start) as usize;
                }

                debug_assert!(record_from <= fl_input.len());
                self.enter_state(StateType::Recording);
                self.record(
                    global_ctr,
                    &fl_input[record_from..],
                    &fr_input[record_from..],
                );
                global_ctr.reset_to(self.id, (fl_input.len() - record_from) as u64);
            }
            StateType::RecordingQueuedOnRisingEdge(thresh) => {
                for i in 0..fl_input.len() {
                    if fl_input[i] > thresh || fr_input[i] > thresh {
                        self.enter_state(StateType::Recording);
                        self.record(global_ctr, &fl_input[i..], &fr_input[i..]);
                        global_ctr.reset_to(self.id, (fl_input.len() - i) as u64);
                        break;
                    }
                }
            }
            StateType::Recording => self.record(global_ctr, fl_input, fr_input),
            StateType::OverdubbingQueued(idx) => {
                if end < idx {
                    self.record(global_ctr, fl_input, fr_input);
                } else {
                    let mut overdub_from = 0;
                    if start < idx {
                        overdub_from = (idx - start) as usize;
                    }

                    debug_assert!(overdub_from <= fl_input.len());
                    if overdub_from > 0 {
                        self.record(
                            global_ctr,
                            &fl_input[..overdub_from],
                            &fr_input[..overdub_from],
                        );
                    }

                    self.overdub(&fl_input[overdub_from..], &fr_input[overdub_from..]);
                    self.enter_state(StateType::Overdubbing);
                }
            }
            StateType::Overdubbing => {
                if self.fl_buffer.is_empty() {
                    return;
                }

                self.overdub(fl_input, fr_input);
            }
            _ => (),
        }
    }

    /// Mix track onto the provided output buffers.
    pub fn write_to(
        &mut self,
        global_ctr: &mut GlobalCounter,
        fl_output: &mut [f32],
        fr_output: &mut [f32],
    ) {
        if self.fl_buffer.is_empty() {
            return;
        } else if self.state.is_stopped() || self.state.is_recording() {
            self.read_head = 0;
            return;
        }

        let start = global_ctr.absolute(self.sync_ctr_id);
        let end = start + fl_output.len() as u64;

        let mut play_from: usize = 0;
        if let StateType::PlayingQueued(idx) = self.state {
            if end < idx {
                return;
            }

            play_from = (idx - start) as usize;
            self.enter_state(StateType::Playing);
        }

        debug_assert!(play_from <= fl_output.len());
        for i in play_from..fl_output.len() {
            if self.read_head >= self.fl_buffer.len() {
                if self.state == StateType::Recording {
                    break;
                }

                self.read_head = 0;
            }

            fl_output[i] += self.fl_buffer[self.read_head];
            fr_output[i] += self.fr_buffer[self.read_head];
            self.read_head += 1;
        }

        self.write_head = self.read_head;
    }

    /// Complete reset of track state.
    pub fn clear(&mut self) {
        self.status_changed = true;
        self.state = StateType::Idle;
        self.read_head = 0;
        self.write_head = 0;
        self.fl_buffer.clear();
        self.fr_buffer.clear();
    }

    /// Low-level function to store audio data in track buffers.
    fn record(&mut self, global_ctr: &mut GlobalCounter, fl_input: &[f32], fr_input: &[f32]) {
        self.fl_buffer.extend_from_slice(fl_input);
        self.fr_buffer.extend_from_slice(fr_input);
        self.write_head = self.fl_buffer.len();
        global_ctr.set_len(self.id, self.fl_buffer.len() as u64);
    }

    /// Low-level function to overdub audio data in track buffers.
    fn overdub(&mut self, fl_input: &[f32], fr_input: &[f32]) {
        for i in 0..fl_input.len() {
            if self.write_head >= self.fl_buffer.len() {
                self.write_head = 0;
            }

            self.fl_buffer[self.write_head] += fl_input[i];
            self.fr_buffer[self.write_head] += fr_input[i];
            self.write_head += 1;
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn assert_defaults(track: &Track) {
        assert_eq!(track.state, StateType::Idle);
        assert_eq!(track.read_head, 0);
        assert_eq!(track.write_head, 0);
        assert_eq!(track.fl_buffer.len(), 0);
        assert_eq!(track.fr_buffer.len(), 0);
    }

    #[test]
    fn test_clear() {
        let mut global_ctr = GlobalCounter::new(48000);
        let mut track = Track::new(TrackId::A);
        track.enter_state(StateType::Recording);
        let input_buffer = (0..1024).map(|x| x as f32).collect::<Vec<f32>>();
        track.read_from(&mut global_ctr, &input_buffer, &input_buffer);
        track.clear();
        assert_defaults(&track);
    }

    #[test]
    fn test_track_synced_recording() {
        let mut global_ctr = GlobalCounter::new(48000);
        let mut track = Track::new(TrackId::A);
        assert_defaults(&track);

        // Configure track A to sync with track B (with 1024 frame length)
        track.configure(Settings {
            sync: SyncTo::Track(TrackId::B),
            speed: None,
        });
        global_ctr.set_len(TrackId::B, 1024);
        global_ctr.advance_all(512);

        // Queue recording to occur on next track B loop
        track.advance_state(&mut global_ctr);
        assert_eq!(track.state, StateType::RecordingQueuedOnTick(1024));

        // Read from input buffer (only 512 samples should be captured)
        let input_buffer = (0..1024).map(|x| x as f32).collect::<Vec<f32>>();
        track.read_from(&mut global_ctr, &input_buffer, &input_buffer);
        global_ctr.advance_all(1024);
        assert_eq!(track.state, StateType::Recording);
        assert_eq!(track.read_head, 0);
        assert_eq!(track.write_head, 512);
        assert_eq!(track.fl_buffer.len(), 512);
        assert_eq!(track.fr_buffer.len(), 512);

        // Queue overdubbing to begin on next loop
        track.advance_state(&mut global_ctr);
        assert_eq!(track.state, StateType::OverdubbingQueued(2048));

        // Read from input buffer (only 512 samples should be captured)
        track.read_from(&mut global_ctr, &input_buffer, &input_buffer);
        global_ctr.advance_all(1024);
        assert_eq!(track.state, StateType::Overdubbing);
        assert_eq!(track.read_head, 0);
        assert_eq!(track.write_head, 512);
        assert_eq!(track.fl_buffer.len(), 1024);
        assert_eq!(track.fr_buffer.len(), 1024);

        // Check audio buffers (split and reversed from input)
        let expected_buffer = (512..1024)
            .map(|x| (2 * x) as f32)
            .chain((0..512).map(|x| x as f32))
            .collect::<Vec<f32>>();
        assert_eq!(track.fl_buffer, expected_buffer);
        assert_eq!(track.fr_buffer, expected_buffer);
    }
}
