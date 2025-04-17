# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands
- Build firmware: `nix-build` (produces `result/bin/deloop_mk0.elf`)
- Build with nix-shell: `nix-shell && mkdir build && cd build && cmake -DCMAKE_TOOLCHAIN_FILE:PATH="cmake/arm-none-eabi-gcc.cmake" -DCMAKE_BUILD_TYPE=Debug .. && make`
- Flash firmware: `./scripts/flash.sh`
- Debug: `./scripts/debug.sh`
- Run REPL: `./result/bin/deloop_mk0_repl`
- Run tests: `python -m unittest tests/test_log_table.py`

## Code Style
- C++23 standard
- Use namespaces for project code (`deloop::`)
- Follow existing error handling with `DELOOP_RETURN_IF_ERROR` macro
- Use strong typing with enum classes
- Logging: `DELOOP_LOG_INFO/WARNING/ERROR(fmt, ...)`
- 2-space indentation
- Use .hpp for C++ headers
- Prefer `std::array` over raw arrays
- Follow FreeRTOS/STM32 HAL conventions for platform code
- Use `Error` enum for error handling, not exceptions
