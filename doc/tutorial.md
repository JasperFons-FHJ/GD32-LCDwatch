# Mac 配置 GD32 开发环境教程

## 1. 教程目标

本文档说明如何在 macOS 上配置 GD32 单片机开发环境，实现以下能力：

- 不使用 Keil。
- 不依赖 Windows 原厂 IDE。
- 使用 GCC 工具链编译 GD32 固件。
- 使用 CMake/Ninja 管理工程构建。
- 使用 ST-Link 烧录和验证固件。
- 保持原厂 SDK 目录只读，不直接修改原始资料。

本项目目标芯片为 `GD32F303RCT6`，属于 GD32F30x 高密度 Cortex-M4F 系列。

## 2. 环境组成

macOS 上推荐使用以下工具：

| 工具 | 作用 |
| --- | --- |
| Homebrew | macOS 包管理器，用于安装开发工具 |
| arm-none-eabi-gcc | Arm Cortex-M 裸机交叉编译器 |
| CMake | 工程配置和构建生成工具 |
| Ninja | 高效构建后端 |
| stlink | ST-Link 探测、烧录、读回工具 |
| OpenOCD | 可选调试工具，后续可用于 GDB 调试 |

## 3. 安装 Homebrew

如果 Mac 上还没有 Homebrew，先安装 Homebrew：

```sh
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

安装完成后确认：

```sh
brew --version
```

如果命令不存在，需要根据 Homebrew 安装提示把 `/opt/homebrew/bin` 加入 `PATH`。

Apple Silicon Mac 常见路径：

```sh
eval "$(/opt/homebrew/bin/brew shellenv)"
```

Intel Mac 常见路径：

```sh
eval "$(/usr/local/bin/brew shellenv)"
```

## 4. 安装 GD32 开发工具链

执行：

```sh
brew install cmake ninja arm-none-eabi-gcc stlink open-ocd
```

安装完成后验证：

```sh
arm-none-eabi-gcc --version
cmake --version
ninja --version
st-info --version
st-flash --version
openocd --version
```

如果这些命令能正常输出版本号，说明基础工具链已经安装完成。

## 5. 连接 ST-Link

将 ST-Link 连接到 Mac，并连接开发板 SWD 接口。

常见 SWD 接线：

| ST-Link | 开发板 |
| --- | --- |
| SWDIO | SWDIO |
| SWCLK | SWCLK |
| GND | GND |
| 3.3V | 3.3V 参考电压 |
| NRST | 可选，连接后复位更可靠 |

注意：

- `3.3V` 通常用于电平参考，不一定给开发板供电，具体看 ST-Link 型号和开发板供电方式。
- 必须共地，即 ST-Link 的 `GND` 要连接开发板 `GND`。
- 如果不连接 `NRST`，`st-flash` 仍可能通过软件复位工作，但日志会提示 `NRST is not connected`。

## 6. 验证 ST-Link 和芯片连接

执行：

```sh
st-info --probe
```

正常情况下会看到类似输出：

```text
Found 1 stlink programmers
version:    V2J37S7
serial:     XXXXXXXXXXXX
flash:      262144 (pagesize: 2048)
sram:       65536
chipid:     0x414
dev-type:   F1xx_HD
```

重点确认：

- `Found 1 stlink programmers`：ST-Link 已被识别。
- `flash: 262144`：Flash 为 256 KiB。
- `sram: 65536`：ST-Link 兼容协议报告值，不直接作为链接脚本 SRAM 大小。
- `dev-type: F1xx_HD`：GD32F303RCT6 与 STM32F1 高密度烧录协议兼容，stlink 会这样识别。

本板原厂 Keil 工程声明 `IRAM(0x20000000,0x0C000)`，实际链接脚本应按 48 KiB SRAM 配置。若按 64 KiB 设置 `_estack = 0x20010000`，启动阶段会因栈顶越界触发 HardFault。

如果没有识别到 ST-Link，检查：

- USB 线是否支持数据传输。
- ST-Link 是否供电。
- macOS 是否授予 USB 访问权限。
- 是否有其他软件占用了 ST-Link。

如果识别到 ST-Link 但识别不到芯片，检查：

- SWDIO/SWCLK 是否接反。
- GND 是否连接。
- 开发板是否供电。
- 芯片是否被固件关闭 SWD。
- 必要时按住复位再执行探测。

## 7. 准备 GD32 SDK

本项目已有原厂资料目录：

```text
.../GD32F303RxT6 KIT
```

SDK 使用路径：

```text
GD32F303RxT6 KIT/3. SDK/GD32F303RCT6 Demo/Library/GD32F30x_Firmware_Library_V2.1.0/Firmware
```

这个目录包含：

- CMSIS 核心头文件。
- GD32F30x 设备头文件。
- `system_gd32f30x.c` 系统初始化文件。
- GD32 标准外设库源码。

建议原则：

- 原厂 SDK 只读引用。
- 不在原厂 KIT 中改代码。
- 自己的工程文件放在独立目录，例如本项目的 `firmware/`。

这样做可以保证原始资料可回溯，后续升级 SDK 时也容易比较差异。

## 8. 建立推荐工程结构

推荐工程结构：

```text
firmware/
├── CMakeLists.txt
├── cmake/
│   └── arm-none-eabi-gcc.cmake
├── include/
│   ├── board.h
│   ├── gd32f30x_libopt.h
│   ├── stdbool.h
│   ├── stdint.h
│   └── systick.h
├── ld/
│   └── GD32F303RCT6.ld
├── src/
│   ├── board.c
│   ├── gd32f30x_it.c
│   ├── main.c
│   └── systick.c
└── startup/
    └── startup_gd32f30x_hd_gcc.S
```

各文件作用：

| 文件 | 作用 |
| --- | --- |
| `CMakeLists.txt` | 定义工程源码、头文件路径、编译参数、链接参数和输出文件 |
| `cmake/arm-none-eabi-gcc.cmake` | 指定交叉编译器为 `arm-none-eabi-gcc` |
| `ld/GD32F303RCT6.ld` | 定义 Flash/RAM 地址、启动入口和段布局 |
| `startup/startup_gd32f30x_hd_gcc.S` | GNU 启动文件和中断向量表 |
| `include/gd32f30x_libopt.h` | 选择需要启用的 GD32 外设库头文件 |
| `src/board.c` | 板级 GPIO、USART 初始化 |
| `src/systick.c` | 1 ms 系统节拍和延时 |
| `src/main.c` | 应用主逻辑 |

## 9. CMake 工具链文件

`firmware/cmake/arm-none-eabi-gcc.cmake` 用于告诉 CMake：这是一个裸机交叉编译工程，不是 macOS 本机程序。

核心配置：

```cmake
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

find_program(ARM_NONE_EABI_GCC arm-none-eabi-gcc REQUIRED)
find_program(ARM_NONE_EABI_OBJCOPY arm-none-eabi-objcopy REQUIRED)
find_program(ARM_NONE_EABI_SIZE arm-none-eabi-size REQUIRED)

set(CMAKE_C_COMPILER "${ARM_NONE_EABI_GCC}")
set(CMAKE_ASM_COMPILER "${ARM_NONE_EABI_GCC}")
set(CMAKE_OBJCOPY "${ARM_NONE_EABI_OBJCOPY}")
set(CMAKE_SIZE "${ARM_NONE_EABI_SIZE}")
```

关键点：

- `Generic` 表示目标不是 macOS、Linux 这类操作系统。
- `STATIC_LIBRARY` 避免 CMake 在配置阶段尝试链接可执行程序。
- `arm-none-eabi-gcc` 同时用于 C 和汇编文件。

## 10. 芯片编译参数

GD32F303RCT6 是 Cortex-M4F，因此编译参数为：

```text
-mcpu=cortex-m4
-mthumb
-mfpu=fpv4-sp-d16
-mfloat-abi=hard
```

工程宏定义：

```text
GD32F30X_HD
USE_STDPERIPH_DRIVER
HXTAL_VALUE=8000000
```

说明：

- `GD32F30X_HD`：选择 GD32F30x 高密度型号。
- `USE_STDPERIPH_DRIVER`：启用标准外设库。
- `HXTAL_VALUE=8000000`：外部晶振按 8 MHz 配置。

## 11. 链接脚本配置

GD32F303RCT6 Flash/SRAM 布局：

```ld
FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 256K
RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 48K
```

启动栈顶：

```ld
_estack = ORIGIN(RAM) + LENGTH(RAM);
```

链接脚本需要处理：

- `.isr_vector`：中断向量表，必须放在 Flash 起始地址。
- `.text`：程序代码，放入 Flash。
- `.rodata`：只读常量，放入 Flash。
- `.data`：已初始化全局变量，运行在 RAM，初值存放在 Flash。
- `.bss`：未初始化全局变量，运行前清零。
- heap/stack 保留区域。

## 12. 启动文件配置

启动文件负责芯片上电后的最早期初始化。

启动流程：

1. 从中断向量表读取初始栈顶。
2. 跳转到 `Reset_Handler`。
3. 拷贝 `.data` 段到 RAM。
4. 清零 `.bss` 段。
5. 调用 `SystemInit()` 配置系统时钟。
6. 调用 `main()`。

如果从 Keil/IAR 工程迁移到 GCC，需要注意：

- Keil 启动文件通常不能直接被 GCC 使用。
- GCC 需要 GNU 汇编格式启动文件。
- 中断向量表符号名要和 GD32 头文件、中断函数名保持一致。

## 13. C 标准库注意事项

Homebrew 的 `arm-none-eabi-gcc` 可能不附带完整 newlib 头文件，因此编译时可能出现：

```text
fatal error: stdint.h: No such file or directory
```

本项目的处理方式是在 `firmware/include/` 下提供最小裸机头文件：

```text
firmware/include/stdint.h
firmware/include/stdbool.h
```

同时链接时使用：

```text
-nostdlib
```

并只链接 GCC 内部库：

```text
-lgcc
```

这样可以满足 CMSIS 和当前固件所需的基础类型，不依赖完整 C 标准库。

如果后续需要 `printf`、`malloc`、文件 IO 等完整 C 库能力，建议改用带 newlib 的 Arm GNU Toolchain，或者单独配置 newlib/newlib-nano。

## 14. 构建固件

进入项目根目录：

```sh
cd ...
```

配置工程：

```sh
cmake -S firmware -B firmware/build -G Ninja -DCMAKE_TOOLCHAIN_FILE=.../firmware/cmake/arm-none-eabi-gcc.cmake -DCMAKE_BUILD_TYPE=Release
```

编译工程：

```sh
cmake --build firmware/build
```

成功后会生成：

```text
firmware/build/gd32f303rct6_firmware.elf
firmware/build/gd32f303rct6_firmware.bin
firmware/build/gd32f303rct6_firmware.hex
firmware/build/gd32f303rct6_firmware.map
```

本项目当前构建结果：

```text
FLASH: 25484 B / 256 KiB
RAM:   4240 B / 48 KiB
```

## 15. 烧录固件

先确认 ST-Link：

```sh
st-info --probe
```

烧录 `.bin` 文件：

```sh
st-flash --reset write firmware/build/gd32f303rct6_firmware.bin 0x08000000
```

烧录成功时会看到：

```text
Flash written and verified! jolly good!
```

如果提示：

```text
NRST is not connected --> using software reset via AIRCR
```

说明没有连接硬件复位线，工具改用软件复位。只要写入和校验成功，这不是错误。

## 16. 读回校验

为了确认固件确实写入正确，可以从芯片 Flash 读回：

```sh
st-flash read firmware/build/gd32f303rct6_firmware_readback.bin 0x08000000 3004
```

对比本地文件和读回文件：

```sh
cmp firmware/build/gd32f303rct6_firmware.bin firmware/build/gd32f303rct6_firmware_readback.bin
```

如果 `cmp` 没有输出，说明两个文件完全一致。

也可以计算 SHA256：

```sh
shasum -a 256 firmware/build/gd32f303rct6_firmware.bin firmware/build/gd32f303rct6_firmware_readback.bin
```

本项目当前校验结果：

```text
cbee46a74aaf4cbf79b1123ec4cea892f6a5cdc9b86f91249bc92ca1f08114b0  firmware/build/gd32f303rct6_firmware.bin
cbee46a74aaf4cbf79b1123ec4cea892f6a5cdc9b86f91249bc92ca1f08114b0  firmware/build/gd32f303rct6_firmware_readback.bin
```

## 17. 运行验证

当前示例固件运行后：

- RGB LED 使用 `PC0`、`PC1`、`PC2`。
- 红、绿、蓝 LED 每 `250 ms` 顺序翻转。
- USART0 使用 `PA9` TX、`PA10` RX。
- 串口参数为 `115200 8N1`。
- 串口输出启动信息和周期性 `tick`。

串口连接方式：

| USB-TTL | 开发板 |
| --- | --- |
| RX | PA9 / USART0_TX |
| TX | PA10 / USART0_RX |
| GND | GND |

串口工具可以使用：

- `screen`
- `minicom`
- `picocom`
- macOS 图形串口工具

示例：

```sh
screen /dev/tty.usbserial-xxxx 115200
```

## 18. 常见问题

### 18.1 找不到 `arm-none-eabi-gcc`

检查是否安装：

```sh
brew list --versions arm-none-eabi-gcc
```

检查命令路径：

```sh
command -v arm-none-eabi-gcc
```

如果没有输出，检查 Homebrew 是否加入 `PATH`。

### 18.2 CMake 找不到工具链文件

建议使用绝对路径：

```sh
-DCMAKE_TOOLCHAIN_FILE=.../firmware/cmake/arm-none-eabi-gcc.cmake
```

### 18.3 找不到 `stdint.h`

原因通常是当前 Arm GCC 没有完整 newlib 头文件。

本项目已经提供：

```text
firmware/include/stdint.h
firmware/include/stdbool.h
```

如果新建工程，需要保留类似处理，或者安装带 newlib 的完整 Arm GNU Toolchain。

### 18.4 ST-Link 找不到目标芯片

检查：

- 开发板是否供电。
- SWDIO/SWCLK 是否接对。
- GND 是否共地。
- 是否需要连接 NRST。
- 固件是否关闭了 SWD。

### 18.5 烧录后无法再次连接

可能原因：

- 程序改了 SWD/JTAG 引脚配置。
- 程序进入低功耗模式。
- 时钟配置错误导致芯片异常。

建议：

- 固件中保留 SWD。
- 连接 NRST。
- 按住复位执行 `st-info --probe` 或重新烧录。

## 19. 推荐工作流

日常开发推荐按以下流程：

```sh
cd ...
cmake --build firmware/build
st-flash --reset write firmware/build/gd32f303rct6_firmware.bin 0x08000000
```

需要完整重配时：

```sh
cmake -S firmware -B firmware/build -G Ninja -DCMAKE_TOOLCHAIN_FILE=.../firmware/cmake/arm-none-eabi-gcc.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build firmware/build
```

需要确认烧录内容时：

```sh
st-flash read firmware/build/gd32f303rct6_firmware_readback.bin 0x08000000 3004
cmp firmware/build/gd32f303rct6_firmware.bin firmware/build/gd32f303rct6_firmware_readback.bin
```

## 20. 总结

Mac 上配置 GD32 环境的核心是：

- 用 `arm-none-eabi-gcc` 替代 Keil 编译器。
- 用 CMake/Ninja 管理构建。
- 用自定义链接脚本定义 Flash/RAM。
- 用 GNU 启动文件完成上电初始化。
- 用 ST-Link 完成烧录和校验。
- 原厂 SDK 只读引用，不直接修改。

当前项目已经完成工具链安装、工程搭建、固件构建、ST-Link 烧录和读回校验，可以作为后续 GD32 裸机开发的基础模板。
