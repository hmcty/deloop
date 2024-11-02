use super::Synth;

pub struct BasicSynth {
    read_head: usize,
    fl_buffer: Vec<f32>,
    fr_buffer: Vec<f32>,
}

impl BasicSynth {
    pub fn new() -> Self {
        let mut fl_buffer = Vec::new();
        let mut fr_buffer = Vec::new();

        // Generate a sine wave
        for i in 0..44100 {
            fl_buffer.push((i as f32 * 440.0 * 2.0 * std::f32::consts::PI / 44100.0).sin());
            fr_buffer.push((i as f32 * 440.0 * 2.0 * std::f32::consts::PI / 44100.0).sin());
        }

        BasicSynth {
            fl_buffer,
            fr_buffer,
        }
    }
}

impl Synth for BasicSynth {
    fn handle_midi_event(&mut self, event: &[u8]) {
        // Do nothing
    }

    fn write_to(&mut self, fl_output: &mut [f32], fr_output: &mut [f32]) {
        for i in 0..fl_output.len() {
            if self.read_head >= self.fl_buffer.len() {
                self.read_head = 0;
            }

            fl_output[i] = self.fl_buffer[self.read_head];
            fr_output[i] = self.fr_buffer[self.read_head];
            self.read_head += 1;
        }
    }

    fn reset(&mut self) {
        self.fl_buffer.clear();
        self.fr_buffer.clear();
    }
}
