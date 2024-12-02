# Deloop - Mk0

Assumes STM32F4 platform. In particular, this was built and tested on a Nucleo F446RE.

## Usage

```sh
nix-build
```

Will produce `result/bin/deloop_mk0.elf`.

After building, you can open a GDB server with:

```sh
./scripts/debug.sh
```
