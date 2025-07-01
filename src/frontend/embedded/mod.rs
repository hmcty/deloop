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
macro_rules! add_track_interrupt {
    ($gpio:expr, $client:expr, $track_id:expr) => {{
        let sender = $client.command_sender().clone();
        let mut last_rising: Option<std::time::Instant> = None;
        $gpio.set_async_interrupt(
            Trigger::Both,
            Some(Duration::from_millis(25)),
            move |event| {
                println!("Track {:?} button event: {:?}", $track_id, event.trigger);
                if event.trigger == Trigger::FallingEdge {
                    last_rising.replace(std::time::Instant::now());
                    return;
                }

                // If button was held for 2 seconds, clear the track.
                if let Some(last_rise_time) = last_rising {
                    if last_rise_time.elapsed() > Duration::from_secs(2) {
                        println!(
                            "Track {:?} button held for 2 seconds, clearing track",
                            $track_id
                        );
                        deloop::send_clear_track(&sender, $track_id).unwrap_or_else(|e| {
                            error!("Failed to clear track {:?}: {}", $track_id, e)
                        });
                        last_rising = None;
                        return;
                    }

                    last_rising = None; // Reset after processing
                    deloop::send_advance_track(&sender, $track_id).unwrap_or_else(|e| {
                        error!("Failed to advance track {:?}: {}", $track_id, e)
                    });
                }
            },
        )
    }};
}

pub fn run() -> Result<(), Box<dyn std::error::Error>> {
    let mut client = deloop::Client::default();

    // On each press, advance track state.
    let mut a_btn = Gpio::new()?.get(TRACK_A_PIN)?.into_input_pullup();
    add_track_interrupt!(&mut a_btn, client, deloop::TrackId::A);

    let mut b_btn = Gpio::new()?.get(TRACK_B_PIN)?.into_input_pullup();
    add_track_interrupt!(&mut b_btn, client, deloop::TrackId::B);

    // Loop until exit signal is received.
    loop {
        for update in client.get_track_updates() {
            match update {
                deloop::TrackInfo::StatusUpdate(id, status) => {
                    println!("TRACK {:?}: {:?}", id, status.state);
                }
                _ => {}
            }
        }

        std::thread::sleep(Duration::from_millis(100));
    }
    Ok(())
}
