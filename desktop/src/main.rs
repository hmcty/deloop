use clap::Parser;

pub mod deloop;
mod gui;
mod util;

#[derive(Parser, Debug)]
#[command(version, about, long_about = None)]
struct Args {}

fn main() -> eframe::Result {
    env_logger::init(); // Log to stderr (if you run with `RUST_LOG=debug`).
    let _ = gui::run();
    Ok(())
}
