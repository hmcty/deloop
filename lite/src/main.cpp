#include <daisy_seed.h>
#include <ratio>

#ifdef DLP_DAISYSP
#include <daisysp.h>
#endif

#include "WS2812B.hpp"
#include "display.hpp"
#include "engine.h"

using namespace daisy;

constexpr size_t kAudioBlockSize = 48;
constexpr size_t kAudioBufferSize = kAudioBlockSize * 2;

DaisySeed hw;
// static Switch FOOTSW_A;
// static Switch FOOTSW_B;
// static Switch FOOTSW_C;
static GPIO FOOTSW_A;
static GPIO FOOTSW_B;
static GPIO FOOTSW_C;

static PWMHandle led_pwm;
constexpr size_t kNumLeds = 16;
static WS2812B<kNumLeds> led_strip;

#ifndef DLP_HEADLESS
static GPIO DC;
static GPIO RST;
static SpiHandle spi_handle;
#endif

static float input_gain = 1.0f;
static float gain_buffer[kAudioBufferSize];

#ifdef DLP_DAISYSP
static daisysp::SquareNoise osc;
static float phase = 0.0f;
#endif

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

#ifdef DLP_DAISYSP
  for (size_t i = 0; i < size; i++) {
  }
#else
  for (size_t i = 0; i < size; i++) {
    gain_buffer[i] = in[i] * input_gain;
  }

  dlp_engine_process_audio(gain_buffer, out, size);
#endif // DLP_DAISYSP

  for (size_t i = 0; i < size; i++) {
    out[i] = fmaxf(fminf(out[i], 1.0f), -1.0f);
  }
}

int main(void) {
  hw.Init();
  hw.StartLog(true);
  hw.PrintLine("Hello\n");

  hw.SetAudioBlockSize(kAudioBlockSize);
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

  float update_rate =
      hw.AudioSampleRate() / static_cast<float>(kAudioBlockSize);
  // FOOTSW_A.Init(seed::D23, update_rate);
  // FOOTSW_B.Init(seed::D22, update_rate);
  // FOOTSW_C.Init(seed::D21, update_rate);
  FOOTSW_A.Init(seed::D23, GPIO::Mode::INPUT, GPIO::Pull::PULLUP);
  FOOTSW_B.Init(seed::D22, GPIO::Mode::INPUT, GPIO::Pull::PULLUP);
  FOOTSW_C.Init(seed::D21, GPIO::Mode::INPUT, GPIO::Pull::PULLUP);

  uint32_t prescaler = 0;
  auto config =
      PWMHandle::Config(PWMHandle::Config::Peripheral::TIM_5,
                        prescaler, // prescaler
                        249        // period (at 200MHz, gives 800KHz PWM freq)
      );
  auto result = led_pwm.Init(config);

  PWMHandle::Channel::Config channel_config;
  channel_config.pin = seed::D16;
  auto led_channel = led_pwm.Channel4();
  result = led_channel.Init(channel_config);

  uint32_t tim5_clk = (System::GetPClk1Freq() * 2) / (prescaler + 1);

  hw.PrintLine("TIM5 clock: %u Hz, %u", tim5_clk, prescaler);

  // led_channel.Set(0.5f); // 50% brightness
  led_strip.Init(&led_channel);

  int active_led = 0;
  led_strip.FillColor(0, 0, 0);
  led_strip.SetPixelColor(active_led, 15, 0, 0);
  led_strip.Render();
  // System::Delay(100);
  // led_strip.Render();
#ifdef DLP_DAISYSP
  // Initialize DaisySP components

#else
  dlp_engine_init();
#endif // DLP_DAISYSP

  hw.StartAudio(audio_callback);

#ifndef DLP_HEADLESS
  // Configure display
  DC.Init(seed::D11, GPIO::Mode::OUTPUT, GPIO::Pull::NOPULL,
          GPIO::Speed::VERY_HIGH);
  RST.Init(seed::D12, GPIO::Mode::OUTPUT, GPIO::Pull::PULLUP,
           GPIO::Speed::MEDIUM);
  spi_handle.Init(default_spi_config());
  setup_display(&spi_handle, &DC, &RST);
#endif

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

      led_strip.SetPixelColor(active_led, 0, 0, 0);
      active_led = (active_led + 1) % kNumLeds;
      led_strip.SetPixelColor(active_led, 15, 0, 0);
      led_strip.Render();
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
      if (footsw_b_last) {
        dlp_engine_command_t cmd = {
            .cmd_type = DLP_ENGINE_CMD_STOP_ALL,
        };
        dlp_engine_send_command(&cmd);
      }
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

#ifndef DLP_HEADLESS
    display_tick();
#endif

    System::Delay(10);
  }
}
