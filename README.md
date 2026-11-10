# STM32F103 MiniBootloader

基于 STM32F103C8T6 的串口 IAP（In-Application Programming）固件升级系统，支持通过 XMODEM-CRC 协议实现固件下载、完整性校验、Flash 暂存、APP 更新以及异常恢复。

## 1. 开发环境

- **MCU：** STM32F103C8T6
- **开发工具：** STM32CubeMX + Keil MDK5（ARM Compiler 5）
- **软件框架：** STM32 HAL
- **通信接口：** USART1（115200 bps）
- **传输协议：** XMODEM-CRC（128 字节数据包）
- **串口工具：** SecureCRT
- **固件打包：** Python

## 2. Flash 分区设计

| 分区 | 起始地址 | 容量 |
|---|---|---|
| Bootloader | 0x08000000 | 16 KB |
| APP 运行区 | 0x08004000 | 24 KB |
| 固件暂存区 | 0x0800A000 | 23 KB |
| 升级状态区 | 0x0800FC00 | 1 KB |

系统采用固件暂存机制，新固件下载并校验成功后才覆盖 APP。

**当前最大可升级固件大小为 23 KiB。**

## 3. 核心功能

- Cortex-M3 中断向量表重定位与 APP 跳转
- Flash 按页擦除、半字编程和读回校验
- XMODEM-CRC 串口文件传输
- BUP 固件封装与容量检查
- 固件 CRC16/CRC32 完整性校验
- 固件暂存及 APP 覆盖更新
- 追加式升级状态记录
- 固件安装中断后的自动恢复
- 串口命令交互与升级状态查看

## 4. 固件升级流程

```text
PC端 APP.bin
      |
      v
make_bup.py 固件打包
      |
      v
SecureCRT XMODEM-CRC 发送
      |
      v
STM32 Bootloader 接收
      |
      v
Flash 暂存区 (23KB)
      |
      v
固件 CRC32 校验
      |
      v
记录升级状态
      |
      v
覆盖 APP 运行区
      |
      v
完整性校验与 APP 跳转
```

## 5. 工程目录

```text
MiniBootloader/
├── Bootloader/       # Bootloader 工程
├── UsrCode/          # 用户 APP 示例工程
├── make_bup.py       # 固件打包脚本
├── UsrCode.bin       # APP 二进制文件
├── UsrCode.bup       # 固件升级包
└── README.md
```

## 6. 固件升级操作

**第一步：编译用户程序**

在 Keil 中编译 `UsrCode`，APP 起始地址为 `0x08004000`，并生成 `UsrCode.bin`。

**第二步：生成升级包**

```bash
python make_bup.py UsrCode.bin UsrCode.bup
```

**第三步：串口升级**

使用 SecureCRT 连接 USART1，在 Bootloader 菜单中输入 `u`，选择：

`Transfer → Send Xmodem`

选择 `UsrCode.bup`，使用 128 字节 XMODEM-CRC 模式发送。

**第四步：验证固件**

等待 Bootloader 完成暂存区校验、APP 安装及最终校验。升级成功后可通过命令 `j` 或自动启动运行 APP。

## 7. 已完成测试

| 测试内容 | 结果 |
|---|---|
| 正常 XMODEM 固件升级 | 通过 |
| 固件传输过程中断电 | 通过 |
| APP 擦除后软件复位恢复 | 通过 |
| APP 部分写入后软件复位恢复 | 通过 |
| CRC32 错误固件拒绝安装 | 通过 |
| 损坏固件不覆盖旧 APP | 通过 |
| 多次升级元数据追加记录 | 通过 |

以上结果基于当前开发板的阶段性实验，尚不代表已完成全部真实掉电与极端异常条件验证。

## 8. 后续开发计划

- 完善升级状态标记异常判断
- 增加固件版本管理与兼容性检查
- 完善升级状态区回收机制
- 增加异常固件和越界保护测试
- 完善真实断电情况下的恢复验证
- 补充系统架构图、升级时序图和演示视频

## 9. 项目说明

本项目用于学习和实践 STM32 底层开发、Bootloader 设计、Flash 存储管理、串口传输协议及嵌入式系统可靠性设计。

目前已实现基本的 IAP 固件升级与自动恢复流程，后续持续优化。
