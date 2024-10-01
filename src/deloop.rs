#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(dead_code)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::ffi::{CStr, CString};

pub fn client_init() -> i32 {
    unsafe { deloop_client_init() }
}

pub fn client_cleanup() {
    unsafe { deloop_client_cleanup() }
}

pub fn client_add_track() -> u32 {
    unsafe { deloop_client_add_track() }
}

pub fn client_perform_action() {
    unsafe { deloop_client_perform_action() }
}

pub fn client_get_focused_track() -> u32 {
    unsafe { deloop_client_get_focused_track() }
}

pub fn client_set_focused_track(id: u32) {
    unsafe { deloop_client_set_focused_track(id) }
}

#[derive(Debug, Serialize, Deserialize)]
pub struct AudioDevice {
    pub FL_port_name: Option<String>,
    pub FR_port_name: Option<String>,
    pub is_selected: bool,
}

impl AudioDevice {
    pub fn configure_as_input(&self, enable: bool) {
        let source_FL = CString::new(self.FL_port_name.as_deref().unwrap()).unwrap();
        let source_FR = CString::new(self.FR_port_name.as_deref().unwrap()).unwrap();

        if enable {
            unsafe { deloop_client_add_source(source_FL.as_ptr(), source_FR.as_ptr()) };
        } else {
            unsafe { deloop_client_remove_source(source_FL.as_ptr(), source_FR.as_ptr()) };
        }
    }

    pub fn configure_as_output(&self) {
        let sink_FL = CString::new(self.FL_port_name.as_deref().unwrap()).unwrap();
        let sink_FR = CString::new(self.FR_port_name.as_deref().unwrap()).unwrap();

        unsafe { deloop_client_configure_sink(sink_FL.as_ptr(), sink_FR.as_ptr()) };
    }
}

#[derive(Debug, Serialize, Deserialize)]
pub struct MidiDevice {
    pub port_name: String,
}

impl MidiDevice {
    pub fn configure_as_control(&self) {
        let control_port = CString::new(self.port_name.as_str()).unwrap();
        unsafe { deloop_client_configure_control(control_port.as_ptr()) };
    }
}

pub struct Track {
    pub id: u32,
    internal_ptr: *mut deloop_track_t,
}

impl Track {
    pub fn new() -> Track {
        let id = client_add_track();
        Track {
            id,
            internal_ptr: unsafe { deloop_client_get_track(id) },
        }
    }

    pub fn is_focused(&self) -> bool {
        self.id == client_get_focused_track()
    }

    pub fn get_progress(&self) -> f32 {
        unsafe { deloop_track_get_progress(self.internal_ptr) }
    }

    pub fn is_awaiting(&self) -> bool {
        unsafe { (*self.internal_ptr).state == deloop_track_state_t_DELOOP_TRACK_STATE_AWAITING }
    }

    pub fn is_recording(&self) -> bool {
        unsafe { (*self.internal_ptr).state == deloop_track_state_t_DELOOP_TRACK_STATE_RECORDING }
    }

    pub fn get_read_amplitude(&self) -> f32 {
        unsafe { (*self.internal_ptr).ramp * 100.0 }
    }

    pub fn get_write_amplitude(&self) -> f32 {
        unsafe { (*self.internal_ptr).wamp * 100.0 }
    }
}

pub fn get_track_midi_opts(result: &mut HashMap<String, MidiDevice>) {
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

pub fn get_track_audio_opts(result: &mut HashMap<String, AudioDevice>, is_input: bool) {
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

            if (*ports).port_type == deloop_device_port_type_t_DELOOP_DEVICE_PORT_TYPE_STEREO_FL {
                result
                    .entry(client_name.clone())
                    .or_insert(AudioDevice {
                        FL_port_name: Some(port_name.clone()),
                        FR_port_name: None,
                        is_selected: false,
                    })
                    .FL_port_name = Some(port_name.clone());
            } else if (*ports).port_type
                == deloop_device_port_type_t_DELOOP_DEVICE_PORT_TYPE_STEREO_FR
            {
                result
                    .entry(client_name.clone())
                    .or_insert(AudioDevice {
                        FL_port_name: None,
                        FR_port_name: Some(port_name.clone()),
                        is_selected: false,
                    })
                    .FR_port_name = Some(port_name.clone());
            } else if (*ports).port_type == deloop_device_port_type_t_DELOOP_DEVICE_PORT_TYPE_MONO {
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
