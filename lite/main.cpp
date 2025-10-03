#include "daisy_seed.h"

#include "GC9A01.hpp"

using namespace daisy;

DaisySeed hw;
// TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

int main(void) {
  // Initialize the hardware
  hw.Init();

  // tft.init();
  // tft.setRotation(1);
  // tft.fillScreen(TFT_DARKGREY);
  // tft.fillRectHGradient(0, 0, 240, 240, TFT_MAGENTA, TFT_BLUE);

  GPIO DC;
  DC.Init(seed::D11, GPIO::Mode::OUTPUT, GPIO::Pull::NOPULL,
          GPIO::Speed::VERY_HIGH);

  GPIO RST;
  RST.Init(seed::D12, GPIO::Mode::OUTPUT, GPIO::Pull::PULLUP,
           GPIO::Speed::MEDIUM);

  // Handle we'll use to interact with SPI
  SpiHandle spi_handle;

  // Structure to configure the SPI handle
  SpiHandle::Config spi_conf;

  spi_conf.mode = SpiHandle::Config::Mode::MASTER; // we're in charge

  spi_conf.periph =
      SpiHandle::Config::Peripheral::SPI_1; // Use the SPI_1 Peripheral

  spi_conf.baud_prescaler = SpiHandle::Config::BaudPrescaler::PS_16;

  // Pins to use. These must be available on the selected peripheral
  spi_conf.pin_config.sclk = seed::D8;  // Use pin D8 as SCLK
  spi_conf.pin_config.miso = Pin();     // We won't need this
  spi_conf.pin_config.mosi = seed::D10; // Use D10 as MOSI
  spi_conf.pin_config.nss = seed::D7;   // use D7 as NSS

  // data will flow from master to slave over just the MOSI line
  spi_conf.direction = SpiHandle::Config::Direction::TWO_LINES_TX_ONLY;

  // The master will output on the NSS line
  spi_conf.nss = SpiHandle::Config::NSS::HARD_OUTPUT;

  // Initialize the SPI Handle
  spi_handle.Init(spi_conf);

  GC9A01 display = GC9A01(spi_handle, DC, RST);
  display.Init();
  display.FillScreen(COLOR_WHITE);
  System::Delay(1000);

  display.FillScreen(COLOR_BLACK);
  System::Delay(1000);

  // display.FillScreen(0x0000);
  // display.FillScreen(0xAAAAAAAA);
  // System::Delay(1000);
  // display.FillScreen(0x00FF);
  // System::Delay(1000);

  // display.FillScreen(0xF000);
  // System::Delay(1000);
  // display.FillScreen(0x00000000);
  // System::Delay(1000);

  // display.FillScreen(0xFFFFFFFF);
  display.DrawRectangle(100, 100, 20, 20, COLOR_RED); // Red rectangle
  System::Delay(1000);

  display.DrawRectangle(20, 20, 100, 100, COLOR_BLUE); // Red rectangle
  System::Delay(1000);

  display.DrawRectangle(50, 30, 100, 100, COLOR_GREEN); // Red rectangle
  System::Delay(1000);

  display.DrawRectangle(10, 0, 50, 50, COLOR_RED); // Red rectangle
  System::Delay(1000);

  display.DrawRectangle(200, 200, 50, 50, COLOR_BLUE); // Red rectangle
  System::Delay(1000);

  display.FillScreen(COLOR_BLACK);

  // display.DrawRectangle(120, 0, 100, 0, 0x00F0); // Red rectangle
  // System::Delay(1000);
  // display.DrawRectangle(120, 60, 100, 50, 0x00F0); // Red rectangle
  // System::Delay(1000);
  // display.DrawRectangle(100, 100, 20, 20, 0x0000); // Red rectangle
  // display.DrawRectangle(200, 100, 20, 20, 0xF000); // Red rectangle

  bool led_state = true;

  // loop forever
  while (1) {
    // put these four bytes in a buffer
    // uint8_t buffer[4] = {0, 1, 2, 3};

    // transmit those 4 bytes
    // spi_handle.BlockingTransmit(buffer, 4);

    // Set the onboard LED
    hw.SetLed(led_state);

    // Toggle the LED state for the next time around.
    led_state = !led_state;

    // wait 500 ms
    System::Delay(500);
  }
}
