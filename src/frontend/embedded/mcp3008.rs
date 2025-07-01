use rppal::spi::{BitOrder, Bus, Mode, SlaveSelect, Spi};

use snafu::{ResultExt, Snafu};

const MCP3XXX_MODE_SINGLE: u8 = 0b1; // Single-ended mode
const MCP3008_RESOLUTION: u8 = 10;
const MCP3008_ADDR_BITS: u8 = 3;

#[derive(Debug, Snafu)]
pub enum Error {
    #[snafu(display("Encountered SPI error: {msg}"))]
    SpiError {
        source: rppal::spi::Error,
        msg: String,
    },

    #[snafu(display("Invalid argument: {msg}"))]
    InvalidArgument { msg: String },
}

pub struct Mcp3008 {
    spi: Spi,
}

impl Mcp3008 {
    pub fn new(spi: Spi) -> Self {
        spi.set_mode(Mode::Mode0).expect("Failed to set SPI mode");
        spi.set_clock_speed(1_000_000)
            .expect("Failed to set SPI speed");
        spi.set_bit_order(BitOrder::MsbFirst)
            .expect("Failed to set SPI bit order");
        Mcp3008 { spi }
    }

    /// Simple single-ended read from the MCP3008 ADC. Converted to 0-1 range.
    pub fn read(&mut self, chan: u8) -> Result<f32, Error> {
        if chan > 7 {
            return Err(Error::InvalidArgument {
                msg: format!("Channel {} is out of range (0-7)", chan),
            });
        }

        // START[1] + MODE[1] + ADDR[3] + SAMPLE[1] + NULL[1] + DATA[10]
        let size = 1 + 1 + MCP3008_ADDR_BITS + 1 + 1 + MCP3008_RESOLUTION;
        let bytes = (f32::from(size) / 8f32).ceil() as u8;

        let command: u32 = (1u32 << u32::from(size - 1))
            | ((MCP3XXX_MODE_SINGLE as u32) << u32::from(size - 2))
            | ((u32::from(chan)) << (MCP3008_RESOLUTION + 2)) as u32;

        let mut tx: Vec<u8> = Vec::with_capacity(bytes as usize);
        for i in (0..bytes).rev() {
            let shift = u32::from(8u8 * i);
            tx.push(((command & (0b_1111_1111u32 << shift)) >> shift) as u8);
        }

        let mut rx: Vec<u8> = Vec::with_capacity(bytes as usize);
        for _ in 0..bytes {
            rx.push(0);
        }

        self.spi
            .transfer(&mut rx.as_mut_slice(), &tx.as_slice())
            .context(SpiSnafu {
                msg: "Failed to transfer data over SPI",
            })?;

        let mut result: u32 = 0;
        for (i, byte) in rx.iter().enumerate() {
            result |= (u32::from(*byte) << (u32::from(bytes - 1 - i as u8) * 8)) as u32;
        }

        Ok(result as f32 / ((1u32 << MCP3008_RESOLUTION) - 1) as f32)
    }
}
