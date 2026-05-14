# GD32 Watch

<div align="center">
  <img src="lcd-photo.png" alt="LCD Watch" width="480">
</div>

在 macOS 上为 **GD32F303RCT6** 开发板实现的独立固件工程——手表风格 LCD 界面，显示实时时间与日期。

## 功能

- 手表/手环风格 LCD 界面，蓝黑色背景
- 实时显示 `HH:MM` 大号数字 + 秒数
- 星期缩写 + 日期显示
- 1.47 寸 ST7789V3 LCD，320×172 横屏
- 120 MHz 主频 (HXTAL 8 MHz × PLL)
- SysTick 毫秒时钟驱动时间递增

## 环境

macOS + Homebrew，无需 Keil 或 Windows。

```sh
brew install cmake ninja arm-none-eabi-gcc stlink
```

## 快速开始

```sh
cd firmware
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build
st-flash --reset write build/gd32f303rct6_firmware.bin 0x08000000
```

## 硬件

| 项目 | 参数 |
|------|------|
| MCU | GD32F303RCT6 (Cortex-M4F) |
| Flash | 256 KiB |
| SRAM | 48 KiB |
| 屏幕 | 1.47 inch ST7789V3 |
| 分辨率 | 320 × 172 横屏 |
| 接口 | SPI2 (PB3 SCK, PB5 MOSI) |
| 烧录 | ST-Link V2 (SWD) |

## 工程结构

```
firmware/
├── CMakeLists.txt              # 构建定义
├── cmake/                      # 交叉编译工具链
├── include/                    # 项目头文件
├── ld/                         # 链接脚本
├── sdk/                        # GD32 SDK (自包含)
│   ├── cmsis/                  #   CMSIS 核心头文件
│   ├── device/                 #   设备头文件 + system_gd32f30x
│   ├── peripheral/             #   标准外设库 (gpio/rcu/spi/usart/misc)
│   └── lcd/                    #   ST7789V3 驱动 + 字库
├── src/
│   ├── main.c                  # 手表界面主逻辑
│   ├── board.c                 # LED / USART 板级驱动
│   ├── lcd_spi.c               # SPI2 LCD 适配
│   ├── systick.c               # SysTick 毫秒时钟
│   └── gd32f30x_it.c           # 中断服务
└── startup/                    # GCC 启动汇编
```

## 屏幕截图对比

| 1.14 寸驱动 (240×135) | 1.47 寸驱动 (320×172) |
|---|---|
| 内容居中，四周对称噪点 | 内容铺满全屏，无噪点 |

## 调试记录

- **SRAM 边界**：原始链接脚本按 64 KiB 设置栈顶导致启动阶段 `HardFault`。通过 OpenOCD 读取 `CFSR/HFSR/BFAR` 定位到栈顶越界，改回 48 KiB 后正常。
- **屏幕尺寸**：最初使用 1.14 寸驱动 (`240×135`)，界面居中四周有对称噪点。切换到 1.47 寸驱动 (`320×172`) 后铺满全屏。
- **时间来源**：当前时间从固件构建时间 (`__DATE__` / `__TIME__`) 启动，通过 SysTick 每秒递增。未接入 RTC 校时，断电后重置。

## 文档

| 文件 | 内容 |
|------|------|
| [doc/setup.md](doc/setup.md) | 环境实现说明 |
| [doc/tutorial.md](doc/tutorial.md) | Mac 配置 GD32 开发教程 |
| [doc/flashing.md](doc/flashing.md) | 烧录实操记录 |
| [doc/lcd-watch.md](doc/lcd-watch.md) | LCD 手表界面配置 |
| [doc/firmware_build.md](doc/firmware_build.md) | 构建记录 |

PDF 版本见 `doc/*.pdf`。

## License

MIT
