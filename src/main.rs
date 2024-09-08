#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

use std::ffi::CStr;
use eframe::egui::{self, DragValue, Event, Vec2, Slider, ProgressBar, ComboBox};

fn get_track_audio_input_opts() -> Vec<String> {
    let mut result = Vec::new();
    unsafe{
        let output_ports = bizzy_get_output_audio_ports();
        if output_ports.is_null() {
            panic!("Failed to find any output ports");
        }

        let mut i = 0;
        while !(*output_ports.offset(i)).is_null() {
            let c_str = CStr::from_ptr(*output_ports.offset(i));
            if let Ok(s) = c_str.to_str() {
                result.push(s.to_owned());
            }
            i += 1;
        }

        bizzy_port_list_free(output_ports);
    };
    result
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

struct WavePlot {
    audio_input_opts: Vec<String>, 
    track_1_input: String,
    track_1_playing: bool,
    track_1_recording: bool,
    track_1_duration_s: u32,
}

impl Default for WavePlot {
    fn default() -> Self {
        Self {
            audio_input_opts: get_track_audio_input_opts(),
            track_1_input: "None".to_string(),
            track_1_playing: true,
            track_1_recording: false,
            track_1_duration_s: 5,
        }
    }
}

impl eframe::App for WavePlot {
    fn update(&mut self, ctx: &egui::Context, _: &mut eframe::Frame) {
        ctx.request_repaint(); // Run continously
        egui::CentralPanel::default().show(ctx, |ui| {

            ComboBox::from_label("Input")
                .selected_text(&self.track_1_input)
                .show_ui(ui, |ui| {
                for input in &self.audio_input_opts {
                    ui.selectable_value(&mut self.track_1_input, input.to_string(), input);
                }
            });

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

          //if ui.checkbox(
          //    &mut self.track_1_recording,
          //    "Track 1 recording",
          //).clicked() {
          //    if self.track_1_recording {
          //        unsafe { bizzy_track_start_recording(bizzy_get_track()); }
          //    } else {
          //        unsafe { bizzy_track_stop_recording(bizzy_get_track()); }
          //    }
          //}

          //if ui.add(
          //    Slider::new(&mut self.track_1_duration_s, 1..=30)
          //        .text("Duration (s)")
          //).changed() {
          //    unsafe {
          //        println!("Setting duration to {}", self.track_1_duration_s);
          //        bizzy_track_set_duration(
          //            bizzy_get_track(),
          //            self.track_1_duration_s,
          //        );
          //    }
          //}

            let progress: f32 = unsafe {
                bizzy_track_get_progress(bizzy_get_track())
            };
            ui.add(ProgressBar::new(progress));
        });
    }
}
