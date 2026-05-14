# GD32F303RCT6 GCC Firmware

在 macOS 上为 GD32F303RCT6 开发板构建的独立固件工程，不依赖 Keil / Windows。

## 硬件

| 项目 | 说明 |
|------|------|
| MCU | GD32F303RCT6 (Cortex-M4F, 256 KiB Flash, 48 KiB SRAM) |
| 屏幕 | 1.47 寸 ST7789V3 LCD (320x172, 横屏) |
| 烧录器 | ST-Link V2 |
| 晶振 | 8 MHz HXTAL → 120 MHz PLL |

## 工具链 (macOS + Homebrew)

```sh
brew install cmake ninja arm-none-eabi-gcc stlink open-ocd
```

## 构建

```sh
cd firmware
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## 烧录

```sh
# 探测
st-info --probe

# 烧录
st-flash --reset write build/gd32f303rct6_firmware.bin 0x08000000
```

## 当前固件

手表风格界面，蓝黑色背景，显示：

- `HELLO JASPER` 标题
- 实时时间 `HH:MM` + 秒
- 星期缩写 + 日期 (从固件构建时间开始走时)

## 工程结构

```
firmware/
├── CMakeLists.txt          # 构建定义
├── cmake/                  # 交叉编译工具链
├── include/                # 头文件 (board, spi, systick, main)
├── ld/                     # 链接脚本 (48 KiB SRAM)
├── src/
│   ├── main.c              # 手表界面主逻辑
│   ├── board.c             # LED / USART 板级驱动
│   ├── lcd_spi.c           # SPI2 LCD 适配层
│   ├── systick.c           # SysTick 毫秒时钟
│   └── gd32f30x_it.c       # 中断服务
└── startup/                # GCC 启动汇编
```

## 文档

| 文件 | 内容 |
|------|------|
| [doc/setup.md](doc/setup.md) | 环境实现说明 |
| [doc/tutorial.md](doc/tutorial.md) | Mac 配置 GD32 开发环境教程 |
| [doc/flashing.md](doc/flashing.md) | 固件烧录实操记录 |
| [doc/lcd-watch.md](doc/lcd-watch.md) | LCD 手表界面配置记录 |
| [doc/firmware_build.md](doc/firmware_build.md) | 构建与烧录简要记录 |

## 注意

- 原厂 KIT 目录 `GD32F303RxT6 KIT` 为只读依赖，不在其中修改文件。
- ST-Link 报告 SRAM 为 64 KiB，但本板实际可用 48 KiB (参考 Keil 工程 `IRAM(0x20000000,0x0C000)`)。
- 当前时间从固件编译时间开始，断电后重置，未接入 RTC 校时。
