#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

use eframe::egui::{self, ComboBox, Event, ProgressBar};
use std::collections::HashMap;
use std::ffi::{CStr, CString};

struct AudioDevice {
    FL_port_name: Option<String>,
    FR_port_name: Option<String>,
    is_selected: bool,
}

struct MidiDevice {
    port_name: String,
}

fn get_track_midi_opts(result: &mut HashMap<String, MidiDevice>) {
    unsafe {
        let mut ports = deloop_client_find_midi_devices();
        while !ports.is_null() {
            let client_name = CStr::from_ptr((*ports).client_name)
                .to_str()
                .unwrap()
                .to_owned();
            let port_name = CStr::from_ptr((*ports).port_name)
                .to_str()
                .unwrap()
                .to_owned();

            result.insert(
                client_name.clone(),
                MidiDevice {
                    port_name: port_name.clone(),
                },
            );

            println!("Client: {}, Port: {}", client_name, port_name);
            ports = (*ports).last;
        }

        deloop_client_device_list_free(ports);
    };
}

fn get_track_audio_opts(result: &mut HashMap<String, AudioDevice>, is_input: bool) {
    unsafe {
        let mut ports = deloop_client_find_audio_devices(is_input, !is_input);
        while !ports.is_null() {
            let client_name = CStr::from_ptr((*ports).client_name)
                .to_str()
                .unwrap()
                .to_owned();
            let port_name = CStr::from_ptr((*ports).port_name)
                .to_str()
                .unwrap()
                .to_owned();

            if (*ports).port_type == deloop_device_port_type_t_deloop_DEVICE_PORT_TYPE_STEREO_FL {
                result
                    .entry(client_name.clone())
                    .or_insert(AudioDevice {
                        FL_port_name: Some(port_name.clone()),
                        FR_port_name: None,
                        is_selected: false,
                    })
                    .FL_port_name = Some(port_name.clone());
            } else if (*ports).port_type
                == deloop_device_port_type_t_deloop_DEVICE_PORT_TYPE_STEREO_FR
            {
                result
                    .entry(client_name.clone())
                    .or_insert(AudioDevice {
                        FL_port_name: None,
                        FR_port_name: Some(port_name.clone()),
                        is_selected: false,
                    })
                    .FR_port_name = Some(port_name.clone());
            } else if (*ports).port_type == deloop_device_port_type_t_deloop_DEVICE_PORT_TYPE_MONO {
                result.entry(client_name.clone()).or_insert(AudioDevice {
                    FL_port_name: Some(port_name.clone()),
                    FR_port_name: Some(port_name.clone()),
                    is_selected: false,
                });
            } else {
                panic!("Unknown port type: {}", (*ports).port_type);
            }

            println!("Client: {}, Port: {}", client_name, port_name);
            ports = (*ports).last;
        }

        deloop_client_device_list_free(ports);
    };
}

fn truncate_string(s: &str, max_len: usize) -> String {
    if s.len() > max_len {
        format!("{}...", &s[..max_len])
    } else {
        s.to_string()
    }
}

fn main() -> eframe::Result {
    env_logger::init(); // Log to stderr (if you run with `RUST_LOG=debug`).
    unsafe {
        let ret = deloop_client_init();
        if ret != 0 {
            panic!("Failed to initialize deloop");
        }
    }

    let options = eframe::NativeOptions::default();
    eframe::run_native(
        "Plot",
        options,
        Box::new(|_cc| Ok(Box::<WavePlot>::default())),
    )
    .unwrap();

    unsafe { deloop_client_cleanup() }
    Ok(())
}

struct TrackInterface {
    track_id: u32,
}

impl TrackInterface {
    fn new() -> TrackInterface {
        let track_id = unsafe { deloop_client_add_track() };
        // @TODO: Make IDs non-zero random hashes.
        // if track_id == 0 {
        //    panic!("Failed to create track");
        // }

        TrackInterface { track_id }
    }

    fn show(&mut self, ui: &mut egui::Ui) {
        let is_focused = unsafe { deloop_client_get_focused_track() == self.track_id };

        ui.separator();

        egui::Frame::none()
            .fill(if is_focused {
                egui::Color32::from_rgb(64, 64, 64)
            } else {
                egui::Color32::from_rgb(0x2a, 0x2a, 0x2a)
            })
            .show(ui, |ui| {
                // Write track and id
                ui.label(format!("Track {}", self.track_id));

                let progress: f32 =
                    unsafe { deloop_track_get_progress(deloop_client_get_track(self.track_id)) };
                ui.add(ProgressBar::new(progress));
            });
    }
}

struct WavePlot {
    audio_input_map: HashMap<String, AudioDevice>,

    audio_output_map: HashMap<String, AudioDevice>,
    output_device: String,

    midi_control_map: HashMap<String, MidiDevice>,
    control_device: String,

    tracks: Vec<TrackInterface>,
}

impl Default for WavePlot {
    fn default() -> Self {
        let mut audio_input_map = HashMap::new();
        get_track_audio_opts(&mut audio_input_map, false);

        let mut audio_output_map = HashMap::new();
        get_track_audio_opts(&mut audio_output_map, true);

        let mut midi_control_map = HashMap::new();
        get_track_midi_opts(&mut midi_control_map);

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

impl eframe::App for WavePlot {
    fn update(&mut self, ctx: &egui::Context, _: &mut eframe::Frame) {
        ctx.request_repaint(); // Run continously

        egui::SidePanel::left("").show(ctx, |ui| {
            ui.label("Input Devices");
            for (client, device) in &mut self.audio_input_map {
                if ui.checkbox(&mut device.is_selected, client).clicked() {
                    unsafe {
                        if device.is_selected {
                            let source_FL =
                                CString::new(device.FL_port_name.as_deref().unwrap()).unwrap();
                            let source_FR =
                                CString::new(device.FR_port_name.as_deref().unwrap()).unwrap();
                            deloop_client_add_source(source_FL.as_ptr(), source_FR.as_ptr());
                        } else {
                            let source_FL =
                                CString::new(device.FL_port_name.as_deref().unwrap()).unwrap();
                            let source_FR =
                                CString::new(device.FR_port_name.as_deref().unwrap()).unwrap();
                            deloop_client_remove_source(source_FL.as_ptr(), source_FR.as_ptr());
                        }
                    }
                }
            }

            ui.separator();
            ComboBox::from_label("Output Device")
                .selected_text(&self.output_device)
                .show_ui(ui, |ui| {
                    for (client, device) in &self.audio_output_map {
                        let value = ui.selectable_value(
                            &mut self.output_device,
                            truncate_string(client, 15),
                            client,
                        );
                        if value.clicked() {
                            unsafe {
                                let sink_FL =
                                    CString::new(device.FL_port_name.as_deref().unwrap()).unwrap();
                                let sink_FR =
                                    CString::new(device.FR_port_name.as_deref().unwrap()).unwrap();
                                deloop_client_configure_sink(sink_FL.as_ptr(), sink_FR.as_ptr());
                            }
                        }
                    }
                });

            ComboBox::from_label("Control Device")
                .selected_text(&self.control_device)
                .show_ui(ui, |ui| {
                    for (client, device) in &self.midi_control_map {
                        let value = ui.selectable_value(
                            &mut self.control_device,
                            truncate_string(client, 15),
                            client,
                        );
                        if value.clicked() {
                            unsafe {
                                let control_port = CString::new(device.port_name.as_str()).unwrap();
                                deloop_client_configure_control(control_port.as_ptr());
                            }
                        }
                    }
                });

            if ui.button("Update Devices").clicked() {
                get_track_audio_opts(&mut self.audio_input_map, false);
                get_track_audio_opts(&mut self.audio_output_map, true);
                get_track_midi_opts(&mut self.midi_control_map);
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
                                egui::Key::Num0 => unsafe {
                                    deloop_client_set_focused_track(0);
                                },
                                egui::Key::Num1 => unsafe {
                                    deloop_client_set_focused_track(1);
                                },
                                egui::Key::Num2 => unsafe {
                                    deloop_client_set_focused_track(2);
                                },
                                egui::Key::Num3 => unsafe {
                                    deloop_client_set_focused_track(3);
                                },
                                egui::Key::Num4 => unsafe {
                                    deloop_client_set_focused_track(4);
                                },
                                _ => {}
                            }
                        }
                    }
                }
            });
        });
    }
}
