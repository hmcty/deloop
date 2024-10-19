use eframe::egui::{self, ComboBox, Event};
use egui::{containers::Frame, emath, epaint, epaint::PathStroke, pos2, vec2, Color32, Pos2, Rect};
use serde::{Deserialize, Serialize};
use std::collections::HashMap;

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

struct TrackInterface {
    track: deloop::Track,
    amp_rbuffer: Vec<f32>,
    amp_wbuffer: Vec<f32>,
}

impl TrackInterface {
    fn new() -> TrackInterface {
        TrackInterface {
            track: deloop::Track::new(),
            amp_rbuffer: Vec::new(),
            amp_wbuffer: Vec::new(),
        }
    }

    fn show(&mut self, ui: &mut egui::Ui) {
        let is_focused = self.track.is_focused();

        ui.separator();

        egui::Frame::none()
            .fill(if is_focused {
                egui::Color32::from_rgb(64, 64, 64)
            } else {
                egui::Color32::from_rgb(0x2a, 0x2a, 0x2a)
            })
            .show(ui, |ui| {
                // Write track and id
                ui.label(format!("Track {}", self.track.id));
                let mut progress: f32 = self.track.get_progress();

                Frame::canvas(ui.style()).show(ui, |ui| {
                    let desired_size = ui.available_width() * vec2(1.0, 0.35);
                    let (_id, rect) = ui.allocate_space(desired_size);

                    let to_screen = emath::RectTransform::from_to(
                        Rect::from_x_y_ranges(0.0..=1.0, -1.0..=1.0),
                        rect,
                    );
                    if self.track.is_awaiting() {
                        self.amp_rbuffer.clear();
                        self.amp_wbuffer.clear();
                        return;
                    }

                    let mut shapes = vec![];
                    let ramp = self.track.get_write_amplitude().clamp(-1.0, 1.0);
                    let wamp = self.track.get_write_amplitude().clamp(-1.0, 1.0);

                    shapes.push(create_wave_shape(
                        &self.amp_rbuffer,
                        to_screen,
                        2.5,
                        Color32::from_rgb(125, 125, 125),
                    ));
                    if self.track.is_recording() {
                        self.amp_rbuffer.push(wamp);
                        progress = 1.0;
                    } else {
                        let idx = (progress * (self.amp_rbuffer.len() as f32)) as usize;
                        if let Some(x) = self.amp_rbuffer.get_mut(idx) {
                            *x = ramp;
                        }
                    }

                    let indicator_top = to_screen * pos2(progress, 1.0);
                    let indicator_bottom = to_screen * pos2(progress, -1.0);

                    shapes.push(epaint::Shape::line(
                        vec![indicator_top, indicator_bottom],
                        PathStroke::new(1.0, Color32::from_rgb(125, 10, 10)),
                    ));

                    ui.painter().extend(shapes);
                });
            });
    }
}

#[derive(Serialize, Deserialize)]
struct DeloopControlPanel {
    audio_input_map: HashMap<String, deloop::AudioDevice>,
    audio_output_map: HashMap<String, deloop::AudioDevice>,
    output_device: String,
    midi_control_map: HashMap<String, deloop::MidiDevice>,
    control_device: String,

    #[serde(skip)]
    tracks: Vec<TrackInterface>,
}

impl LoopDeLoopControlPanel {
    fn new(cc: &eframe::CreationContext<'_>) -> Self {
        let mut control_panel = Self::default();
        if let Some(storage) = cc.storage {
            if let Some(s) = storage.get_string("control_panel") {
                let stored_control_panel: LoopDeLoopControlPanel =
                    serde_json::from_str(&s).unwrap();

                for (k, device) in stored_control_panel.audio_input_map {
                    if control_panel.audio_input_map.contains_key(&k) && device.is_selected {
                        device.configure_as_input(true);
                        control_panel
                            .audio_input_map
                            .get_mut(&k)
                            .unwrap()
                            .is_selected = true;
                    }
                }

                let output_device = control_panel
                    .audio_output_map
                    .get(&stored_control_panel.output_device);
                if let Some(output_device) = output_device {
                    output_device.configure_as_output();
                    control_panel.output_device = stored_control_panel.output_device;
                }

                let control_device = control_panel
                    .midi_control_map
                    .get(&stored_control_panel.control_device);
                if let Some(control_device) = control_device {
                    control_device.configure_as_control();
                    control_panel.control_device = stored_control_panel.control_device;
                }
            }
        }

        control_panel
    }

    fn default() -> Self {
        let mut audio_input_map = HashMap::new();
        deloop::get_track_audio_opts(&mut audio_input_map, false);

        let mut audio_output_map = HashMap::new();
        deloop::get_track_audio_opts(&mut audio_output_map, true);

        let mut midi_control_map = HashMap::new();
        deloop::get_track_midi_opts(&mut midi_control_map);

        let tracks = vec![TrackInterface::new()];

        Self {
            audio_input_map,
            audio_output_map,
            output_device: "None".to_string(),
            midi_control_map,
            control_device: "None".to_string(),
            tracks,
        }
    }
}

impl eframe::App for LoopDeLoopControlPanel {
    fn save(&mut self, storage: &mut dyn eframe::Storage) {
        let j = serde_json::to_string(&self).unwrap();
        storage.set_string("control_panel", j);
    }

    fn update(&mut self, ctx: &egui::Context, _: &mut eframe::Frame) {
        ctx.request_repaint(); // Run continuously

        egui::SidePanel::left("").show(ctx, |ui| {
            ui.label("Input Devices");
            for (client, device) in &mut self.audio_input_map {
                if ui.checkbox(&mut device.is_selected, client).clicked() {
                    device.configure_as_input(device.is_selected);
                }
            }

            ui.separator();
            ComboBox::from_label("Output Device")
                .selected_text(util::truncate_string(&self.output_device, 15))
                .show_ui(ui, |ui| {
                    for (client, device) in &self.audio_output_map {
                        let value = ui.selectable_value(
                            &mut self.output_device,
                            client.to_string(),
                            client,
                        );
                        if value.clicked() {
                            device.configure_as_output();
                        }
                    }
                });

            ComboBox::from_label("Control Device")
                .selected_text(util::truncate_string(&self.control_device, 15))
                .show_ui(ui, |ui| {
                    for (client, device) in &self.midi_control_map {
                        let value = ui.selectable_value(
                            &mut self.control_device,
                            client.to_string(),
                            client,
                        );
                        if value.clicked() {
                            device.configure_as_control();
                        }
                    }
                });

            if ui.button("Update Devices").clicked() {
                deloop::get_track_audio_opts(&mut self.audio_input_map, false);
                deloop::get_track_audio_opts(&mut self.audio_output_map, true);
                deloop::get_track_midi_opts(&mut self.midi_control_map);
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
                        key,
                        physical_key: _,
                        pressed,
                        repeat: _,
                        modifiers: _,
                    } = event
                    {
                        if *pressed {
                            match key {
                                egui::Key::Space => deloop::client_perform_action(),
                                egui::Key::Num0 => deloop::client_set_focused_track(0),
                                egui::Key::Num1 => deloop::client_set_focused_track(1),
                                egui::Key::Num2 => deloop::client_set_focused_track(2),
                                egui::Key::Num3 => deloop::client_set_focused_track(3),
                                egui::Key::Num4 => deloop::client_set_focused_track(4),
                                _ => {}
                            }
                        }
                    }
                }
            });
        });
    }
}
