#include <daisy_seed.h>
#include <lvgl.h>

#include "display.hpp"
#include "engine.h"

using namespace daisy;

constexpr size_t kAudioBlockSize = 48;
constexpr size_t kAudioBufferSize = kAudioBlockSize * 2;

static DaisySeed hw;
static GPIO DC;
static GPIO RST;
static GPIO FOOTSW_A;
static GPIO FOOTSW_B;
static GPIO FOOTSW_C;
static SpiHandle spi_handle;

static float input_gain = 10.0f;
static float gain_buffer[kAudioBufferSize];

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

void audio_callback(daisy::AudioHandle::InterleavingInputBuffer in,
                    daisy::AudioHandle::InterleavingOutputBuffer out,
                    size_t size) {
  if (size != kAudioBufferSize) {
    // TODO: Report error
    return;
  }

  for (size_t i = 0; i < size; i++) {
    gain_buffer[i] = in[i] * input_gain;
  }

  dlp_engine_process_audio(gain_buffer, out, size);

  for (size_t i = 0; i < size; i++) {
    out[i] = fmaxf(fminf(out[i], 1.0f), -1.0f);
  }
}

int main(void) {
  hw.Init();
  hw.StartLog();

  // Start processing audio
  dlp_engine_init();
  hw.SetAudioBlockSize(kAudioBlockSize);
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
  hw.StartAudio(audio_callback);

  FOOTSW_A.Init(seed::D23, GPIO::Mode::INPUT, GPIO::Pull::PULLUP);
  FOOTSW_B.Init(seed::D22, GPIO::Mode::INPUT, GPIO::Pull::PULLUP);
  FOOTSW_C.Init(seed::D21, GPIO::Mode::INPUT, GPIO::Pull::PULLUP);

  // Configure display
  // DC.Init(seed::D11, GPIO::Mode::OUTPUT, GPIO::Pull::NOPULL,
  //         GPIO::Speed::VERY_HIGH);
  // RST.Init(seed::D12, GPIO::Mode::OUTPUT, GPIO::Pull::PULLUP,
  //          GPIO::Speed::MEDIUM);
  // spi_handle.Init(default_spi_config());
  // setup_display(&spi_handle, &DC, &RST);

  uint32_t last = System::GetNow();
  bool led_state = true;

  bool footsw_a_last = true;
  bool footsw_b_last = true;
  bool footsw_c_last = true;
  while (1) {
    if (System::GetNow() - last > 500) {
      last = System::GetNow();
      hw.SetLed(led_state);
      led_state = !led_state;
    }

    dlp_engine_response_t resp;
    if (dlp_engine_check_response(&resp) == DLP_SUCCESS) {
      hw.PrintLine("Received response for cmd %d: %d", resp.cmd_id,
                   resp.resp_type);
    }

    if (FOOTSW_A.Read() != footsw_a_last) {
      footsw_a_last = FOOTSW_A.Read();
      if (footsw_a_last) {
        dlp_engine_command_t cmd = {
            .cmd_type = DLP_ENGINE_CMD_ADVANCE,
            .track_id = DLP_TRACK_A,
        };
        dlp_engine_send_command(&cmd);
      }
    }

    if (FOOTSW_B.Read() != footsw_b_last) {
      footsw_b_last = FOOTSW_B.Read();
      hw.PrintLine("Footswitch B: %d", !footsw_b_last);
    }

    if (FOOTSW_C.Read() != footsw_c_last) {
      footsw_c_last = FOOTSW_C.Read();
      if (footsw_c_last) {
        dlp_engine_command_t cmd = {
            .cmd_type = DLP_ENGINE_CMD_ADVANCE,
            .track_id = DLP_TRACK_B,
        };
        dlp_engine_send_command(&cmd);
      }
    }

    // display_tick();

    System::Delay(10);
  }
}
