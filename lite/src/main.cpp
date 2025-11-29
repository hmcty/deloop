#include <daisy_seed.h>
#include <ff.h>
#include <hid/logger.h>

#include "fatfs.h"

#ifdef DLP_DAISYSP
#include <daisysp.h>
#endif

#include "cli.hpp"
#include "engine.h"
#include "fs.hpp"
#include "logging.hpp"
#include "ui.hpp"

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
static Ui ui;
// static WS2812B<kNumLeds> led_circle_a;
// static WS2812B<kNumLeds> led_circle_b;

static float input_gain = 1.0f;
static float gain_buffer[kAudioBufferSize];

#ifdef DLP_DAISYSP
static daisysp::SquareNoise osc;
static float phase = 0.0f;
#endif

void deloop::LogInternal(deloop::LogLevel level, const char* format, ...) {
  switch (level) {
    case deloop::LogLevel::INFO:
      Logger<LOGGER_INTERNAL>::Print("[INFO][%d] ", System::GetNow());
      break;
    case deloop::LogLevel::WARN:
      Logger<LOGGER_INTERNAL>::Print("[WARN][%d] ", System::GetNow());
      break;
    case deloop::LogLevel::ERROR:
      Logger<LOGGER_INTERNAL>::Print("[ERROR][%d] ", System::GetNow());
      break;
  }
  daisy::System::Delay(5);

  va_list va;
  va_start(va, format);
  Logger<LOGGER_INTERNAL>::PrintLineV(format, va);
  va_end(va);
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
#endif  // DLP_DAISYSP

  for (size_t i = 0; i < size; i++) {
    out[i] = fmaxf(fminf(out[i], 1.0f), -1.0f);
  }
}

int main(void) {
  hw.Init();
  hw.StartLog();
  hw.usb_handle.SetReceiveCallback(cli::UsbCallback,
                                   UsbHandle::UsbPeriph::FS_INTERNAL);

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
                        prescaler,  // prescaler
                        249         // period (at 200MHz, gives 800KHz PWM freq)
      );
  auto result = led_pwm.Init(config);

  PWMHandle::Channel::Config channel_config_a;
  channel_config_a.pin = seed::D24;
  led_pwm.Channel2().Init(channel_config_a);

  PWMHandle::Channel::Config channel_config_b;
  channel_config_b.pin = seed::D16;
  led_pwm.Channel4().Init(channel_config_b);

  ui.Init(&led_pwm.Channel2(), &led_pwm.Channel4());
  cli::Init();
  fs::init();

#ifdef DLP_DAISYSP
  // Initialize DaisySP components
#else
  dlp_engine_init();
#endif  // DLP_DAISYSP

  hw.StartAudio(audio_callback);

  uint32_t last = System::GetNow();
  bool led_state = true;

  bool footsw_a_last = true;
  bool footsw_b_last = true;
  bool footsw_c_last = true;
  while (1) {
    cli::Step(System::GetNow());
    ui.Step(System::GetNow());

    if (System::GetNow() - last > 500) {
      last = System::GetNow();
      hw.SetLed(led_state);
      led_state = !led_state;
    }

    dlp_engine_response_t resp;
    dlp_error_t err = dlp_engine_check_response(&resp);
    if (err == DLP_SUCCESS) {
      hw.PrintLine("Received response for cmd %d: %d", resp.cmd_id,
                   resp.resp_type);
    } else if (err == DLP_ENGINE_RESP_TRACK_STATUS) {
      dlp_track_status_t status = resp.data.track_status;
      switch (status.id) {
        case DLP_TRACK_A:
          break;
        case DLP_TRACK_B:
          break;
        default:
          break;
      }
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

    System::Delay(10);
  }
}
