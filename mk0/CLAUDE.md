# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Commands

- Build firmware: `nix-build -A firmware`
- Build SDK: `nix-build -A sdk`
- Run all tests: `nix-build -A tests`

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
