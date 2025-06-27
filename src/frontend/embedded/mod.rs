use rppal::gpio::{Event, Gpio, Trigger};
use std::sync::{Arc, Condvar, Mutex};
use std::time::Duration;

use crate::deloop;

// Expected pin configuration.
const TRACK_ADVANCE_PIN: u8 = 16;

struct FrontendState {
    should_exit: bool,
}

pub fn run() -> Result<(), Box<dyn std::error::Error>> {
    let client = deloop::Client::default();
    let state = Arc::new(Mutex::new(FrontendState { should_exit: false }));

    // On each press, advance track state.
    let mut track_adv = Gpio::new()?.get(TRACK_ADVANCE_PIN)?.into_input_pullup();
    track_adv.set_async_interrupt(
        Trigger::FallingEdge,
        Some(Duration::from_millis(25)),
        move |event| {
            client.advance_track_state();
        },
    )?;

    // Loop until exit signal is received.
    loop {
        let (lock, cvar) = &*state;
        let mut lock = lock.lock().unwrap();
        while !lock.should_exit {
            lock = cvar.wait(lock).unwrap();
        }

        break;
    }

    Ok(())
}
