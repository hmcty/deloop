use std::collections::BTreeMap;

struct WrappedCounter {
    cnt: u64,
    len: u64,
}

impl Default for WrappedCounter {
    fn default() -> Self {
        WrappedCounter { cnt: 0, len: 0 }
    }
}

/// A collection of counters shared by all tracks.
pub struct GlobalCounter {
    /// Sample rate of the audio engine.
    /// Provides a conversion factor between counter units and time.
    sample_rate: u64,

    /// A collection of counters indexed by unique identifiers.
    counters: BTreeMap<usize, WrappedCounter>,
}

impl GlobalCounter {
    pub fn new(sample_rate: u64) -> Self {
        GlobalCounter {
            sample_rate,
            counters: BTreeMap::new(),
        }
    }

    pub fn sample_rate(&self) -> u64 {
        self.sample_rate
    }

    pub fn register(&mut self, ctr_id: usize) {
        self.counters.insert(ctr_id, WrappedCounter::default());
    }

    pub fn advance(&mut self, ctr_id: usize, amount: u64) {
        self.counters.get_mut(&ctr_id).unwrap().cnt += amount;
    }

    pub fn advance_all(&mut self, amount: u64) {
        for counter in self.counters.values_mut() {
            counter.cnt += amount;
        }
    }

    pub fn reset_to(&mut self, ctr_id: usize, amount: u64) {
        self.counters.get_mut(&ctr_id).unwrap().cnt = amount;
    }

    pub fn set_len(&mut self, ctr_id: usize, len: u64) {
        self.counters.get_mut(&ctr_id).unwrap().len = len;
    }

    pub fn get_len(&self, ctr_id: usize) -> u64 {
        self.counters.get(&ctr_id).unwrap().len
    }

    pub fn absolute(&self, ctr_id: usize) -> u64 {
        self.counters.get(&ctr_id).unwrap().cnt
    }

    pub fn relative(&self, ctr_id: usize) -> u64 {
        let counter = self.counters.get(&ctr_id).unwrap();
        if counter.len == 0 {
            return 0;
        }

        counter.cnt % counter.len
    }

    pub fn get_next_loop(&mut self, ctr_id: usize) -> u64 {
        let counter = self.counters.get_mut(&ctr_id).unwrap();
        if counter.len == 0 {
            return 0;
        }

        (counter.len - (counter.cnt % counter.len)) + counter.cnt + 1
    }
}
