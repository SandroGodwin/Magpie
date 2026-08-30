<p align="center">
  <img src="./src/Magpie/Icons/SVG/Magpie Icon Full Disabled.svg" width="150" height="150" alt="Magpie">
</p>
<h1 align="center">Magpie Experimental</h1>

<div align="center">

[![许可协议](https://img.shields.io/github/license/Blinue/Magpie)](./LICENSE)

</div>

🌍 [English](./README.md) | **简体中文**

这是 [Blinue/Magpie](https://github.com/Blinue/Magpie) 的非官方实验分支，主要提供基于窗口捕获画面的 DLSS、DLSS Frame Generation、DLSSNR、XeSS、FSR 和 RTX Video 实验效果。它不代表 Magpie 官方，也不由上游项目提供支持。

## 下载与安装

1. 从 [GitHub Releases](https://github.com/SAOG0721/Magpie/releases) 下载最新的 `Magpie-Experimental-x64.zip`。
2. 完全退出正在运行的 Magpie。
3. 将 ZIP 完整解压到一个新目录，不要直接在压缩包内运行，也不建议覆盖旧版本目录。
4. 运行 `Magpie.exe`。

设置默认保存在 `%LOCALAPPDATA%\Magpie\config\v4\config.json`。升级程序不会自动替换已有缩放模式。

## 默认缩放模式

全新配置包含以下模式，默认选择 Lanczos：

| 模式 | 主要用途 | 硬件 |
| --- | --- | --- |
| Lanczos | 通用空间缩放 | DirectX 11 GPU |
| FSR | 通用空间放大与锐化 | DirectX 11 GPU |
| DLSS SR | 实验性 DLSS 超分辨率 | NVIDIA RTX |
| RTX Video VSR Ultra | 视频、视觉小说和压缩画面增强 | NVIDIA RTX |
| DLSSFG | 实验性 x2/x3/x4 帧生成 | NVIDIA RTX |
| XeSSFG | 通用显卡实验性 x2 帧生成 | 兼容 Intel、NVIDIA 或 AMD GPU |
| DLSSNR | 同分辨率 SDR AI 滤镜 | 支持运行时的 NVIDIA RTX |

旧版本用户可以在“缩放模式”页面导入 Release 附带的 `ScalingModes-v0.5.7-experimental.json`。该文件只追加 `DLSSFG`、`XeSSFG` 和 `DLSSNR`，不会删除或替换已有模式。

## 推荐配置

### DLSS SR

- `Use Motion Vectors`：默认开启。
- `Use Estimated Depth (Experimental)`：默认关闭，只有在具体应用中验证确实改善画面时再开启。
- 旧的 `DLSS\DLSS_ZeroMV` 配置会自动迁移到 `DLSS\DLSS_SR`，原参数和缩放方式会保留。

Magpie 无法取得游戏引擎原生运动矢量、深度、曝光、相机矩阵或 UI 分离信息。这里使用的是从捕获颜色画面估算的数据，因此不能等同于游戏原生 DLSS 接入。

### DLSS Frame Generation

- `Frame Multiplier`：可选择 x2、x3 或 x4，实际能力取决于 GPU、驱动和显示链路。
- `Use Motion Vectors`：默认开启。
- `Use Estimated Depth (Experimental)`：默认关闭。
- 不要同时启用 DLSS FG、XeSS FG、NVIDIA Smooth Motion 或其他帧生成方案。

x3/x4 需要足够高的显示器刷新率。生成目标高于显示器刷新率时，监视器无法显示所有帧，最终帧率不会简单等于输入帧率乘倍率。

### DLSSNR

DLSSNR 是同分辨率 SDR 后处理，不负责放大。它可能同时改变游戏画面、文字和 UI；HDR 暂不支持。

| 参数 | 范围 | 默认值 | 说明 |
| --- | --- | --- | --- |
| NR Preset | 0–3 | 0 | Default、Preset #1、#2、#3 |
| NR Style | 0–2 | 0 | Default、Natural、Cinematic |
| NR Intensity | 0–2 | 1 | 整体处理强度 |
| Local Tone Strength | 0–2 | 1 | 局部色调强度 |
| Local Structure Strength | 0–2 | 1 | 局部结构强度 |
| Skin Structure Strength | -1–2 | -1 | -1 使用默认行为 |
| Automatic Mask | 0/1 | 0 | 自动遮罩，默认关闭 |
| NR UI Correction | 0/1 | 0 | UI 修正，默认关闭 |
| Frame Guidance | 0–3 | 0 | Available、Force Zero、Motion Only、Depth Only |
| Depth Inference Interval | 1–8 | 4 | 估算深度的最小真实帧间隔 |

通常保持 `Frame Guidance=0 Available` 和 `Depth Inference Interval=4` 即可。运动较快时可以测试更低的 Interval，但它不会改变 NR 模型本身，只影响估算深度的更新频率和 GPU 开销。

### XeSS FG 与 RTX Video

XeSSFG 默认使用跨厂商 x2 Zero-MV 路径，目前没有接入 DLSS 使用的估算 Motion/Depth。RTX Video VSR Ultra 开销较高，建议根据目标分辨率和 GPU 余量决定是否使用。

## DLSSNR DLL

主 Release Pack 携带社区修改的 `nvngx_dlssnr.dll` 310.8.0.0，目标是兼容 RTX 40 系和 RTX 50 系。它不是保持 NVIDIA 原始签名完整性的官方文件，Windows Authenticode 会报告文件哈希不匹配。

同一 Release 还提供 `DLSSNR-DLL-Options-310.8.0.0.zip`：

- `NVIDIA-Original`：NVIDIA 原版签名文件。
- `Community-RTX40-RTX50`：Release Pack 默认使用的社区兼容文件。

切换 DLL 时，请先完全退出 Magpie，再替换程序目录中的 `nvngx_dlssnr.dll`。第三方 DLL 不包含在源码仓库或 GitHub 自动生成的源码归档中。

## NGX OTA 临时工具

`NGX_OTA_Switch.bat` 用于处理部分系统中 `nvngx_update.exe` 进程异常累积的问题。它可以临时关闭系统级 NGX OTA、结束现有更新进程，或删除设置以恢复 NVIDIA 默认行为。

该工具需要管理员权限，设置会影响系统中其他使用 NGX 的程序。禁用 OTA 可能阻止 NVIDIA 在线更新 NGX 组件，请在测试结束后按需恢复默认设置。

## 排错

- 日志位于程序目录的 `logs\magpie.log`。
- DLSSNR 是否真正创建并执行，可搜索 `DLSSNR STATUS`；NVIDIA Indicator 不保证在所有环境显示。
- 帧生成没有画面时，确认没有同时启用其他帧生成方案，并检查日志中的 `DLSSFG` 或 `XeSSFG` 错误。
- 如果效果文件或 DLL 缺失，请重新完整解压 Release，不要只复制 `Magpie.exe`。
- 报告问题时请附上 Magpie 版本、GPU、驱动版本、效果链、输入/输出分辨率和日志。

## 系统要求与许可

- Windows 10 v1903+ 或 Windows 11
- DirectX 功能级别 11
- x64 系统

Magpie 派生源码采用 [GPLv3](./LICENSE)。第三方 SDK、模型和运行库适用各自的许可证，详见 [第三方组件与再分发说明](./docs/THIRD_PARTY_AND_REDISTRIBUTION.md)。

- [实验版发布页](https://github.com/SAOG0721/Magpie/releases)
- [Magpie 上游项目](https://github.com/Blinue/Magpie)
- [上游 FAQ](https://github.com/Blinue/Magpie/wiki/FAQ)
