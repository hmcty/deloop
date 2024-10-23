use eframe::egui::{self, ComboBox, Event};
use egui::{containers::Frame, emath, epaint, epaint::PathStroke, pos2, vec2, Color32, Pos2, Rect};
use log::error;
use serde::{Deserialize, Serialize};
use std::collections::HashSet;

use crate::deloop;
use crate::util;

pub fn run() -> eframe::Result {
    let options = eframe::NativeOptions::default();
    eframe::run_native(
        "Plot",
        options,
        Box::new(|cc| Ok(Box::new(DeloopControlPanel::new(cc)))),
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

#[derive(Serialize, Deserialize)]
struct DeloopControlPanel {
    #[serde(skip)]
    client: deloop::Client,

    audio_sources: HashSet<String>,
    audio_sinks: HashSet<String>,
    midi_sources: HashSet<String>,

    selected_audio_sources: HashSet<String>,
    selected_audio_sink: Option<String>,
    selected_control_source: Option<String>,

    #[serde(skip)]
    tracks: Vec<TrackInterface>,
}

impl DeloopControlPanel {
    fn new(cc: &eframe::CreationContext<'_>) -> Self {
        if let Some(storage) = cc.storage {
            if let Some(s) = storage.get_string("control_panel") {
                let control_panel = serde_json::from_str::<DeloopControlPanel>(&s);
                if let Ok(control_panel) = control_panel {
                    //for audio_source in &control_panel.selected_audio_sources {
                    //    control_panel.toggle_subscription_to_audio_source(audio_source.as_str());
                    //}

                    return control_panel;
                }
                // client.connect(select_audio_sink)
                // client.connect(select_midi_source)
            }
        }

        Self::default()
    }

    fn default() -> Self {
        let client = deloop::Client::new();
        let audio_sources = client.audio_sources();
        let audio_sinks = client.audio_sinks();
        let midi_sources = client.midi_sources();

        Self {
            client,
            audio_sources,
            audio_sinks,
            midi_sources,
            selected_audio_sources: HashSet::new(),
            selected_audio_sink: None,
            selected_control_source: None,
            tracks: vec![TrackInterface::new()],
        }
    }

    fn toggle_subscription_to_audio_source(&mut self, audio_source: &str) {
        if self.selected_audio_sources.contains(audio_source) {
            if log_on_err(self.client.unsubscribe_from(audio_source)).is_ok() {
                self.selected_audio_sources.remove(audio_source);
            }
        } else if log_on_err(self.client.subscribe_to(audio_source)).is_ok() {
            self.selected_audio_sources.insert(audio_source.to_string());
        }
    }

    fn select_audio_sink(&mut self, audio_sink: Option<String>) {
        if self.selected_audio_sink == audio_sink {
            return;
        }

        if log_on_err(self.client.stop_publishing()).is_err() {
            self.selected_audio_sink = None;
        } else if let Some(selected_sink) = audio_sink {
            if log_on_err(self.client.publish_to(selected_sink.as_str())).is_err() {
                self.selected_audio_sink = None;
                let _ = log_on_err(self.client.stop_publishing());
            }
        }
    }
}

impl eframe::App for DeloopControlPanel {
    fn save(&mut self, storage: &mut dyn eframe::Storage) {
        let j = serde_json::to_string(&self).unwrap();
        storage.set_string("control_panel", j);
    }

    fn update(&mut self, ctx: &egui::Context, _: &mut eframe::Frame) {
        ctx.request_repaint(); // Run continuously

        egui::SidePanel::left("").show(ctx, |ui| {
            ui.label("Audio Sources");
            let mut freshly_selected = Vec::new();
            for audio_source in &self.audio_sources {
                let mut selected = self.selected_audio_sources.contains(audio_source);
                if ui.checkbox(&mut selected, audio_source.clone()).clicked() {
                    freshly_selected.push(audio_source.clone());
                }
            }

            for audio_source in freshly_selected {
                self.toggle_subscription_to_audio_source(audio_source.as_str());
            }

            ui.separator();
            ComboBox::from_label("Audio Sink")
                .selected_text(util::truncate_string(
                    &self
                        .selected_audio_sink
                        .clone()
                        .unwrap_or("None".to_string()),
                    15,
                ))
                .show_ui(ui, |ui| {
                    if ui
                        .selectable_value(&mut self.selected_audio_sink, None, "None")
                        .clicked()
                    {
                        self.select_audio_sink(None);
                    }

                    let mut selected_audio_sink = self.selected_audio_sink.clone();
                    for audio_sink in &self.audio_sinks {
                        ui.selectable_value(
                            &mut selected_audio_sink,
                            Some(audio_sink.clone()),
                            audio_sink,
                        );
                    }

                    if let Some(selected_audio_sink) = selected_audio_sink {
                        self.select_audio_sink(Some(selected_audio_sink));
                    }
                });

            ComboBox::from_label("Control Source")
                .selected_text(util::truncate_string(
                    &self
                        .selected_control_source
                        .clone()
                        .unwrap_or("None".to_string()),
                    15,
                ))
                .show_ui(ui, |ui| {
                    for midi_source in &self.midi_sources {
                        let value = ui.selectable_value(
                            &mut self.selected_control_source,
                            Some(midi_source.to_string()),
                            midi_source,
                        );
                        if value.clicked() {}
                    }
                });

            if ui.button("Refresh").clicked() {}
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
