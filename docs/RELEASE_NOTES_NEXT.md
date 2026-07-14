# Magpie Experimental v0.2.0 — 2026-07-15 / Magpie 实验版 v0.2.0

## 中文

### 新增与调整

- 加入 FSR 3.1.5 上采样实验后端，提供 Zero-MV 和 50% 分辨率颜色光流模式；不包含帧生成。
- 加入 XeSS 3.0.1 D3D12 DP4a 实验后端，提供 Zero-MV 和 50% 分辨率颜色光流模式。
- DLSS Zero-MV、Jitter 和 Optical Flow 的请求锐化值统一调整为 `0.3`。此前实际值为零初始化的 `0.0`，并非 `0.2`。当前 NGX 已将该内建参数标记为弃用，新版模型或驱动可能忽略它。
- 调整时序辅助遮罩：DLSS 当前颜色偏置 `0.5`；FSR2 Zero-MV/Optical Flow 反应遮罩分别为 `0.9`/`0.5`；FSR3 分别为 `0.8`/`0.2`；XeSS 响应像素遮罩为 `0.5`。
- 光流估算保持为 50% 分辨率，以控制 100% 分辨率估流带来的性能开销。
- README 中英文版补充 FSR3、XeSS 说明，以及各模型/后端的优缺点和推荐表。
- 首次启动的默认缩放配置新增五个独立选项：DLSS Zero-MV、DLSS Jitter、FSR2 Zero-MV、RTX Video VSR High 和 RTX Video VSR Ultra。其余实验 Effect 仍保留在手动选择列表中；已有用户配置不会被覆盖。

以上遮罩数值会影响时序历史排斥，但不是能够在不同 SDK 之间直接比较的统一“历史权重”。

### FSR4 调研结论

- FidelityFX SDK 2.3 包含官方 FSR Upscaling 4.1.1，但 AMD 文档限定于 Radeon RX 9000 和 RX 7000 系列独立显卡。
- 社区 FSR4 4.0.2 INT8 兼容路径并非 SDK 中的同一版本，也不能真正支持全部显卡；OptiScaler 不附带该运行库。
- 本版本不接入 FSR4，也不打包来源或再分发条款不明确的非官方运行库。

### 分发说明

- 当前完整实验包包含较大的 NVIDIA VFX 运行库和模型，ZIP 约为 580 MiB。
- 所有 DLSS、FSR2、FSR3 和 XeSS 时序模式仍缺少游戏引擎提供的真实深度、运动向量、曝光和投影 jitter，画质与原生接入不可直接比较。

## English

### Additions and changes

- Added an experimental FSR 3.1.5 upscaling backend with Zero-MV and 50%-resolution colour optical-flow modes. Frame generation is not included.
- Added an experimental XeSS 3.0.1 D3D12 DP4a backend with Zero-MV and 50%-resolution colour optical-flow modes.
- Set the requested DLSS sharpness to `0.3` for Zero-MV, Jitter, and Optical Flow. The previous effective value was the zero-initialized `0.0`, not `0.2`. Current NGX headers mark this built-in parameter as deprecated, so newer models or drivers may ignore it.
- Tuned temporal auxiliary masks: DLSS current-colour bias `0.5`; FSR2 Zero-MV/Optical Flow reactive masks `0.9`/`0.5`; FSR3 `0.8`/`0.2`; XeSS responsive-pixel mask `0.5`.
- Kept optical-flow estimation at 50% resolution to avoid the high cost observed at full resolution.
- Expanded both README languages with FSR3 and XeSS documentation plus a model/backend advantages, disadvantages, and recommendation table.
- Added five independent scaling modes to the first-run defaults: DLSS Zero-MV, DLSS Jitter, FSR2 Zero-MV, RTX Video VSR High, and RTX Video VSR Ultra. Other experimental Effects remain available for manual selection, and existing user configurations are not overwritten.

These masks affect temporal history rejection, but they are not a common, directly comparable "history weight" across the different SDKs.

### FSR4 research conclusion

- FidelityFX SDK 2.3 contains the official FSR Upscaling 4.1.1 provider, but AMD documents it for Radeon RX 9000- and RX 7000-series discrete GPUs.
- The community FSR4 4.0.2 INT8 compatibility path is not the same version as the SDK provider, is not universally compatible with all GPUs, and is not distributed by OptiScaler.
- FSR4 is not integrated in this release, and no unofficial runtime with unclear provenance or redistribution terms is bundled.

### Distribution notes

- The complete experimental package includes the large NVIDIA VFX runtime and models; the current ZIP is approximately 580 MiB.
- All DLSS, FSR2, FSR3, and XeSS temporal modes still lack real engine-provided depth, motion vectors, exposure, and projection jitter. Their image quality is not directly comparable with native game integrations.
