use rppal::gpio::{Event, Gpio, Trigger};
use std::sync::{Arc, Mutex};
use std::time::Duration;

use crate::deloop;

fn btn_callback(event: Event, data: Arc<Mutex<u8>>) {
    println!("Button event: {:?}", event);
}

pub fn run() -> Result<(), Box<dyn std::error::Error>> {
    // let shared_data = Arc::new(Mutex::new(0));
    let mut pin = Gpio::new()?.get(16)?.into_input_pullup();
    // let shared_state_hold = shared_data.clone();

    let client = deloop::Client::default();
    pin.set_async_interrupt(
        Trigger::FallingEdge,
        Some(Duration::from_millis(25)),
        move |event| {
            // btn_callback(event, shared_state_hold.clone());
            println!("Button event: {:?}", event);
            client.advance_track_state();
        },
    )?;

    loop {}

    #[allow(unreachable_code)]
    Ok(())
}
