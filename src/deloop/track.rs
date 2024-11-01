#[allow(dead_code)]
pub trait Track {
    fn process(
        &mut self,
        ps: &jack::ProcessScope,
        output_fl: &jack::AudioOut,
        output_fr: &jack::AudioOut,
    );
}

#[allow(dead_code)]
pub struct AudioTrack {
    input_fl: jack::Port<jack::AudioIn>,
    input_fr: jack::Port<jack::AudioIn>,
}

#[allow(dead_code)]
pub struct MidiTrack {
    input: jack::Port<jack::MidiIn>,
}

#[allow(dead_code)]
pub struct WavTrack {
    // TODO: Implement
}

#[allow(dead_code)]
pub enum TrackType {
    Audio(AudioTrack),
    Midi(MidiTrack),
    Wav(WavTrack),
}
