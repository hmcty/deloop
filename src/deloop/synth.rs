// List synth modules

pub trait Synth {
    fn handle_midi_event(&mut self, event: &[u8]);
    fn write_to(&mut self, fl_output: &mut [f32], fr_output: &mut [f32]);
    fn reset(&mut self);
}
