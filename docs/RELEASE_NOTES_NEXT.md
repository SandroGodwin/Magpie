# Magpie Experimental v0.3.0 — 2026-07-15 / Magpie 实验版 v0.3.0

## 中文

### 新增

- 加入 FSR 4.1.1 INT8 的 Zero-MV、伪 jitter 和 50% 分辨率颜色光流模式，并在 Magpie 进程内绕过 `IsSupported(device)` 检查，使其他厂商显卡也可以尝试运行。有限测试中的画质大致处于 DLSS CNN/J-K 模型水平。
- 为 FSR2、FSR3 和 XeSS 增加伪 jitter 模式。当前只有 DLSS 的伪 jitter 结果可以使用；FSR2、FSR3、FSR4 和 XeSS 的伪 jitter 结果完全不可用，不作推荐。
- 加入 MLAA。
- 加入基于捕获颜色帧近似实现的 SMAA T2x 和 SMAA 4x，两者均提供 jitter 与无 jitter 版本。

## English

### Added

- Added FSR 4.1.1 INT8 Zero-MV, synthetic-jitter, and 50%-resolution colour-optical-flow modes. A process-local bypass of `IsSupported(device)` lets non-AMD GPUs attempt to run it. Limited testing placed its image quality roughly around the DLSS CNN/J-K models.
- Added synthetic-jitter modes for FSR2, FSR3, and XeSS. Only DLSS produced usable synthetic-jitter results; the FSR2, FSR3, FSR4, and XeSS variants were visually unusable and are not recommended.
- Added MLAA.
- Added captured-colour-frame approximations of SMAA T2x and SMAA 4x, each with jittered and non-jittered variants.
