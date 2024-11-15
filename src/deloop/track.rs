use super::counter::GlobalCounter;

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

#[derive(Clone, Copy, PartialEq)]
pub enum SyncTo {
    None,
    Track(usize),
}

impl std::fmt::Debug for SyncTo {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match self {
            SyncTo::None => write!(f, "None"),
            SyncTo::Track(track_id) => write!(f, "Track (id={})", track_id),
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
    pub ctr: usize,
}

pub struct Track {
    id: usize,
    settings: Settings,
    sync_ctr_id: usize,
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
    pub fn new(id: usize, global_ctr: &mut GlobalCounter) -> Track {
        global_ctr.register(id);

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

    /// Size of the track's buffer.
    pub fn len(&self) -> usize {
        // Note: Assumes FL and FR buffers are always the same length.
        debug_assert_eq!(self.fl_buffer.len(), self.fr_buffer.len());
        self.fl_buffer.len()
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

    // pub fn fetch_status(&mut self, global_ctr: &GlobalCounter) -> Status {
    //     let metadata = Metadata {
    //         state: self.state,
    //         buf_index: self.read_head,
    //         buf_size: self.fl_buffer.len(),
    //         ctr: global_ctr.absolute(self.id),
    //         relative_ctr: global_ctr.relative(self.id),
    //     };
    //     match self.state {
    //         StateType::Recording | StateType::Overdubbing => {
    //             let mut fl_snippet: Vec<f32> = Vec::new();
    //             let mut fr_snippet: Vec<f32> = Vec::new();
    //             if self.write_head < self.last_write_head {
    //                 fl_snippet.extend_from_slice(
    //                     &self.fl_buffer[self.last_write_head..self.fl_buffer.len()],
    //                 );
    //                 fr_snippet.extend_from_slice(
    //                     &self.fr_buffer[self.last_write_head..self.fr_buffer.len()],
    //                 );
    //                 fl_snippet.extend_from_slice(&self.fl_buffer[0..self.write_head]);
    //                 fr_snippet.extend_from_slice(&self.fr_buffer[0..self.write_head]);
    //             } else {
    //                 fl_snippet
    //                     .extend_from_slice(&self.fl_buffer[self.last_write_head..self.write_head]);
    //                 fr_snippet
    //                     .extend_from_slice(&self.fr_buffer[self.last_write_head..self.write_head]);
    //             }

    //             Status::Waveform(metadata, fl_snippet, fr_snippet)
    //         }
    //         _ => Status::Metadata(metadata),
    //     }
    // }

    pub fn handle_midi_event(&mut self, global_ctr: &mut GlobalCounter, event: &[u8]) {
        if event.len() != 3 {
            return;
        }

        // CC 64 (sustain pedal) pressed
        // Act on press: https://x.com/ID_AA_Carmack/status/1787850053912064005
        if event[0] == 0xB0 && event[1] == 0x40 && event[2] == 0x7F {
            self.advance_state(global_ctr);
        } else if event[0] == 0xB0 && event[1] == 0x40 && event[2] == 0x00 {
            if self.state == StateType::Recording {
                self.advance_state(global_ctr);
            }
        }
    }

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
        let pre_idx = global_ctr.absolute(ctr_id);
        let post_idx = pre_idx + self.len() as u64;

        match self.state {
            StateType::RecordingQueued(idx) => {
                if post_idx < idx {
                    return;
                }

                let mut start_idx = 0;
                if pre_idx < idx {
                    start_idx = (idx - pre_idx) as usize;
                }

                debug_assert!(start_idx <= fl_input.len());
                self.fl_buffer.extend_from_slice(&fl_input[start_idx..]);
                self.fr_buffer.extend_from_slice(&fr_input[start_idx..]);
                self.state = StateType::Recording;
                global_ctr.set_len(self.id, self.fl_buffer.len() as u64);
                global_ctr.reset_to(self.id, (fl_input.len() - start_idx) as u64);
            }
            StateType::Recording => {
                self.fl_buffer.extend_from_slice(fl_input);
                self.fr_buffer.extend_from_slice(fr_input);
                self.write_head = self.fl_buffer.len();
                global_ctr.set_len(self.id, self.fl_buffer.len() as u64);
            }
            StateType::OverdubbingQueued(idx) => {
                if post_idx < idx {
                    self.fl_buffer.extend_from_slice(fl_input);
                    self.fr_buffer.extend_from_slice(fr_input);
                    self.write_head = self.fl_buffer.len();
                    global_ctr.set_len(self.id, self.fl_buffer.len() as u64);
                } else {
                    let mut start_idx = 0;
                    if pre_idx < idx {
                        start_idx = (idx - pre_idx) as usize;
                    }

                    // TODO: Fix this
                    debug_assert!(start_idx <= fl_input.len());
                    self.fl_buffer.extend_from_slice(&fl_input[start_idx..]);
                    self.fr_buffer.extend_from_slice(&fr_input[start_idx..]);
                    self.state = StateType::Overdubbing;
                }
            }
            StateType::Overdubbing => {
                if self.fl_buffer.len() == 0 {
                    return;
                }

                for i in 0..fl_input.len() {
                    if self.write_head >= self.fl_buffer.len() {
                        self.write_head = 0;
                    }

                    self.fl_buffer[self.write_head] += fl_input[i];
                    self.fr_buffer[self.write_head] += fr_input[i];
                    self.write_head += 1;
                }
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
        if self.state == StateType::Idle {
            return;
        } else if self.state == StateType::Paused || self.state == StateType::Recording {
            self.read_head = 0;
            return;
        } else if self.fl_buffer.len() == 0 {
            return;
        }

        let pre_idx = global_ctr.absolute(self.sync_ctr_id);
        let mut start_idx: usize = 0;
        if let StateType::PlayingQueued(idx) = self.state {
            if pre_idx < idx {
                return;
            }

            start_idx = (idx - pre_idx) as usize;
            self.state = StateType::Playing;
        }

        debug_assert!(start_idx <= fl_output.len());
        for i in start_idx..fl_output.len() {
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
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_stop() {
        let gcnt = MonotonicCtr::new(48000);
        let mut track = Track::new(48000);
        track.state = StateType::Recording;
        let input_buffer = (0..1024).map(|x| x as f32).collect::<Vec<f32>>();
        track.read_from(0, &input_buffer, &input_buffer);
        track.stop();

        assert_eq!(track.state, StateType::Idle);
        assert_eq!(track.last_state, StateType::Idle);
        assert_eq!(track.last_state_change.elapsed().as_secs(), 0);
        assert_eq!(track.read_head, 0);
        assert_eq!(track.write_head, 0);
        assert_eq!(track.fl_buffer.len(), 0);
        assert_eq!(track.fr_buffer.len(), 0);
    }

    #[test]
    fn test_bpm_sync() {
        let frame_rate = 48000;
        let bpm = 120.0;
        let frames_per_beat = (frame_rate as f32 * 60.0) / bpm;

        // Set up track to record with BPM sync
        let mut track = Track::new(frame_rate);
        let settings = Settings {
            sync: SyncTo::Bpm(bpm),
            speed: None,
        };
        track.configure(settings);

        // A single beat is unchanged
        let one_beat = vec![0.0; frames_per_beat as usize];
        track.state = StateType::Recording;
        track.read_from(0, &one_beat, &one_beat);
        track.advance_state();

        assert_eq!(track.state, StateType::Overdubbing);
        assert_eq!(track.fl_buffer, one_beat);
        assert_eq!(track.fr_buffer, one_beat);
        track.stop();

        // Round down to nearest beat
        let less_than_one_beat = vec![0.0; frames_per_beat as usize - 1];
        track.state = StateType::Recording;
        track.read_from(0, &less_than_one_beat, &less_than_one_beat);
        track.advance_state();

        assert_eq!(track.state, StateType::Recording);
        assert_eq!(track.fl_buffer, less_than_one_beat);
        assert_eq!(track.fr_buffer, less_than_one_beat);

        track.read_from(0, &less_than_one_beat, &less_than_one_beat);
        assert_eq!(track.state, StateType::Overdubbing);
        assert_eq!(track.fl_buffer, one_beat);
        assert_eq!(track.fr_buffer, one_beat);
        track.stop();

        // Round up to nearest beat
        let more_than_one_beat = vec![0.0; frames_per_beat as usize + 1];
        track.state = StateType::Recording;
        track.read_from(0, &more_than_one_beat, &more_than_one_beat);
        track.advance_state();

        assert_eq!(track.state, StateType::Overdubbing);
        assert_eq!(track.fl_buffer, one_beat);
        assert_eq!(track.fr_buffer, one_beat);
        track.stop();
    }
}
