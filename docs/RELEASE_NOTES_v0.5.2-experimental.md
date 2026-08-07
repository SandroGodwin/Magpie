# Magpie Experimental v0.5.2

## 中文

本版本继续完善实验性帧生成、重启兼容性与 Renderer 内部结构。

### 新增与改进

- 新增 XeSS Frame Generation：
  - 通用显卡 x2 Zero-MV 路径。
  - Intel Arc 显卡 x2-x4 Multi-Frame Generation 路径；实际可用倍率取决于 GPU 与驱动报告的能力。
- 完善 DLSS Frame Generation 失败保护：
  - 失败时继续显示真实捕获帧。
  - 首次失败重置历史，随后最多重建一次。
  - 连续失败后仅在当前缩放会话禁用 DLSSFG，避免无限重试导致卡死。
  - 继续保留 CPU/GPU Fence 同步。
- 提升 Smooth Motion 兼容模式的重启可靠性：
  - 保留普通窗口、最小化或托盘状态。
  - 替代进程等待旧进程完全退出后再初始化。
  - 旧进程会提前释放快捷键，减少重启后的快捷键注册冲突。
- 重构 Renderer 原生后端分派：
  - DLSS、FSR、XeSS 与 RTX Video 等原生后端统一通过 `NativeEffectBackend` 和 `NativeEffectBackendFactory` 管理。
  - 帧生成 Presenter 继续作为最终呈现阶段的独立路径。
- 暂时关闭内置更新检查；本版本不会后台联网检查，也不显示手动检查更新入口。

### 注意

- XeSSFG 和 DLSSFG 都是缺少真实运动向量、深度及 UI 分离信息的实验接入，画质、延迟和稳定性不能等同于游戏原生实现。
- 不要将任何帧生成 Effect 与 NVIDIA Smooth Motion 同时使用，也不要叠加多个帧生成 Effect。
- XeSS x3/x4 多帧生成主要面向 Intel Arc；其他显卡会被限制或回退到 x2。

## English

This release further improves experimental Frame Generation, restart compatibility, and Renderer internals.

### Added and improved

- Added XeSS Frame Generation:
  - An x2 Zero-MV path for supported GPUs across vendors.
  - An Intel Arc x2-x4 Multi-Frame Generation path; the available multiplier depends on capabilities reported by the GPU and driver.
- Improved DLSS Frame Generation failure protection:
  - Real captured frames remain visible when generation fails.
  - The first failure resets history, followed by at most one feature recreation.
  - Repeated failures disable DLSSFG only for the current scaling session instead of retrying indefinitely.
  - CPU/GPU fence synchronization remains enabled.
- Improved the reliability of Smooth Motion compatibility-mode restarts:
  - Preserve normal-window, minimized, or tray state.
  - The replacement waits for the old process to exit before initialization.
  - The old process releases shortcuts early to reduce registration conflicts after restart.
- Refactored native backend dispatch inside Renderer:
  - DLSS, FSR, XeSS, RTX Video, and other native backends now share `NativeEffectBackend` and `NativeEffectBackendFactory`.
  - Frame-generation presenters remain independent terminal presentation paths.
- Temporarily disabled built-in update checking. This release does not perform background update requests or expose the manual update-check interface.

### Notes

- XeSSFG and DLSSFG are experimental colour-frame-only integrations without real motion vectors, depth, or UI separation. Image quality, latency, and stability cannot match native game integrations.
- Do not combine any Frame Generation Effect with NVIDIA Smooth Motion, and do not stack multiple Frame Generation Effects.
- XeSS x3/x4 multi-frame generation primarily targets Intel Arc; other GPUs are limited or fall back to x2.
