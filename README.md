<br>
<p align="center">
  <img src="./src/Magpie/Icons/SVG/Magpie Icon Full Disabled.svg" width="150px" height="150px" alt="Logo">
</p>
<h1 align="center">Magpie</h1>

<div align="center">

[![License](https://img.shields.io/github/license/Blinue/Magpie)](./LICENSE)
[![build](https://github.com/Blinue/Magpie/actions/workflows/build.yml/badge.svg)](https://github.com/Blinue/Magpie/actions/workflows/build.yml)
[![All Contributors](https://img.shields.io/github/all-contributors/Blinue/Magpie)](#acknowledgement-)
[![Translation status](https://hosted.weblate.org/widget/magpie/svg-badge.svg)](https://hosted.weblate.org/engage/magpie)

</div>

🌍 **English** | [简体中文](./README_ZH.md)

Magpie is a lightweight window upscaling tool that comes equipped with a variety of efficient scaling algorithms and filters.

## About this experimental fork

This repository is an independent experimental fork of [Blinue/Magpie](https://github.com/Blinue/Magpie). Magpie and the majority of this codebase were created by Blinue and the upstream contributors. This fork is not an official Magpie release and is not endorsed or supported by the upstream project.

The fork owner used OpenAI Codex as a development assistant to add and test experimental colour-frame-only integrations for:

- NVIDIA DLSS Super Resolution with zero motion vectors, synthetic jitter metadata, or 50%-resolution colour optical flow.
- AMD FidelityFX Super Resolution 2.2.1 with zero motion vectors or 50%-resolution colour optical flow.
- NVIDIA VideoSuperRes for same-resolution denoising and VSR upscaling.

These integrations do not have access to the real depth, motion vectors, exposure, reactive masks, or projection jitter produced by a game engine. They may therefore produce ghosting, unstable detail, or worse image quality than a native in-game integration. They are research prototypes, not replacements for native DLSS or FSR support.

RTX Video VSR has worked on the developer's Windows 11 test machine. A Windows 10 test machine returned `The requested feature is not yet implemented (-2)` while creating VideoSuperRes. This is only an observation, not a confirmed Windows 11 requirement: driver version, GPU support, and the loaded VFX runtime may also affect the result.

### Initial test observations

The following observations are subjective results from a limited set of systems and games; they are not general performance or image-quality claims.

#### DLSS Super Resolution

The current implementation provides three experimental paths: Zero-MV, synthetic jitter metadata, and 50%-resolution colour optical flow. DLSS noticeably improved edge smoothing and image stability in some older 3D games or games with weak native antialiasing. Suitable test cases included Frostpunk, Dyson Sphere Program, Minecraft, and 3D games running in Android emulators. In some scenes, the subjective result was better than FSR1.

Without real depth and motion vectors, moving objects, camera motion, and disocclusion can still produce ghosting or history trails. Additional model/preset comparisons made through external overrides suggested an antialiasing and smoothing order of roughly `L > M > J/K > CNN`, while resistance to ghosting often followed the opposite trend. L and M appeared more dependent on correct temporal inputs: they have high potential in native integrations, but may also retain incorrect history longer when auxiliary data is missing. This comparison does not mean these choices are built into Magpie; the public source currently requests Balanced mode and Preset J.

#### FSR 2.2.1

FSR2 provides Zero-MV and 50%-resolution colour optical-flow paths. In static or low-motion scenes, edge quality and stability could subjectively exceed FSR1, but history artefacts remained possible in motion. Colour optical flow did not consistently outperform Zero-MV and was worse in some scenes, possibly because of errors in flow direction, scale, occlusion, or non-rigid motion. The optical-flow mode therefore remains a research and comparison feature.

#### NVIDIA RTX Video

RTX Video provides same-resolution denoising and true VSR upscaling. VSR produced particularly visible improvements in visual novels, Galgames, and some low-resolution 2D content, cleaning compression noise and improving blurred lines, text, and character edges. This is currently the clearest use case among the experimental integrations. The trade-off is package size: the NVIDIA VFX runtime and models make the complete experimental distribution approximately 508 MiB.

### External development dependencies

Third-party SDKs, models, wheels, and proprietary NVIDIA binaries are not part of this source repository. Obtain them from their original publishers and accept their respective licenses:

- [Original Magpie repository](https://github.com/Blinue/Magpie)
- [NVIDIA DLSS SDK](https://github.com/NVIDIA/DLSS)
- [AMD FidelityFX FSR 2.2.1](https://github.com/GPUOpen-Effects/FidelityFX-FSR2/tree/v2.2.1)
- [Community FSR2 DirectX 11 backend](https://github.com/gameplug-labs/FidelityFX-FSR2-DX11)
- [NVIDIA Video Effects SDK samples and setup instructions](https://github.com/NVIDIA-Maxine/VFX-SDK-Samples)
- [NVIDIA `nvidia-vfx` package](https://pypi.org/project/nvidia-vfx/)

To build the optional backends, create `src/BuildOptions.props.user` and define the feature switches and local SDK paths there. The file is excluded by `.gitignore`; it is machine-specific and must not be committed to the public repository.

### Source and binary distribution

The Magpie-derived source remains licensed under GPLv3. Source distributions must preserve the upstream copyright and license notices. AMD FSR2 has its own MIT notice. NVIDIA SDKs and runtime files remain under NVIDIA's proprietary terms.

Do not treat a GitHub download link as permission to redistribute a third-party SDK. In particular, do not commit SDK folders, models, wheels, or NVIDIA DLLs to this repository. Public binary releases that combine GPLv3 Magpie code with proprietary NVIDIA components require a separate license-compatibility and redistribution review. Until that review is complete, publish source code only, or publish builds without the proprietary components and have users obtain permitted dependencies from NVIDIA. This is a project-maintainer precaution, not legal advice.

👉 [Original Magpie releases](https://github.com/Blinue/Magpie/releases)

👉 [FAQ](https://github.com/Blinue/Magpie/wiki/FAQ%20(EN))

👉 [Built-in effects](https://github.com/Blinue/Magpie/wiki/Built-in%20effects)

👉 [Compilation guide](https://github.com/Blinue/Magpie/wiki/Compilation%20guide)

## Features

* Supports both fullscreen and windowed scaling
* Includes a variety of built-in algorithms and filters, including [Anime4K](https://github.com/bloc97/Anime4K), [FSR](https://github.com/GPUOpen-Effects/FidelityFX-FSR), CRT shaders, and more
* WinUI-based user interface with support for light and dark themes
* Multi-monitor support

## Screenshots

<div style="display:flex; gap:10px;">
  <img src="img/main-window.png" alt= "Main window" height="300">
  <img src="img/screenshot.png" alt= "Main window" height="300">
</div>

## Hints

1. If you have set DPI scaling and the window you want to scale does not support high DPI (which is common in older games), it is recommended to first enter the program's compatibility settings and set "High DPI scaling override" to "Application".

2. Some games support zooming the window, but with extremely naive algorithms. Please set the resolution to the built-in (best) option.

## System requirements

1. Windows 10 v1903+ or Windows 11
2. DirectX feature level 11

## Localization

Thanks to [Weblate](https://weblate.org) for hosting! Click the image below to visit the translation page.

[![Translation status](https://hosted.weblate.org/widget/magpie/multi-auto.svg)](https://hosted.weblate.org/engage/magpie)

## Acknowledgement

Thanks go to these wonderful people:

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

This project follows the [all-contributors](https://allcontributors.org/) specification. Contributions of any kind are welcome!

## License

This project is licensed under GPLv3.
