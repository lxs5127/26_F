# 硬件开源资料（PCB 原理图 / 接线图）

本目录存放 2026 电赛 F 题小车的硬件开源资料，**所有连线均已公开，可 1:1 复刻硬件**。

## 建议放入的文件

| 文件名 | 说明 |
| --- | --- |
| `schematic.pdf` | 原理图（PDF，推荐，便于查看） |
| `pcb_top.png` | PCB 顶层实物 / 布局图 |
| `pcb_bottom.png` | PCB 底层布线图 |
| `bom.xlsx` | 物料清单（型号 / 数量 / 封装） |
| `pinout.png` | 接口定义与对外接线图 |

> 如使用立创 EDA / Altium，可同时上传源工程文件（`.epro` / `.SchDoc` / `.PcbDoc`）并压缩为 zip。

## 接线速查

| 功能 | 外设 | 引脚 |
| --- | --- | --- |
| 电机 PWM 左 / 右 | TIMA0 CH0 / CH1 | PB14 / PA7 |
| 电机方向 左 AIN1/AIN2 | GPIO | PB7 / PB6 |
| 电机方向 右 BIN1/BIN2 | GPIO | PB9 / PB10 |
| 编码器 左 A/B | GPIO 中断 | PB4 / PB5 |
| 编码器 右 A/B | GPIO 中断 | PB11 / PB12 |
| 灰度传感器 1~8 | GPIO | PB19, PB17, PA16, PA14, PB20, PB25, PA25, PA27 |
| MPU6050 | I2C1 | SDA=PB3, SCL=PB2, INT=PB1 |
| OLED (SSD1306) | I2C0 | SDA=PA0, SCL=PA1 |
| 视觉模块 (MaixCam/K230) | UART3 | RX=PA13, TX=PA26 |
| 闭环步进驱动器 | UART1 | TX=PA8, RX=PA9 |
| VOFA+ 调试 | UART2 | TX=PB15, RX=PB16 |
| 按键 KEY1/2/3 | GPIO | PA23 / PA21 / PB18 |
| LED | GPIO | PA22 |
| SWD 调试 | Debug | PA19 / PA20 |

## 主要参数

| 项目 | 规格 |
| --- | --- |
| 主控 | MSPM0G3507（LQFP-64）@ 80 MHz |
| 时钟 | 外部 HFXT 40 MHz → SYSPLL |
| 电机 | MG513 减速电机 1:30，编码器 1560 脉冲/圈（4 倍频） |
| 轮径 | 6.5 cm |
| 电机 PWM | 20 kHz |
| 供电 | 见原理图（电机与 MCU 分开供电，共地） |

> 修改引脚后请同步更新 `mspm0-modules.syscfg`，并在 CCS 中重新构建。
