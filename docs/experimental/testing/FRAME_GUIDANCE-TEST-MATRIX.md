# Frame Guidance / DLSSNR 测试矩阵

每次测试记录 Magpie commit/工作区、GPU、驱动、分辨率、捕获方式、后端、Depth Interval 和日志路径。截图或 DDS 使用 `日期-场景-guidanceMode-frameId` 命名。

| 组别 | Guidance Mode | 预期 Provider | 关键检查 |
| --- | --- | --- | --- |
| Zero | Force Zero | 无 | 无 NVOF/DAV2 运行日志；DLSSNR Evaluate 持续成功 |
| Motion | Motion Only | NVOF | 静止接近零；水平/垂直方向和幅度正确；遮挡置信度下降 |
| Depth | Depth Only | DAV2 | 近 1 远 0；无 NVOF Execute；非推理帧 hold/reproject 行为明确 |
| Both | Available | NVOF + DAV2 | 深度 residual 稳定；无新增方向性拖尾 |

## 场景

- 静止画面
- 匀速水平平移
- 匀速垂直平移
- 快速转场
- 前景横穿
- 镜面或透明物体
- 固定 UI
- resize
- 暂停恢复
- 重复捕获帧

## 每组记录

- DLSSNR STATUS：创建路径、Feature 18、Evaluate result/计数、disabled 状态。
- Frame time：平均、P95、P99。
- Provider：运行/跳过次数、CPU/GPU 时间、显存。
- 产物：Color、Depth、Motion、Confidence、DepthResidual。
- 结论：正确、性能不合格、画质不合格或无法判断，并附日志证据。
