#include "cli.hpp"

#include <daisy_seed.h>

#include "fs.hpp"
#include "logging.hpp"
#include "time.hpp"

using namespace daisy;

static void HandleResetCmd();
static void HandleVersionCmd();
static void HandleTreeCmd();
static void HandlePrintMetadataCmd();

constexpr size_t kNumCommands = 4;
constexpr size_t kMaxCmdLength = 16;
struct CommandEntry {
  char name[kMaxCmdLength];
  void (*handler)();
};

static struct {
  bool initialized;
  FIFO<FixedCapStr<128>, 16> fifo;
  size_t remaining_cmd_bytes;
  size_t cmd_length;
  char cmd_buffer[kMaxCmdLength];
  CommandEntry commands[kNumCommands];
} state_ = {.initialized = false,
            .remaining_cmd_bytes = 0,
            .cmd_length = kMaxCmdLength,
            .commands = {{"RST", HandleResetCmd},
                         {"VER", HandleVersionCmd},
                         {"TREE", HandleTreeCmd},
                         {"PMETA", HandlePrintMetadataCmd}}};

static void HandleResetCmd() {
  DELOOP_LOG_INFO("Resetting system per CLI command.");
  DELAY_MS(100);
  NVIC_SystemReset();
}

static void HandleVersionCmd() { DELOOP_LOG_INFO("CLI Version 1.0.0"); }

static void HandleTreeCmd() { fs::Tree("/", 0); }

static void HandlePrintMetadataCmd() {
  auto &md = fs::GetMetadata();
  DELOOP_LOG_INFO("Metadata:");
  DELOOP_LOG_INFO("-- Magic: 0x%08X", md.magic);
  DELOOP_LOG_INFO("-- Version: %d", md.version);
  DELOOP_LOG_INFO("-- Active Buffer: %d", md.active_buffer);
  DELOOP_LOG_INFO("-- CRC32: 0x%08X", md.crc32);
}

static void HandleCommand() {
  for (size_t i = 0; i < kNumCommands; i++) {
    if (std::strncmp(state_.cmd_buffer, state_.commands[i].name,
                     state_.cmd_length) == 0) {
      state_.commands[i].handler();
      return;
    }
  }

  // hw.Print("\nUnknown command: [");
  // for (size_t i = 0; i < state_.cmd_length; i++) {
  //   hw.Print("0x%02X ", (uint8_t)state_.cmd_buffer[i]);
  // }
  // hw.PrintLine("]");
}

void cli::Init() {
  if (state_.initialized) return;

  state_.initialized = true;
  state_.remaining_cmd_bytes = 0;
  state_.cmd_length = 0;
}

void cli::UsbCallback(uint8_t *buff, uint32_t *length) {
  if (!state_.initialized) return;

  if (buff && length) {
    FixedCapStr<128> rx((const char *)buff, *length);
    state_.fifo.PushBack(rx);
  }
}

void cli::Step(uint32_t now) {
  if (!state_.initialized) return;

  while (!state_.fifo.IsEmpty()) {
    auto msg = state_.fifo.PopFront();
    for (size_t i = 0; i < msg.Size(); i++) {
      if (state_.remaining_cmd_bytes > 0) {
        // Newline indicates end of command
        if (msg.Data()[i] == '\r' || msg.Data()[i] == '\n') {
          state_.remaining_cmd_bytes = 0;
        } else {
          state_.cmd_buffer[state_.cmd_length] = msg.Data()[i];
          state_.remaining_cmd_bytes--;
          state_.cmd_length++;
        }

        if (state_.remaining_cmd_bytes == 0) {
          HandleCommand();
          memset(state_.cmd_buffer, 0, kMaxCmdLength);
          state_.cmd_length = 0;
        }
      } else {
        if (msg.Data()[i] == '@') {
          state_.remaining_cmd_bytes = kMaxCmdLength;
        }
      }
    }
  }
}
