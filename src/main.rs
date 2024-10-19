use clap::Parser;

pub mod deloop;
mod util;

use deloop::Client;

#[derive(Parser, Debug)]
#[command(version, about, long_about = None)]
struct Args {
    /// Instead of opening the GUI, open an interactive REPL.
    #[arg(short, long)]
    open_repl: bool,
}

fn main() -> eframe::Result {
    env_logger::init(); // Log to stderr (if you run with `RUST_LOG=debug`).

    let client = Client::new();

    let devices = client.audio_sources();
    println!("AUDIO SOURCES");
    for device in devices {
        println!("{device}");
    }

    let devices = client.audio_sinks();
    println!("AUDIO SINKS");
    for device in devices {
        println!("{device}");
    }

    let devices = client.midi_sources();
    println!("MIDI SOURCES");
    for device in devices {
        println!("{device}");
    }

    // Sleep for a while, otherwise the program will exit immediately.
    // std::thread::sleep(std::time::Duration::from_secs(15));

    Ok(())
}
