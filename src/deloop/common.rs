#[derive(Debug)]
pub struct LabeledPorts {
    pub fl: Option<String>,
    pub fr: Option<String>,
    pub mono: Option<String>,
}

impl LabeledPorts {
    // Identifies port types by name.
    pub fn from_ports_names(port_names: &Vec<String>) -> Self {
        let mut fl = None;
        let mut fr = None;
        let mut mono = None;
        for port_name in port_names {
            match port_name.as_str() {
                _ if port_name.ends_with("_FL") => fl = Some(port_name.clone()),
                _ if port_name.ends_with("_FR") => fr = Some(port_name.clone()),
                _ if port_name.ends_with("_MONO") => mono = Some(port_name.clone()),
                _ => {}
            }
        }

        LabeledPorts { fl, fr, mono }
    }
}

pub enum RelativeCnt {
    Start,
    End,
}

/// A counter that strictly increases and wraps when it reaches a
/// maximum, configurable value.
#[derive(Debug, Clone, Copy)]
pub struct MonotonicCtr {
    pub cnt: usize,
    pub len: usize,
}

impl Default for MonotonicCtr {
    fn default() -> Self {
        MonotonicCtr { cnt: 0, len: 0 }
    }
}

impl MonotonicCtr {
    pub fn from_rel(&self, rel: RelativeCnt) -> Self {
        match rel {
            RelativeCnt::Start => self.next_start(),
            RelativeCnt::End => self.next_end(),
        }
    }

    pub fn next_start(&self) -> Self {
        if self.len == 0 {
            return MonotonicCtr { cnt: 0, len: 0 };
        }

        MonotonicCtr {
            cnt: (self.len - (self.cnt % self.len)) + self.cnt + 1,
            len: self.len,
        }
    }

    pub fn next_end(&self) -> Self {
        if self.len == 0 {
            return MonotonicCtr { cnt: 0, len: 0 };
        }

        MonotonicCtr {
            cnt: (self.len - (self.cnt % self.len)) + self.cnt,
            len: self.len,
        }
    }

    pub fn set_len(&mut self, len: usize) {
        self.len = len;
    }
}

impl std::ops::Add<usize> for MonotonicCtr {
    type Output = MonotonicCtr;
    fn add(self, rhs: usize) -> Self::Output {
        MonotonicCtr {
            cnt: self.cnt + rhs,
            len: self.len,
        }
    }
}

impl std::ops::AddAssign<usize> for MonotonicCtr {
    fn add_assign(&mut self, rhs: usize) {
        self.cnt += rhs;
    }
}

impl std::ops::Sub for MonotonicCtr {
    type Output = usize;
    fn sub(self, rhs: Self) -> Self::Output {
        if self.cnt < rhs.cnt {
            0
        } else {
            self.cnt - rhs.cnt
        }
    }
}

impl std::cmp::PartialEq for MonotonicCtr {
    fn eq(&self, other: &Self) -> bool {
        self.cnt == other.cnt
    }
}

impl std::cmp::PartialOrd for MonotonicCtr {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cnt.cmp(&other.cnt))
    }

    fn lt(&self, rhs: &Self) -> bool {
        self.cnt < rhs.cnt
    }
}
