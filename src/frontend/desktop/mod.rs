mod control_panel;
mod track_interface;

use crate::frontend::desktop::control_panel::ControlPanel;

pub fn run() -> eframe::Result {
    let options = eframe::NativeOptions::default();
    eframe::run_native(
        "Plot",
        options,
        Box::new(|cc| Ok(Box::new(ControlPanel::new(cc)))),
    )
    .unwrap();
    Ok(())
}
