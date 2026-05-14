# GD32 固件烧录实操记录

## 1. 目的

本文档记录一次完整的 GD32 固件烧录流程，包含：

- 构建固件
- 探测 ST-Link
- 烧录固件到 Flash
- 从 Flash 读回固件
- 校验读回内容和本地固件是否一致

本次操作对象：

```text
项目目录：/Users/jasperfons/Ascent/project/project1
芯片目标：GD32F303RCT6 / GD32F30x HD
烧录工具：ST-Link
固件文件：firmware/build/gd32f303rct6_firmware.bin
烧录地址：0x08000000
```

## 2. 前置条件

确认已经安装以下工具：

```sh
brew install cmake ninja arm-none-eabi-gcc stlink open-ocd
```

确认命令可用：

```sh
arm-none-eabi-gcc --version
cmake --version
ninja --version
st-info --version
st-flash --version
```

硬件连接要求：

| ST-Link | GD32 开发板 |
| --- | --- |
| SWDIO | SWDIO |
| SWCLK | SWCLK |
| GND | GND |
| 3.3V | 3.3V 参考电压 |
| NRST | 可选 |

注意：必须共地，否则 ST-Link 可能能被 Mac 识别，但无法识别目标芯片。

## 3. 进入项目目录

执行：

```sh
cd /Users/jasperfons/Ascent/project/project1
```

后续命令都在该目录下执行。

## 4. 第一步：构建固件

执行：

```sh
cmake --build firmware/build
```

本次实际输出：

```text
ninja: no work to do.
```

说明：

- 构建系统检查后发现当前固件已经是最新的。
- 可直接使用现有产物进行烧录。

当前烧录使用的文件：

```text
firmware/build/gd32f303rct6_firmware.bin
```

如果第一次构建或 `firmware/build` 不存在，需要先执行完整配置命令：

```sh
cmake -S firmware -B firmware/build -G Ninja -DCMAKE_TOOLCHAIN_FILE=/Users/jasperfons/Ascent/project/project1/firmware/cmake/arm-none-eabi-gcc.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build firmware/build
```

## 5. 第二步：探测 ST-Link 和目标芯片

执行：

```sh
st-info --probe
```

本次实际输出：

```text
Found 1 stlink programmers
  version:    V2J37S7
  serial:     E1007200D0D2139393740544
  flash:      262144 (pagesize: 2048)
  sram:       65536
  chipid:     0x414
  dev-type:   F1xx_HD
```

结果解释：

- `Found 1 stlink programmers`：Mac 已识别到 1 个 ST-Link。
- `flash: 262144`：目标芯片 Flash 为 256 KiB。
- `sram: 65536`：这是 ST-Link 通过 STM32F1 兼容协议报告的 SRAM 信息，不作为链接脚本依据。
- `dev-type: F1xx_HD`：stlink 使用 STM32F1 高密度兼容协议识别该 GD32 芯片。

注意：后续调试确认本板原厂 Keil 工程声明 `IRAM(0x20000000,0x0C000)`，即 48 KiB 可用 SRAM。链接脚本应使用 `RAM LENGTH = 48K`，否则栈顶会落到无效地址并在 `SystemInit()` 前后触发 HardFault。

看到这个输出后，可以继续烧录。

如果没有看到目标芯片信息，先检查：

- 开发板是否供电。
- SWDIO/SWCLK 是否接对。
- ST-Link 和开发板是否共地。
- USB 线是否支持数据传输。
- 必要时连接 NRST 或按住复位再探测。

## 6. 第三步：烧录固件

执行：

```sh
st-flash --reset write firmware/build/gd32f303rct6_firmware.bin 0x08000000
```

命令含义：

- `st-flash`：使用 ST-Link 执行 Flash 操作。
- `--reset`：写入完成后复位目标芯片。
- `write`：写入文件到芯片 Flash。
- `firmware/build/gd32f303rct6_firmware.bin`：要烧录的二进制固件。
- `0x08000000`：GD32 内部 Flash 起始地址。

本次实际输出：

```text
st-flash 1.8.0
2026-05-15T00:07:43 INFO common.c: F1xx_HD: 64 KiB SRAM, 256 KiB flash in at least 2 KiB pages.
file firmware/build/gd32f303rct6_firmware.bin md5 checksum: a7ef2964b7252eefa5a451875d88bd, stlink checksum: 0x0003f6fc
2026-05-15T00:07:43 INFO common_flash.c: Attempting to write 3004 (0xbbc) bytes to stm32 address: 134217728 (0x8000000)
-> Flash page at 0x8000000 erased (size: 0x800)
-> Flash page at 0x8000800 erased (size: 0x800)
2026-05-15T00:07:43 INFO flash_loader.c: Starting Flash write for VL/F0/F3/F1_XL
2026-05-15T00:07:43 INFO flash_loader.c: Successfully loaded flash loader in sram
2026-05-15T00:07:43 INFO flash_loader.c: Clear DFSR
2026-05-15T00:07:43 INFO flash_loader.c: Clear CFSR
2026-05-15T00:07:43 INFO flash_loader.c: Clear HFSR
  1/2   pages written
  2/2   pages written
2026-05-15T00:07:43 INFO common_flash.c: Starting verification of write complete
2026-05-15T00:07:43 INFO common_flash.c: Flash written and verified! jolly good!
2026-05-15T00:07:43 INFO common.c: NRST is not connected --> using software reset via AIRCR
```

关键结果：

```text
Flash written and verified! jolly good!
```

这表示：

- Flash 已擦除。
- 固件已写入。
- 写入后 stlink 已自动验证成功。

关于这条提示：

```text
NRST is not connected --> using software reset via AIRCR
```

这不是烧录失败。它表示当前 ST-Link 没有连接硬件复位线 `NRST`，所以工具使用软件复位。只要前面出现 `Flash written and verified`，烧录就是成功的。

## 7. 第四步：读回 Flash

为了进一步确认芯片中的内容和本地固件完全一致，执行读回：

```sh
st-flash read firmware/build/gd32f303rct6_firmware_readback.bin 0x08000000 3004
```

命令含义：

- `read`：从芯片 Flash 读取内容。
- `firmware/build/gd32f303rct6_firmware_readback.bin`：读回保存的文件。
- `0x08000000`：读取起始地址。
- `3004`：读取长度，单位字节，对应当前固件大小。

本次实际输出：

```text
st-flash 1.8.0
2026-05-15T00:07:57 INFO common.c: F1xx_HD: 64 KiB SRAM, 256 KiB flash in at least 2 KiB pages.
2026-05-15T00:07:57 INFO common.c: read from address 0x08000000 size 3004
```

读回文件：

```text
firmware/build/gd32f303rct6_firmware_readback.bin
```

## 8. 第五步：比较本地固件和读回固件

执行：

```sh
cmp firmware/build/gd32f303rct6_firmware.bin firmware/build/gd32f303rct6_firmware_readback.bin
```

本次实际结果：

```text
无输出
```

说明：

- `cmp` 没有输出，表示两个文件完全一致。
- 如果文件不同，`cmp` 会输出第一个不同字节的位置。

## 9. 第六步：计算 SHA256

执行：

```sh
shasum -a 256 firmware/build/gd32f303rct6_firmware.bin firmware/build/gd32f303rct6_firmware_readback.bin
```

本次实际输出：

```text
cbee46a74aaf4cbf79b1123ec4cea892f6a5cdc9b86f91249bc92ca1f08114b0  firmware/build/gd32f303rct6_firmware.bin
cbee46a74aaf4cbf79b1123ec4cea892f6a5cdc9b86f91249bc92ca1f08114b0  firmware/build/gd32f303rct6_firmware_readback.bin
```

两个 SHA256 完全一致，说明：

- 本地固件文件和芯片 Flash 读回文件完全一致。
- 本次烧录结果可回溯、可验证。

## 10. 最短烧录命令

如果只是日常开发后快速烧录，使用下面两条命令即可：

```sh
cd /Users/jasperfons/Ascent/project/project1
st-flash --reset write firmware/build/gd32f303rct6_firmware.bin 0x08000000
```

如果代码有改动，烧录前先构建：

```sh
cmake --build firmware/build
st-flash --reset write firmware/build/gd32f303rct6_firmware.bin 0x08000000
```

## 11. 本次结论

本次完整流程结论：

- 固件构建状态正常。
- ST-Link 正常识别。
- 目标芯片正常识别。
- 固件成功写入 `0x08000000`。
- `st-flash` 自动校验成功。
- 手动读回文件和原始 `.bin` 完全一致。
- SHA256 校验一致。

因此，本次 GD32 固件烧录成功。

## 12. 后续现场验证

烧录完成后，板子上应运行当前示例固件：

- RGB LED 使用 `PC0`、`PC1`、`PC2`。
- 三个 LED 每 `250 ms` 顺序翻转。
- USART0 使用 `PA9` / `PA10`，波特率 `115200 8N1`。
- 串口会输出启动信息和周期性 `tick`。

如果需要观察串口，连接 USB-TTL：

| USB-TTL | GD32 开发板 |
| --- | --- |
| RX | PA9 / USART0_TX |
| TX | PA10 / USART0_RX |
| GND | GND |

串口打开示例：

```sh
screen /dev/tty.usbserial-xxxx 115200
```
