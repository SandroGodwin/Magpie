Magpie Experimental v0.5.8 x64

安装 / Installation

1. 完全退出正在运行的 Magpie。
2. 将 ZIP 完整解压到新目录，不要在压缩包内直接运行，也不要只复制 Magpie.exe。
3. 运行 Magpie.exe。旧版本设置通常会从 LocalAppData 自动读取并迁移。

Fully exit Magpie, extract the complete ZIP into a new directory, and run
Magpie.exe. Do not run from inside the archive or copy only the executable.

主要实验效果 / Main experimental effects

- DLSS SR_Experimental：Motion 默认开启，Estimated Depth 默认关闭。
- DLSS FG_Experimental：支持 x2/x3/x4；不要与其他帧生成方案同时使用。
- DLSSNR AI Filter：同分辨率 SDR 滤镜，不负责放大，HDR 暂不支持。
- XeSS FG：通用显卡 x2 Zero-MV 路径。
- RTX Video VSR：面向视频、视觉小说和压缩画面增强。

The captured-frame Motion and Depth inputs are estimates, not engine-native
data. Visual quality and stability are not equivalent to an in-game DLSS
integration.

DLSSNR 参数建议 / DLSSNR defaults

- NR Preset: 0 Default
- NR Style: 0 Default
- NR Intensity / Local Tone / Local Structure: 1
- Skin Structure Strength: -1
- Automatic Mask / NR UI Correction: Off
- Frame Guidance: 0 Available
- Depth Inference Interval: 4

DLSSNR DLL

This package uses the community-modified nvngx_dlssnr.dll 310.8.0.0 intended
for RTX 40-series and RTX 50-series compatibility. Windows Authenticode reports
a file-hash mismatch because the binary is modified. The separate
DLSSNR-DLL-Options-310.8.0.0.zip Release asset also contains the original
NVIDIA-signed runtime.

排错 / Troubleshooting

- 日志位于 logs\magpie.log。
- 搜索 DLSSNR STATUS 确认 DLSSNR 创建和执行状态。
- NVIDIA Indicator 不保证在所有环境显示。
- 报告问题时请附上版本、GPU、驱动、效果链、分辨率和日志。

Source fork: https://github.com/SAOG0721/Magpie
Upstream: https://github.com/Blinue/Magpie

This package includes build-manifest.json, LICENSE-Magpie.txt, and
THIRD-PARTY-NOTICES.md.
