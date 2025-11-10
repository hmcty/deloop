#include "cli.hpp"

#include <daisy_seed.h>

#define CMD_MATCH(str, n)

static void HandleResetCmd(daisy::DaisySeed &hw);
static void HandleVersionCmd(daisy::DaisySeed &hw);

constexpr size_t kNumCommands = 2;
constexpr size_t kMaxCmdLength = 16;
struct CommandEntry {
  char name[kMaxCmdLength];
  void (*handler)(daisy::DaisySeed &hw);
};

static struct {
  bool initialized;
  daisy::FIFO<daisy::FixedCapStr<128>, 16> fifo;
  size_t remaining_cmd_bytes;
  size_t cmd_length;
  char cmd_buffer[kMaxCmdLength];
  CommandEntry commands[kNumCommands];
} state_ = {
    .initialized = false,
    .remaining_cmd_bytes = 0,
    .cmd_length = kMaxCmdLength,
    .commands =
        {
            {"RST", HandleResetCmd},   // Reset command
            {"VER", HandleVersionCmd}, // Version command
        },
};

static void HandleResetCmd(daisy::DaisySeed &hw) {
  hw.PrintLine("\nReset command received. Restarting...");
  hw.DelayMs(200);
  NVIC_SystemReset();
}

static void HandleVersionCmd(daisy::DaisySeed &hw) {
  hw.PrintLine("\nCLI Version 1.0.0");
}

static void HandleCommand(daisy::DaisySeed &hw) {
  for (size_t i = 0; i < kNumCommands; i++) {
    if (std::strncmp(state_.cmd_buffer, state_.commands[i].name,
                     state_.cmd_length) == 0) {
      state_.commands[i].handler(hw);
      return;
    }
  }

  hw.Print("\nUnknown command: [");
  for (size_t i = 0; i < state_.cmd_length; i++) {
    hw.Print("0x%02X ", (uint8_t)state_.cmd_buffer[i]);
  }
  hw.PrintLine("]");
}

void cli::Init() {
  if (state_.initialized)
    return;

  state_.initialized = true;
  state_.remaining_cmd_bytes = 0;
  state_.cmd_length = 0;
}

void cli::UsbCallback(uint8_t *buff, uint32_t *length) {
  if (!state_.initialized)
    return;

  if (buff && length) {
    daisy::FixedCapStr<128> rx((const char *)buff, *length);
    state_.fifo.PushBack(rx);
  }
}

void cli::Step(uint32_t now, daisy::DaisySeed &hw) {
  if (!state_.initialized)
    return;

  while (!state_.fifo.IsEmpty()) {
    auto msg = state_.fifo.PopFront();
    hw.Print(msg);

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
          HandleCommand(hw);
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
