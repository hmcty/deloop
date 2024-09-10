#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

use std::ffi::{CStr, CString};
use eframe::egui::{self, DragValue, Event, Vec2, Slider, ProgressBar, ComboBox};
use std::collections::HashMap;

struct AudioDevice {
    FL_port_name: Option<String>,
    FR_port_name: Option<String>,
}

fn get_track_audio_input_opts() -> HashMap<String, AudioDevice> {
    let mut result: HashMap<String, AudioDevice> = HashMap::new();
    unsafe{
        let mut output_ports = bizzy_client_find_output_devices();
        while !output_ports.is_null() {
            let client_name = CStr::from_ptr((*output_ports).client_name)
                .to_str().unwrap().to_owned();
            let port_name = CStr::from_ptr((*output_ports).port_name)
                .to_str().unwrap().to_owned();

            if (*output_ports).port_type == bizzy_device_port_t_BIZZY_DEVICE_PORT_TYPE_STEREO_FL {
                result.entry(client_name.clone())
                    .or_insert(AudioDevice {
                        FL_port_name: Some(port_name.clone()),
                        FR_port_name: None,
                    }).FR_port_name = Some(port_name.clone());
            } else if (*output_ports).port_type == bizzy_device_port_t_BIZZY_DEVICE_PORT_TYPE_STEREO_FR {
                result.entry(client_name.clone())
                    .or_insert(AudioDevice {
                        FL_port_name: None,
                        FR_port_name: Some(port_name.clone()),
                    }).FL_port_name = Some(port_name.clone());
            } else {
                panic!("Unknown port type: {}", (*output_ports).port_type);
            }

            println!("Client: {}, Port: {}", client_name, port_name);
            output_ports = (*output_ports).last;
        }

        bizzy_client_device_list_free(output_ports);
    };

    result
}

fn main() -> eframe::Result {
    env_logger::init(); // Log to stderr (if you run with `RUST_LOG=debug`).
    unsafe { 
        let ret = bizzy_client_init();
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

    unsafe { bizzy_client_cleanup() }
    Ok(())
}

struct WavePlot {
    audio_input_map: HashMap<String, AudioDevice>,
    audio_input_opts: Vec<String>, 
    track_1_input: String,
    track_1_playing: bool,
    track_1_recording: bool,
    track_1_duration_s: u32,
}

impl Default for WavePlot {
    fn default() -> Self {
        let audio_input_map = get_track_audio_input_opts();
        let audio_input_opts = audio_input_map.keys().cloned().collect();
        Self {
            audio_input_map: audio_input_map,
            audio_input_opts: audio_input_opts,
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

            if ui.button("Update Devices").clicked() {
                self.audio_input_map = get_track_audio_input_opts();
                self.audio_input_opts = self.audio_input_map
                    .keys().cloned().collect();
            }

            ComboBox::from_label("Input Device")
                .selected_text(&self.track_1_input)
                .show_ui(ui, |ui| {
                    for input in &self.audio_input_opts {
                        let value = ui.selectable_value(
                            &mut self.track_1_input,
                            input.to_string(),
                            input);
                        if value.clicked() {
                            println!("Selected input: {}", input);
                            unsafe {
                                self.audio_input_map.get(input).map(|audio_device| {
                                    let source_FL = audio_device.FL_port_name
                                        .as_ref().map(String::as_str).unwrap();
                                    let source_FR = audio_device.FR_port_name
                                        .as_ref().map(String::as_str).unwrap();
                                    bizzy_client_configure_source(
                                        CString::new(source_FL).unwrap().as_ptr(),
                                        CString::new(source_FR).unwrap().as_ptr(),
                                    );
                                });
                            }
                        }
                    }
                });

            if ui.checkbox(
                &mut self.track_1_playing,
                "Track 1 playing",
            ).clicked() {
                if self.track_1_playing {
                    unsafe { bizzy_track_start_playing(bizzy_client_get_track()); }
                } else {
                    unsafe { bizzy_track_stop_playing(bizzy_client_get_track()); }
                }
            }

            let progress: f32 = unsafe {
                bizzy_track_get_progress(bizzy_client_get_track())
            };
            ui.add(ProgressBar::new(progress));
        });
    }
}
