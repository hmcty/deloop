Seed MCU: STM32H750IB

## Building

```sh
mkdir build && cd build
cmake --preset=arm-gcc ..
make
```

## Flashing

```sh
make dfu_bootloader
make dfu_app
```
