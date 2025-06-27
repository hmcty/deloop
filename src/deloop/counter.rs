use super::TrackId;

#[derive(Debug, Clone, Copy, Default)]
struct WrappedCounter {
    cnt: u64,
    len: u64,
}

/// A collection of counters shared by all tracks.
#[derive(Debug, Clone, Copy)]
pub struct GlobalCounter {
    /// Sample rate of the audio engine.
    /// Provides a conversion factor between counter units and time.
    sample_rate: u64,

    /// A collection of counters indexed by unique identifiers.
    counters: [WrappedCounter; TrackId::NUM_TRACKS],
}

impl GlobalCounter {
    pub fn new(sample_rate: u64) -> Self {
        GlobalCounter {
            sample_rate,
            counters: [
                WrappedCounter::default(),
                WrappedCounter::default(),
                WrappedCounter::default(),
                WrappedCounter::default(),
            ],
        }
    }

    pub fn sample_rate(&self) -> u64 {
        self.sample_rate
    }

    pub fn advance(&mut self, ctr_id: TrackId, amount: u64) {
        self.counters[ctr_id as usize].cnt += amount;
    }

    pub fn advance_all(&mut self, amount: u64) {
        for counter in self.counters.iter_mut() {
            counter.cnt += amount;
        }
    }

    pub fn reset_to(&mut self, ctr_id: TrackId, amount: u64) {
        self.counters[ctr_id as usize].cnt = amount;
    }

    pub fn set_len(&mut self, ctr_id: TrackId, len: u64) {
        self.counters[ctr_id as usize].len = len;
    }

    pub fn get_len(&self, ctr_id: TrackId) -> u64 {
        self.counters[ctr_id as usize].len
    }

    /// Returns the current tick of a track.
    pub fn absolute(&self, ctr_id: TrackId) -> u64 {
        self.counters[ctr_id as usize].cnt
    }

    /// Returns the tick modulo the track's loop duration.
    pub fn relative(&self, ctr_id: TrackId) -> u64 {
        let counter = self.counters[ctr_id as usize];
        if counter.len == 0 {
            return 0;
        }

        counter.cnt % counter.len
    }

    /// Returns the next starting tick of a track's loop.
    pub fn next_loop(&self, ctr_id: TrackId) -> u64 {
        let counter = self.counters[ctr_id as usize];
        if counter.len == 0 {
            return 0;
        }

        (counter.len - (counter.cnt % counter.len)) + counter.cnt
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_helpers() {
        let mut global_ctr = GlobalCounter::new(48000);
        assert_eq!(global_ctr.sample_rate, 48000);
        for track_id in TrackId::ALL_TRACKS {
            assert_eq!(global_ctr.absolute(track_id), 0);
        }

        global_ctr.advance_all(100);
        for track_id in TrackId::ALL_TRACKS {
            assert_eq!(global_ctr.absolute(track_id), 100);
        }

        for track_id in TrackId::ALL_TRACKS {
            global_ctr.set_len(track_id, 200);
            assert_eq!(global_ctr.get_len(track_id), 200);
            assert_eq!(global_ctr.relative(track_id), 100);
            assert_eq!(global_ctr.next_loop(track_id), 200);
        }

        global_ctr.advance_all(200);
        for track_id in TrackId::ALL_TRACKS {
            assert_eq!(global_ctr.get_len(track_id), 200);
            assert_eq!(global_ctr.relative(track_id), 100);
            assert_eq!(global_ctr.next_loop(track_id), 400);
        }
    }
}
