#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

use eframe::egui::{self, DragValue, Event, Vec2, Slider};

fn main() -> eframe::Result {
    env_logger::init(); // Log to stderr (if you run with `RUST_LOG=debug`).
    unsafe { 
        let ret = bizzy_init();
        if ret != 0 {
            panic!("Failed to initialize Bizzy");
        }
    }

    let options = eframe::NativeOptions::default();
    eframe::run_native(
        "Plot",
        options,
        Box::new(|_cc| Ok(Box::<WavePlot>::default())),
    );

    unsafe { bizzy_cleanup() }
    Ok(())
}

struct WavePlot {
    track_1_playing: bool,
    track_1_recording: bool,
    track_1_duration_s: u32,
}

impl Default for WavePlot {
    fn default() -> Self {
        Self {
            track_1_playing: false,
            track_1_recording: false,
            track_1_duration_s: 5,
        }
    }
}

impl eframe::App for WavePlot {
    fn update(&mut self, ctx: &egui::Context, _: &mut eframe::Frame) {
        egui::CentralPanel::default().show(ctx, |ui| {
            if ui.checkbox(
                &mut self.track_1_playing,
                "Track 1 playing",
            ).clicked() {
                if self.track_1_playing {
                    unsafe { bizzy_track_start_playing(bizzy_get_track()); }
                } else {
                    unsafe { bizzy_track_stop_playing(bizzy_get_track()); }
                }
            }

            if ui.checkbox(
                &mut self.track_1_recording,
                "Track 1 recording",
            ).clicked() {
                if self.track_1_recording {
                    unsafe { bizzy_track_start_recording(bizzy_get_track()); }
                } else {
                    unsafe { bizzy_track_stop_recording(bizzy_get_track()); }
                }
            }

            if ui.add(
                Slider::new(&mut self.track_1_duration_s, 1..=30)
                    .text("Duration (s)")
            ).changed() {
                unsafe {
                    println!("Setting duration to {}", self.track_1_duration_s);
                    bizzy_track_set_duration(
                        bizzy_get_track(),
                        self.track_1_duration_s,
                    );
                }
            }
        });
    }
}
