mod io_selector;

#[cfg(feature = "egui-frontend")]
mod egui;

pub fn run() -> Result<(), Box<dyn std::error::Error>> {
    #[cfg(feature = "egui-frontend")]
    {
        egui::run()?;
    }

    #[cfg(not(feature = "egui-frontend"))]
    {
        eprintln!(
            "Egui frontend is not enabled. Please enable it with the 'egui-frontend' feature."
        );
    }

    Ok(())
}
