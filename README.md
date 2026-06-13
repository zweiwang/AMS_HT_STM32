# AMS_HT - STM32 温湿度监控加热系统

基于 STM32F103C8T6 的温湿度监测与加热控制系统，通过 AHT20/BMP280 传感器采集环境数据，MAX7219 数码管实时显示，EC11 旋转编码器交互操作，ESP8266 WiFi 模块提供 HTTP 数据接口。

## 硬件组成

| 模块 | 型号 | 接口 | 功能 |
|------|------|------|------|
| MCU | STM32F103C8T6 | - | 主控芯片 |
| 温湿度传感器 | AHT20 | I2C1 | 温度 ±0.3°C，湿度 ±2%RH |
| 气压传感器 | BMP280 | I2C1 | 温度 + 气压 (kPa) |
| 数码管驱动 | MAX7219 ×2 | SPI1 | 8位7段数码管，共16位 |
| 旋转编码器 | EC11 | GPIO (PB12/13/14) | 旋钮调节 + 按键确认 |
| WiFi 模块 | ESP8266 | USART1 | HTTP 服务器 (端口 80) |
| 蜂鸣器 | 有源蜂鸣器 | GPIO (PB9) | 倒计时结束报警 |
| 加热控制 | 继电器/电热丝 | GPIO (PA8) | 低电平触发加热 |

## 接线图

```
STM32F103C8T6
├── I2C1 (PB6-SCL, PB7-SDA) → AHT20 + BMP280
├── SPI1 (PA5-SCK, PA7-MOSI) → MAX7219 ×2 (级联)
├── USART1 (PA9-TX, PA10-RX) → ESP8266
├── GPIO PB12 → EC11 CLK
├── GPIO PB13 → EC11 DT
├── GPIO PB14 → EC11 SW (按键)
├── GPIO PB9  → 蜂鸣器 (低电平触发)
├── GPIO PA8  → 加热控制 (低电平开启)
└── GPIO PC13 → LED 指示灯
```

## 功能说明

### 数码管显示（16位）

**屏1**：状态码 + 倒计时时间
- `0-00.05.00` — 正常状态，倒计时 5 分钟
- `1-00.04.30` — 加热中，剩余 4 分 30 秒
- `9-00.00.00` — ESP8266 通信错误
- `d-00.00.00` — WiFi 断开

**屏2**：3 秒轮播切换
- AHT20 温度 + 湿度：`T25.3H45.7`（25.3°C, 45.7%RH）
- BMP280 温度 + 气压：`T25.1P101`（25.1°C, 101kPa）
- IP 地址分段：`-192.168 ` / ` 050.002-`

### EC11 编码器操作

| 操作 | 功能 |
|------|------|
| 旋转 | 调节倒计时时间 (0~9999 秒)，支持加速 |
| 短按 | 启动/停止倒计时 + 加热 |
| 长按 (1秒) | 进入/退出温度阈值配置模式 |

### 温控逻辑

- 倒计时启动后持续加热
- 温度超过设定**高温阈值** → 自动停止加热
- 温度低于设定**低温阈值** (高温-10°C) → 自动恢复加热
- 倒计时归零 → 关闭加热 + 蜂鸣器报警 (2秒周期，持续10分钟)

### WiFi HTTP 接口

ESP8266 自动连接 WiFi `Plasma`，开启 TCP 服务器 (端口 80)。

**请求**：任意 HTTP GET 请求  
**响应**：512 字节固定长度，包含传感器数据和状态

```
AHT20: 25.3 C, 45.7 %RH
BMP280: 25.1 C, 101 kPa
Countdown: 00:04:30
Temp Threshold: 55-65 C
Heating: ON
```

### PC 客户端工具

`read_temp.py` / `AMS_HT_Temp.exe`：

```bash
python read_temp.py                       # 默认自动刷新 (读取上次IP)
python read_temp.py 192.168.1.100         # 指定IP，自动刷新
python read_temp.py --once                # 单次读取
python read_temp.py --new-ip              # 强制重新输入IP
python read_temp.py -l 3                  # 3秒刷新间隔
```

- 自动记录上次连接 IP (`~/.ams_ht_config.json`)
- 默认自动刷新模式，Ctrl+C 退出

## 开发环境

| 工具 | 版本 |
|------|------|
| STM32CubeMX | 6.15.0 |
| STM32CubeIDE | 1.19.0 |
| STM32Cube FW_F1 | 1.8.7 |
| GCC for ARM | 13.3 rel1 |
| Python | 3.12+ |

## 项目结构

```
AMS_HT/
├── AMS_HT.ioc              # STM32CubeMX 项目配置
├── Core/
│   ├── Inc/                # 头文件
│   │   ├── aht20.h
│   │   ├── bmp280.h
│   │   └── max7219.h
│   ├── Src/                # 源文件
│   │   ├── main.c          # 主程序 (核心逻辑)
│   │   ├── aht20.c         # AHT20 驱动
│   │   ├── bmp280.c        # BMP280 驱动
│   │   └── max7219.c       # MAX7219 驱动
│   └── Startup/            # 启动文件
├── Drivers/                # HAL 库
├── Debug/                  # 编译产物
├── reference/              # 参考代码
├── read_temp.py            # PC 客户端脚本
├── STM32F103C8TX_FLASH.ld  # 链接脚本
└── README.md
```

## 编译与烧录

```bash
# 编译
cd Debug
make -j8

# 烧录 (SWD)
STM32_Programmer_CLI.exe -c port=SWD freq=4000 reset=HWrst -w AMS_HT.elf 0x08000000 -v

# 复位
STM32_Programmer_CLI.exe -c port=SWD reset=HWrst -hardRst
```

## License

MIT License