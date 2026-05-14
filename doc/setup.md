# GD32 环境实现说明

## 目标

在 macOS 上为 `GD32F303RCT6` 开发板搭建一套不依赖 Keil、不修改原始 `GD32F303RxT6 KIT` 目录的固件开发、构建、烧录和验证环境。

最终产物是可以通过 ST-Link 烧录的固件文件：

- `firmware/build/gd32f303rct6_firmware.elf`
- `firmware/build/gd32f303rct6_firmware.bin`
- `firmware/build/gd32f303rct6_firmware.hex`
- `firmware/build/gd32f303rct6_firmware.map`

## 实现方式概览

环境采用 `Homebrew + Arm GCC + CMake + Ninja + ST-Link` 实现。

整体思路如下：

1. 使用 Homebrew 安装 macOS 可用的嵌入式工具链。
2. 使用 `arm-none-eabi-gcc` 作为 Cortex-M4 裸机交叉编译器。
3. 使用 CMake 管理工程源码、编译选项、链接脚本和产物生成。
4. 使用 Ninja 作为构建后端，提高构建速度和可重复性。
5. 使用 `stlink` 工具中的 `st-info` 和 `st-flash` 识别、烧录和校验开发板。
6. 原厂 `GD32F303RxT6 KIT` 只作为只读 SDK 依赖，不在其中修改任何文件。

## 工具链安装

本机通过 Homebrew 安装以下工具：

```sh
brew install cmake ninja arm-none-eabi-gcc stlink open-ocd
```

各工具作用：

- `cmake`：生成固件工程构建系统。
- `ninja`：执行实际编译和链接。
- `arm-none-eabi-gcc`：编译 Cortex-M4 裸机 C/ASM 代码。
- `arm-none-eabi-objcopy`：从 ELF 生成 `.bin` 和 `.hex`。
- `arm-none-eabi-size`：统计 Flash 和 RAM 占用。
- `st-info`：探测 ST-Link 和目标芯片信息。
- `st-flash`：通过 ST-Link 写入、读取和校验 Flash。
- `openocd`：预留调试工具，本次烧录流程主要使用 `stlink`。

## 工具链验证

安装后验证命令：

```sh
arm-none-eabi-gcc --version
cmake --version
ninja --version
st-info --version
openocd --version
```

ST-Link 探测命令：

```sh
st-info --probe
```

实际探测结果：

```text
Found 1 stlink programmers
version:    V2J37S7
serial:     E1007200D0D2139393740544
flash:      262144 (pagesize: 2048)
sram:       65536
chipid:     0x414
dev-type:   F1xx_HD
```

这说明 ST-Link 已被 macOS 正常识别。注意：这里的 `sram: 65536` 是 STM32F1 兼容协议报告值；本板原厂 Keil 工程声明 `IRAM(0x20000000,0x0C000)`，实际链接脚本按 48 KiB SRAM 配置。

## 工程目录设计

新增工程目录位于：

```text
/Users/jasperfons/Ascent/project/project1/firmware
```

目录结构：

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

各部分职责：

- `CMakeLists.txt`：定义固件目标、SDK 路径、编译参数、链接参数和产物生成规则。
- `cmake/arm-none-eabi-gcc.cmake`：定义交叉编译器和裸机构建环境。
- `ld/GD32F303RCT6.ld`：定义 Flash、RAM、启动入口和段布局。
- `startup/startup_gd32f30x_hd_gcc.S`：GNU 汇编格式启动文件和中断向量表。
- `include/gd32f30x_libopt.h`：选择本工程实际使用的 GD32 标准外设库头文件。
- `src/main.c`：验证固件主循环。
- `src/board.c`：板级 LED 和 USART0 初始化。
- `src/systick.c`：1 ms 系统节拍和延时函数。
- `src/gd32f30x_it.c`：中断服务函数。

## 原厂 SDK 的使用方式

原厂 SDK 路径：

```text
GD32F303RxT6 KIT/3. SDK/GD32F303RCT6 Demo/Library/GD32F30x_Firmware_Library_V2.1.0/Firmware
```

本工程只引用其中的源码和头文件，不修改原目录。

引用内容包括：

- CMSIS 头文件：`Firmware/CMSIS`
- GD32 设备头文件：`Firmware/CMSIS/GD/GD32F30x/Include`
- 系统初始化源码：`system_gd32f30x.c`
- 标准外设库头文件：`Firmware/GD32F30x_standard_peripheral/Include`
- 标准外设库源码：`gd32f30x_gpio.c`、`gd32f30x_rcu.c`、`gd32f30x_usart.c`

这样做的好处：

- 保持原始 KIT 可追溯、可还原。
- 工程配置全部集中在 `firmware/` 内。
- 后续升级 SDK 时可以明确比较差异。

## 芯片和编译参数

目标芯片按 GD32F30x 高密度系列配置：

```cmake
GD32F30X_HD
USE_STDPERIPH_DRIVER
HXTAL_VALUE=8000000
```

CPU 编译参数：

```text
-mcpu=cortex-m4
-mthumb
-mfpu=fpv4-sp-d16
-mfloat-abi=hard
```

含义：

- `-mcpu=cortex-m4`：目标内核为 Cortex-M4。
- `-mthumb`：使用 Thumb 指令集。
- `-mfpu=fpv4-sp-d16`：启用 Cortex-M4F 单精度 FPU。
- `-mfloat-abi=hard`：使用硬浮点 ABI。

## 链接脚本设计

链接脚本文件：

```text
firmware/ld/GD32F303RCT6.ld
```

内存定义：

```ld
FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 256K
RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 48K
```

关键设计：

- 固件入口为 `Reset_Handler`。
- 中断向量表放在 Flash 起始地址 `0x08000000`。
- `.text`、`.rodata` 放入 Flash。
- `.data` 运行在 RAM，加载镜像放在 Flash。
- `.bss` 放入 RAM，并在启动阶段清零。
- 栈顶 `_estack` 设置为 `0x2000c000`。

## 启动文件设计

启动文件：

```text
firmware/startup/startup_gd32f30x_hd_gcc.S
```

启动流程：

1. CPU 上电后从 `0x08000000` 读取初始栈顶。
2. 跳转到 `Reset_Handler`。
3. 将 `.data` 从 Flash 拷贝到 RAM。
4. 将 `.bss` 清零。
5. 调用 `SystemInit()` 配置系统时钟。
6. 调用 `main()` 进入应用程序。

中断向量表按 GD32F30x 高密度系列排列，并为未实现的中断提供弱符号默认处理函数 `Default_Handler`。

## C 标准库头文件处理

Homebrew 的 `arm-none-eabi-gcc` 包当前没有附带完整 newlib 头文件，因此编译 CMSIS 时会找不到 `stdint.h`。

为保证裸机构建可用，本工程在 `firmware/include/` 下提供了最小 freestanding 版本：

- `firmware/include/stdint.h`
- `firmware/include/stdbool.h`

这些文件只提供当前工程和 CMSIS 所需的基础类型定义，不引入完整 C 运行库。

链接阶段也使用：

```text
-nostdlib
```

并只链接 GCC 内部运行库：

```text
-lgcc
```

这样可以避免依赖缺失的系统级 C 库。

## 板级外设配置

板级代码文件：

```text
firmware/src/board.c
```

RGB LED 引脚来自原厂 RGB LED 示例：

```text
红灯：PC0
绿灯：PC1
蓝灯：PC2
```

USART0 引脚：

```text
TX：PA9
RX：PA10
波特率：115200
格式：8N1
```

为了保证烧录和调试接口不会被关闭，初始化 GPIO 时保留 SWD：

```c
gpio_pin_remap_config(GPIO_SWJ_SWDPENABLE_REMAP, ENABLE);
```

含义是关闭 JTAG、保留 SWD，避免固件运行后 ST-Link 无法继续连接。

## 固件运行逻辑

主程序文件：

```text
firmware/src/main.c
```

运行逻辑：

1. 初始化 SysTick。
2. 初始化 LED 和 USART0。
3. 通过 USART0 输出启动信息。
4. 每 `250 ms` 顺序翻转 RGB 三个 LED。
5. 周期性通过串口输出 `tick`。

预期串口输出：

```text
GD32F303RCT6 GCC firmware ready
tick
tick
...
```

## 构建命令

进入工程根目录：

```sh
cd /Users/jasperfons/Ascent/project/project1
```

配置 CMake：

```sh
cmake -S firmware -B firmware/build -G Ninja -DCMAKE_TOOLCHAIN_FILE=/Users/jasperfons/Ascent/project/project1/firmware/cmake/arm-none-eabi-gcc.cmake -DCMAKE_BUILD_TYPE=Release
```

执行构建：

```sh
cmake --build firmware/build
```

实际构建结果：

```text
Memory region         Used Size  Region Size  %age Used
FLASH:               25484 B     256 KB       9.72%
RAM:                  4240 B      48 KB       8.63%
```

## 烧录命令

烧录前探测：

```sh
st-info --probe
```

烧录 `.bin` 到 Flash 起始地址：

```sh
st-flash --reset write firmware/build/gd32f303rct6_firmware.bin 0x08000000
```

实际烧录结果：

```text
Flash written and verified! jolly good!
NRST is not connected --> using software reset via AIRCR
```

说明：

- ST-Link 成功擦除并写入 Flash。
- `st-flash` 已完成写入校验。
- 当前 NRST 没有连接，因此使用 AIRCR 软件复位。

## 读回校验

为了确认 Flash 中内容与本地固件完全一致，执行读回：

```sh
st-flash read firmware/build/gd32f303rct6_firmware_readback.bin 0x08000000 3004
```

比较本地固件和读回固件：

```sh
cmp firmware/build/gd32f303rct6_firmware.bin firmware/build/gd32f303rct6_firmware_readback.bin
```

`cmp` 无输出，表示两个文件完全一致。

SHA256 校验：

```text
cbee46a74aaf4cbf79b1123ec4cea892f6a5cdc9b86f91249bc92ca1f08114b0  firmware/build/gd32f303rct6_firmware.bin
cbee46a74aaf4cbf79b1123ec4cea892f6a5cdc9b86f91249bc92ca1f08114b0  firmware/build/gd32f303rct6_firmware_readback.bin
```

两个 SHA256 完全一致，说明烧录内容可回溯、可验证。

## 当前验证结论

已经完成的验证：

- macOS 工具链安装成功。
- `arm-none-eabi-gcc` 可以完成 GD32 固件编译。
- CMake/Ninja 工程可以稳定生成 `.elf/.bin/.hex/.map`。
- ST-Link 可以被识别。
- 目标芯片 Flash/SRAM 信息可读取。
- 固件可以成功写入 `0x08000000`。
- 写入后 `st-flash` 自动校验成功。
- 手动读回文件与原始 `.bin` 完全一致。

需要现场观察的验证：

- RGB LED 是否按顺序闪烁。
- USART0 是否输出启动信息和 `tick`。

## 关键取舍

- 不使用 Keil，避免 Windows/MDK 环境依赖。
- 不修改 `GD32F303RxT6 KIT`，保持原始资料完整。
- 新建 `firmware/` 作为独立工程，便于版本管理和迁移。
- 使用 GCC 自有启动文件和链接脚本，避免依赖 Keil/IAR 专用启动文件。
- 使用最小标准头文件绕过 Homebrew Arm GCC 缺少 newlib 头文件的问题。
- 保留 SWD，降低固件运行后无法重新烧录的风险。

## 后续建议

1. 如果需要串口日志，接入 USB-TTL 到 `PA9/PA10` 并使用 `115200 8N1` 查看输出。
2. 如果需要源码级调试，可基于 `openocd` 增加调试配置。
3. 如果后续引入更多外设，应在 `gd32f30x_libopt.h` 中按需加入对应外设头文件，并在 `CMakeLists.txt` 中加入对应 `.c` 源文件。
4. 如果需要量产烧录，可保留 `.hex` 或 `.bin` 作为发布产物，并记录 SHA256。
