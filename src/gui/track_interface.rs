use egui::{containers::Frame, emath, epaint, epaint::PathStroke, pos2, vec2, Color32, Pos2, Rect};

use crate::deloop;

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

fn resample(a: &[f32], b: &mut [f32]) {
    assert!(!b.is_empty());
    let step_size = a.len() as f32 / b.len() as f32;
    let mut step: f32 = 0.0;

    for v in b.iter_mut() {
        let j = step.round() as usize;
        if j >= a.len() {
            break;
        }

        *v = a[j];
        step += step_size;
    }
}

pub struct TrackInterface {
    pub track_id: usize,
    pub track_settings: deloop::track::Settings,
    track_status: Option<deloop::track::Status>,
    track_progress: f32,
    fl_display: Vec<f32>,
    fr_display: Vec<f32>,
}

impl TrackInterface {
    pub fn new(client: &mut deloop::Client) -> TrackInterface {
        let track_settings = deloop::track::Settings {
            sync: deloop::track::SyncTo::None,
            speed: None,
        };
        TrackInterface {
            track_id: client.add_track(track_settings.clone()).unwrap(),
            track_settings,
            track_status: None,
            track_progress: 0.0,
            fl_display: vec![0.0; 1000],
            fr_display: vec![0.0; 1000],
        }
    }

    pub fn update(&mut self, status: deloop::track::Status) {
        self.track_status = Some(status);

        match status_update {
            deloop::track::Status::Metadata(metadata) => {
                self.track_state = metadata.state;
                self.track_focused = metadata.is_focused;
                self.track_progress = metadata.buf_index as f32 / metadata.buf_size as f32;
                self.last_ctr = metadata.ctr;
                self.last_relative_ctr = metadata.relative_ctr;
            }
            deloop::track::Status::Waveform(metadata, fl_buffer, fr_buffer) => {
                self.track_state = metadata.state;
                self.track_focused = metadata.is_focused;
                self.track_progress = metadata.buf_index as f32 / metadata.buf_size as f32;
                self.last_ctr = metadata.ctr;
                self.last_relative_ctr = metadata.relative_ctr;
                if metadata.buf_size == 0 {
                    return;
                }

                if self.track_state == deloop::track::StateType::Recording {
                    resample(&fl_buffer, &mut self.fl_display);
                    resample(&fr_buffer, &mut self.fr_display);
                } else {
                    let frac = fl_buffer.len() as f32 / metadata.buf_size as f32;
                    let size = (self.fl_display.len() as f32 * frac) as usize;
                    let write_head = (self.track_progress * self.fl_display.len() as f32) as usize;

                    for i in 0..size {
                        let j = (i as f32 / size as f32 * fl_buffer.len() as f32) as usize;
                        if write_head + i >= self.fl_display.len() {
                            break;
                        }

                        self.fl_display[write_head + i] = fl_buffer[j];
                        self.fr_display[write_head + i] = fr_buffer[j];
                    }
                }
            }
        }

        if self.track_state == deloop::track::StateType::Idle {
            self.fl_display.fill(0.0);
            self.fr_display.fill(0.0);
        }
    }

    pub fn show_thumbnail(
        &mut self,
        ui: &mut egui::Ui,
        client: &mut deloop::Client,
        available_width: f32,
        is_focused: bool,
    ) -> egui::Response {
        let mut bg_color = ui.visuals().widgets.noninteractive.bg_fill;
        if is_focused {
            bg_color = ui.visuals().widgets.active.bg_fill;
        }

        Frame::none()
            .fill(bg_color)
            .inner_margin(egui::vec2(available_width * 0.05, 10.0))
            .outer_margin(egui::vec2(0.0, 0.0))
            .rounding(egui::Rounding {
                nw: 10.0,
                ne: 10.0,
                sw: 0.0,
                se: 0.0,
            })
            .show(ui, |ui| {
                Frame::canvas(ui.style())
                    .show(ui, |ui| {
                        let desired_size = available_width * vec2(0.9, 0.5);
                        let resp = ui.allocate_response(desired_size, egui::Sense::click());
                        let rect = resp.rect;
                        if resp.clicked() {
                            client.focus_on_track(self.track_id).unwrap();
                        }
                        let to_screen = emath::RectTransform::from_to(
                            Rect::from_x_y_ranges(0.0..=1.0, -1.0..=1.0),
                            rect,
                        );

                        let (top, bottom) = rect.split_top_bottom_at_fraction(0.5);
                        let fl_to_screen = emath::RectTransform::from_to(
                            Rect::from_x_y_ranges(0.0..=1.0, -1.0..=1.0),
                            top,
                        );
                        let fr_to_screen = emath::RectTransform::from_to(
                            Rect::from_x_y_ranges(0.0..=1.0, -1.0..=1.0),
                            bottom,
                        );

                        let mut shapes = Vec::new();
                        shapes.push(create_wave_shape(
                            &self.fl_display,
                            fl_to_screen,
                            2.5,
                            Color32::from_rgb(125, 125, 125),
                        ));
                        shapes.push(create_wave_shape(
                            &self.fl_display,
                            fr_to_screen,
                            2.5,
                            Color32::from_rgb(125, 125, 125),
                        ));

                        let indicator_top = to_screen * pos2(self.track_progress, 1.0);
                        let indicator_bottom = to_screen * pos2(self.track_progress, -1.0);

                        if is_focused {
                            // let bps = self.quantize_to_bpm / 60.0;
                            // let beat_duration_s = 1.0 / bps;

                            // let now = std::time::Instant::now();
                            // let is_led_on = (now.duration_since(self.last_led_toggle).as_secs_f32()
                            //     % beat_duration_s)
                            //     < (beat_duration_s / 4.0);

                            // if is_led_on {
                            //     shapes.push(epaint::Shape::circle_filled(
                            //         to_screen * egui::pos2(0.95, -0.8),
                            //         5.0,
                            //         Color32::from_rgb(125, 10, 10),
                            //     ));
                            // }
                        }

                        shapes.push(epaint::Shape::line(
                            vec![indicator_top, indicator_bottom],
                            PathStroke::new(1.0, Color32::from_rgb(125, 10, 10)),
                        ));

                        ui.painter().extend(shapes);

                        return resp;
                    })
                    .inner
            })
            .inner
    }

    pub fn show_controls(&mut self, ui: &mut egui::Ui, client: &mut deloop::Client) {
        egui::Frame::none()
            .fill(ui.visuals().widgets.active.bg_fill)
            .inner_margin(egui::vec2(10.0, 10.0))
            .show(ui, |ui| {
                egui::Frame::none()
                    .fill(ui.visuals().widgets.noninteractive.bg_fill)
                    .show(ui, |ui| {
                        ui.label(format!("Track {}", self.track_id));
                        let Some(status) = self.track_status.clone() else {
                            return;
                        };

                        ui.label(format!("State: {:?}", status.state));
                        egui::ComboBox::from_label("Sync Settings")
                            .selected_text(format!("{:?}", self.track_settings.sync))
                            .show_ui(ui, |ui| {
                                ui.selectable_value(
                                    &mut self.track_settings.sync,
                                    deloop::track::SyncTo::None,
                                    "None",
                                )
                                .clicked()
                                .then(|| {
                                    client
                                        .configure_track(
                                            self.track_id,
                                            deloop::track::Settings {
                                                sync: self.track_settings.sync,
                                                speed: None,
                                            },
                                        )
                                        .unwrap();
                                });

                                (0..4).filter(|&i| i != self.track_id).for_each(|i| {
                                    ui.selectable_value(
                                        &mut self.track_settings.sync,
                                        deloop::track::SyncTo::Track(i),
                                        format!("Track {}", i),
                                    )
                                    .clicked()
                                    .then(|| {
                                        client
                                            .configure_track(
                                                self.track_id,
                                                deloop::track::Settings {
                                                    sync: self.track_settings.sync,
                                                    speed: None,
                                                },
                                            )
                                            .unwrap();
                                    });
                                });
                            });
                    });
                ui.set_min_width(ui.available_width());
                ui.set_min_height(ui.available_height());
            });
    }
}
