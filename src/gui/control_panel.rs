use eframe::egui::{self, ComboBox, Event};
use log::info;
use std::collections::HashSet;

use crate::deloop;
use crate::util;

use crate::gui::io_selector::SelectedIO;
use crate::gui::track_interface::TrackInterface;

pub struct ControlPanel {
    client: deloop::Client,
    tracks: [TrackInterface; deloop::TrackId::NUM_TRACKS],
    focused_track: deloop::TrackId,
    audio_sources: HashSet<String>,
    audio_sinks: HashSet<String>,
    midi_sources: HashSet<String>,
    selected_io: SelectedIO,
    is_settings_open: bool,
    latency: f32,
}

impl ControlPanel {
    pub fn new(cc: &eframe::CreationContext<'_>) -> Self {
        let mut control_panel = ControlPanel::default();

        let Some(storage) = cc.storage else {
            info!("No storage found");
            return control_panel;
        };

        let Some(json) = storage.get_string("selected_io") else {
            info!("Selected IO not configured in storage");
            return control_panel;
        };

        if let Ok(selected_io) = serde_json::from_str::<SelectedIO>(&json) {
            for audio_source in &selected_io.audio_sources {
                if !control_panel.audio_sources.contains(audio_source.as_str()) {
                    continue;
                }

                control_panel
                    .selected_io
                    .toggle_audio_source(&mut control_panel.client, audio_source.as_str());
            }

            if let Some(audio_sink) = selected_io.audio_sink {
                control_panel
                    .selected_io
                    .select_audio_sink(&mut control_panel.client, Some(audio_sink));
            }

            if let Some(control_source) = selected_io.control_source {
                control_panel
                    .selected_io
                    .select_control_source(&mut control_panel.client, Some(control_source));
            }
        }

        control_panel
    }

    fn default() -> Self {
        let client = deloop::Client::default();
        let tracks = [
            TrackInterface::new(deloop::TrackId::A),
            TrackInterface::new(deloop::TrackId::B),
            TrackInterface::new(deloop::TrackId::C),
            TrackInterface::new(deloop::TrackId::D),
        ];
        let audio_sources = client.audio_sources();
        let audio_sinks = client.audio_sinks();
        let midi_sources = client.midi_sources();

        Self {
            client,
            tracks,
            focused_track: deloop::TrackId::A,
            audio_sources,
            audio_sinks,
            midi_sources,
            selected_io: SelectedIO::default(),
            is_settings_open: false,
            latency: 0.0,
        }
    }
}

impl eframe::App for ControlPanel {
    fn save(&mut self, storage: &mut dyn eframe::Storage) {
        let j = serde_json::to_string(&self.selected_io).unwrap();
        storage.set_string("selected_io", j);
    }

    fn update(&mut self, ctx: &egui::Context, _: &mut eframe::Frame) {
        ctx.request_repaint(); // Run continuously

        // Process track updates
        for track_info in self.client.get_track_updates() {
            match track_info {
                deloop::TrackInfo::FocusedTrackChanged(track_id) => {
                    self.focused_track = track_id;
                }
                deloop::TrackInfo::StatusUpdate(track_id, status) => {
                    self.tracks[track_id as usize].update(status);
                }
                deloop::TrackInfo::CounterUpdate(global_ctr) => {
                    for track in &mut self.tracks {
                        track.update_counter(&global_ctr);
                    }
                }
                deloop::TrackInfo::WaveformUpdate(track_id, status, fl, fr) => {
                    self.tracks[track_id as usize].update_waveform(status, &fl, &fr);
                }
                deloop::TrackInfo::ProcessingLatency(latency) => {
                    self.latency = latency;
                }
            }
        }

        egui::TopBottomPanel::top("Menu Bar")
            .frame(egui::Frame::none().inner_margin(4.0))
            .show(ctx, |ui| {
                if ui.button("Settings").clicked() {
                    self.is_settings_open = !self.is_settings_open;
                }
            });

        egui::SidePanel::left("Settings")
            .resizable(false)
            .show_animated(ctx, self.is_settings_open, |ui| {
                ui.label("Audio Sources");
                for audio_source in &self.audio_sources {
                    let mut selected = self.selected_io.audio_sources.contains(audio_source);
                    if ui.checkbox(&mut selected, audio_source.clone()).clicked() {
                        self.selected_io
                            .toggle_audio_source(&mut self.client, audio_source);
                    }
                }

                ui.separator();
                ComboBox::from_label("Audio Sink")
                    .selected_text(util::truncate_string(
                        &self
                            .selected_io
                            .audio_sink
                            .clone()
                            .unwrap_or("None".to_string()),
                        15,
                    ))
                    .show_ui(ui, |ui| {
                        let mut selected_audio_sink = self.selected_io.audio_sink.clone();
                        if ui
                            .selectable_value(&mut selected_audio_sink, None, "None")
                            .clicked()
                        {
                            self.selected_io.select_audio_sink(&mut self.client, None);
                        }
                        for audio_sink in &self.audio_sinks {
                            let value = ui.selectable_value(
                                &mut selected_audio_sink,
                                Some(audio_sink.clone()),
                                audio_sink,
                            );
                            if value.clicked() {
                                self.selected_io
                                    .select_audio_sink(&mut self.client, Some(audio_sink.clone()));
                            }
                        }
                    });

                ComboBox::from_label("Control Source")
                    .selected_text(util::truncate_string(
                        &self
                            .selected_io
                            .control_source
                            .clone()
                            .unwrap_or("None".to_string()),
                        15,
                    ))
                    .show_ui(ui, |ui| {
                        let mut selected_control_source = self.selected_io.control_source.clone();
                        if ui
                            .selectable_value(&mut selected_control_source, None, "None")
                            .clicked()
                        {
                            self.selected_io
                                .select_control_source(&mut self.client, None);
                        }
                        for midi_source in &self.midi_sources {
                            let value = ui.selectable_value(
                                &mut selected_control_source,
                                Some(midi_source.clone()),
                                midi_source,
                            );
                            if value.clicked() {
                                self.selected_io.select_control_source(
                                    &mut self.client,
                                    Some(midi_source.clone()),
                                );
                            }
                        }
                    });

                if ui.button("Refresh").clicked() {
                    self.audio_sources = self.client.audio_sources();
                    self.audio_sinks = self.client.audio_sinks();
                    self.midi_sources = self.client.midi_sources();
                }

                ui.separator();
                ui.label(format!("Latency: {:.3} ms", self.latency * 1000.0));
            });

        egui::CentralPanel::default().show(ctx, |ui| {
            let spacing = ui.spacing_mut();
            spacing.item_spacing = egui::vec2(0.0, 0.0);

            let available_width =
                (ui.available_width() - 16.0) / deloop::TrackId::NUM_TRACKS as f32;
            ui.with_layout(egui::Layout::left_to_right(egui::Align::TOP), |ui| {
                for track in &mut self.tracks {
                    let is_focused = track.track_id == self.focused_track;
                    if track
                        .show_thumbnail(ui, &mut self.client, available_width, is_focused)
                        .clicked()
                    {
                        let _ = self.client.focus_on_track(track.track_id);
                    }
                }
            });

            self.tracks[self.focused_track as usize].show_controls(ui, &mut self.client);

            ui.input(|i| {
                for event in &i.raw.events {
                    if let Event::Key {
                        key,
                        physical_key: _,
                        pressed,
                        repeat: _,
                        modifiers: _,
                    } = event
                    {
                        if *pressed && *key == egui::Key::Space {
                            _ = util::log_on_err(self.client.advance_track_state());
                        }
                    }
                }
            });
        });
    }
}
