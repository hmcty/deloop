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

#[allow(dead_code)]
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
    pub track_id: deloop::TrackId,
    pub track_settings: deloop::track::Settings,
    track_status: Option<deloop::track::Status>,
    track_progress: f32,
    counter_progress: f32,
    num_bars: u64,
    fl_display: Vec<f32>,
    #[allow(dead_code)]
    fr_display: Vec<f32>,
}

impl TrackInterface {
    pub fn new(track_id: deloop::TrackId) -> TrackInterface {
        let track_settings = deloop::track::Settings {
            sync: deloop::track::SyncTo::None,
            speed: None,
        };
        TrackInterface {
            track_id,
            track_settings,
            track_status: None,
            track_progress: 0.0,
            counter_progress: 0.0,
            num_bars: 0,
            fl_display: vec![0.0; 1000],
            fr_display: vec![0.0; 1000],
        }
    }

    pub fn update(&mut self, status: deloop::track::Status) {
        let buf_index = status.buf_index as f32;
        let buf_size = status.buf_size as f32;
        self.track_status = Some(status);
        self.track_progress = buf_index / buf_size;
    }

    pub fn update_waveform(&mut self, status: deloop::track::Status, fl: &[f32], fr: &[f32]) {
        self.update(status);
        resample(fl, &mut self.fl_display);
        resample(fr, &mut self.fr_display);
    }

    pub fn update_counter(&mut self, global_ctr: &deloop::GlobalCounter) {
        if let Some(status) = &self.track_status {
            let relative = global_ctr.relative(status.ctr);
            let len = global_ctr.get_len(status.ctr);
            self.counter_progress = relative as f32 / len as f32;

            if status.buf_size as u64 > len {
                self.num_bars = status.buf_size as u64 / len;
            } else {
                self.num_bars = 0;
            }
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
                        shapes.push(epaint::Shape::line(
                            vec![indicator_top, indicator_bottom],
                            PathStroke::new(1.0, Color32::from_rgb(125, 10, 10)),
                        ));

                        let indicator_top = to_screen * pos2(self.counter_progress, 1.0);
                        let indicator_bottom = to_screen * pos2(self.counter_progress, -1.0);
                        shapes.push(epaint::Shape::line(
                            vec![indicator_top, indicator_bottom],
                            PathStroke::new(1.0, Color32::from_rgb(10, 125, 10)),
                        ));

                        for i in 0..self.num_bars {
                            let x = i as f32 / self.num_bars as f32;
                            let indicator_top = to_screen * pos2(x, 1.0);
                            let indicator_bottom = to_screen * pos2(x, -1.0);
                            shapes.push(epaint::Shape::line(
                                vec![indicator_top, indicator_bottom],
                                PathStroke::new(1.0, Color32::from_rgb(10, 10, 125)),
                            ));
                        }

                        ui.painter().extend(shapes);

                        resp
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
                        ui.label(format!("Track {:?}", self.track_id));
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

                                for track_id in deloop::TrackId::ALL_TRACKS {
                                    if track_id == self.track_id {
                                        continue;
                                    }

                                    ui.selectable_value(
                                        &mut self.track_settings.sync,
                                        deloop::track::SyncTo::Track(track_id),
                                        format!("Track {:?}", track_id),
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
                                }
                            });
                    });
                ui.set_min_width(ui.available_width());
                ui.set_min_height(ui.available_height());
            });
    }
}
