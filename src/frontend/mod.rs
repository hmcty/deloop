mod io_selector;

#[cfg(feature = "egui-frontend")]
mod egui;

pub fn run() -> Result<(), Box<dyn std::error::Error>> {
    #[cfg(feature = "egui-frontend")]
    {
        egui::run()?;
    }
    Ok(())
}
