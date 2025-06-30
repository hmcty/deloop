use log::error;
use rppal::gpio::{Gpio, Trigger};
use std::time::Duration;

use crate::deloop;

// Expected pin configuration.
// https://datasheets.raspberrypi.com/cm4io/cm4io-datasheet.pdf
const TRACK_A_PIN: u8 = 16;
const TRACK_B_PIN: u8 = 12;

/// Macro to set up a GPIO pin for track button input.
/// Takes a client, pin number, and track ID and sets up the appropriate interrupt handler.
macro_rules! create_track_button {
    ($client:expr, $pin:expr, $track_id:expr) => {{
        let sender = $client.command_sender().clone();
        Gpio::new()?
            .get($pin)?
            .into_input_pullup()
            .set_async_interrupt(
                Trigger::FallingEdge,
                Some(Duration::from_millis(25)),
                move |_event| {
                    deloop::send_advance_track(&sender, $track_id).unwrap_or_else(|e| {
                        error!("Failed to advance track {:?}: {}", $track_id, e)
                    });
                },
            )?
    }};
}

pub fn run() -> Result<(), Box<dyn std::error::Error>> {
    let client = deloop::Client::default();

    // On each press, advance track state.
    create_track_button!(&client, TRACK_A_PIN, deloop::TrackId::A);
    create_track_button!(&client, TRACK_B_PIN, deloop::TrackId::B);

    // Loop until exit signal is received.
    loop {
        break;
    }

    Ok(())
}
