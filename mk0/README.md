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

Can open REPL with:

```sh
./result/

### With nix-shell

```sh
nix-shell
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE:PATH="cmake/arm-none-eabi-gcc.cmake" -DCMAKE_BUILD_TYPE=Debug ..
make
```
