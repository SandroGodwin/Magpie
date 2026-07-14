# Magpie 实验增强交接

更新日期：2026-07-15
面向：Magpie 项目 owner / maintainer

## 项目概况

本分支为 Magpie 增加了四组可选实验后端：

- NVIDIA DLSS-SR：Zero-MV、伪 jitter、颜色光流。
- AMD FSR 2.2.1：Zero-MV、颜色光流。
- NVIDIA RTX Video：同分辨率降噪和实际 VSR 放大。
- Intel XeSS 3.0.1：通过 D3D11/D3D12 共享资源运行通用 D3D12 DP4a Zero-MV 路径。

所有新增功能均通过构建开关控制，默认关闭。

## 目录说明

- `source`：主项目源码，后续开发和构建均以此为准。
- `release\Magpie-Experimental-x64`：持续覆盖更新的便携分发目录。
- `release\Magpie-Experimental-x64.zip`：持续覆盖更新的同名测试包。
- `dependencies`：集中保存 DLSS、FSR2、XeSS、NVIDIA VFX SDK/运行库及 OptiScaler 参考项目，不属于 Magpie 主源码。

外部 SDK 无需整体复制进分发包，但分发目录中的 DLSS、FSR2、RTX Video、CUDA/TensorRT 及 WinUI DLL 是运行依赖，不能仅因其来自 SDK 而删除。公开发布这些二进制前仍需核对各自许可证。

## 当前状态

已确认：

- Release x64 完整构建成功。
- DLSS、FSR2 和 RTX Video 均已接入 Magpie 的 Effect 管线。
- RTX Video 使用 D3D11/CUDA GPU 互操作，无 CPU 帧回读。
- RTX Video 降噪及 VSR 放大已在 RTX 5070 Ti 上成功加载和运行。
- XeSS 已完成 Release x64 编译；NVIDIA/AMD/Intel 实机兼容性和画质仍待测试。

主要限制：

- Magpie 只能取得最终颜色帧，没有游戏引擎的真实深度、运动向量和投影 jitter。
- DLSS/FSR2 Zero-MV 在动态画面中可能产生拖影或历史粘连。
- 当前 50% 分辨率颜色光流仍是研究实现，主观效果可能比 Zero-MV 更差。
- 伪 jitter 只提交元数据，不能生成真正的新亚像素信息。
- RTX Video 同分辨率降噪对干净画面变化较轻；Ultra 首次加载可能需要数分钟。

## Effect 清单

DLSS：

- `DLSS\DLSS_ZeroMV`
- `DLSS\DLSS_ZeroMV_Jitter`
- `DLSS\DLSS_OpticalFlow`

FSR2：

- `FSR2\FSR2_ZeroMV`
- `FSR2\FSR2_OpticalFlow`

FSR3（仅上采样，不含帧生成）：

- `FSR3\FSR3_ZeroMV`
- `FSR3\FSR3_OpticalFlow`

XeSS：

- `XeSS\XeSS_ZeroMV`
- `XeSS\XeSS_OpticalFlow`

RTX Video：

- `RTXVideo\RTXVideo_Denoise_Low/Medium/High/Ultra`
- `RTXVideo\RTXVideo_VSR_Low/Medium/High/Ultra`

VSR 效果建议使用 `Fit` 缩放类型。Denoise 保持输入尺寸不变。

## 关键代码

原生后端：

- `src/Magpie.Core/DLSSZeroMVUpscaler.*`
- `src/Magpie.Core/FSR2ZeroMVUpscaler.*`
- `src/Magpie.Core/XeSSZeroMVUpscaler.*`
- `src/Magpie.Core/HalfResOpticalFlow.*`
- `src/Magpie.Core/RTXVideoDenoiser.*`

接入与工程配置：

- `src/Magpie.Core/Renderer.*`
- `src/BuildOptions.props`
- `src/Common.Post.props`
- `src/Magpie.Core/Magpie.Core.vcxproj`
- `src/Magpie/Magpie.vcxproj`
- `src/Effects/Effects.vcxproj`

Effect 文件位于 `src/Effects/DLSS`、`FSR2`、`XeSS` 和 `RTXVideo`。

`RTXVideoDenoiser` 目前同时负责降噪和放大，合并前建议重命名。Renderer 现在按 Effect 名称分派原生后端，长期建议改成显式元数据。

## 构建

已验证环境：Visual Studio 2022、Windows SDK 10.0.26100、Python 3.11、Conan 2.30。

在 Visual Studio 2022 Developer PowerShell 中进入仓库根目录并执行：

```powershell
msbuild Magpie.slnx /m /nr:false /v:minimal /p:Configuration=Release /p:Platform=x64
```

本机 SDK 路径应写在 `src/BuildOptions.props.user`。该文件不应提交公共仓库，应改为示例配置或 CI 参数。

保留的外部依赖统一位于工作区的 `dependencies` 目录。

## 开源注意事项

Magpie 使用 GPLv3。建议先在官方仓库的个人 Fork 中建立实验分支，只公开源码、构建开关和依赖获取说明。

当前 NVIDIA VFX wheel 标记为专有许可证。未完成再分发条款及 GPL 兼容性审查前，不建议把现有便携 ZIP 上传 GitHub Release，也不要提交 SDK、模型、wheel 或 DLL。

同样不要提交 `BuildOptions.props.user`、`bin/`、`obj/`、`packages/`、日志和缓存。

## 建议下一步

1. 将 DLSS、FSR2、RTX Video 拆成独立提交，方便审查和回退。
2. 优先整理原生后端接口和 Effect 元数据，再考虑合入上游。
3. 为 RTX Video 增加运行库检测和首次加载提示。
4. 光流继续开发前，先验证 MV 的方向、单位和缩放约定。
5. 发布二进制前单独完成 NVIDIA 相关许可证审查。

## 输出

- 源码：`source`
- Release 输出：`bin/x64/Release/Magpie.exe`
- 本地实验分发包：`release\Magpie-Experimental-x64.zip`
- 自动构建及更新：`source\scripts\Build-Release.ps1`
- 工作区快捷入口：`Update-Release.ps1`

本仓库由 Magpie 官方仓库 Fork，实验修改保存在独立分支，以保留清晰的上游提交历史。
