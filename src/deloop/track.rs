use log::info;

pub enum TrackState {
    Idle,
    Recording,
    Overdubbing,
    Playing,
    Paused,
}

pub struct Track {
    state: TrackState,
    read_head: usize,
    write_head: usize,
    fl_buffer: Vec<f32>,
    fr_buffer: Vec<f32>,
}

impl Track {
    pub fn new() -> Track {
        Track {
            state: TrackState::Idle,
            read_head: 0,
            write_head: 0,
            fl_buffer: Vec::new(),
            fr_buffer: Vec::new(),
        }
    }

    pub fn handle_midi_event(&mut self, event: &[u8]) {
        if event.len() != 3 {
            return;
        }

        // CC 64 (sustain pedal) pressed
        // Act on press: https://x.com/ID_AA_Carmack/status/1787850053912064005
        if event[0] == 0xB0 && event[1] == 0x40 && event[2] == 0x7F {
            self.advance_state();
            info!("Event: {:?}", event);
        }
    }

    pub fn advance_state(&mut self) {
        match self.state {
            TrackState::Idle => self.state = TrackState::Recording,
            TrackState::Recording => {
                self.state = TrackState::Overdubbing;
                self.write_head = 0;
            }
            TrackState::Overdubbing => self.state = TrackState::Playing,
            TrackState::Playing => {
                self.state = TrackState::Paused;
                self.read_head = 0;
            }
            TrackState::Paused => self.state = TrackState::Playing,
        }
    }

    pub fn read_from(&mut self, fl_input: &[f32], fr_input: &[f32]) {
        match self.state {
            TrackState::Idle | TrackState::Playing | TrackState::Paused => (),
            TrackState::Recording => {
                self.fl_buffer.extend_from_slice(fl_input);
                self.fr_buffer.extend_from_slice(fr_input);
            }
            TrackState::Overdubbing => {
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
    }

    pub fn write_to(&mut self, fl_output: &mut [f32], fr_output: &mut [f32]) {
        for i in 0..fl_output.len() {
            if self.read_head >= self.fl_buffer.len() {
                self.read_head = 0;
            }
            if self.read_head < self.fl_buffer.len() {
                fl_output[i] += self.fl_buffer[self.read_head];
            }

            if self.read_head >= self.fr_buffer.len() {
                self.read_head = 0;
            }
            if self.read_head < self.fr_buffer.len() {
                fr_output[i] += self.fr_buffer[self.read_head];
            }
            self.read_head += 1;
        }
    }

    #[allow(dead_code)]
    pub fn stop(&mut self) {
        self.state = TrackState::Idle;
        self.read_head = 0;
        self.write_head = 0;
        self.fl_buffer.clear();
        self.fr_buffer.clear();
    }
}
