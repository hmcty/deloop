use eframe::egui::{self, DragValue, Event, Vec2};
use libc::c_char;

// use bindings::{
//  bizzy_cleanup,
//  bizzy_get_track,
//  bizzy_init,
//  bizzy_track_start_recording,
//  bizzy_track_stop_recording,
//  bizzy_track_t,
//};
//

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

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
    track_1_recording: bool,
}

impl Default for WavePlot {
    fn default() -> Self {
        Self {
            track_1_recording: false,
        }
    }
}

impl eframe::App for WavePlot {
    fn update(&mut self, ctx: &egui::Context, _: &mut eframe::Frame) {
        egui::CentralPanel::default().show(ctx, |ui| {
            // Start / stop button
            if ui.checkbox(
                &mut self.track_1_recording,
                "Track 1 recording",
            ).clicked() {
                if self.track_1_recording {
                    unsafe {
                        bizzy_track_start_recording(bizzy_get_track());
                    }
                } else {
                    unsafe {
                        bizzy_track_stop_recording(bizzy_get_track());
                    }
                }
            }
        });
    }
}
