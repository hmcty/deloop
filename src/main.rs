use clap::Parser;

// pub mod deloop;
pub mod util;
// mod gui;

// #[cfg(not(target_arch = "wasm32"))]

#[derive(Parser, Debug)]
#[command(version, about, long_about = None)]
struct Args {
    /// Instead of opening the GUI, open an interactive REPL.
    #[arg(short, long)]
    open_repl: bool,
}

struct State {
}

impl State {
    fn new() -> Self {
        Self {}
    }

    fn process(&mut self, _client: &jack::Client, _ps: &jack::ProcessScope) -> jack::Control {
        jack::Control::Continue
    }

    fn buffer_size(&mut self, _client: &jack::Client, _len: jack::Frames) -> jack::Control {
        jack::Control::Continue
    }
}


fn main() -> eframe::Result {
    env_logger::init(); // Log to stderr (if you run with `RUST_LOG=debug`).

    let (client, _status) =
        jack::Client::new("deloop", jack::ClientOptions::default()).unwrap();

    let output = client
        .register_port("output", jack::AudioOut::default())
        .unwrap();

    let mut state = State::new();
    let process = jack::contrib::ClosureProcessHandler::with_state(
        state, State::process, State::buffer_size);

    let active_client = client.activate_async((), process).unwrap();

    // Sleep for a while, otherwise the program will exit immediately.
    std::thread::sleep(std::time::Duration::from_secs(15));

    Ok(())
}
