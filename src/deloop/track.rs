use super::counter::GlobalCounter;
use super::TrackId;

#[derive(Clone, Copy, Debug, PartialEq)]
pub enum StateType {
    Idle,
    RecordingQueued(u64),
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

    pub fn is_stopped(&self) -> bool {
        matches!(
            self,
            StateType::Idle
                | StateType::RecordingQueued(_)
                | StateType::Paused
                | StateType::PlayingQueued(_)
        )
    }
}

#[derive(Clone, Copy, PartialEq)]
pub enum SyncTo {
    None,
    Track(TrackId),
}

impl std::fmt::Debug for SyncTo {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match self {
            SyncTo::None => write!(f, "None"),
            SyncTo::Track(track_id) => write!(f, "Track (id={:?})", track_id),
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
    last_state: StateType,
    last_state_change: std::time::Instant,
    read_head: usize,
    write_head: usize,
    last_write_head: usize,
    fl_buffer: Vec<f32>,
    fr_buffer: Vec<f32>,
}

impl Track {
    pub fn new(id: TrackId) -> Track {
        Track {
            id,
            settings: Settings {
                sync: SyncTo::None,
                speed: None,
            },
            sync_ctr_id: id,
            state: StateType::Idle,
            last_state: StateType::Idle,
            last_state_change: std::time::Instant::now(),
            read_head: 0,
            write_head: 0,
            last_write_head: 0,
            fl_buffer: Vec::new(),
            fr_buffer: Vec::new(),
        }
    }

    pub fn id(&self) -> TrackId {
        self.id
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

    /// Get metadata about the track.
    pub fn get_status(&self) -> Status {
        Status {
            state: self.state,
            buf_index: self.read_head,
            buf_size: self.fl_buffer.len(),
            ctr: self.sync_ctr_id,
        }
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
        const DOUBLE_CLICK_DURATION: std::time::Duration = std::time::Duration::from_millis(500);
        let double_click_detected = self.last_state_change.elapsed() < DOUBLE_CLICK_DURATION;

        // Allow short recordings, otherwise clear the track on double click
        if double_click_detected && self.state != StateType::Recording {
            self.clear();
            return;
        }

        self.last_state = self.state;
        self.last_state_change = std::time::Instant::now();

        // Select the counter to sync with

        // Perform state transition
        self.state = match self.state {
            StateType::Idle => {
                // If we're the owner of the counter, request on the next frame
                if self.sync_ctr_id == self.id {
                    StateType::RecordingQueued(global_ctr.absolute(self.sync_ctr_id))
                } else {
                    StateType::RecordingQueued(global_ctr.get_next_loop(self.sync_ctr_id))
                }
            }
            StateType::RecordingQueued(idx) => StateType::RecordingQueued(idx),
            StateType::Recording => {
                // If we're the owner of the counter, request on the next frame
                if self.sync_ctr_id == self.id {
                    StateType::OverdubbingQueued(global_ctr.absolute(self.sync_ctr_id))
                } else {
                    StateType::OverdubbingQueued(global_ctr.get_next_loop(self.sync_ctr_id))
                }
            }
            StateType::OverdubbingQueued(idx) => StateType::OverdubbingQueued(idx),
            StateType::Overdubbing => StateType::Playing,
            StateType::PlayingQueued(idx) => StateType::PlayingQueued(idx),
            StateType::Playing => StateType::Paused,
            StateType::Paused => {
                StateType::PlayingQueued(global_ctr.get_next_loop(self.sync_ctr_id))
            }
        };
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
            StateType::RecordingQueued(idx) => {
                if end < idx {
                    return;
                }

                let mut record_from = 0;
                if start < idx {
                    record_from = (idx - start) as usize;
                }

                debug_assert!(record_from <= fl_input.len());
                self.fl_buffer.extend_from_slice(&fl_input[record_from..]);
                self.fr_buffer.extend_from_slice(&fr_input[record_from..]);
                self.state = StateType::Recording;
                global_ctr.set_len(self.id, self.fl_buffer.len() as u64);

                // Not full accurate if record_from > 0
                global_ctr.reset_to(self.id, 0);
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
                    self.state = StateType::Overdubbing;
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
            self.state = StateType::Playing;
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
        self.state = StateType::Idle;
        self.last_state = StateType::Idle;
        self.last_state_change = std::time::Instant::now();
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

    #[test]
    fn test_stop() {
        let mut global_ctr = GlobalCounter::new(48000);
        let mut track = Track::new(TrackId::A);
        track.state = StateType::Recording;
        let input_buffer = (0..1024).map(|x| x as f32).collect::<Vec<f32>>();
        track.read_from(&mut global_ctr, &input_buffer, &input_buffer);
        track.clear();

        assert_eq!(track.state, StateType::Idle);
        assert_eq!(track.last_state, StateType::Idle);
        assert_eq!(track.last_state_change.elapsed().as_secs(), 0);
        assert_eq!(track.read_head, 0);
        assert_eq!(track.write_head, 0);
        assert_eq!(track.fl_buffer.len(), 0);
        assert_eq!(track.fr_buffer.len(), 0);
    }
}
