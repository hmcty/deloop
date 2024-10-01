use clap::Parser;

pub mod deloop;
pub mod util;
mod gui;

#[derive(Parser, Debug)]
#[command(version, about, long_about = None)]
struct Args {
    /// Instead of opening the GUI, open an interactive REPL.
    #[arg(short, long)]
    open_repl: bool,
}

fn main() -> eframe::Result {
    env_logger::init(); // Log to stderr (if you run with `RUST_LOG=debug`).

    let args = Args::parse();
    if deloop::client_init() != 0 {
        panic!("Failed to initialize deloop client");
    }

    if args.open_repl {
        // TODO
    } else {
        gui::run();
    }

    deloop::client_cleanup();
    Ok(())
}
