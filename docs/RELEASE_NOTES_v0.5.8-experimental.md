# Magpie Experimental v0.5.8

这是基于 v0.5.7 的 x64 实验版本，保留相同的实验功能和配套发布资产，并为窗口模式缩放增加可直接选择的 1x 初始缩放倍率。

## 下载与安装

1. 下载 `Magpie-Experimental-x64.zip`。
2. 完全退出正在运行的 Magpie。
3. 将 ZIP 完整解压到新目录，不要直接在压缩包内运行，也不建议覆盖旧实验版目录。
4. 运行解压目录中的 `Magpie.exe`。

已有配置会自动迁移，原有的“自动”、1.25x、1.5x、1.75x、2x、3x 和“自定义”选项不会改变含义。

## 自 v0.5.7 以来的变化

### 窗口模式缩放

- “初始缩放倍数”下拉框新增 `1x` 预设，无需再选择“自定义”并手动输入 1。
- `1x` 映射到精确的 1.0 缩放倍率，适合 DLSSNR AI Filter、RTX Video 降噪等同分辨率滤镜，避免为了启用滤镜而先对画面重采样。
- 其他初始倍率和“自动”模式的行为保持不变；旧配置中的枚举值保持兼容，无需迁移。

1x 仍沿用现有窗口模式缩放框架。启用裁剪、使用自绘非客户区窗口或在 Windows 10 上使用 WGC 时，可能看到源窗口边缘或捕获边框。

## 沿用的实验功能

- `DLSS SR_Experimental`、`DLSS FG_Experimental` 和 `DLSSNR AI Filter`。
- XeSS FG、FSR 2/3 Zero-MV、XeSS Zero-MV、RTX Video VSR/降噪及实验性 Frame Guidance。
- v0.5.7 中的 DLSSNR 参数、DLSS 组合兼容性和 Frame Generation 稳定性改进均完整保留。

## 使用建议

- 不要同时启用 DLSS FG、XeSS FG、NVIDIA Smooth Motion 或其他帧生成方案。
- Magpie 的 Motion 和 Depth 来自捕获颜色画面的估算，不能等同于游戏引擎原生数据。
- DLSSNR 是同分辨率 SDR 后处理，不负责放大；HDR 暂不支持。
- 日志位于 `logs\magpie.log`，可搜索 `DLSSNR STATUS` 确认 DLSSNR 创建和执行状态。

## DLSSNR DLL 选择

主分发包继续携带社区修改的 `nvngx_dlssnr.dll` 310.8.0.0，目标是同时兼容 RTX 40 系和 RTX 50 系。该文件不是保持 NVIDIA 原始签名完整性的官方 DLL，Windows Authenticode 会报告哈希不匹配。

配套的 `DLSSNR-DLL-Options-310.8.0.0.zip` 同时包含 NVIDIA 原版签名文件和主分发包使用的社区兼容文件。切换 DLL 前请完全退出 Magpie。

`NGX_OTA_Switch.bat`、`Toggle_DLSS_Debug_Info.bat` 和可导入的缩放模式文件继续作为独立发布资产提供，内容与 v0.5.7 相同。

## English summary

Magpie Experimental v0.5.8 keeps the v0.5.7 feature set and release assets, and adds a directly selectable `1x` initial scale factor for windowed mode. The preset maps to an exact 1.0 scale and is intended for same-resolution filters such as DLSSNR AI Filter and RTX Video denoising. Existing saved scale-factor values remain compatible.
