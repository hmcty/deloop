use eframe::egui::{self, DragValue, Event, Vec2};
use libc::c_char;

extern "C" {
    fn bizzy_init() -> i32;
    fn bizzy_cleanup() -> ();
}

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

struct WavePlot {}

impl Default for WavePlot {
    fn default() -> Self {
        Self {}
    }
}

impl eframe::App for WavePlot {
    fn update(&mut self, ctx: &egui::Context, _: &mut eframe::Frame) {
        egui::CentralPanel::default().show(ctx, |ui| {
        });
    }
}
