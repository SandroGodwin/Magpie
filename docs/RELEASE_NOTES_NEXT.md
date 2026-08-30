# Magpie 下一实验版 / Next Experimental Release

当前公开实验版为 [v0.5.6-experimental](RELEASE_NOTES_v0.5.6-experimental.md)。下一版版本号为 **v0.5.7-experimental**。

## v0.5.7 已确认改动

- DLSSNR 增加 `NR Preset`、`Skin Structure Strength` 和 `NR UI Correction` 参数。
- `NR Preset` 提供 `0 Default`、`1 Preset #1`、`2 Preset #2`、`3 Preset #3`，默认使用 `0 Default`。
- `NR Style` 保持 `0 Default`、`1 Natural`、`2 Cinematic`。
- `NR Intensity`、`Local Tone Strength` 与 `Local Structure Strength` 的范围统一为 `0–2`，默认值为 `1`。
- `Skin Structure Strength` 范围为 `-1–2`，默认值为 `-1`；`Automatic Mask` 与 `NR UI Correction` 默认关闭。
- `NR Reset`、`DLSSNR.Enabled` 和 addon 自有的 HDR 转换参数仍由内部管理，不作为用户参数提供。
- 主 DLSS 超分效果标识由 `DLSS\DLSS_ZeroMV` 改为 `DLSS\DLSS_SR`；旧配置会自动迁移并保留参数与缩放类型。

The current public experimental release is [v0.5.6-experimental](RELEASE_NOTES_v0.5.6-experimental.md). The next version is **v0.5.7-experimental**.

## Confirmed v0.5.7 changes

- DLSSNR now exposes `NR Preset`, `Skin Structure Strength`, and `NR UI Correction`.
- `NR Preset` offers `0 Default`, `1 Preset #1`, `2 Preset #2`, and `3 Preset #3`; its default is `0 Default`.
- `NR Style` remains `0 Default`, `1 Natural`, and `2 Cinematic`.
- `NR Intensity`, `Local Tone Strength`, and `Local Structure Strength` now use a `0–2` range with a default of `1`.
- `Skin Structure Strength` uses a `-1–2` range with a default of `-1`; `Automatic Mask` and `NR UI Correction` remain off by default.
- `NR Reset`, `DLSSNR.Enabled`, and the addon's own HDR transfer controls remain internally managed and are not exposed as user parameters.
- The main DLSS super-resolution effect identifier is now `DLSS\DLSS_SR`; existing configurations migrate automatically while preserving parameters and scaling type.
