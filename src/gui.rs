use eframe::egui::{self, ComboBox, Event};
use egui::{containers::Frame, emath, epaint, epaint::PathStroke, pos2, vec2, Color32, Pos2, Rect};
use log::{error, info};
use serde::{Deserialize, Serialize};
use std::collections::HashSet;

use crate::deloop;
use crate::util;

pub fn run() -> eframe::Result {
    let options = eframe::NativeOptions::default();
    eframe::run_native(
        "Plot",
        options,
        Box::new(|cc| Ok(Box::new(ControlPanel::new(cc)))),
    )
    .unwrap();
    Ok(())
}

#[allow(dead_code)]
fn create_wave_shape(
    buffer: &[f32],
    to_screen: emath::RectTransform,
    thickness: f32,
    color: Color32,
) -> epaint::Shape {
    let points: Vec<Pos2> = (0..buffer.len())
        .map(|i| {
            let t = i as f32 / (buffer.len() as f32);
            let y = buffer.get(i).unwrap();
            to_screen * pos2(t, *y)
        })
        .collect();

    epaint::Shape::line(points, PathStroke::new(thickness, color))
}

fn log_on_err<T>(result: Result<T, deloop::Error>) -> Result<T, deloop::Error> {
    match result {
        Ok(t) => Ok(t),
        Err(e) => {
            // TODO(hmcty): Use modal dialog
            error!("{}", e);
            Err(e)
        }
    }
}

struct TrackInterface {
    //track: deloop::Track,
    //amp_rbuffer: Vec<f32>,
    //amp_wbuffer: Vec<f32>,
}

impl TrackInterface {
    fn new() -> TrackInterface {
        TrackInterface {}
    }

    fn show(&mut self, ui: &mut egui::Ui) {
        // let is_focused = self.track.is_focused();

        ui.separator();

        egui::Frame::none()
            .fill(
                //if is_focused {
                //    egui::Color32::from_rgb(64, 64, 64)
                // } else {
                egui::Color32::from_rgb(0x2a, 0x2a, 0x2a),
            )
            // })
            .show(ui, |ui| {
                // Write track and id
                //    ui.label(format!("Track {}", self.track.id));
                //    let mut progress: f32 = self.track.get_progress();

                Frame::canvas(ui.style()).show(ui, |ui| {
                    let desired_size = ui.available_width() * vec2(1.0, 0.35);
                    let (_id, rect) = ui.allocate_space(desired_size);

                    let _to_screen = emath::RectTransform::from_to(
                        Rect::from_x_y_ranges(0.0..=1.0, -1.0..=1.0),
                        rect,
                    );

                    // if self.track.is_awaiting() {
                    //     self.amp_rbuffer.clear();
                    //     self.amp_wbuffer.clear();
                    //     return;
                    // }

                    // let mut shapes = vec![];
                    // let ramp = self.track.get_write_amplitude().clamp(-1.0, 1.0);
                    // let wamp = self.track.get_write_amplitude().clamp(-1.0, 1.0);

                    // shapes.push(create_wave_shape(
                    //     &self.amp_rbuffer,
                    //     to_screen,
                    //     2.5,
                    //     Color32::from_rgb(125, 125, 125),
                    // ));
                    // if self.track.is_recording() {
                    //     self.amp_rbuffer.push(wamp);
                    //     progress = 1.0;
                    // } else {
                    //     let idx = (progress * (self.amp_rbuffer.len() as f32)) as usize;
                    //     if let Some(x) = self.amp_rbuffer.get_mut(idx) {
                    //         *x = ramp;
                    //     }
                    // }

                    // let indicator_top = to_screen * pos2(progress, 1.0);
                    // let indicator_bottom = to_screen * pos2(progress, -1.0);

                    // shapes.push(epaint::Shape::line(
                    //     vec![indicator_top, indicator_bottom],
                    //     PathStroke::new(1.0, Color32::from_rgb(125, 10, 10)),
                    // ));

                    // ui.painter().extend(shapes);
                });
            });
    }
}

#[derive(Serialize, Deserialize, Default)]
struct SelectedIO {
    audio_sources: HashSet<String>,
    audio_sink: Option<String>,
    control_source: Option<String>,
}

impl SelectedIO {
    fn toggle_audio_source(&mut self, client: &mut deloop::Client, audio_source: &str) {
        info!(
            "User toggled subscription to audio source: {:?}",
            audio_source
        );
        if self.audio_sources.contains(audio_source) {
            if log_on_err(client.unsubscribe_from(audio_source)).is_ok() {
                self.audio_sources.remove(audio_source);
            }
        } else if log_on_err(client.subscribe_to(audio_source)).is_ok() {
            self.audio_sources.insert(audio_source.to_string());
        }
    }

    fn select_audio_sink(&mut self, client: &mut deloop::Client, audio_sink: Option<String>) {
        if self.audio_sink == audio_sink {
            return;
        }

        info!("User selected audio sink: {:?}", audio_sink);
        if log_on_err(client.stop_publishing()).is_err() {
            self.audio_sink = None;
        } else if let Some(selected_sink) = audio_sink {
            if log_on_err(client.publish_to(selected_sink.as_str())).is_err() {
                self.audio_sink = None;
                let _ = log_on_err(client.stop_publishing());
            } else {
                self.audio_sink = Some(selected_sink);
            }
        }
    }

    fn select_control_source(
        &mut self,
        client: &mut deloop::Client,
        control_source: Option<String>,
    ) {
        if self.control_source == control_source {
            return;
        }

        info!("User selected control source: {:?}", control_source);
        if let Some(control_source) = &self.control_source {
            if log_on_err(client.unsubscribe_from(control_source)).is_err() {
                return;
            }

            self.control_source = None;
        }

        if let Some(control_source) = control_source {
            if log_on_err(client.subscribe_to(control_source.as_str())).is_err() {
                return;
            }

            self.control_source = Some(control_source);
        }
    }
}

struct ControlPanel {
    client: deloop::Client,
    tracks: Vec<TrackInterface>,
    audio_sources: HashSet<String>,
    audio_sinks: HashSet<String>,
    midi_sources: HashSet<String>,
    selected_io: SelectedIO,
}

impl ControlPanel {
    fn new(cc: &eframe::CreationContext<'_>) -> Self {
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
        let client = deloop::Client::new().unwrap();
        client.add_track();

        let audio_sources = client.audio_sources();
        let audio_sinks = client.audio_sinks();
        let midi_sources = client.midi_sources();

        Self {
            client,
            tracks: vec![TrackInterface::new()],
            audio_sources,
            audio_sinks,
            midi_sources,
            selected_io: SelectedIO::default(),
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

        egui::SidePanel::left("").show(ctx, |ui| {
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
                    ui.selectable_value(&mut selected_audio_sink, None, "None");
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
                    ui.selectable_value(&mut selected_control_source, None, "None");
                    for midi_source in &self.midi_sources {
                        let value = ui.selectable_value(
                            &mut selected_control_source,
                            Some(midi_source.clone()),
                            midi_source,
                        );
                        if value.clicked() {
                            self.selected_io
                                .select_control_source(&mut self.client, Some(midi_source.clone()));
                        }
                    }
                });

            if ui.button("Refresh").clicked() {
                self.audio_sources = self.client.audio_sources();
                self.audio_sinks = self.client.audio_sinks();
                self.midi_sources = self.client.midi_sources();
            }
        });

        egui::CentralPanel::default().show(ctx, |ui| {
            if ui.button("Add Track").clicked() {
                self.tracks.push(TrackInterface::new());
            }

            for track in &mut self.tracks {
                track.show(ui);
            }

            ui.input(|i| {
                for event in &i.raw.events {
                    if let Event::Key {
                        key: _,
                        physical_key: _,
                        pressed,
                        repeat: _,
                        modifiers: _,
                    } = event
                    {
                        if *pressed {
                            // match key {
                            // egui::Key::Space => deloop::client_perform_action(),
                            // egui::Key::Num0 => deloop::client_set_focused_track(0),
                            // egui::Key::Num1 => deloop::client_set_focused_track(1),
                            // egui::Key::Num2 => deloop::client_set_focused_track(2),
                            // egui::Key::Num3 => deloop::client_set_focused_track(3),
                            // egui::Key::Num4 => deloop::client_set_focused_track(4),
                            // _ => {}
                            //  }
                        }
                    }
                }
            });
        });
    }
}
