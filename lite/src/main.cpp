#include <daisy_seed.h>
#include <lvgl.h>

#include "display.hpp"
#include "engine.h"

using namespace daisy;

static DaisySeed hw;
static GPIO DC;
static GPIO RST;
static SpiHandle spi_handle;

SpiHandle::Config default_spi_config() {
  SpiHandle::Config spi_conf;
  spi_conf.mode = SpiHandle::Config::Mode::MASTER;
  spi_conf.periph = SpiHandle::Config::Peripheral::SPI_1;

  // At 8 and below, transactions become unreliable
  spi_conf.baud_prescaler = SpiHandle::Config::BaudPrescaler::PS_16;

  spi_conf.pin_config.sclk = seed::D8;
  spi_conf.pin_config.miso = Pin();
  spi_conf.pin_config.mosi = seed::D10;
  spi_conf.pin_config.nss = seed::D7;

  spi_conf.direction = SpiHandle::Config::Direction::TWO_LINES_TX_ONLY;
  spi_conf.nss = SpiHandle::Config::NSS::HARD_OUTPUT;

  return spi_conf;
}

void audio_callback(daisy::AudioHandle::InputBuffer in,
                    daisy::AudioHandle::OutputBuffer out, size_t size) {}

int main(void) {
  hw.Init();

  deloop_engine_init();

  // Configure audio
  // hw.StartAudio(audio_callback);

  // Configure display
  DC.Init(seed::D11, GPIO::Mode::OUTPUT, GPIO::Pull::NOPULL,
          GPIO::Speed::VERY_HIGH);
  RST.Init(seed::D12, GPIO::Mode::OUTPUT, GPIO::Pull::PULLUP,
           GPIO::Speed::MEDIUM);
  spi_handle.Init(default_spi_config());
  setup_display(&spi_handle, &DC, &RST);

  bool led_state = true;
  while (1) {
    hw.SetLed(led_state);
    led_state = !led_state;
    display_tick();
    deloop_track_tick();

    // wait 5 ms
    System::Delay(5);
  }
}
