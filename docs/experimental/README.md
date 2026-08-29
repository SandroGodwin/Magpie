# Magpie 实验分支文档索引

本目录只收纳实验功能的路线图、TODO、测试矩阵和运行记录。上游通用 Wiki 文档继续保留在 `docs/` 根目录，避免实验说明与用户文档混在一起。

## 当前工作

- [DLSSNR 参数与性能短期 TODO](todos/20260829-DLSSNR-parameters-performance-short-term-TODO.md)：修正 Indicator/参数合同，跟踪异步 DAV2、四槽 bridge 与 GPU 验收。
- [DLSSNR 性能与可观测性 TODO](todos/20260829-DLSSNR-performance-observability-TODO.md)：处理严重卡顿、pass-through 仍运行 Guidance、DLSS Indicator 不可靠和 DAV2 同步瓶颈。
- [Frame Guidance 测试矩阵](testing/FRAME_GUIDANCE-TEST-MATRIX.md)：统一记录诊断视图、四组 Guidance、场景、性能和日志证据。

## 已完成的路线图

- [DLSS5 Frame Guidance 阶段 1–4](todos/completed/20260829-164413-DLSS5-FrameGuidance-short-term-TODO.md)：工程实现已完成，GPU 运行时验收转入当前 TODO。

## 交接、发布与授权

- [实验分支交接](../EXPERIMENTAL_HANDOFF_ZH.md)
- [下一版 Release Notes](../RELEASE_NOTES_NEXT.md)
- [v0.5.6 实验版说明](../RELEASE_NOTES_v0.5.6-experimental.md)
- [v0.5.3 实验版说明](../RELEASE_NOTES_v0.5.3-experimental.md)
- [实验包 README](../README-EXPERIMENTAL-RELEASE.txt)
- [第三方组件与再分发](../THIRD_PARTY_AND_REDISTRIBUTION.md)

## 目录约定

- `todos/`：按日期命名的执行清单；完成的阶段性清单归档到 `todos/completed/`。
- `testing/`：可复用测试矩阵、素材约定、截图/DDS/日志记录格式。
- 后续若增加设计说明，放入 `design/`；若增加一次性测试结果，放入 `results/YYYYMMDD/`。
