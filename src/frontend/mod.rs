mod io_selector;

#[cfg(feature = "egui")]
mod desktop;

#[cfg(feature = "embedded")]
mod embedded;

pub fn run() -> Result<(), Box<dyn std::error::Error>> {
    #[cfg(feature = "egui")]
    {
        desktop::run()?;
    }

    #[cfg(feature = "embedded")]
    {
        embedded::run()?;
    }

    Ok(())
}
