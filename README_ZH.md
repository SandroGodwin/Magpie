<br>
<p align="center">
  <img src="./src/Magpie/Icons/SVG/Magpie Icon Full Disabled.svg" width="150px" height="150px" alt="Logo">
</p>
<h1 align="center">Magpie</h1>

<div align="center">

[![许可协议](https://img.shields.io/github/license/Blinue/Magpie)](./LICENSE)
[![build](https://github.com/Blinue/Magpie/actions/workflows/build.yml/badge.svg)](https://github.com/Blinue/Magpie/actions/workflows/build.yml)
[![All Contributors](https://img.shields.io/github/all-contributors/Blinue/Magpie)](#%E8%B4%A1%E7%8C%AE%E8%80%85-)
[![翻译状态](https://hosted.weblate.org/widget/magpie/svg-badge.svg)](https://hosted.weblate.org/engage/magpie)

</div>

🌍 [English](./README.md) | **简体中文**

Magpie 是一个轻量级的窗口超分辨率工具，内置众多高效的算法和滤镜。

## 关于这个实验性 Fork

本仓库是 [Blinue/Magpie](https://github.com/Blinue/Magpie) 的非官方实验性 Fork。Magpie 及本代码库的大部分内容由 Blinue 和上游贡献者创作。本 Fork 不代表 Magpie 官方，也不由上游项目提供支持。

Fork 维护者使用 OpenAI Codex 辅助开发和测试了以下仅依赖最终颜色帧的实验功能：

- NVIDIA DLSS Super Resolution：零运动向量、伪 jitter 元数据或 50% 分辨率颜色光流。
- AMD FidelityFX Super Resolution 2.2.1：零运动向量、伪 jitter 元数据或 50% 分辨率颜色光流。
- AMD FidelityFX Super Resolution 3.1.5 上采样（不含帧生成）：通过 D3D11/D3D12 互操作提供零运动向量、伪 jitter 元数据或 50% 分辨率颜色光流模式。
- AMD FidelityFX Super Resolution 4.1.1 INT8：提供 Zero-MV、伪 jitter 和 50% 分辨率颜色光流实验模式。
- Intel XeSS 3.0.1 Super Resolution：通过 D3D11/D3D12 互操作提供零运动向量、伪 jitter 元数据或 50% 分辨率颜色光流模式。
- NVIDIA VideoSuperRes：同分辨率降噪和 VSR 放大。
- MLAA，以及基于捕获颜色帧近似实现的 SMAA T2x/4x（含 jitter 与无 jitter 版本）。

这些接入无法获得游戏引擎提供的真实深度、运动向量、曝光、反应遮罩和投影 jitter，因此可能出现拖影、细节不稳定，效果也可能弱于游戏原生接入。它们属于研究原型，不能视为原生 DLSS 或 FSR 的替代品。

### 本次新增

- 加入 FSR 4.1.1 INT8，并在 Magpie 进程内绕过其 `IsSupported(device)` 检查，使其他厂商显卡也可以尝试运行。有限测试中的画质大致处于 DLSS CNN/J-K 模型的水平，仍属于不推荐的兼容性实验。
- 为 FSR2、FSR3、FSR4 和 XeSS 增加伪 jitter 模式。当前只有 DLSS 的伪 jitter 结果可以使用；其他模型的伪 jitter 实测画面完全不可用，不作推荐。
- 加入 MLAA，以及 SMAA T2x/4x 的捕获帧实验实现；SMAA T2x/4x 均提供 jitter 和无 jitter 版本。
- 新增可选的 Smooth Motion 兼容模式：每次缩放结束后自动重启 Magpie，以释放 NVIDIA Smooth Motion 驱动驻留的显存。默认关闭。

### 初步测试观察

以下结论来自有限设备和游戏中的主观对比，不代表普遍性能或画质结论。

日常使用建议在 `DLSS_ZeroMV`、`DLSS_ZeroMV_Jitter` 和 `RTXVideo_VSR_Ultra` 中三选一，不建议叠加。VSR Ultra 性能不足时可退到 VSR High。非 NVIDIA 显卡也可以考虑 FSR2 Zero-MV；当前 FSR3、XeSS 和所有 Optical Flow 版本则主要保留作研究和对照。

#### DLSS Super Resolution

当前实现提供 Zero-MV、伪 jitter 和 50% 分辨率颜色光流三种实验路径。在部分较老或原生抗锯齿较弱的 3D 游戏中，DLSS 对边缘平滑和画面稳定性有明显改善；测试中较适合的场景包括 Frostpunk、戴森球计划、Minecraft，以及安卓模拟器中的 3D 游戏。部分场景下的主观效果优于 FSR1。

| Effect | 实测优点 | 实测缺点 | 推荐结论 |
| --- | --- | --- | --- |
| `DLSS_ZeroMV` | 在合适的游戏中，抗锯齿和边缘平滑改善明显，额外开销相对较低，也不会引入 Jitter 模式的偶发抖动 | 动态画面可能保留错误历史并产生拖影 | 推荐，作为 DLSS 的默认首选 |
| `DLSS_ZeroMV_Jitter` | 部分测试中的拖影似乎比 Zero-MV 更少，可能是最近帧获得了更高影响力 | 偶尔出现可见抖动；源游戏投影并未真正 jitter，因此原因仍只是推测 | 推荐，与DLSS_ZeroMV为3d游戏二选一 |
| `DLSS_OpticalFlow` | 尝试用颜色运动估计对齐动态历史 | 估算光流不等于引擎运动向量；实测画质较差，额外开销不值得 | 不推荐 |

由于缺少真实深度和运动向量，动态物体、镜头移动及遮挡变化时仍可能出现拖影和历史粘连。优先在 Zero-MV 与 Jitter 两个 Effect 之间对比：Zero-MV 更稳定，Jitter 可能减少拖影但会偶发抖动。Optical Flow 不推荐。

<details>
<summary>补充：驱动层 DLSS 模型/预设覆盖</summary>

下面的 M、L、J/K 和 CNN 是通过驱动层覆盖选择的 DLSS 模型/预设，不是 Magpie 中的独立 Effect，也不改变上述三个 Effect 的主次关系。

| DLSS 模型/预设 | 实测优点 | 实测缺点 | 推荐结论 |
| --- | --- | --- | --- |
| M | AA 表现良好，综合最均衡；其时域权重似乎低于 L，因此拖影更小 | 平滑程度略弱于 L | 推荐，均衡 |
| L | AA 和边缘最平滑 | 时域拖影比 M 稍明显；用于 DLAA 或接近原生分辨率渲染时开销偏大 | 更重视平滑度且 GPU 性能充足时 |
| J/K | 可以正常运行 | 在这种仅输入颜色帧的接入方式中，没有表现出相对 M/L 的明显优势，整体效果一般 | 不推荐 |
| CNN | 时序处理相对保守 | 实测 AA 和重建表现不及 M/L，整体效果一般 | 不推荐 |

测试驱动层 DLSS 模型或预设覆盖时，建议使用 NVIDIA Profile Inspector 为 Magpie 创建独立应用 Profile。不同游戏可在 M 和 L 之间切换：M 更均衡，L 更强调平滑 AA。当前公开代码仍请求 Balanced 模式和 Preset J；驱动覆盖不属于 Magpie 内置功能。

</details>

#### FSR 2.2.1

| Effect | 实测优点 | 实测缺点 | 推荐结论 |
| --- | --- | --- | --- |
| `FSR2_ZeroMV` | 实测效果尚可，画面比较平滑；支持跨厂商，开销低于 Optical Flow | 缺少真实反遮挡数据，运动时仍可能留下历史残影 | 推荐，尤其适合无法使用 DLSS 的设备 |
| `FSR2_OpticalFlow` | 提供颜色运动输入实验 | 实测画质明显差于 Zero-MV，同时增加估流开销 | 不推荐 |

#### FSR 3.1.5 Upscaler

FSR3 使用 AMD 签名的 DirectX 12 FSR API 运行库。Magpie 仍是 D3D11 渲染器，因此该后端需要在 D3D11/D3D12 之间共享和复制资源，并通过围栏同步。这里没有接入帧生成。

| Effect | 实测优点 | 实测缺点 | 推荐结论 |
| --- | --- | --- | --- |
| `FSR3_ZeroMV` | 后端可能已经正常生效，可作为接入实验 | 当前画质较差；D3D11→D3D12 桥接增加复制和同步，也不能完全排除输入处理仍存在问题 | 不推荐 |
| `FSR3_OpticalFlow` | 验证上采样器对估算运动输入的响应 | 同时承担跨 API 与不可靠颜色光流的开销，实测结果较差 | 不推荐 |

#### NVIDIA RTX Video

RTX Video 提供同分辨率降噪和实际 VSR 放大。实测最适合 Galgame、视觉小说和流媒体视频，可以改善压缩画面、模糊线条、文字和角色边缘。

| Effect/档位 | 实测结果 | 推荐结论 |
| --- | --- | --- |
| 同分辨率 Denoise | 改善通常不明显，而且不具备放大效果 | 不推荐 |
| VSR Low | 开销较低，但画质收益有限 | 不推荐 |
| VSR Medium | 强于 Low，但相比 High/Ultra 没有足够优势 | 不推荐 |
| VSR High | 画质改善明显，开销低于 Ultra | Ultra 性能不足时推荐 |
| VSR Ultra | Galgame 和流媒体视频中的重建与降噪效果最好 | Galgame、流媒体视频优先推荐 |

代价是 RTX Video 仅支持 NVIDIA，而且 VFX 运行库和模型会显著增加分发包体积。VSR 已在开发者的 Windows 11 测试机上生效；一台 Windows 10 测试机无法创建 VideoSuperRes。GPU、驱动、系统和运行库版本都可能影响兼容性。

#### Intel XeSS 3.0.1

XeSS 使用跨厂商 D3D12 DP4a 路径，可以在兼容的 Intel、NVIDIA 和 AMD GPU 上运行。Magpie 需要与 D3D12 共享纹理和围栏；这里没有使用仅限 Intel 的原生 D3D11 XeSS 路径。

| Effect | 实测优点 | 实测缺点 | 推荐结论 |
| --- | --- | --- | --- |
| `XeSS_ZeroMV` | 后端可能已经正常生效，可用于验证跨厂商 DP4a 路径 | 当前画质较差；恒定深度无法识别反遮挡，D3D11→D3D12 桥接也增加开销和接入不确定性 | 不推荐 |
| `XeSS_OpticalFlow` | 验证 XeSS 对估算运动输入的响应 | 仍缺少真实深度，并叠加估流与跨 API 开销，实测结果较差 | 不推荐 |

#### Optical Flow 与 DLDSR

当前所有基于 50% 分辨率颜色光流的版本均不推荐，包括 DLSS、FSR2、FSR3 和 XeSS Optical Flow。颜色光流无法替代引擎提供的真实运动向量和反遮挡数据；实测没有稳定提升画质，部分模式反而明显变差，同时还会消耗额外 GPU 时间。

在 NVIDIA 平台上，如果游戏和显示模式支持，并且能够接受额外 GPU 开销，DLDSR 通常比从捕获画面估算运动更值得优先考虑。DLDSR 会让游戏以更高分辨率渲染后再缩小，并不是时域上采样的直接替代品，但它能从更可靠的原始采样开始处理。

### 外部开发依赖

本源码仓库不包含第三方 SDK、模型、wheel 或 NVIDIA 专有二进制文件。请从原作者处获取，并自行接受对应许可证：

- [Magpie 原始仓库](https://github.com/Blinue/Magpie)
- [NVIDIA DLSS SDK](https://github.com/NVIDIA/DLSS)
- [AMD FidelityFX FSR 2.2.1](https://github.com/GPUOpen-Effects/FidelityFX-FSR2/tree/v2.2.1)
- [社区 FSR2 DirectX 11 后端](https://github.com/gameplug-labs/FidelityFX-FSR2-DX11)
- [AMD FSR SDK](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK)
- [Intel XeSS SDK](https://github.com/intel/xess)
- [NVIDIA Video Effects SDK 示例和安装说明](https://github.com/NVIDIA-Maxine/VFX-SDK-Samples)
- [NVIDIA `nvidia-vfx` 软件包](https://pypi.org/project/nvidia-vfx/)

如需编译可选后端，请自行新建 `src/BuildOptions.props.user`，并在其中设置功能开关和本机 SDK 路径。该文件已被 `.gitignore` 排除，只用于本机配置，不应提交到公共仓库。

### 源码与二进制分发

Magpie 派生源码继续采用 GPLv3，发布源码时必须保留上游版权和许可证声明。AMD FSR2/FSR3 和 Intel XeSS 具有各自的许可声明与条款；NVIDIA SDK 和运行库仍受 NVIDIA 专有条款约束。

第三方文件可以下载，并不自动代表可以再次分发。不要将 SDK 目录、模型、wheel 或 NVIDIA DLL 提交进本仓库。把 GPLv3 Magpie 程序与 NVIDIA 专有组件组合成公开二进制 Release，仍需单独审核许可证兼容性和再分发权限。在审核完成前，建议只发布源码；或者发布不包含专有组件的构建，让用户从 NVIDIA 获取允许使用的依赖。本段是项目维护层面的谨慎建议，不构成法律意见。

👉 [Magpie 官方版本下载](https://github.com/Blinue/Magpie/releases)

👉 [FAQ](https://github.com/Blinue/Magpie/wiki/FAQ)

👉 [内置效果介绍](https://github.com/Blinue/Magpie/wiki/内置效果介绍)

👉 [编译指南](https://github.com/Blinue/Magpie/wiki/编译指南)

## 功能

* 支持全屏和窗口模式缩放
* 众多内置算法和滤镜，如 [Anime4K](https://github.com/bloc97/Anime4K)、[FSR](https://github.com/GPUOpen-Effects/FidelityFX-FSR)、CRT 着色器等
* 基于 WinUI 的用户界面，支持浅色和深色主题
* 支持多屏幕

## 截图

<div style="display:flex; gap:10px;">
  <img src="img/main-window-zh.png" alt= "Main window" height="300">
  <img src="img/screenshot.png" alt= "Main window" height="300">
</div>

## 使用提示

1. 如果你设置了 DPI 缩放，而要放大的窗口没有高 DPI 支持（这在老游戏中很常见），推荐首先进入该程序的兼容性设置，将“高 DPI 缩放替代”设置为“应用程序”。
2. 一些游戏支持调整窗口的大小，但只使用简单的缩放算法，这时请先将其设为原始（最佳）分辨率。

## 系统需求

1. Windows 10 v1903+ 或 Windows 11
2. DirectX 功能级别 11

## 本地化

感谢 [Weblate](https://weblate.org) 提供托管服务！点击下面的图片可以进入翻译页面。

[![翻译状态](https://hosted.weblate.org/widget/magpie/multi-auto.svg)](https://hosted.weblate.org/engage/magpie)

## 贡献者

衷心感谢所有为本项目做出贡献的人：

<!-- ALL-CONTRIBUTORS-LIST:START - Do not remove or modify this section -->
<!-- prettier-ignore-start -->
<!-- markdownlint-disable -->
<table>
  <tbody>
    <tr>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/Blinue"><img src="https://avatars.githubusercontent.com/u/34770031?v=4?s=100" width="100px;" alt="Xu"/><br /><sub><b>Xu</b></sub></a><br /><a href="#maintenance-Blinue" title="Maintenance">🚧</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/hooke007"><img src="https://avatars.githubusercontent.com/u/41094733?v=4?s=100" width="100px;" alt="hooke007"/><br /><sub><b>hooke007</b></sub></a><br /><a href="https://github.com/Blinue/Magpie/commits?author=hooke007" title="Documentation">📖</a> <a href="#question-hooke007" title="Answering Questions">💬</a> <a href="#userTesting-hooke007" title="User Testing">📓</a> <a href="https://github.com/Blinue/Magpie/commits?author=hooke007" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="http://palxex.ys168.com"><img src="https://avatars.githubusercontent.com/u/58222?v=4?s=100" width="100px;" alt="Pal Lockheart"/><br /><sub><b>Pal Lockheart</b></sub></a><br /><a href="#userTesting-palxex" title="User Testing">📓</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://www.stevedonaghy.com/"><img src="https://avatars.githubusercontent.com/u/1029699?v=4?s=100" width="100px;" alt="Steve Donaghy"/><br /><sub><b>Steve Donaghy</b></sub></a><br /><a href="https://github.com/Blinue/Magpie/commits?author=neoKushan" title="Code">💻</a> <a href="#translation-neoKushan" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="http://gyrojeff.top"><img src="https://avatars.githubusercontent.com/u/30655701?v=4?s=100" width="100px;" alt="gyro永不抽风"/><br /><sub><b>gyro永不抽风</b></sub></a><br /><a href="https://github.com/Blinue/Magpie/commits?author=JeffersonQin" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/ButtERRbrod"><img src="https://avatars.githubusercontent.com/u/89013889?v=4?s=100" width="100px;" alt="ButtERRbrod"/><br /><sub><b>ButtERRbrod</b></sub></a><br /><a href="#translation-ButtERRbrod" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/0x4E69676874466F78"><img src="https://avatars.githubusercontent.com/u/4449851?v=4?s=100" width="100px;" alt="NightFox"/><br /><sub><b>NightFox</b></sub></a><br /><a href="#translation-0x4E69676874466F78" title="Translation">🌍</a></td>
    </tr>
    <tr>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/Tzugimaa"><img src="https://avatars.githubusercontent.com/u/4981077?v=4?s=100" width="100px;" alt="Tzugimaa"/><br /><sub><b>Tzugimaa</b></sub></a><br /><a href="https://github.com/Blinue/Magpie/commits?author=Tzugimaa" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/WHMHammer"><img src="https://avatars.githubusercontent.com/u/35433952?v=4?s=100" width="100px;" alt="WHMHammer"/><br /><sub><b>WHMHammer</b></sub></a><br /><a href="#translation-WHMHammer" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/kato-megumi"><img src="https://avatars.githubusercontent.com/u/29451351?v=4?s=100" width="100px;" alt="kato-megumi"/><br /><sub><b>kato-megumi</b></sub></a><br /><a href="https://github.com/Blinue/Magpie/commits?author=kato-megumi" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/MikeWang000000"><img src="https://avatars.githubusercontent.com/u/11748152?v=4?s=100" width="100px;" alt="Mike Wang"/><br /><sub><b>Mike Wang</b></sub></a><br /><a href="#userTesting-MikeWang000000" title="User Testing">📓</a></td>
      <td align="center" valign="top" width="14.28%"><a href="http://sammyhori.com"><img src="https://avatars.githubusercontent.com/u/116026761?v=4?s=100" width="100px;" alt="Sammy Hori"/><br /><sub><b>Sammy Hori</b></sub></a><br /><a href="#translation-sammyhori" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/NeilTohno"><img src="https://avatars.githubusercontent.com/u/28284594?v=4?s=100" width="100px;" alt="NeilTohno"/><br /><sub><b>NeilTohno</b></sub></a><br /><a href="#translation-NeilTohno" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/a0193143"><img src="https://avatars.githubusercontent.com/u/32773311?v=4?s=100" width="100px;" alt="a0193143"/><br /><sub><b>a0193143</b></sub></a><br /><a href="#translation-a0193143" title="Translation">🌍</a></td>
    </tr>
    <tr>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/soulset001"><img src="https://avatars.githubusercontent.com/u/121711747?v=4?s=100" width="100px;" alt="soulset001"/><br /><sub><b>soulset001</b></sub></a><br /><a href="#translation-soulset001" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/WluhWluh"><img src="https://avatars.githubusercontent.com/u/52004526?v=4?s=100" width="100px;" alt="WluhWluh"/><br /><sub><b>WluhWluh</b></sub></a><br /><a href="#design-WluhWluh" title="Design">🎨</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/SerdarSaglam"><img src="https://avatars.githubusercontent.com/u/42881121?v=4?s=100" width="100px;" alt="Serdar Sağlam"/><br /><sub><b>Serdar Sağlam</b></sub></a><br /><a href="#translation-SerdarSaglam" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/AndrusGerman"><img src="https://avatars.githubusercontent.com/u/30560543?v=4?s=100" width="100px;" alt="Andrus Diaz German"/><br /><sub><b>Andrus Diaz German</b></sub></a><br /><a href="#translation-AndrusGerman" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/Kefir2105"><img src="https://avatars.githubusercontent.com/u/103105829?v=4?s=100" width="100px;" alt="Kefir2105"/><br /><sub><b>Kefir2105</b></sub></a><br /><a href="#translation-Kefir2105" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/animeojisan"><img src="https://avatars.githubusercontent.com/u/132756551?v=4?s=100" width="100px;" alt="animeojisan"/><br /><sub><b>animeojisan</b></sub></a><br /><a href="#translation-animeojisan" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/MuscularPuky"><img src="https://avatars.githubusercontent.com/u/93962018?v=4?s=100" width="100px;" alt="MuscularPuky"/><br /><sub><b>MuscularPuky</b></sub></a><br /><a href="#translation-MuscularPuky" title="Translation">🌍</a></td>
    </tr>
    <tr>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/Zoommod"><img src="https://avatars.githubusercontent.com/u/71239440?v=4?s=100" width="100px;" alt="Zoommod"/><br /><sub><b>Zoommod</b></sub></a><br /><a href="#translation-Zoommod" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/fil08"><img src="https://avatars.githubusercontent.com/u/125665523?v=4?s=100" width="100px;" alt="fil08"/><br /><sub><b>fil08</b></sub></a><br /><a href="#translation-fil08" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/IsaiasYang"><img src="https://avatars.githubusercontent.com/u/20205571?v=4?s=100" width="100px;" alt="攸羚"/><br /><sub><b>攸羚</b></sub></a><br /><a href="https://github.com/Blinue/Magpie/commits?author=IsaiasYang" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="http://ohaiibuzzle.dev"><img src="https://avatars.githubusercontent.com/u/23693150?v=4?s=100" width="100px;" alt="OHaiiBuzzle"/><br /><sub><b>OHaiiBuzzle</b></sub></a><br /><a href="#translation-ohaiibuzzle" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/Rastadu23"><img src="https://avatars.githubusercontent.com/u/52637051?v=4?s=100" width="100px;" alt="Rastadu23"/><br /><sub><b>Rastadu23</b></sub></a><br /><a href="#translation-Rastadu23" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/hauuau"><img src="https://avatars.githubusercontent.com/u/52239673?v=4?s=100" width="100px;" alt="hauuau"/><br /><sub><b>hauuau</b></sub></a><br /><a href="https://github.com/Blinue/Magpie/commits?author=hauuau" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/nellydocs"><img src="https://avatars.githubusercontent.com/u/71311423?v=4?s=100" width="100px;" alt="nellydocs"/><br /><sub><b>nellydocs</b></sub></a><br /><a href="#translation-nellydocs" title="Translation">🌍</a></td>
    </tr>
    <tr>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/funnyplanter"><img src="https://avatars.githubusercontent.com/u/173073947?v=4?s=100" width="100px;" alt="funnyplanter"/><br /><sub><b>funnyplanter</b></sub></a><br /><a href="https://github.com/Blinue/Magpie/commits?author=funnyplanter" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/eriforce"><img src="https://avatars.githubusercontent.com/u/8393109?v=4?s=100" width="100px;" alt="Erich Yu"/><br /><sub><b>Erich Yu</b></sub></a><br /><a href="https://github.com/Blinue/Magpie/commits?author=eriforce" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/TamilNeram"><img src="https://avatars.githubusercontent.com/u/67970539?v=4?s=100" width="100px;" alt="தமிழ் நேரம்"/><br /><sub><b>தமிழ் நேரம்</b></sub></a><br /><a href="#translation-TamilNeram" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/mhtvsSFrpHdE"><img src="https://avatars.githubusercontent.com/u/10773245?v=4?s=100" width="100px;" alt="mhtvsSFrpHdE"/><br /><sub><b>mhtvsSFrpHdE</b></sub></a><br /><a href="https://github.com/Blinue/Magpie/commits?author=mhtvsSFrpHdE" title="Documentation">📖</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/kangurek-kao"><img src="https://avatars.githubusercontent.com/u/116571935?v=4?s=100" width="100px;" alt="Krzysztof"/><br /><sub><b>Krzysztof</b></sub></a><br /><a href="#translation-kangurek-kao" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/Howard20181"><img src="https://avatars.githubusercontent.com/u/40033067?v=4?s=100" width="100px;" alt="Howard Wu"/><br /><sub><b>Howard Wu</b></sub></a><br /><a href="https://github.com/Blinue/Magpie/commits?author=Howard20181" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/arifpedia"><img src="https://avatars.githubusercontent.com/u/4081293?v=4?s=100" width="100px;" alt="Arif Budiman"/><br /><sub><b>Arif Budiman</b></sub></a><br /><a href="#translation-arifpedia" title="Translation">🌍</a></td>
    </tr>
    <tr>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/Androidlate"><img src="https://avatars.githubusercontent.com/u/194900061?v=4?s=100" width="100px;" alt="Raphael"/><br /><sub><b>Raphael</b></sub></a><br /><a href="#translation-Androidlate" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/rezorrand"><img src="https://avatars.githubusercontent.com/u/7170353?v=4?s=100" width="100px;" alt="Pate L"/><br /><sub><b>Pate L</b></sub></a><br /><a href="#translation-rezorrand" title="Translation">🌍</a></td>
    </tr>
  </tbody>
</table>

<!-- markdownlint-restore -->
<!-- prettier-ignore-end -->

<!-- ALL-CONTRIBUTORS-LIST:END -->

本项目遵循 [all-contributors](https://allcontributors.org/) 规范，欢迎任何形式的贡献！

## 许可协议

本项目采用 GPLv3 许可协议。
