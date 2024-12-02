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

    pub fn absolute(&self, ctr_id: TrackId) -> u64 {
        self.counters[ctr_id as usize].cnt
    }

    pub fn relative(&self, ctr_id: TrackId) -> u64 {
        let counter = self.counters[ctr_id as usize];
        if counter.len == 0 {
            return 0;
        }

        counter.cnt % counter.len
    }

    pub fn get_next_loop(&mut self, ctr_id: TrackId) -> u64 {
        let counter = self.counters[ctr_id as usize];
        if counter.len == 0 {
            return 0;
        }

        (counter.len - (counter.cnt % counter.len)) + counter.cnt + 1
    }
}
