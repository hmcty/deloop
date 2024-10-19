use clap::Parser;

mod deloop;
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

    let _client = Client::new();

    // Sleep for a while, otherwise the program will exit immediately.
    std::thread::sleep(std::time::Duration::from_secs(15));

    Ok(())
}
