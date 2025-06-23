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
