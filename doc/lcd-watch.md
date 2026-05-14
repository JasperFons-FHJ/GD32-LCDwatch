# GD32 LCD 手表界面配置记录

## 1. 目标

在当前 GD32F303RCT6 开发板的 LCD 屏幕上显示：

```text
Jasper NB
```

要求仍然保持原厂 `GD32F303RxT6 KIT` 目录只读，不修改原厂文件。

## 2. 使用的 LCD 示例

最初使用 KIT 中的 1.14 寸示例，后经实测确认为 1.47 寸屏，已切换到：

```text
GD32F303RxT6 KIT/3. SDK/GD32F303RCT6 Demo/Examples/LCD 1.47inch
```

核心驱动文件：

```text
Components/Devices/TFTLCD/tftlcd.c
Components/Devices/TFTLCD/tftlcd.h
Components/Devices/TFTLCD/tft_font.h
```

这些文件仍然保留在原厂 KIT 中，本工程只在 `firmware/CMakeLists.txt` 中引用它们，不直接修改。

## 3. LCD 接线和引脚

根据 KIT 的 `LCD 1.14inch` 示例，LCD 使用 SPI2 和 GPIO 控制脚。

| 功能 | GD32 引脚 |
| --- | --- |
| SPI SCK | PB3 |
| SPI MOSI | PB5 |
| LCD DC | PB4 |
| LCD RST | PB6 |
| LCD PWR / BL | PB7 |
| 辅助控制脚 | PA15 |

注意：

- `PB3`、`PB4`、`PA15` 与 JTAG 复用相关。
- 工程中已经调用 `gpio_pin_remap_config(GPIO_SWJ_SWDPENABLE_REMAP, ENABLE)`，关闭 JTAG、保留 SWD。
- 这样既能使用 LCD 引脚，也不会关闭 ST-Link 的 SWD 烧录能力。

## 4. 工程改动

本次改动全部发生在 `firmware/` 目录。

新增文件：

```text
firmware/include/main.h
firmware/include/spi.h
firmware/src/lcd_spi.c
```

修改文件：

```text
firmware/CMakeLists.txt
firmware/include/gd32f30x_libopt.h
firmware/include/systick.h
firmware/src/systick.c
firmware/src/main.c
```

## 5. SPI2 适配

`firmware/src/lcd_spi.c` 实现了 KIT LCD 驱动需要的两个函数：

```c
void spi_lcd_init(void);
void SPI2_WriteBytes(uint8_t *pbuffer, uint32_t length);
```

配置参数：

```text
SPI：SPI2
模式：Master
传输方向：单线发送
帧大小：8 bit
极性/相位：SPI_CK_PL_HIGH_PH_2EDGE
NSS：软件控制
预分频：SPI_PSC_8
字节序：MSB first
```

## 6. CMake 接入方式

`firmware/CMakeLists.txt` 增加 LCD 驱动路径：

```cmake
set(LCD_ROOT "${PROJECT1_DIR}/GD32F303RxT6 KIT/3. SDK/GD32F303RCT6 Demo/Examples/LCD 1.14inch/Components/Devices/TFTLCD")
```

增加源码：

```cmake
src/lcd_spi.c
"${LCD_ROOT}/tftlcd.c"
"${PERIPH_ROOT}/Source/gd32f30x_spi.c"
```

增加头文件路径：

```cmake
"${LCD_ROOT}"
```

增加外设头文件：

```c
#include "gd32f30x_spi.h"
```

## 7. 显示代码

`firmware/src/main.c` 中加入：

```c
#include "spi.h"
#include "tftlcd.h"
```

初始化流程：

```c
systick_config();
board_init();
spi_lcd_init();
LCD_Init();
```

显示内容：

```c
LCD_Clear(BLACK);
BACK_COLOR = BLACK;
POINT_COLOR = CYAN;
LCD_ShowString(48U, 50U, 160U, 32U, 32U, "Jasper NB");
```

显示效果：

- 背景：黑色
- 字体颜色：青色
- 字体大小：32
- 位置：1.14 寸横屏下接近居中

## 8. 构建

执行：

```sh
cd /Users/jasperfons/Ascent/project/project1
cmake -S firmware -B firmware/build -G Ninja -DCMAKE_TOOLCHAIN_FILE=/Users/jasperfons/Ascent/project/project1/firmware/cmake/arm-none-eabi-gcc.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build firmware/build
```

本次构建结果：

```text
 FLASH: 25484 B / 256 KiB
 RAM:   4240 B / 48 KiB
```

说明：

- LCD 字库和绘图函数加入后，Flash 占用从约 3 KiB 增加到约 25 KiB。
- RAM 占用增加到约 4.2 KiB，主要包含 LCD 发送缓冲区。
- 原厂 Keil 工程声明 `IRAM(0x20000000,0x0C000)`，所以当前链接脚本按 48 KiB SRAM 设置。

构建时出现了原厂 LCD 文件的若干 warning，例如多行注释、char 下标等。这些来自 KIT 原始驱动文件，不影响本次固件生成和烧录。

## 9. 烧录

探测 ST-Link：

```sh
st-info --probe
```

实际结果：

```text
Found 1 stlink programmers
version:    V2J37S7
flash:      262144 (pagesize: 2048)
sram:       65536
chipid:     0x414
dev-type:   F1xx_HD
```

注意：`st-info` 通过 STM32F1 兼容协议报告 `sram: 65536`，但该值不能作为本板链接脚本依据。本板原厂 Keil 工程使用 48 KiB SRAM，实际调试也确认栈顶放到 `0x20010000` 会触发总线错误。

烧录：

```sh
st-flash --reset write firmware/build/gd32f303rct6_firmware.bin 0x08000000
```

实际结果：

```text
Attempting to write 25244 (0x629c) bytes to stm32 address: 134217728 (0x8000000)
13/13 pages written
Flash written and verified! jolly good!
NRST is not connected --> using software reset via AIRCR
```

说明：

- 固件已写入 Flash 起始地址 `0x08000000`。
- `st-flash` 自动校验成功。
- 当前未连接 NRST，因此使用软件复位，这不是错误。

## 10. 读回校验

读回 Flash：

```sh
st-flash read firmware/build/gd32f303rct6_firmware_readback.bin 0x08000000 25484
```

比较本地固件与读回固件：

```sh
cmp firmware/build/gd32f303rct6_firmware.bin firmware/build/gd32f303rct6_firmware_readback.bin
```

结果：

```text
无输出，表示完全一致
```

SHA256：

```text
fd4b70e163f8b8826224a1d9addb642d04829d9938f04d1fdf54921c56dfe4eb  firmware/build/gd32f303rct6_firmware.bin
fd4b70e163f8b8826224a1d9addb642d04829d9938f04d1fdf54921c56dfe4eb  firmware/build/gd32f303rct6_firmware_readback.bin
```

## 11. SRAM 栈顶修正与运行验证

LCD 版本最初烧录后没有进入 `main()`，OpenOCD 显示处理器停在 `HardFault_Handler`。

关键寄存器：

```text
CFSR = 0x00009600
HFSR = 0x40000000
BFAR = 0x2000fffc
MSP  = 0x2000ffd8
```

含义：异常发生时访问了 `0x2000fffc`，这是原链接脚本 64 KiB SRAM 配置下的栈顶附近地址。原厂 Keil 工程实际声明：

```text
IRAM(0x20000000,0x0C000)
```

也就是 48 KiB SRAM。因此已将链接脚本改为：

```ld
RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 48K
```

修正后 `_estack = 0x2000c000`。OpenOCD 验证结果：

```text
pc   = 0x08000130  // main
msp  = 0x2000c000
CFSR = 0x00000000
HFSR = 0x00000000
```

继续运行 1.5 秒后再次暂停，仍未进入 HardFault，`CFSR/HFSR` 保持为 0。

## 12. 当前结论

本次已经完成：

- 接入 KIT 的 1.14 寸 LCD 驱动。
- 配置 SPI2 和 LCD 控制 GPIO。
- 固件启动后执行 LCD 初始化。
- 屏幕清黑底。
- 显示青色 `Jasper NB`。
- 固件构建成功。
- ST-Link 烧录成功。
- Flash 读回校验一致。
- 已修正 SRAM 配置导致的早期 HardFault。

## 13. 如果屏幕没有显示

优先检查：

1. 开发板实际安装的屏幕是否为 1.14 寸。
2. 屏幕排线或插座是否接触良好。
3. ST-Link 烧录后板子是否正常复位。
4. `PB3/PB5/PB4/PB6/PB7/PA15` 是否被其他外设占用。
5. 若实际屏幕为 0.96 寸或 1.47 寸，需要切换到 KIT 对应 LCD 示例目录。

可选 LCD 示例目录：

```text
Examples/LCD 0.96inch BOE
Examples/LCD 1.14inch
Examples/LCD 1.47inch
```

当前工程默认使用：

```text
Examples/LCD 1.14inch
```

## 14. 手表风格界面更新

已将原来的 LCD 颜色循环诊断界面替换为手表/手环风格主界面。

显示内容：

- 顶部显示 `HELLO JASPER`。
- 背景使用蓝黑色。
- 中间显示大号时间 `HH:MM` 和秒数。
- 底部显示星期缩写和日期，例如 `FRI 2026-05-15`。
- 不再全屏循环刷新不同背景色，只每秒局部更新时间区域。

当前时间来源：

- 固件启动时从编译宏 `__DATE__` 和 `__TIME__` 读取构建时间。
- 运行后通过 SysTick 每秒递增。
- 当前还没有接入 RTC 校时、串口校时或网络校时，所以断电/复位后会重新从固件构建时间开始计时。

本次构建结果：

```text
FLASH: 27636 B / 256 KiB
RAM:    4240 B / 48 KiB
```

本次烧录和读回校验：

```text
Flash written and verified! jolly good!
SHA256: 1a87aec4fb68ef1dbc905cebd6b89b8bd6d9a0a436996e1b60f6f06099f438fe
```

## 15. 实际屏幕尺寸修正

观察到蓝黑色界面位于屏幕中间，四周存在对称噪点区域。该现象说明之前按 `LCD 1.14inch` 驱动写入的 `240x135` 显示窗口，只覆盖了实际屏幕中间区域。

对比 KIT 驱动配置：

- `LCD 1.14inch`：横屏 `240x135`，地址偏移 `x + 40`、`y + 53`。
- `LCD 1.47inch`：横屏 `320x172`，地址偏移 `x + 0`、`y + 34`。

实际现象与 1.47 寸屏吻合：`320x172` 屏幕中显示 `240x135` 内容，会留下左右约 40 像素、上下约 18 像素的对称未刷新区域。

已修正：

- `firmware/CMakeLists.txt` 中 `LCD_ROOT` 已切换到 `Examples/LCD 1.47inch/Components/Devices/TFTLCD`。
- 手表风格界面布局已按 `320x172` 重新放大和居中。
- 全屏清理使用 `LCD_Fill(0, 0, LCD_Width - 1, LCD_Height - 1, color)`，避免越界。

修正后构建结果：

```text
FLASH: 27524 B / 256 KiB
RAM:    4240 B / 48 KiB
```

修正后烧录和读回校验：

```text
Flash written and verified! jolly good!
SHA256: 6db7f399329685225b9eca20833d9154a0f4fa2689b419313520429f9de89386
```
