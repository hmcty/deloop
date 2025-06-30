use clap::Parser;

pub mod deloop;
mod frontend;
mod util;

#[derive(Parser, Debug)]
#[command(version, about, long_about = None)]
struct Args {}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    env_logger::init(); // Log to stderr (if you run with `RUST_LOG=debug`).
    frontend::run()?;
    Ok(())
}
