# GD32F303RCT6 Firmware Build And Flash Record

## Goal

Build a burnable firmware image on macOS without Keil and without modifying `GD32F303RxT6 KIT`.

## Target

- Board SDK: `GD32F303RxT6 KIT/3. SDK/GD32F303RCT6 Demo`
- MCU class: GD32F30x high-density, Cortex-M4F
- Memory map: 256 KiB Flash at `0x08000000`, 48 KiB usable SRAM at `0x20000000`
- ST-Link probe result: `V2J37S7`, flash `262144`, SRAM `65536`, chipid `0x414`, dev-type `F1xx_HD`
- SRAM note: ST-Link reports 64 KiB through the STM32F1-compatible path, but the vendor Keil projects declare `IRAM(0x20000000,0x0C000)`, so the linker script uses 48 KiB.

## Toolchain

Installed with Homebrew:

```sh
brew install cmake ninja arm-none-eabi-gcc stlink open-ocd
```

Verified versions:

```sh
arm-none-eabi-gcc --version
cmake --version
ninja --version
st-info --version
openocd --version
```

## Architecture

- `firmware/CMakeLists.txt`: GCC/CMake build definition
- `firmware/cmake/arm-none-eabi-gcc.cmake`: cross toolchain file
- `firmware/ld/GD32F303RCT6.ld`: 256 KiB Flash / 48 KiB SRAM linker script
- `firmware/startup/startup_gd32f30x_hd_gcc.S`: GNU startup and vector table
- `firmware/src/main.c`: validation firmware loop
- `firmware/src/board.c`: RGB LED and USART0 board support
- `firmware/src/systick.c`: millisecond timebase
- `firmware/include/stdint.h` and `firmware/include/stdbool.h`: freestanding C headers for the Homebrew Arm GCC package, which does not ship newlib headers
- `GD32F303RxT6 KIT`: read-only SDK dependency

## Build

From `...`:

```sh
cmake -S firmware -B firmware/build -G Ninja -DCMAKE_TOOLCHAIN_FILE=.../firmware/cmake/arm-none-eabi-gcc.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build firmware/build
```

Expected outputs:

- `firmware/build/gd32f303rct6_firmware.elf`
- `firmware/build/gd32f303rct6_firmware.bin`
- `firmware/build/gd32f303rct6_firmware.hex`
- `firmware/build/gd32f303rct6_firmware.map`

Current LCD diagnostic build after SRAM correction:

```text
FLASH: 25484 B / 256 KiB
RAM:   4240 B / 48 KiB
```

## Flash

Probe:

```sh
st-info --probe
```

Write and reset:

```sh
st-flash --reset write firmware/build/gd32f303rct6_firmware.bin 0x08000000
```

Flash result on 2026-05-14:

```text
Flash written and verified! jolly good!
NRST is not connected --> using software reset via AIRCR
```

Readback verification:

```sh
st-flash read firmware/build/gd32f303rct6_firmware_readback.bin 0x08000000 3004
cmp firmware/build/gd32f303rct6_firmware.bin firmware/build/gd32f303rct6_firmware_readback.bin
shasum -a 256 firmware/build/gd32f303rct6_firmware.bin firmware/build/gd32f303rct6_firmware_readback.bin
```

Readback SHA256:

```text
cbee46a74aaf4cbf79b1123ec4cea892f6a5cdc9b86f91249bc92ca1f08114b0  firmware/build/gd32f303rct6_firmware.bin
cbee46a74aaf4cbf79b1123ec4cea892f6a5cdc9b86f91249bc92ca1f08114b0  firmware/build/gd32f303rct6_firmware_readback.bin
```

## Runtime Validation

- RGB LED pins are `PC0`, `PC1`, `PC2`, based on the vendor RGB LED example.
- The firmware toggles red, green, and blue channels in sequence every 250 ms.
- USART0 uses `PA9` TX and `PA10` RX at `115200 8N1` and emits `GD32F303RCT6 GCC firmware ready` followed by `tick` lines.
- `gpio_pin_remap_config(GPIO_SWJ_SWDPENABLE_REMAP, ENABLE)` keeps SWD enabled after boot.
