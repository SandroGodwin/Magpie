# Magpie Experimental v0.2.0 — 2026-07-15 / Magpie 实验版 v0.2.0

## 中文

### 新增与调整

- 加入 FSR 3.1.5 上采样实验后端，提供 Zero-MV 和 50% 分辨率颜色光流模式；不包含帧生成。
- 加入 XeSS 3.0.1 D3D12 DP4a 实验后端，提供 Zero-MV 和 50% 分辨率颜色光流模式。
- DLSS Zero-MV、Jitter 和 Optical Flow 向 NGX 请求的锐化值统一调整为 `0.3`；运行库和模型可能忽略该请求值。
- 调整全屏恒定时序辅助遮罩：DLSS 当前颜色偏置 `0.5`；FSR2 Zero-MV/Optical Flow 反应遮罩分别为 `0.9`/`0.5`；FSR3 分别为 `0.8`/`0.2`；XeSS 响应像素遮罩为 `0.5`。
- 光流估算保持为 50% 分辨率，以控制 100% 分辨率估流带来的性能开销。
- README 中英文版补充 FSR3、XeSS 说明，以及各模型/后端的优缺点和推荐表。
- 首次启动的默认缩放配置新增五个独立选项：DLSS Zero-MV、DLSS Jitter、FSR2 Zero-MV、RTX Video VSR High 和 RTX Video VSR Ultra。其余实验 Effect 仍保留在手动选择列表中；已有用户配置不会被覆盖。

以上遮罩数值会影响时序历史排斥，但不是能够在不同 SDK 之间直接比较的统一“历史权重”。

### 未包含

- 本版本未接入 FSR4，也不包含 FSR3 帧生成。

### 分发说明

- 当前完整实验包包含较大的 NVIDIA VFX 运行库和模型，ZIP 约为 580 MiB。
- 所有 DLSS、FSR2、FSR3 和 XeSS 时序模式仍缺少游戏引擎提供的真实深度、运动向量、曝光和投影 jitter，画质与原生接入不可直接比较。

## English

### Additions and changes

- Added an experimental FSR 3.1.5 upscaling backend with Zero-MV and 50%-resolution colour optical-flow modes. Frame generation is not included.
- Added an experimental XeSS 3.0.1 D3D12 DP4a backend with Zero-MV and 50%-resolution colour optical-flow modes.
- Set the DLSS sharpness value requested from NGX to `0.3` for Zero-MV, Jitter, and Optical Flow. The runtime and model may ignore this requested value.
- Tuned full-screen constant temporal auxiliary masks: DLSS current-colour bias `0.5`; FSR2 Zero-MV/Optical Flow reactive masks `0.9`/`0.5`; FSR3 `0.8`/`0.2`; XeSS responsive-pixel mask `0.5`.
- Kept optical-flow estimation at 50% resolution to avoid the high cost observed at full resolution.
- Expanded both README languages with FSR3 and XeSS documentation plus a model/backend advantages, disadvantages, and recommendation table.
- Added five independent scaling modes to the first-run defaults: DLSS Zero-MV, DLSS Jitter, FSR2 Zero-MV, RTX Video VSR High, and RTX Video VSR Ultra. Other experimental Effects remain available for manual selection, and existing user configurations are not overwritten.

These masks affect temporal history rejection, but they are not a common, directly comparable "history weight" across the different SDKs.

### Not included

- FSR4 and FSR3 Frame Generation are not integrated in this release.

### Distribution notes

- The complete experimental package includes the large NVIDIA VFX runtime and models; the current ZIP is approximately 580 MiB.
- All DLSS, FSR2, FSR3, and XeSS temporal modes still lack real engine-provided depth, motion vectors, exposure, and projection jitter. Their image quality is not directly comparable with native game integrations.
