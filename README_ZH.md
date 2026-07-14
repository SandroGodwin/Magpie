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
- AMD FidelityFX Super Resolution 2.2.1：零运动向量或 50% 分辨率颜色光流。
- NVIDIA VideoSuperRes：同分辨率降噪和 VSR 放大。

这些接入无法获得游戏引擎提供的真实深度、运动向量、曝光、反应遮罩和投影 jitter，因此可能出现拖影、细节不稳定，效果也可能弱于游戏原生接入。它们属于研究原型，不能视为原生 DLSS 或 FSR 的替代品。

RTX Video VSR 已在开发者的 Windows 11 测试机上运行。另一台 Windows 10 测试机在创建 VideoSuperRes 时返回了 `The requested feature is not yet implemented (-2)`。这只是当前测试现象，尚不能证明 VSR 必须使用 Windows 11；驱动版本、GPU 支持以及实际加载的 VFX 运行库也可能影响结果。

### 外部开发依赖

本源码仓库不包含第三方 SDK、模型、wheel 或 NVIDIA 专有二进制文件。请从原作者处获取，并自行接受对应许可证：

- [Magpie 原始仓库](https://github.com/Blinue/Magpie)
- [NVIDIA DLSS SDK](https://github.com/NVIDIA/DLSS)
- [AMD FidelityFX FSR 2.2.1](https://github.com/GPUOpen-Effects/FidelityFX-FSR2/tree/v2.2.1)
- [社区 FSR2 DirectX 11 后端](https://github.com/gameplug-labs/FidelityFX-FSR2-DX11)
- [NVIDIA Video Effects SDK 示例和安装说明](https://github.com/NVIDIA-Maxine/VFX-SDK-Samples)
- [NVIDIA `nvidia-vfx` 软件包](https://pypi.org/project/nvidia-vfx/)

可选后端开关和本机 SDK 路径应写入 `src/BuildOptions.props.user`。这是本机配置文件，不应提交到公共仓库。

### 源码与二进制分发

Magpie 派生源码继续采用 GPLv3，发布源码时必须保留上游版权和许可证声明。AMD FSR2 另有 MIT 声明；NVIDIA SDK 和运行库仍受 NVIDIA 专有条款约束。

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
