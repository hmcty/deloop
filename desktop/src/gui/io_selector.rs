use log::info;
use serde::{Deserialize, Serialize};
use std::collections::HashSet;

use crate::deloop;
use crate::util;

#[derive(Serialize, Deserialize, Default)]
pub struct SelectedIO {
    pub audio_sources: HashSet<String>,
    pub audio_sink: Option<String>,
    pub control_source: Option<String>,
}

impl SelectedIO {
    pub fn toggle_audio_source(&mut self, client: &mut deloop::Client, audio_source: &str) {
        info!(
            "User toggled subscription to audio source: {:?}",
            audio_source
        );
        if self.audio_sources.contains(audio_source) {
            if util::log_on_err(client.unsubscribe_from(audio_source)).is_ok() {
                self.audio_sources.remove(audio_source);
            }
        } else if util::log_on_err(client.subscribe_to(audio_source)).is_ok() {
            self.audio_sources.insert(audio_source.to_string());
        }
    }

    pub fn select_audio_sink(&mut self, client: &mut deloop::Client, audio_sink: Option<String>) {
        if self.audio_sink == audio_sink {
            return;
        }

        info!("User selected audio sink: {:?}", audio_sink);
        if util::log_on_err(client.stop_publishing()).is_ok() {
            self.audio_sink = None;
        } else {
            return;
        }

        if let Some(selected_sink) = audio_sink {
            if util::log_on_err(client.publish_to(selected_sink.as_str())).is_err() {
                self.audio_sink = None;
                let _ = util::log_on_err(client.stop_publishing());
            } else {
                self.audio_sink = Some(selected_sink);
            }
        }
    }

    pub fn select_control_source(
        &mut self,
        client: &mut deloop::Client,
        control_source: Option<String>,
    ) {
        if self.control_source == control_source {
            return;
        }

        info!("User selected control source: {:?}", control_source);
        if let Some(current_control_source) = &self.control_source {
            if util::log_on_err(client.unsubscribe_from(current_control_source)).is_err() {
                return;
            }

            self.control_source = None;
        }

        if let Some(control_source) = control_source {
            if util::log_on_err(client.subscribe_to(control_source.as_str())).is_err() {
                return;
            }

            self.control_source = Some(control_source);
        }
    }
}
